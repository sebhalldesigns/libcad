---
title: Phase 4 - Boundary Representation (B-Rep) Topology Kernel Architecture
description: Architectural specification for libcad's B-Rep topology kernel for solid modeling
phase: 4
status: design
related-files:
  - lib/lc_entity.h (modify - add B-Rep entity types)
  - lib/lc_brep.h (new)
  - lib/lc_brep.c (new)
  - lib/lc_topology.h (new)
  - lib/lc_topology.c (new)
  - lib/lc_geometry.h (new)
  - lib/lc_geometry.c (new)
  - include/libcad/libcad.h (extend with B-Rep API)
  - lib/libcad.c (wire B-Rep stubs)
---

# Phase 4: Boundary Representation (B-Rep) Topology Kernel Architecture

## Overview

This document specifies the design for libcad's Phase 4: a Boundary Representation (B-Rep) topology kernel for 3D solid modeling. This phase builds on Phase 2's entity system and prepares for Phase 6's feature-based modeling operations (extrude, revolve, boolean operations).

B-Rep is the industry-standard representation for CAD solids. It separates topology (connectivity) from geometry (shape), enabling robust solid modeling operations. This design prioritizes simplicity and understandability over advanced features like NURBS trimming or non-manifold geometry.

## Requirements

### Functional Requirements

1. **Topological Hierarchy**
   - **Solid**: Container for shells
   - **Shell**: Closed or open collection of faces
   - **Face**: Bounded surface with one outer loop and zero or more inner loops (holes)
   - **Loop**: Ordered sequence of edges forming a closed boundary
   - **Edge**: Curve connecting two vertices, shared by 0-2 faces
   - **Vertex**: Point in 3D space, shared by multiple edges

2. **Topological Queries**
   - Navigate relationships: solid → shells → faces → loops → edges → vertices
   - Reverse queries: vertex → edges, edge → faces, face → adjacent faces
   - Orientation queries: edge direction in loop, face normal direction in shell
   - Manifold validation: verify solid is a valid closed manifold

3. **Geometric Data**
   - **Curves**: Line segment, circular arc, elliptical arc (defer: B-spline)
   - **Surfaces**: Plane, cylinder, sphere, cone, torus (defer: NURBS)
   - Separation of topology (connectivity) from geometry (shape)
   - Parametric representation: edges reference curves with u0, u1 bounds

4. **Operations Support (Phase 6 Prep)**
   - Euler operators: primitive topology modifications that maintain validity
   - Data structures must support: extrude, revolve, sweep, loft
   - Data structures must support: boolean operations (union, intersection, difference)
   - Tessellation: convert B-Rep to triangle mesh for rendering

5. **Integration with Entity System**
   - Vertex, Edge, Face, Loop, Shell, Solid are all entity types
   - Leverage Phase 2's tree structure, undo/redo, and serialization
   - B-Rep elements are children of body entities

### Non-Functional Requirements

1. **Performance**
   - Support models with: 10,000 faces, 30,000 edges, 20,000 vertices
   - Topology queries: O(1) for adjacency, O(n) for traversal
   - Tessellation: O(f) where f = number of faces
   - Euler operations: O(1) to O(log n) depending on operation

2. **Memory**
   - Vertex: ~48 bytes (entity slot + position data)
   - Edge: ~80 bytes (entity slot + curve reference + topology links)
   - Face: ~96 bytes (entity slot + surface reference + topology links)
   - Loop: ~32 bytes (entity slot + edge list reference)
   - Shell: ~32 bytes (entity slot + face list reference)
   - Solid: ~32 bytes (entity slot + shell list reference)

3. **Platform Support**
   - Strict C99 compliance
   - Cross-platform: Windows, macOS, Linux, Emscripten
   - No external geometry kernel (no OpenCascade, no CGAL)

4. **Simplicity**
   - Start with planar faces, linear edges, cylindrical/spherical surfaces
   - Defer: NURBS, trimmed surfaces, non-manifold topology
   - Defer: Spline curves, blend surfaces, G2 continuity
   - Focus on correctness and understandability

## Design

### Module Structure

Phase 4 adds three new modules and extends the entity system:

```
lib/
  lc_brep.h          /* B-Rep topology API, Euler operators */
  lc_brep.c          /* Topology construction, traversal, validation */
  lc_topology.h      /* Topological query API (adjacency, orientation) */
  lc_topology.c      /* Query implementation, manifold checks */
  lc_geometry.h      /* Geometric kernels (curves, surfaces, intersection) */
  lc_geometry.c      /* Curve/surface definitions, evaluation, tessellation */
  lc_entity.h        /* EXTEND: add B-Rep entity types */
  lc_entity.c        /* MODIFY: B-Rep entity creation */
  libcad_internal.h  /* EXTEND: B-Rep internal types */
```

### Data Structures

#### B-Rep Entity Types (lc_entity.h extension)

Add new entity types to `lc_entity_type_t`:

```c
/* Extend lc_entity_type_t enum in lc_entity.h */
typedef enum lc_entity_type_t
{
    /* ... existing types ... */
    LC_ENTITY_TYPE_VERTEX,        /* B-Rep vertex (3D point) */
    LC_ENTITY_TYPE_EDGE,          /* B-Rep edge (curve between two vertices) */
    LC_ENTITY_TYPE_LOOP,          /* B-Rep loop (closed chain of edges) */
    LC_ENTITY_TYPE_FACE,          /* B-Rep face (bounded surface) */
    LC_ENTITY_TYPE_SHELL,         /* B-Rep shell (collection of faces) */
    LC_ENTITY_TYPE_SOLID,         /* B-Rep solid (collection of shells) */
    /* ... */
} lc_entity_type_t;
```

