/***************************************************************
**
** libcad Header File
**
** File         :  lc_entity.h
** Module       :  libcad (entity system)
** Author       :  SH
** Created      :  2026-01-29 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Entity handle-based storage system with generational
**                 indices. Provides create, destroy, validation, tree
**                 manipulation, and type-specific data access.
**
***************************************************************/

#ifndef LC_ENTITY_H
#define LC_ENTITY_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <cglm/cglm.h>

/* Forward declarations for geometry handles (from lc_geometry.h) */
typedef uint32_t lc_curve_handle_t;
typedef uint32_t lc_surface_handle_t;

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/* Entity handle is a 32-bit value:
 *   - Upper 16 bits: generation counter (prevents stale handles)
 *   - Lower 16 bits: slot index (max 65535 entities)
 */
typedef uint32_t lc_entity_handle_t;

#define LC_ENTITY_INVALID ((lc_entity_handle_t)0)

/* Extract index from handle */
#define LC_ENTITY_INDEX(h)      ((uint16_t)((h) & 0xFFFF))

/* Extract generation from handle */
#define LC_ENTITY_GENERATION(h) ((uint16_t)(((h) >> 16) & 0xFFFF))

/* Construct handle from index and generation */
#define LC_ENTITY_MAKE_HANDLE(idx, gen) \
    (((uint32_t)(gen) << 16) | (uint32_t)(idx))

/* Maximum number of entity slots */
#define LC_ENTITY_MAX_SLOTS 65536

/* Entity metadata bit flags */
#define LC_ENTITY_FLAG_VISIBLE   (1 << 0)  /* Entity is visible */
#define LC_ENTITY_FLAG_LOCKED    (1 << 1)  /* Entity cannot be modified */
#define LC_ENTITY_FLAG_SELECTED  (1 << 2)  /* Entity is selected */
#define LC_ENTITY_FLAG_DELETED   (1 << 3)  /* Entity is soft-deleted (for undo) */

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* Entity type enumeration.
 * Determines which union field in lc_entity_slot_t is valid. */
typedef enum lc_entity_type_t
{
    LC_ENTITY_TYPE_INVALID = 0,
    LC_ENTITY_TYPE_ASSEMBLY,      /* Root or sub-assembly */
    LC_ENTITY_TYPE_SKETCH,        /* 2D sketch plane */
    LC_ENTITY_TYPE_BODY,          /* 3D solid body (placeholder, not B-Rep solid) */
    LC_ENTITY_TYPE_CONSTRAINT,    /* 2D constraint */
    LC_ENTITY_TYPE_GEOMETRY_LINE, /* 2D line in sketch */
    LC_ENTITY_TYPE_GEOMETRY_CIRCLE, /* 2D circle in sketch */
    LC_ENTITY_TYPE_GEOMETRY_RECT,  /* 2D rectangle in sketch */
    /* B-Rep topology types (Phase 4) */
    LC_ENTITY_TYPE_VERTEX,        /* B-Rep vertex (3D point) */
    LC_ENTITY_TYPE_EDGE,          /* B-Rep edge (curve between two vertices) */
    LC_ENTITY_TYPE_EDGE_USE,      /* B-Rep edge use (edge participation in loop) */
    LC_ENTITY_TYPE_LOOP,          /* B-Rep loop (closed chain of edges) */
    LC_ENTITY_TYPE_FACE,          /* B-Rep face (bounded surface) */
    LC_ENTITY_TYPE_SHELL,         /* B-Rep shell (collection of faces) */
    LC_ENTITY_TYPE_SOLID,         /* B-Rep solid (volumetric body) */
    LC_ENTITY_TYPE_COUNT
} lc_entity_type_t;

/* Sketch data attached to LC_ENTITY_TYPE_SKETCH entities. */
typedef struct lc_sketch_data_t
{
    vec3 origin;          /* World position of sketch plane origin */
    vec3 normal;          /* Sketch plane normal (unit vector) */
    vec3 x_axis;          /* Sketch plane X-axis (unit vector) */

    /* Geometry children are stored in entity tree (first_child list) */
    /* No explicit array needed; walk first_child -> next_sibling chain */

} lc_sketch_data_t;

/* Body data attached to LC_ENTITY_TYPE_BODY entities.
 * For Phase 2, bodies are empty containers; Phase 4 adds B-Rep. */
typedef struct lc_body_data_t
{
    /* B-Rep data will be added in Phase 4 */
    void *brep_data;  /* Placeholder; NULL for now */

} lc_body_data_t;

/* Constraint type enumeration.
 * Determines which error function to use and which parameters are valid. */