#### B-Rep Topology Data Structures

**Vertex Data**

```c
/* Vertex data attached to LC_ENTITY_TYPE_VERTEX entities.
 * Represents a point in 3D space. */
typedef struct lc_vertex_data_t
{
    vec3 position;  /* 3D position in world coordinates */

    /* Topology links: use entity tree (parent = face, siblings = other vertices) */
    /* Edge list: stored as parent → first_child chain in entity system */

} lc_vertex_data_t;
```

**Edge Data**

```c
/* Edge data attached to LC_ENTITY_TYPE_EDGE entities.
 * Represents a curve between two vertices. */
typedef struct lc_edge_data_t
{
    /* Topology: start and end vertices */
    lc_entity_handle_t vertex_start;  /* Start vertex */
    lc_entity_handle_t vertex_end;    /* End vertex */

    /* Geometry: curve definition */
    lc_curve_handle_t curve;          /* Curve geometry (line, arc, spline) */
    float u_start;                    /* Curve parameter at start vertex */
    float u_end;                      /* Curve parameter at end vertex */

    /* Topology links: edge usage records (one per adjacent face) */
    /* Stored as children: LC_ENTITY_TYPE_EDGE_USE (see below) */

} lc_edge_data_t;
```

**Edge Use** (internal helper entity)

Each edge can be shared by two faces. An "edge use" represents one face's view of an edge, including orientation.

```c
/* Edge use: represents an edge's participation in a loop.
 * Edge uses are children of edge entities.
 * This allows one edge to be shared by two faces with different orientations. */
typedef struct lc_edge_use_data_t
{
    lc_entity_handle_t edge;          /* Parent edge */
    lc_entity_handle_t loop;          /* Loop containing this edge use */
    lc_entity_handle_t next_in_loop;  /* Next edge use in loop */
    lc_entity_handle_t prev_in_loop;  /* Previous edge use in loop */
    bool forward;                     /* True if edge direction matches loop direction */

} lc_edge_use_data_t;
```

**Loop Data**

```c
/* Loop data attached to LC_ENTITY_TYPE_LOOP entities.
 * Represents a closed chain of edges forming a face boundary. */
typedef struct lc_loop_data_t
{
    lc_entity_handle_t face;          /* Parent face */
    bool is_outer;                    /* True if outer loop, false if hole */

    /* Edge use list: stored as parent → first_child chain */
    /* First edge use: lc_entity_get_first_child(loop_handle) */
    /* Traverse: edge_use → next_in_loop → ... → back to first */

} lc_loop_data_t;
```

**Face Data**

```c
/* Face data attached to LC_ENTITY_TYPE_FACE entities.
 * Represents a bounded surface. */
typedef struct lc_face_data_t
{
    /* Geometry: surface definition */
    lc_surface_handle_t surface;      /* Surface geometry (plane, cylinder, sphere, etc.) */

    /* Topology: loops (one outer, zero or more inner holes) */
    lc_entity_handle_t outer_loop;    /* Outer boundary loop */
    /* Inner loops (holes): stored as children after outer loop */

    /* Orientation: surface normal direction */
    bool forward;                     /* True if surface normal matches face orientation */

    /* Tessellation cache (for rendering) */
    uint32_t mesh_vertex_count;
    uint32_t mesh_triangle_count;
    void *mesh_data;                  /* Triangle mesh (vertices + indices) */
    bool mesh_dirty;                  /* True if mesh needs regeneration */

} lc_face_data_t;
```

**Shell Data**

```c
/* Shell data attached to LC_ENTITY_TYPE_SHELL entities.
 * Represents a collection of faces forming a closed or open surface. */
typedef struct lc_shell_data_t
{
    bool is_closed;                   /* True if shell is closed (manifold) */

    /* Face list: stored as parent → first_child chain in entity system */

} lc_shell_data_t;
```

**Solid Data**

```c
/* Solid data attached to LC_ENTITY_TYPE_SOLID entities.
 * Represents a volumetric solid body. */
typedef struct lc_solid_data_t
{
    /* Shell list: stored as parent → first_child chain */
    /* First shell is outer shell; subsequent shells are voids (holes) */

    /* Bounding box (for culling and selection) */
    vec3 bbox_min;
    vec3 bbox_max;
    bool bbox_dirty;                  /* True if bbox needs recalculation */

} lc_solid_data_t;
```

#### Geometric Primitives (lc_geometry.h)

**Curve Types**

```c
/* Curve type enumeration. */
typedef enum lc_curve_type_t
{
    LC_CURVE_INVALID = 0,
    LC_CURVE_LINE,              /* Straight line segment */
    LC_CURVE_CIRCLE,            /* Circular arc */
    LC_CURVE_ELLIPSE,           /* Elliptical arc */
    LC_CURVE_BSPLINE,           /* B-spline curve (future) */
} lc_curve_type_t;

/* Opaque curve handle (internal index into curve table) */
typedef uint32_t lc_curve_handle_t;
#define LC_CURVE_INVALID ((lc_curve_handle_t)0)

/* Line curve: parameterized as P(u) = origin + u * direction, u ∈ [0, 1] */
typedef struct lc_curve_line_t
{
    vec3 origin;                /* Line origin (u = 0) */
    vec3 direction;             /* Line direction (unit vector) */
    float length;               /* Line length (u = 1 corresponds to length) */
} lc_curve_line_t;

/* Circular arc curve: parameterized as P(u) = center + radius * (cos(u) * x_axis + sin(u) * y_axis) */
typedef struct lc_curve_circle_t
{
    vec3 center;                /* Circle center */
    vec3 x_axis;                /* Circle X-axis (unit vector) */
    vec3 y_axis;                /* Circle Y-axis (unit vector, perpendicular to x_axis) */
    float radius;               /* Circle radius */
    float u_start;              /* Start angle in radians */
    float u_end;                /* End angle in radians */
} lc_curve_circle_t;

/* Ellipse arc curve: similar to circle but with two radii */
typedef struct lc_curve_ellipse_t
{
    vec3 center;
    vec3 x_axis;                /* Major axis direction (unit vector) */
    vec3 y_axis;                /* Minor axis direction (unit vector) */
    float radius_major;
    float radius_minor;
    float u_start;              /* Start angle in radians */
    float u_end;                /* End angle in radians */
} lc_curve_ellipse_t;
```

**Surface Types**

```c
/* Surface type enumeration. */
typedef enum lc_surface_type_t
{
    LC_SURFACE_INVALID = 0,
    LC_SURFACE_PLANE,           /* Infinite plane */
    LC_SURFACE_CYLINDER,        /* Cylindrical surface */
    LC_SURFACE_SPHERE,          /* Spherical surface */
    LC_SURFACE_CONE,            /* Conical surface */
    LC_SURFACE_TORUS,           /* Toroidal surface */
    LC_SURFACE_BSPLINE,         /* B-spline surface (future) */
} lc_surface_type_t;

/* Opaque surface handle (internal index into surface table) */
typedef uint32_t lc_surface_handle_t;
#define LC_SURFACE_INVALID ((lc_surface_handle_t)0)

/* Plane surface: parameterized as P(u,v) = origin + u * x_axis + v * y_axis */
typedef struct lc_surface_plane_t
{
    vec3 origin;                /* Plane origin */
    vec3 x_axis;                /* U-direction (unit vector) */
    vec3 y_axis;                /* V-direction (unit vector) */
    vec3 normal;                /* Plane normal (cross product of x_axis and y_axis) */
} lc_surface_plane_t;

/* Cylindrical surface: parameterized as P(u,v) = origin + v * axis + radius * (cos(u) * x_axis + sin(u) * y_axis) */
typedef struct lc_surface_cylinder_t
{
    vec3 origin;                /* Cylinder base center */
    vec3 axis;                  /* Cylinder axis (unit vector) */
    vec3 x_axis;                /* Radial X-axis (unit vector, perpendicular to axis) */
    vec3 y_axis;                /* Radial Y-axis (unit vector, perpendicular to axis and x_axis) */
    float radius;               /* Cylinder radius */
} lc_surface_cylinder_t;

/* Spherical surface: parameterized as P(u,v) = center + radius * (cos(v) * cos(u) * x_axis + cos(v) * sin(u) * y_axis + sin(v) * z_axis) */
typedef struct lc_surface_sphere_t
{
    vec3 center;                /* Sphere center */
    vec3 x_axis;                /* Sphere X-axis (unit vector) */
    vec3 y_axis;                /* Sphere Y-axis (unit vector) */
    vec3 z_axis;                /* Sphere Z-axis (unit vector) */
    float radius;               /* Sphere radius */
} lc_surface_sphere_t;

/* Conical surface: parameterized as P(u,v) = apex + v * axis + (v * tan(angle)) * (cos(u) * x_axis + sin(u) * y_axis) */
typedef struct lc_surface_cone_t
{
    vec3 apex;                  /* Cone apex */
    vec3 axis;                  /* Cone axis (unit vector) */
    vec3 x_axis;                /* Radial X-axis (unit vector) */
    vec3 y_axis;                /* Radial Y-axis (unit vector) */
    float angle;                /* Half-angle in radians (angle between axis and surface) */
} lc_surface_cone_t;

/* Toroidal surface: parameterized as P(u,v) = center + (major_radius + minor_radius * cos(v)) * (cos(u) * x_axis + sin(u) * y_axis) + minor_radius * sin(v) * z_axis */
typedef struct lc_surface_torus_t
{
    vec3 center;                /* Torus center */
    vec3 x_axis;                /* Major X-axis (unit vector) */
    vec3 y_axis;                /* Major Y-axis (unit vector) */
    vec3 z_axis;                /* Minor axis (unit vector) */
    float major_radius;         /* Distance from center to tube center */
    float minor_radius;         /* Tube radius */
} lc_surface_torus_t;
```

**Geometry Storage**

Curves and surfaces are stored in separate dense arrays (not in entity system) because:
- Multiple edges can reference the same curve (e.g., opposite edges of a cylinder)
- Multiple faces can reference the same surface (e.g., faces on the same plane)
- Geometry data is immutable (create once, reference many times)