typedef enum lc_constraint_type_t
{
    LC_CONSTRAINT_DISTANCE_POINT_POINT = 0,  /* Distance between two points */
    LC_CONSTRAINT_DISTANCE_POINT_LINE,       /* Perpendicular distance from point to line */
    LC_CONSTRAINT_DISTANCE_LINE_LINE_PARALLEL, /* Parallel distance between two parallel lines */
    LC_CONSTRAINT_ANGLE_LINE_LINE,           /* Angle between two lines */
    LC_CONSTRAINT_COINCIDENT_POINT_POINT,    /* Two points at same location */
    LC_CONSTRAINT_COINCIDENT_POINT_LINE,     /* Point lies on line */
    LC_CONSTRAINT_COINCIDENT_POINT_CIRCLE,   /* Point lies on circle */
    LC_CONSTRAINT_PARALLEL,                  /* Two lines parallel */
    LC_CONSTRAINT_PERPENDICULAR,             /* Two lines perpendicular */
    LC_CONSTRAINT_HORIZONTAL,                /* Line is horizontal */
    LC_CONSTRAINT_VERTICAL,                  /* Line is vertical */
    LC_CONSTRAINT_TANGENT_LINE_CIRCLE,       /* Line is tangent to circle */
    LC_CONSTRAINT_TANGENT_CIRCLE_CIRCLE,     /* Two circles are tangent */
    LC_CONSTRAINT_EQUAL_LENGTH,              /* Two line segments have equal length */
    LC_CONSTRAINT_EQUAL_RADIUS,              /* Two circles have equal radius */
    LC_CONSTRAINT_FIX_POINT,                 /* Point locked to specific position */
    LC_CONSTRAINT_TYPE_COUNT
} lc_constraint_type_t;

/* Constraint state flags */
#define LC_CONSTRAINT_FLAG_SATISFIED   (1 << 0)  /* Error below tolerance */
#define LC_CONSTRAINT_FLAG_CONFLICTED  (1 << 1)  /* Conflicts with other constraints */

/* Constraint data attached to LC_ENTITY_TYPE_CONSTRAINT entities.
 * Stores constraint type, referenced entities, parameters, and solver state. */
typedef struct lc_constraint_data_t
{
    lc_constraint_type_t type;        /* Constraint type */
    lc_entity_handle_t entities[4];   /* Referenced entities (1-4) */
    float value;                       /* Parameter (distance, angle) */
    float weight;                      /* Solver weight (1.0 = normal) */
    float error;                       /* Current error/residual */
    uint32_t flags;                    /* Status flags */
} lc_constraint_data_t;

/* 2D line geometry in a sketch. */
typedef struct lc_geometry_line_data_t
{
    vec2 start;  /* Start point in sketch-local coordinates */
    vec2 end;    /* End point in sketch-local coordinates */
    uint32_t color;  /* RGBA packed color */
    float thickness; /* Line thickness in pixels */
} lc_geometry_line_data_t;

/* 2D circle geometry in a sketch. */
typedef struct lc_geometry_circle_data_t
{
    vec2 center;     /* Center in sketch-local coordinates */
    float radius;
    uint32_t color;
} lc_geometry_circle_data_t;

/* 2D rectangle geometry in a sketch. */
typedef struct lc_geometry_rect_data_t
{
    vec2 min;  /* Bottom-left corner in sketch-local coordinates */
    vec2 max;  /* Top-right corner in sketch-local coordinates */
    uint32_t color;
} lc_geometry_rect_data_t;

/* B-Rep Topology Data Structures (Phase 4) */

/* Vertex data attached to LC_ENTITY_TYPE_VERTEX entities.
 * Represents a point in 3D space. */
typedef struct lc_vertex_data_t
{
    vec3 position;  /* 3D position in world coordinates */

    /* Topology links: use entity tree (parent = solid, siblings = other vertices) */
    /* Edge list: stored as parent → first_child chain in entity system */

} lc_vertex_data_t;

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

/* Edge use data attached to LC_ENTITY_TYPE_EDGE_USE entities.
 * Represents an edge's participation in a loop.
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

/* Shell data attached to LC_ENTITY_TYPE_SHELL entities.
 * Represents a collection of faces forming a closed or open surface. */
typedef struct lc_shell_data_t
{
    bool is_closed;                   /* True if shell is closed (manifold) */

    /* Face list: stored as parent → first_child chain in entity system */

} lc_shell_data_t;

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

/* Entity storage slot.
 * Uses discriminated union pattern for type-specific data. */