```c
/* Geometry registry in lc_geometry.c (static, not exposed) */
#define LC_GEOMETRY_MAX_CURVES   65536
#define LC_GEOMETRY_MAX_SURFACES 65536

typedef struct lc_geometry_registry_t
{
    /* Curve storage (discriminated union) */
    struct {
        lc_curve_type_t type;
        union {
            lc_curve_line_t line;
            lc_curve_circle_t circle;
            lc_curve_ellipse_t ellipse;
        } data;
    } curves[LC_GEOMETRY_MAX_CURVES];
    uint32_t curve_count;

    /* Surface storage (discriminated union) */
    struct {
        lc_surface_type_t type;
        union {
            lc_surface_plane_t plane;
            lc_surface_cylinder_t cylinder;
            lc_surface_sphere_t sphere;
            lc_surface_cone_t cone;
            lc_surface_torus_t torus;
        } data;
    } surfaces[LC_GEOMETRY_MAX_SURFACES];
    uint32_t surface_count;

} lc_geometry_registry_t;
```

#### Euler Operators (lc_brep.h)

Euler operators are primitive topology modifications that maintain manifold validity. They are the building blocks for all solid modeling operations.

```c
/* Euler operators return true on success, false on failure.
 * All operators automatically update entity tree and record undo commands. */

/* MAKE-VERTEX-EDGE-FACE (MVEF): Create a new face with one edge and two vertices.
 * Used to start a new solid. */
bool lc_euler_mvef(lc_entity_handle_t solid,
                   vec3 v1_pos, vec3 v2_pos,
                   lc_entity_handle_t *out_face,
                   lc_entity_handle_t *out_edge,
                   lc_entity_handle_t *out_v1,
                   lc_entity_handle_t *out_v2);

/* MAKE-EDGE-VERTEX (MEV): Add a new vertex and edge to an existing loop.
 * Splits an existing vertex; new edge connects old vertex to new vertex. */
bool lc_euler_mev(lc_entity_handle_t loop,
                  lc_entity_handle_t existing_vertex,
                  vec3 new_vertex_pos,
                  lc_entity_handle_t *out_vertex,
                  lc_entity_handle_t *out_edge);

/* MAKE-EDGE-FACE (MEF): Split a face into two by connecting two vertices.
 * Creates a new edge and a new face. */
bool lc_euler_mef(lc_entity_handle_t face,
                  lc_entity_handle_t vertex1,
                  lc_entity_handle_t vertex2,
                  lc_entity_handle_t *out_face,
                  lc_entity_handle_t *out_edge);

/* KILL-EDGE-FACE (KEF): Merge two adjacent faces by removing their common edge.
 * Inverse of MEF. */
bool lc_euler_kef(lc_entity_handle_t edge);

/* KILL-EDGE-VERTEX (KEV): Remove an edge and merge its end vertex into start vertex.
 * Inverse of MEV. */
bool lc_euler_kev(lc_entity_handle_t edge);

/* MAKE-EDGE-KILL-LOOP (MEKL): Close a loop by adding an edge.
 * Used to create holes in faces. */
bool lc_euler_mekl(lc_entity_handle_t loop,
                   lc_entity_handle_t vertex1,
                   lc_entity_handle_t vertex2,
                   lc_entity_handle_t *out_edge);

/* KILL-EDGE-MAKE-LOOP (KEML): Split a loop by removing an edge.
 * Inverse of MEKL. */
bool lc_euler_keml(lc_entity_handle_t edge);
```

### API Surface

#### B-Rep Construction (lc_brep.h)

```c
/* Initialize B-Rep system. */
void lc_brep_init(void);

/* Shutdown B-Rep system and free all resources. */
void lc_brep_shutdown(void);

/* Create a new empty solid entity. */
lc_entity_handle_t lc_brep_create_solid(void);

/* Create a box solid (rectangular prism).
 * Returns solid entity handle. */
lc_entity_handle_t lc_brep_create_box(vec3 origin, vec3 dimensions);

/* Create a cylinder solid.
 * Returns solid entity handle. */
lc_entity_handle_t lc_brep_create_cylinder(vec3 base_center, vec3 axis, float radius, float height);

/* Create a sphere solid.
 * Returns solid entity handle. */
lc_entity_handle_t lc_brep_create_sphere(vec3 center, float radius);

/* Validate solid: check manifold, closed shells, consistent orientations.
 * Returns true if valid, false if invalid. */
bool lc_brep_validate_solid(lc_entity_handle_t solid);

/* Tessellate face: convert surface to triangle mesh for rendering.
 * Returns true if successful, false on error. */
bool lc_brep_tessellate_face(lc_entity_handle_t face);

/* Tessellate all faces in solid.
 * Returns true if successful, false on error. */
bool lc_brep_tessellate_solid(lc_entity_handle_t solid);
```

#### Topological Queries (lc_topology.h)

```c
/* Initialize topology query system. */
void lc_topology_init(void);

/* Shutdown topology query system. */
void lc_topology_shutdown(void);

/* Get all shells in a solid.
 * Fills out_handles array with up to max_count handles.
 * Returns actual count (may exceed max_count). */
size_t lc_topology_get_shells(lc_entity_handle_t solid,
                               lc_entity_handle_t *out_handles,
                               size_t max_count);

/* Get all faces in a shell.
 * Fills out_handles array with up to max_count handles.
 * Returns actual count (may exceed max_count). */
size_t lc_topology_get_faces(lc_entity_handle_t shell,
                              lc_entity_handle_t *out_handles,
                              size_t max_count);

/* Get all loops in a face (outer loop + holes). */
size_t lc_topology_get_loops(lc_entity_handle_t face,
                              lc_entity_handle_t *out_handles,
                              size_t max_count);

/* Get all edges in a loop (ordered). */
size_t lc_topology_get_edges(lc_entity_handle_t loop,
                              lc_entity_handle_t *out_handles,
                              size_t max_count);

/* Get vertices of an edge (start and end). */
bool lc_topology_get_edge_vertices(lc_entity_handle_t edge,
                                    lc_entity_handle_t *out_start,
                                    lc_entity_handle_t *out_end);

/* Get vertex position. */
bool lc_topology_get_vertex_position(lc_entity_handle_t vertex, vec3 out_pos);

/* Get all faces adjacent to an edge (0, 1, or 2 faces). */
size_t lc_topology_get_edge_faces(lc_entity_handle_t edge,
                                   lc_entity_handle_t *out_handles,
                                   size_t max_count);

/* Get all edges connected to a vertex. */
size_t lc_topology_get_vertex_edges(lc_entity_handle_t vertex,
                                     lc_entity_handle_t *out_handles,
                                     size_t max_count);

/* Get all faces adjacent to a face (across shared edges). */
size_t lc_topology_get_adjacent_faces(lc_entity_handle_t face,
                                       lc_entity_handle_t *out_handles,
                                       size_t max_count);

/* Check if solid is closed manifold (valid for boolean ops). */
bool lc_topology_is_manifold(lc_entity_handle_t solid);

/* Check if solid is oriented consistently (all face normals point outward). */
bool lc_topology_is_oriented(lc_entity_handle_t solid);

/* Compute bounding box of solid. */
void lc_topology_compute_bbox(lc_entity_handle_t solid, vec3 out_min, vec3 out_max);

/* Count B-Rep elements in solid (Euler characteristic check: V - E + F = 2 for closed manifold). */
void lc_topology_count_elements(lc_entity_handle_t solid,
                                 size_t *out_vertices,
                                 size_t *out_edges,
                                 size_t *out_faces);
```

#### Geometry Evaluation (lc_geometry.h)

```c
/* Initialize geometry system. */
void lc_geometry_init(void);

/* Shutdown geometry system and free all resources. */
void lc_geometry_shutdown(void);

/* Create curve definitions */
lc_curve_handle_t lc_geometry_create_line(vec3 origin, vec3 direction, float length);
lc_curve_handle_t lc_geometry_create_circle(vec3 center, vec3 x_axis, vec3 y_axis, float radius, float u_start, float u_end);
lc_curve_handle_t lc_geometry_create_ellipse(vec3 center, vec3 x_axis, vec3 y_axis, float radius_major, float radius_minor, float u_start, float u_end);

/* Create surface definitions */
lc_surface_handle_t lc_geometry_create_plane(vec3 origin, vec3 x_axis, vec3 y_axis);
lc_surface_handle_t lc_geometry_create_cylinder(vec3 origin, vec3 axis, vec3 x_axis, float radius);
lc_surface_handle_t lc_geometry_create_sphere(vec3 center, vec3 x_axis, vec3 y_axis, vec3 z_axis, float radius);
lc_surface_handle_t lc_geometry_create_cone(vec3 apex, vec3 axis, vec3 x_axis, float angle);
lc_surface_handle_t lc_geometry_create_torus(vec3 center, vec3 x_axis, vec3 y_axis, vec3 z_axis, float major_radius, float minor_radius);

/* Evaluate curve at parameter u (returns 3D point). */
void lc_geometry_eval_curve(lc_curve_handle_t curve, float u, vec3 out_point);

/* Evaluate curve tangent at parameter u (returns unit vector). */
void lc_geometry_eval_curve_tangent(lc_curve_handle_t curve, float u, vec3 out_tangent);

/* Evaluate surface at parameters (u, v) (returns 3D point). */
void lc_geometry_eval_surface(lc_surface_handle_t surface, float u, float v, vec3 out_point);

/* Evaluate surface normal at parameters (u, v) (returns unit vector). */
void lc_geometry_eval_surface_normal(lc_surface_handle_t surface, float u, float v, vec3 out_normal);

/* Compute curve length (exact for lines/arcs, approximate for splines). */
float lc_geometry_curve_length(lc_curve_handle_t curve);

/* Project point onto curve (returns parameter u of closest point). */
float lc_geometry_project_point_curve(lc_curve_handle_t curve, vec3 point);

/* Project point onto surface (returns parameters u, v of closest point). */
void lc_geometry_project_point_surface(lc_surface_handle_t surface, vec3 point, float *out_u, float *out_v);

/* Intersect two curves (returns parameter u1 on curve1 and u2 on curve2).
 * Returns true if intersection found, false otherwise.
 * For Phase 4, only line-line and line-circle implemented. */
bool lc_geometry_intersect_curves(lc_curve_handle_t curve1,
                                   lc_curve_handle_t curve2,
                                   float *out_u1,
                                   float *out_u2);
```

### Internal Interfaces

These functions are called by `libcad.c` to wire up the public API.

```c
/* In lc_brep.c - called by cad_create_box() */
lc_entity_handle_t lc_brep_create_box_internal(vec3 origin, vec3 dimensions);

/* In lc_topology.c - called by cad_get_body_faces() */
size_t lc_topology_enumerate_faces(lc_entity_handle_t body,
                                    lc_entity_handle_t *out_faces,
                                    size_t max_count);

/* In lc_brep.c - called during rendering */
void lc_brep_render_solid(lc_entity_handle_t solid, const lc_render_context_t *ctx);
```