typedef struct lc_entity_slot_t
{
    lc_entity_handle_t handle;    /* Current handle (includes generation) */
    lc_entity_type_t type;        /* Entity type discriminator */
    uint32_t flags;               /* Metadata flags (see above) */

    /* Parent/child relationships (used by document tree) */
    lc_entity_handle_t parent;    /* Parent entity (LC_ENTITY_INVALID if root) */
    lc_entity_handle_t first_child;
    lc_entity_handle_t next_sibling;
    lc_entity_handle_t prev_sibling;

    /* Entity-specific data */
    union
    {
        lc_sketch_data_t *sketch;
        lc_body_data_t *body;
        lc_constraint_data_t *constraint;
        lc_geometry_line_data_t *geometry_line;
        lc_geometry_circle_data_t *geometry_circle;
        lc_geometry_rect_data_t *geometry_rect;
        /* B-Rep topology data (Phase 4) */
        lc_vertex_data_t *vertex;
        lc_edge_data_t *edge;
        lc_edge_use_data_t *edge_use;
        lc_loop_data_t *loop;
        lc_face_data_t *face;
        lc_shell_data_t *shell;
        lc_solid_data_t *solid;
        void *generic;            /* For future entity types */
    } data;

    /* User data attachment (optional) */
    void *user_data;
    void (*user_data_destructor)(void *);

} lc_entity_slot_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* Initialize entity system.
 * Call once at startup before any entity operations. */
void lc_entity_init(void);

/* Shutdown entity system and free all resources.
 * Call once at shutdown; invalidates all entity handles. */
void lc_entity_shutdown(void);

/* Create a new entity of the specified type.
 * Returns valid handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_entity_create(lc_entity_type_t type);

/* Destroy an entity and invalidate its handle.
 * Returns true if entity existed and was destroyed, false otherwise. */
bool lc_entity_destroy(lc_entity_handle_t handle);

/* Check if a handle is valid (entity exists and generation matches).
 * Returns true if valid, false if handle is stale or entity was deleted. */
bool lc_entity_is_valid(lc_entity_handle_t handle);

/* Get entity type.
 * Returns LC_ENTITY_TYPE_INVALID if handle is invalid. */
lc_entity_type_t lc_entity_get_type(lc_entity_handle_t handle);

/* Get entity flags (visibility, locked, selected, etc.).
 * Returns 0 if handle is invalid. */
uint32_t lc_entity_get_flags(lc_entity_handle_t handle);

/* Set entity flags.
 * Returns true if successful, false if handle is invalid. */
bool lc_entity_set_flags(lc_entity_handle_t handle, uint32_t flags);

/* Get type-specific data pointer.
 * Returns NULL if handle is invalid or type mismatch.
 * Caller must cast to appropriate type based on entity type. */
void* lc_entity_get_data(lc_entity_handle_t handle);

/* Set type-specific data pointer.
 * Caller is responsible for allocating data.
 * Returns true if successful, false if handle is invalid.
 * Note: Does not free old data; caller must do that first if needed. */
bool lc_entity_set_data(lc_entity_handle_t handle, void *data);

/* Attach user data to an entity.
 * If destructor is non-NULL, it will be called when entity is destroyed.
 * Returns true if successful, false if handle is invalid. */
bool lc_entity_set_user_data(lc_entity_handle_t handle, void *user_data,
                              void (*destructor)(void*));

/* Get user data attached to an entity.
 * Returns NULL if no user data or handle is invalid. */
void* lc_entity_get_user_data(lc_entity_handle_t handle);

/* Parent/child tree manipulation */
bool lc_entity_add_child(lc_entity_handle_t parent, lc_entity_handle_t child);
bool lc_entity_remove_child(lc_entity_handle_t parent, lc_entity_handle_t child);
lc_entity_handle_t lc_entity_get_parent(lc_entity_handle_t entity);
lc_entity_handle_t lc_entity_get_first_child(lc_entity_handle_t entity);
lc_entity_handle_t lc_entity_get_next_sibling(lc_entity_handle_t entity);
lc_entity_handle_t lc_entity_get_prev_sibling(lc_entity_handle_t entity);

/* Enumerate all entities of a given type.
 * Fills out_handles array with up to max_count handles.
 * Returns actual number of entities found (may exceed max_count). */
size_t lc_entity_enumerate_type(lc_entity_type_t type,
                                 lc_entity_handle_t *out_handles,
                                 size_t max_count);

/* Type-specific creation functions (called by libcad.c) */

/* Create a sketch entity with specified plane parameters.
 * Returns valid handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_entity_create_sketch(vec3 origin, vec3 normal, vec3 x_axis);

/* Create a line geometry entity within a sketch.
 * Returns valid handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_entity_create_line(lc_entity_handle_t sketch,
                                          vec2 start, vec2 end,
                                          uint32_t color, float thickness);

/* Create a circle geometry entity within a sketch.
 * Returns valid handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_entity_create_circle(lc_entity_handle_t sketch,
                                            vec2 center, float radius,
                                            uint32_t color);

/* Create a rectangle geometry entity within a sketch.
 * Returns valid handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_entity_create_rect(lc_entity_handle_t sketch,
                                          vec2 min, vec2 max,
                                          uint32_t color);

#ifdef __cplusplus
}
#endif

#endif /* LC_ENTITY_H */