### Data Flow

**Solid Creation Flow (Box Example)**

```
User calls cad_create_box(origin, dimensions)
  ↓
libcad.c → lc_brep_create_box_internal(origin, dimensions)
  ↓
lc_brep.c:
  1. Create solid entity (LC_ENTITY_TYPE_SOLID)
  2. Create shell entity (LC_ENTITY_TYPE_SHELL)
  3. Create 6 face entities (LC_ENTITY_TYPE_FACE)
  4. For each face:
       a. Create plane surface (lc_geometry_create_plane)
       b. Create outer loop (LC_ENTITY_TYPE_LOOP)
       c. Create 4 edges (LC_ENTITY_TYPE_EDGE)
       d. Create 4 edge uses (LC_ENTITY_TYPE_EDGE_USE)
       e. Create line curves for edges (lc_geometry_create_line)
  5. Create 8 vertex entities (LC_ENTITY_TYPE_VERTEX)
  6. Link topology: solid → shell → faces → loops → edges → vertices
  7. Validate manifold (lc_brep_validate_solid)
  ↓
lc_undo.c:
  8. Record LC_COMMAND_CREATE_ENTITY for all created entities (grouped)
  ↓
lc_document.c:
  9. Add solid to document tree (child of root assembly)
  10. Mark document dirty
  ↓
Return solid handle to user
```

**Topology Query Flow**

```
User calls cad_get_face_edges(face_handle)
  ↓
libcad.c → lc_topology_get_edges(face_handle, out_array, max_count)
  ↓
lc_topology.c:
  1. Validate face_handle (lc_entity_is_valid)
  2. Get face data (lc_entity_get_data)
  3. Get outer loop (face_data->outer_loop)
  4. Get first edge use (lc_entity_get_first_child(loop))
  5. Traverse edge uses:
       edge_use = first_child
       while (edge_use != LC_ENTITY_INVALID)
           out_array[i++] = edge_use_data->edge
           edge_use = edge_use_data->next_in_loop
  6. Return edge count
  ↓
Return edge array to user
```

**Tessellation Flow (for rendering)**

```
cad_render_viewport()
  ↓
lc_scene_render_ctx(ctx)
  ↓
lc_scene.c:
  1. Query all solids in document (lc_entity_enumerate_type)
  2. For each solid:
       a. Check if visible (lc_document_is_visible)
       b. Tessellate if mesh dirty (lc_brep_tessellate_solid)
       c. Upload mesh to GPU (lc_gpu_create_buffer)
       d. Render mesh (glDrawElements via lc_gpu API)
```

### Platform Considerations

**Cross-Platform Compatibility**

1. **No C++ features**: All code is strict C99. Use `/* */` comments.
2. **No platform-specific types**: Use stdint.h types.
3. **Endianness**: JSON serialization handles geometry data as text.
4. **Math library**: Use cglm for vector/matrix operations.

**Memory Management**

1. **Geometry storage**: Static arrays in lc_geometry.c (no malloc per curve/surface).
2. **Entity storage**: Leverages Phase 2's generational index system.
3. **Tessellation cache**: Allocated per face; freed when face is deleted.
4. **No reference counting**: Geometry is immutable; curves/surfaces never deleted (future: add refcount + GC).

**Build System**

Add to CMakeLists.txt:

```cmake
target_sources(libcad PRIVATE
    lib/lc_brep.c
    lib/lc_topology.c
    lib/lc_geometry.c
)
```

## Alternatives Considered

### 1. Topology Representation: Half-Edge vs Winged-Edge vs Radial-Edge

**Chosen: Edge-Use Pattern (Simplified Half-Edge)**

Pros:
- Integrates cleanly with entity system (edge uses are entities)
- Explicit orientation (forward/backward flag per edge use)
- Easy to traverse loops (next_in_loop pointer)
- Supports non-manifold edges (edge can have 0, 1, 2+ edge uses)

Cons:
- Extra entity overhead per edge use (64 bytes per edge use)
- More complex than direct edge → face pointers

**Winged-Edge**

Pros:
- Compact representation (edges store pointers to both adjacent faces)
- Fast face adjacency queries

Cons:
- Hard to extend to non-manifold topology
- Orientation bookkeeping is tricky

**Radial-Edge (CGAL-style)**

Pros:
- Handles non-manifold topology natively
- Very general

Cons:
- Complex implementation (~2000 lines for just data structures)
- Overkill for simple manifold solids

**Verdict**: Edge-use pattern is simple, integrates with entity system, and can be extended to non-manifold in Phase 6 if needed.

### 2. Geometry Storage: Embedded vs Handle-Based

**Chosen: Handle-Based (Curve/Surface Registry)**

Pros:
- Geometry sharing (multiple edges reference same curve)
- Immutable geometry (can't accidentally modify shared curves)
- Compact references (32-bit handle vs 64+ bytes for embedded data)

Cons:
- Extra indirection (handle → registry lookup)

**Embedded in Edge/Face Data**

Pros:
- No indirection; geometry data is in cache with topology

Cons:
- Wastes memory (duplicate curve data for shared edges)
- Harder to share geometry between entities
- Larger entity slots (>128 bytes)

**Verdict**: Handle-based saves memory and enables geometry sharing, which is critical for cylindrical/spherical faces (all edges on a cylinder share the same circular arc curve).

### 3. Euler Operators vs Direct Construction

**Chosen: Euler Operators**

Pros:
- Guarantees manifold validity (operators maintain Euler characteristic V - E + F = 2)
- Standard approach in CAD kernels (OpenCascade, Parasolid)
- Composable (complex operations built from primitives)

Cons:
- Steeper learning curve (MEV, MEF, MEKL naming is cryptic)
- More code (~800 lines vs ~200 for direct construction)

**Direct Construction (Create Faces, Add to Shell)**

Pros:
- Simpler implementation
- More intuitive for simple shapes (box, cylinder)

Cons:
- Easy to create invalid topology (holes, non-manifold, inconsistent orientation)
- Validation becomes a separate complex step

**Verdict**: Euler operators are the right long-term choice. For Phase 4A-4C, implement direct construction for primitives (box, cylinder, sphere); add Euler operators in Phase 4D-4E.

### 4. Tessellation: On-Demand vs Pre-Computed

**Chosen: On-Demand with Caching**

Pros:
- Lazy evaluation (only tessellate visible faces)
- Cache invalidation is explicit (mesh_dirty flag)
- Adjustable tessellation density (future: LOD based on distance)

Cons:
- First render frame is slow (must tessellate all faces)
- Cache management complexity

**Pre-Computed on Creation**

Pros:
- First render is fast
- Simpler implementation

Cons:
- Wastes memory (tessellate all faces even if hidden)
- No LOD support

**Verdict**: On-demand caching is more flexible and is standard in modern CAD systems.

## Implementation Notes

### Suggested Implementation Order

**Phase 4A: Geometry System (2-3 days)**

1. Create `lib/lc_geometry.h` and `lib/lc_geometry.c`
2. Implement geometry registry (static arrays for curves and surfaces)
3. Implement curve creation: line, circle, ellipse
4. Implement surface creation: plane, cylinder, sphere
5. Implement evaluation functions: eval_curve, eval_surface, eval_surface_normal
6. Test: Create 1000 curves/surfaces, verify evaluation at various parameters

**Phase 4B: B-Rep Entity Types (1-2 days)**

1. Extend `lc_entity.h` with B-Rep entity types
2. Add type-specific data structures: vertex, edge, loop, face, shell, solid
3. Modify `lc_entity.c` to handle new types
4. Test: Create vertex, edge, face entities; verify entity tree structure

**Phase 4C: Primitive Construction (2-3 days)**

1. Create `lib/lc_brep.h` and `lib/lc_brep.c`
2. Implement `lc_brep_create_box()` (6 faces, 12 edges, 8 vertices)
3. Implement `lc_brep_create_cylinder()` (3 faces: 2 circles + 1 cylinder)
4. Implement `lc_brep_create_sphere()` (1 face with spherical surface)
5. Wire up to `libcad.c` public API
6. Test: Create primitives, verify entity counts (V - E + F = 2 for closed solids)

**Phase 4D: Topological Queries (1-2 days)**

1. Create `lib/lc_topology.h` and `lib/lc_topology.c`
2. Implement traversal functions: get_shells, get_faces, get_loops, get_edges
3. Implement reverse queries: get_edge_faces, get_vertex_edges
4. Implement validation: is_manifold, is_oriented
5. Test: Create box, query all elements, verify connectivity

**Phase 4E: Tessellation (2-3 days)**

1. Implement `lc_brep_tessellate_face()` in `lc_brep.c`
2. Tessellate planar faces: triangulate polygon loops
3. Tessellate cylindrical faces: generate quad strips
4. Tessellate spherical faces: generate UV grid
5. Cache mesh data in `lc_face_data_t`
6. Test: Tessellate box, cylinder, sphere; verify triangle counts

**Phase 4F: Rendering Integration (1-2 days)**

1. Modify `lc_scene.c` to enumerate solids and render tessellated meshes
2. Upload mesh data to GPU using `lc_gpu` API
3. Render with basic shading (flat or smooth normals)
4. Test: Create primitives, verify they render correctly in viewport

**Phase 4G: Euler Operators (2-3 days, optional for Phase 4)**

1. Implement MVEF (make-vertex-edge-face)
2. Implement MEV (make-edge-vertex)
3. Implement MEF (make-edge-face)
4. Implement KEF, KEV (inverse operators)
5. Test: Build box using Euler operators; compare with direct construction

**Total Estimated Effort: 12-18 days**

### Known Risks and Open Questions

**Risk: Non-Manifold Edges**

Some CAD operations produce non-manifold edges (edge shared by 3+ faces). Current design supports this (edge uses allow multiple faces per edge), but validation and boolean operations become more complex.

**Mitigation**: Document that Phase 4 only handles manifold solids. Defer non-manifold support to Phase 6.

**Risk: Numerical Precision**

Floating-point errors can cause topology inconsistencies (e.g., two "coincident" vertices with slightly different positions).

**Mitigation**: Use tolerance-based comparisons (e.g., `fabs(v1.x - v2.x) < 1e-6`). Document tolerance value (1e-6 for now; may need adjustment based on testing).

**Question: Edge Sharing Between Faces**

How to detect when two faces share an edge (e.g., adjacent faces on a box)?

**Recommendation**: During construction, maintain a hash table of edge (vertex1, vertex2) pairs. When creating a new edge, check if the reverse edge (vertex2, vertex1) already exists. If yes, reuse the edge entity and create a second edge use. If no, create a new edge.

**Question: Loop Orientation**

How to determine if a loop is oriented correctly (counterclockwise when viewed from face normal)?

**Recommendation**: After constructing a loop, compute signed area using cross products. If negative, reverse loop orientation. Defer to tessellation phase (Phase 4E).

**Question: Serialization of Geometry**

How to serialize curves and surfaces to JSON?

**Recommendation**: Geometry handles are indices into registry. Serialize registry as separate JSON array:
```json
{
  "curves": [
    { "type": "line", "origin": [0,0,0], "direction": [1,0,0], "length": 10.0 },
    { "type": "circle", "center": [0,0,0], ... }
  ],
  "surfaces": [
    { "type": "plane", "origin": [0,0,0], "x_axis": [1,0,0], "y_axis": [0,1,0] }
  ]
}
```
Each edge/face entity stores curve/surface handle as integer index.

**Defer to Phase 4F (after tessellation is working).**

### Dependencies on Existing Code

**Requires from Phase 2**

- Entity system (`lc_entity.h`, `lc_entity.c`) - B-Rep elements are entities
- Document system (`lc_document.h`, `lc_document.c`) - Solids are children of root assembly
- Undo/redo system (`lc_undo.h`, `lc_undo.c`) - B-Rep creation/modification is undoable

**Requires from Phase 1**

- `lc_render_context_t` - Used for rendering tessellated meshes
- `lc_gpu` API - Upload mesh data to GPU
- cglm - Vector and matrix math

**Integrates with**

- `lc_scene.c` - Modified to enumerate and render solids
- `libcad.c` - Extended with B-Rep creation API (cad_create_box, etc.)

**No changes required to**

- `lc_canvas.c` - 2D sketching is independent of 3D solids
- `lc_draw.c` - Rendering primitives unchanged
- `lc_constraint.c` - Constraints are independent of B-Rep (Phase 3)

### Testing Strategy

**Unit Tests**

1. **Geometry Evaluation**
   - Create line curve, evaluate at u=0, 0.5, 1; verify points
   - Create circle curve, evaluate at u=0, π/2, π; verify points on circle
   - Create plane surface, evaluate at (u,v); verify point on plane
   - Create cylinder surface, evaluate at (u,v); verify point on cylinder

2. **B-Rep Construction**
   - Create box, verify V=8, E=12, F=6 (Euler: 8 - 12 + 6 = 2 ✓)
   - Create cylinder, verify V=0, E=2, F=3 (Euler: 0 - 2 + 3 = 1 for open cylinder; close with caps → 2)
   - Create sphere, verify V=0, E=0, F=1 (degenerate case; Euler not applicable)

3. **Topological Queries**
   - Create box, query all faces, verify 6 faces returned
   - Query edges of one face, verify 4 edges
   - Query vertices of one edge, verify 2 vertices
   - Query adjacent faces of one face, verify 4 adjacent faces

4. **Tessellation**
   - Tessellate box face (planar), verify 2 triangles generated
   - Tessellate cylinder side, verify ~20 quads (40 triangles) at default resolution
   - Verify triangle winding (CCW when viewed from outside)

**Integration Tests**

1. Call public API from Python/PyQt test harness
2. Create box, render, verify correct appearance
3. Create cylinder, rotate camera, verify shading correct
4. Select face, verify selection highlighting

**Performance Tests**

1. Create 1000 boxes (6000 faces, 12000 edges), measure time (should be < 5 seconds)
2. Tessellate 10000 faces, measure time (should be < 500ms)
3. Query 100000 topology relationships, measure time (should be < 100ms)

### Migration Path from Phase 2

**Current State (Phase 2)**

Solids exist as empty containers (`lc_body_data_t` with `brep_data = NULL`). No topology or geometry.

**Phase 4 Migration**

1. Replace `void *brep_data` with actual B-Rep data (point to root solid entity)
2. Update `lc_body_data_t`:
   ```c
   typedef struct lc_body_data_t
   {
       lc_entity_handle_t solid;  /* Root B-Rep solid entity (was void *brep_data) */
   } lc_body_data_t;
   ```
3. When creating a body, automatically create an associated solid entity
4. Rendering queries body → solid → shells → faces → mesh

This allows gradual migration: existing Phase 2 code continues to work, new Phase 4 code adds B-Rep topology.

## Summary

This architecture provides a simple, robust B-Rep kernel for libcad. Key design decisions:

- **Edge-use pattern** for topology (simplified half-edge, integrates with entity system)
- **Handle-based geometry** for memory efficiency and sharing
- **Euler operators** for guaranteed manifold validity (future; Phase 4 starts with direct construction)
- **On-demand tessellation** with caching for efficient rendering
- **Pure C99** with no external dependencies (no OpenCascade, no CGAL)

The implementation is organized into three modules:
- `lc_brep.c` - Construction and high-level operations
- `lc_topology.c` - Query and traversal
- `lc_geometry.c` - Curve/surface evaluation and intersection

Estimated effort is 12-18 days for a complete, tested implementation covering primitives (box, cylinder, sphere), topological queries, and tessellation.

After Phase 4, libcad will have solid modeling capabilities, enabling feature-based operations (Phase 6: extrude, revolve, boolean) and generative design workflows.
