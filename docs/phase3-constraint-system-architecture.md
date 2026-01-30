---
title: Phase 3 - 2D Parametric Constraint System Architecture
description: Architectural specification for libcad's 2D parametric constraints, preparing for solver integration in Phase 5
phase: 3
status: design
related-files:
  - lib/lc_entity.h (modify - add constraint entity data)
  - lib/lc_constraint.h (new)
  - lib/lc_constraint.c (new)
  - include/libcad/libcad.h (extend with constraint API)
  - lib/libcad.c (wire constraint stubs)
---

# Phase 3: 2D Parametric Constraint System Architecture

## Overview

This document specifies the design for libcad's Phase 3: a 2D parametric constraint system that enables constraint-driven sketch modeling. This phase builds on Phase 2's entity system and prepares for Phase 5's numerical solver integration.

The constraint system defines geometric relationships (distance, angle, coincident, parallel, etc.) between sketch entities. Each constraint generates one or more equations that a numerical solver will satisfy. This phase designs the constraint storage, error functions, and dependency tracking without implementing the solver itself.

## Requirements

### Functional Requirements

1. **Constraint Types (Geometric)**
   - **Distance**: Point-to-point, point-to-line, parallel line distance
   - **Angle**: Between two lines
   - **Coincident**: Point-on-point, point-on-line, point-on-circle
   - **Parallel**: Two lines
   - **Perpendicular**: Two lines
   - **Horizontal/Vertical**: Single line
   - **Tangent**: Line-to-circle, circle-to-circle
   - **Equal**: Equal lengths, equal radii
   - **Fix**: Lock point position or lock geometry parameters

2. **Constraint Storage**
   - Integrate with existing entity system (use `LC_ENTITY_TYPE_CONSTRAINT` from Phase 2)
   - Each constraint references 1-4 geometry entities (lines, circles, points)
   - Store constraint parameters (distance value, angle value, etc.)
   - Track constraint satisfaction status (satisfied, violated, over-constrained)

3. **Constraint Evaluation**
   - Compute error/residual for each constraint (how much is it violated?)
   - Error function should be differentiable for gradient-based solvers
   - Support constraint weighting (some constraints more important than others)

4. **Dependency Tracking**
   - Build constraint graph: entities as nodes, constraints as edges
   - Detect over-constrained systems (more constraints than degrees of freedom)
   - Detect under-constrained systems (insufficient constraints)
   - Identify rigid clusters (groups of fully-constrained entities)

5. **Undo/Redo Integration**
   - Constraints are entities, so creation/deletion uses existing undo system
   - Changing constraint parameters (distance value, etc.) uses property modification commands

### Non-Functional Requirements

1. **Performance**
   - Constraint graph construction: O(n) where n = number of constraints
   - Error evaluation: O(c) where c = number of constraints (each constraint is O(1))
   - DOF analysis: O(n + m) where n = entities, m = constraints
   - Typical sketch: 100-1000 constraints should evaluate in < 10ms

2. **Memory**
   - Constraint entity: ~80 bytes (fits in entity slot + constraint data)
   - Constraint graph: ~16 bytes per edge (adjacency list)
   - DOF tracking: ~8 bytes per entity

3. **Platform Support**
   - Strict C99 compliance
   - No external solver dependencies (Phase 5 will add solver)
   - Cross-platform: Windows, macOS, Linux, Emscripten

## Design

### Module Structure

Phase 3 adds one new module and extends the existing entity system:

```
lib/
  lc_constraint.h    /* Constraint API, error functions, DOF analysis */
  lc_constraint.c    /* Constraint evaluation, dependency graph */
  lc_entity.h        /* EXTEND: add constraint-specific entity data */
  lc_entity.c        /* MODIFY: constraint entity creation */
  libcad_internal.h  /* EXTEND: constraint graph types */
```

### Data Structures

#### Constraint Types (lc_constraint.h)

```c
/* Constraint type enumeration.
 * Determines which error function to use and which parameters are valid. */
typedef enum lc_constraint_type_t
{
    LC_CONSTRAINT_INVALID = 0,

    /* Distance constraints */
    LC_CONSTRAINT_DISTANCE_POINT_POINT,     /* Distance between two points */
    LC_CONSTRAINT_DISTANCE_POINT_LINE,      /* Perpendicular distance from point to line */
    LC_CONSTRAINT_DISTANCE_LINE_LINE,       /* Parallel distance between two parallel lines */

    /* Angle constraints */
    LC_CONSTRAINT_ANGLE_LINE_LINE,          /* Angle between two lines */

    /* Coincident constraints */
    LC_CONSTRAINT_COINCIDENT_POINT_POINT,   /* Two points at same location */
    LC_CONSTRAINT_COINCIDENT_POINT_LINE,    /* Point lies on line */
    LC_CONSTRAINT_COINCIDENT_POINT_CIRCLE,  /* Point lies on circle */

    /* Parallelism and perpendicularity */
    LC_CONSTRAINT_PARALLEL,                 /* Two lines parallel */
    LC_CONSTRAINT_PERPENDICULAR,            /* Two lines perpendicular */

    /* Orientation constraints */
    LC_CONSTRAINT_HORIZONTAL,               /* Line is horizontal */
    LC_CONSTRAINT_VERTICAL,                 /* Line is vertical */

    /* Tangency constraints */
    LC_CONSTRAINT_TANGENT_LINE_CIRCLE,      /* Line is tangent to circle */
    LC_CONSTRAINT_TANGENT_CIRCLE_CIRCLE,    /* Two circles are tangent */

    /* Equality constraints */
    LC_CONSTRAINT_EQUAL_LENGTH,             /* Two line segments have equal length */
    LC_CONSTRAINT_EQUAL_RADIUS,             /* Two circles have equal radius */

    /* Fix constraints (locks DOF) */
    LC_CONSTRAINT_FIX_POINT,                /* Point locked to specific position */
    LC_CONSTRAINT_FIX_LENGTH,               /* Line length locked to value */
    LC_CONSTRAINT_FIX_RADIUS,               /* Circle radius locked to value */

    LC_CONSTRAINT_TYPE_COUNT
} lc_constraint_type_t;
```

#### Constraint Data (lc_entity.h extension)

Update `lc_constraint_data_t` in `lc_entity.h`:

```c
/* Constraint data attached to LC_ENTITY_TYPE_CONSTRAINT entities.
 * Stores constraint type, referenced entities, parameters, and solver state. */
typedef struct lc_constraint_data_t
{
    lc_constraint_type_t constraint_type;

    /* Referenced entities (lines, circles, points as geometry entities).
     * Number of valid entities depends on constraint_type:
     *   - Distance point-point: entities[0], entities[1] (two points)
     *   - Distance point-line: entities[0] (point), entities[1] (line)
     *   - Angle line-line: entities[0], entities[1] (two lines)
     *   - Tangent circle-circle: entities[0], entities[1] (two circles)
     *   - Fix point: entities[0] (one point)
     *   - Horizontal line: entities[0] (one line)
     *   - etc.
     * Unused slots are set to LC_ENTITY_INVALID. */
    lc_entity_handle_t entities[4];

    /* Constraint parameter (meaning depends on constraint_type).
     * Examples:
     *   - Distance constraints: target distance in sketch units
     *   - Angle constraints: target angle in radians
     *   - Fix constraints: fixed value (position, length, radius)
     *   - Equality/parallel/perpendicular: unused (set to 0.0)
     */
    float value;

    /* Constraint weight (higher = more important in solver).
     * Default: 1.0. Fix constraints typically use higher weight (e.g., 10.0). */
    float weight;

    /* Error/residual: how much constraint is currently violated.
     * Updated during constraint evaluation.
     * Zero = satisfied, non-zero = violated.
     * Sign may indicate direction of violation. */
    float error;

    /* Constraint state flags */
    uint32_t state_flags;

} lc_constraint_data_t;

/* Constraint state flags */
#define LC_CONSTRAINT_STATE_SATISFIED      (1 << 0)  /* Error below tolerance */
#define LC_CONSTRAINT_STATE_VIOLATED       (1 << 1)  /* Error above tolerance */
#define LC_CONSTRAINT_STATE_REDUNDANT      (1 << 2)  /* Constraint is redundant */
#define LC_CONSTRAINT_STATE_CONFLICTING    (1 << 3)  /* Conflicts with other constraints */
```

#### Constraint Graph (libcad_internal.h extension)

```c
/* Constraint graph: tracks dependencies between entities and constraints.
 * Used for DOF analysis and solver ordering.
 * Structure is built on-demand and cached until entity/constraint changes. */

/* Graph node: represents a constrained entity (point, line, circle, etc.) */
typedef struct lc_constraint_graph_node_t
{
    lc_entity_handle_t entity;      /* Entity this node represents */
    lc_entity_type_t entity_type;   /* Type of entity (line, circle, etc.) */
    int dof;                         /* Degrees of freedom (2 for point, 4 for line, etc.) */
    int constrained_dof;             /* DOF constrained by constraints */
    uint32_t flags;                  /* Node flags (fixed, fully_constrained, etc.) */
} lc_constraint_graph_node_t;

/* Graph edge: represents a constraint connecting entities */
typedef struct lc_constraint_graph_edge_t
{
    lc_entity_handle_t constraint;  /* Constraint entity */
    lc_constraint_type_t type;      /* Constraint type */
    int dof_constrained;             /* How many DOF this constraint removes */
} lc_constraint_graph_edge_t;

/* Node flags */
#define LC_GRAPH_NODE_FIXED           (1 << 0)  /* Node is fixed by constraint */
#define LC_GRAPH_NODE_FULLY_CONSTRAINED (1 << 1) /* All DOF constrained */
#define LC_GRAPH_NODE_UNDER_CONSTRAINED (1 << 2) /* Some DOF unconstrained */
#define LC_GRAPH_NODE_OVER_CONSTRAINED  (1 << 3) /* More constraints than DOF */
```

### API Surface

#### Constraint Creation (lc_constraint.h)

```c
/* Create a constraint entity of the specified type.
 * entities[] contains handles to geometry entities (lines, circles, points).
 * num_entities must match the expected count for constraint_type.
 * value is the constraint parameter (distance, angle, etc.).
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create(lc_constraint_type_t type,
                                         const lc_entity_handle_t *entities,
                                         int num_entities,
                                         float value);

/* Destroy a constraint entity.
 * Wrapper around lc_entity_destroy() that also removes constraint from graph.
 * Returns true if successful, false if constraint handle is invalid. */
bool lc_constraint_destroy(lc_entity_handle_t constraint);

/* Get constraint data from a constraint entity.
 * Returns NULL if handle is invalid or entity is not a constraint.
 * Convenience wrapper around lc_entity_get_data(). */
lc_constraint_data_t* lc_constraint_get_data(lc_entity_handle_t constraint);

/* Set constraint parameter value (distance, angle, etc.).
 * Records undo command for property change.
 * Returns true if successful, false if constraint handle is invalid. */
bool lc_constraint_set_value(lc_entity_handle_t constraint, float value);

/* Set constraint weight (importance for solver).
 * Higher weight = constraint satisfied more precisely.
 * Returns true if successful, false if constraint handle is invalid. */
bool lc_constraint_set_weight(lc_entity_handle_t constraint, float weight);
```

#### Constraint Evaluation (lc_constraint.h)

```c
/* Evaluate a single constraint: compute error/residual.
 * Reads current entity geometry (line endpoints, circle center/radius, etc.)
 * and computes how much the constraint is violated.
 * Returns error value (0.0 = satisfied, non-zero = violated).
 * Also updates constraint_data->error field. */
float lc_constraint_evaluate(lc_entity_handle_t constraint);

/* Evaluate all constraints in a sketch.
 * Returns total error (sum of squared errors).
 * Updates error field for each constraint entity.
 * If out_max_error is non-NULL, stores maximum individual constraint error. */
float lc_constraint_evaluate_sketch(lc_entity_handle_t sketch, float *out_max_error);

/* Check if a constraint is satisfied (error below tolerance).
 * Tolerance is typically 1e-6 for distance constraints, 1e-4 for angle constraints.
 * Returns true if satisfied, false if violated. */
bool lc_constraint_is_satisfied(lc_entity_handle_t constraint, float tolerance);
```

#### Constraint Graph & DOF Analysis (lc_constraint.h)

```c
/* Build constraint graph for a sketch.
 * Analyzes all geometry entities and constraints in the sketch.
 * Detects over-constrained, under-constrained, and fully-constrained entities.
 * Graph is cached until entities or constraints are added/removed/modified.
 * Returns true if graph was built successfully. */
bool lc_constraint_build_graph(lc_entity_handle_t sketch);

/* Invalidate constraint graph (call when entities or constraints change).
 * Forces rebuild on next call to lc_constraint_build_graph(). */
void lc_constraint_invalidate_graph(lc_entity_handle_t sketch);

/* Get degrees of freedom analysis for a sketch.
 * Returns total unconstrained DOF (0 = fully constrained, >0 = under-constrained).
 * If negative, sketch is over-constrained. */
int lc_constraint_get_dof(lc_entity_handle_t sketch);

/* Check if sketch is fully constrained (all entities fully constrained).
 * Returns true if DOF = 0. */
bool lc_constraint_is_fully_constrained(lc_entity_handle_t sketch);

/* Check if sketch is over-constrained (conflicting constraints).
 * Returns true if DOF < 0. */
bool lc_constraint_is_over_constrained(lc_entity_handle_t sketch);

/* Enumerate all constraints affecting a specific entity.
 * Fills out_constraints array with up to max_count constraint handles.
 * Returns actual number of constraints found (may exceed max_count). */
size_t lc_constraint_get_entity_constraints(lc_entity_handle_t entity,
                                              lc_entity_handle_t *out_constraints,
                                              size_t max_count);
```

#### Public API Extensions (libcad.h)

```c
/* ---- Constraint System (Phase 3) ---- */

/* Create a distance constraint between two points.
 * distance_value is in sketch-local units.
 * Returns constraint handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t cad_constraint_distance_point_point(cad_ctx_t ctx,
                                                          cad_entity_t point1,
                                                          cad_entity_t point2,
                                                          float distance_value);

/* Create a distance constraint from a point to a line.
 * distance_value is perpendicular distance.
 * Returns constraint handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t cad_constraint_distance_point_line(cad_ctx_t ctx,
                                                         cad_entity_t point,
                                                         cad_entity_t line,
                                                         float distance_value);

/* Create an angle constraint between two lines.
 * angle_value is in radians.
 * Returns constraint handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t cad_constraint_angle(cad_ctx_t ctx,
                                          cad_entity_t line1,
                                          cad_entity_t line2,
                                          float angle_radians);

/* Create a coincident constraint (point on point, point on line, etc.).
 * Coincident constraints have no value parameter.
 * Returns constraint handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t cad_constraint_coincident(cad_ctx_t ctx,
                                                cad_entity_t entity1,
                                                cad_entity_t entity2);

/* Create a parallel constraint between two lines.
 * Returns constraint handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t cad_constraint_parallel(cad_ctx_t ctx,
                                              cad_entity_t line1,
                                              cad_entity_t line2);

/* Create a perpendicular constraint between two lines.
 * Returns constraint handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t cad_constraint_perpendicular(cad_ctx_t ctx,
                                                   cad_entity_t line1,
                                                   cad_entity_t line2);

/* Create a horizontal constraint on a line.
 * Returns constraint handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t cad_constraint_horizontal(cad_ctx_t ctx, cad_entity_t line);

/* Create a vertical constraint on a line.
 * Returns constraint handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t cad_constraint_vertical(cad_ctx_t ctx, cad_entity_t line);

/* Create a tangent constraint (line-circle or circle-circle).
 * Returns constraint handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t cad_constraint_tangent(cad_ctx_t ctx,
                                             cad_entity_t entity1,
                                             cad_entity_t entity2);

/* Create an equal length constraint between two line segments.
 * Returns constraint handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t cad_constraint_equal_length(cad_ctx_t ctx,
                                                  cad_entity_t line1,
                                                  cad_entity_t line2);

/* Create an equal radius constraint between two circles.
 * Returns constraint handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t cad_constraint_equal_radius(cad_ctx_t ctx,
                                                  cad_entity_t circle1,
                                                  cad_entity_t circle2);

/* Create a fix constraint that locks a point position.
 * Returns constraint handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t cad_constraint_fix_point(cad_ctx_t ctx, cad_entity_t point);

/* Delete a constraint entity.
 * Returns true if successful, false if constraint handle is invalid. */
EXPORT bool cad_constraint_delete(cad_ctx_t ctx, cad_entity_t constraint);

/* Query constraint status */
EXPORT bool cad_constraint_is_satisfied(cad_ctx_t ctx, cad_entity_t constraint);
EXPORT float cad_constraint_get_error(cad_ctx_t ctx, cad_entity_t constraint);

/* Query sketch constraint status */
EXPORT int cad_sketch_get_dof(cad_ctx_t ctx, cad_sketch_t sketch);
EXPORT bool cad_sketch_is_fully_constrained(cad_ctx_t ctx, cad_sketch_t sketch);
```

### Internal Interfaces

```c
/* In lc_constraint.c - called by public API wrappers in libcad.c */

/* Initialize constraint system (called once at startup) */
void lc_constraint_init(void);

/* Shutdown constraint system (called once at shutdown) */
void lc_constraint_shutdown(void);

/* Helper: get point position from entity (for constraint evaluation).
 * Points are stored as line endpoints, circle centers, etc.
 * This helper extracts the point position based on entity type.
 * Returns true if successful, false if entity is not a point-like entity. */
bool lc_constraint_get_point_position(lc_entity_handle_t entity, vec2 *out_pos);

/* Helper: get line start/end from entity (for constraint evaluation).
 * Returns true if successful, false if entity is not a line. */
bool lc_constraint_get_line_endpoints(lc_entity_handle_t entity,
                                       vec2 *out_start, vec2 *out_end);

/* Helper: get circle center/radius from entity (for constraint evaluation).
 * Returns true if successful, false if entity is not a circle. */
bool lc_constraint_get_circle_params(lc_entity_handle_t entity,
                                      vec2 *out_center, float *out_radius);
```

### Error Function Design

Each constraint type has an associated error function that computes how much the constraint is violated. Error functions should be:
- **Differentiable**: Smooth for gradient-based solvers
- **Zero at satisfaction**: Error = 0 when constraint is perfectly satisfied
- **Continuous**: No discontinuities (makes solver convergence easier)

#### Distance Constraints

**Point-to-Point Distance**

```c
/* Error function: e = |p1 - p2| - d_target
 * where p1, p2 are point positions, d_target is desired distance.
 * Zero when distance equals target.
 * Positive when distance > target, negative when distance < target. */
float error_distance_point_point(vec2 p1, vec2 p2, float d_target)
{
    float dx = p2[0] - p1[0];
    float dy = p2[1] - p1[1];
    float d_actual = sqrtf(dx*dx + dy*dy);
    return d_actual - d_target;
}
```

**Point-to-Line Distance**

```c
/* Error function: e = |dot(p - L0, N)| - d_target
 * where p is point, L0 is line origin, N is line normal (perpendicular).
 * Zero when perpendicular distance equals target. */
float error_distance_point_line(vec2 p, vec2 L0, vec2 L1, float d_target)
{
    /* Compute line direction and normal */
    vec2 dir = {L1[0] - L0[0], L1[1] - L0[1]};
    float len = sqrtf(dir[0]*dir[0] + dir[1]*dir[1]);
    vec2 norm = {-dir[1] / len, dir[0] / len};  /* Perpendicular */

    /* Compute signed distance */
    vec2 dp = {p[0] - L0[0], p[1] - L0[1]};
    float d_actual = dp[0]*norm[0] + dp[1]*norm[1];

    return fabsf(d_actual) - d_target;
}
```

#### Angle Constraints

**Line-to-Line Angle**

```c
/* Error function: e = angle(dir1, dir2) - theta_target
 * where dir1, dir2 are line direction vectors.
 * Use atan2 for signed angle in range [-pi, pi]. */
float error_angle_line_line(vec2 L1_start, vec2 L1_end,
                             vec2 L2_start, vec2 L2_end,
                             float theta_target)
{
    /* Compute direction vectors */
    vec2 dir1 = {L1_end[0] - L1_start[0], L1_end[1] - L1_start[1]};
    vec2 dir2 = {L2_end[0] - L2_start[0], L2_end[1] - L2_start[1]};

    /* Normalize */
    float len1 = sqrtf(dir1[0]*dir1[0] + dir1[1]*dir1[1]);
    float len2 = sqrtf(dir2[0]*dir2[0] + dir2[1]*dir2[1]);
    dir1[0] /= len1; dir1[1] /= len1;
    dir2[0] /= len2; dir2[1] /= len2;

    /* Compute angle using atan2 for full range */
    float dot = dir1[0]*dir2[0] + dir1[1]*dir2[1];
    float cross = dir1[0]*dir2[1] - dir1[1]*dir2[0];
    float theta_actual = atan2f(cross, dot);

    return theta_actual - theta_target;
}
```

#### Coincident Constraints

**Point-on-Point**

```c
/* Error function: e = |p1 - p2|^2
 * Squared distance for smoothness (avoids sqrt).
 * Zero when points coincide. */
float error_coincident_point_point(vec2 p1, vec2 p2)
{
    float dx = p2[0] - p1[0];
    float dy = p2[1] - p1[1];
    return dx*dx + dy*dy;  /* Squared error */
}
```

**Point-on-Line**

```c
/* Error function: e = dot(p - L0, N)^2
 * Squared perpendicular distance.
 * Zero when point lies on line. */
float error_coincident_point_line(vec2 p, vec2 L0, vec2 L1)
{
    /* Compute line normal */
    vec2 dir = {L1[0] - L0[0], L1[1] - L0[1]};
    float len = sqrtf(dir[0]*dir[0] + dir[1]*dir[1]);
    vec2 norm = {-dir[1] / len, dir[0] / len};

    /* Compute signed distance */
    vec2 dp = {p[0] - L0[0], p[1] - L0[1]};
    float d = dp[0]*norm[0] + dp[1]*norm[1];

    return d*d;  /* Squared error */
}
```

**Point-on-Circle**

```c
/* Error function: e = (|p - C| - r)^2
 * where C is circle center, r is radius.
 * Zero when point lies on circle. */
float error_coincident_point_circle(vec2 p, vec2 C, float r)
{
    float dx = p[0] - C[0];
    float dy = p[1] - C[1];
    float dist = sqrtf(dx*dx + dy*dy);
    float err = dist - r;
    return err*err;  /* Squared error */
}
```

#### Parallel and Perpendicular Constraints

**Parallel Lines**

```c
/* Error function: e = |cross(dir1, dir2)|^2
 * Zero when directions are parallel (cross product = 0). */
float error_parallel(vec2 L1_start, vec2 L1_end, vec2 L2_start, vec2 L2_end)
{
    vec2 dir1 = {L1_end[0] - L1_start[0], L1_end[1] - L1_start[1]};
    vec2 dir2 = {L2_end[0] - L2_start[0], L2_end[1] - L2_start[1]};

    /* Normalize */
    float len1 = sqrtf(dir1[0]*dir1[0] + dir1[1]*dir1[1]);
    float len2 = sqrtf(dir2[0]*dir2[0] + dir2[1]*dir2[1]);
    dir1[0] /= len1; dir1[1] /= len1;
    dir2[0] /= len2; dir2[1] /= len2;

    /* Cross product in 2D: dir1.x * dir2.y - dir1.y * dir2.x */
    float cross = dir1[0]*dir2[1] - dir1[1]*dir2[0];
    return cross*cross;  /* Squared error */
}
```

**Perpendicular Lines**

```c
/* Error function: e = dot(dir1, dir2)^2
 * Zero when directions are perpendicular (dot product = 0). */
float error_perpendicular(vec2 L1_start, vec2 L1_end, vec2 L2_start, vec2 L2_end)
{
    vec2 dir1 = {L1_end[0] - L1_start[0], L1_end[1] - L1_start[1]};
    vec2 dir2 = {L2_end[0] - L2_start[0], L2_end[1] - L2_start[1]};

    /* Normalize */
    float len1 = sqrtf(dir1[0]*dir1[0] + dir1[1]*dir1[1]);
    float len2 = sqrtf(dir2[0]*dir2[0] + dir2[1]*dir2[1]);
    dir1[0] /= len1; dir1[1] /= len1;
    dir2[0] /= len2; dir2[1] /= len2;

    /* Dot product */
    float dot = dir1[0]*dir2[0] + dir1[1]*dir2[1];
    return dot*dot;  /* Squared error */
}
```

#### Horizontal and Vertical Constraints

**Horizontal Line**

```c
/* Error function: e = (L_end.y - L_start.y)^2
 * Zero when line is horizontal (no vertical component). */
float error_horizontal(vec2 L_start, vec2 L_end)
{
    float dy = L_end[1] - L_start[1];
    return dy*dy;  /* Squared error */
}
```

**Vertical Line**

```c
/* Error function: e = (L_end.x - L_start.x)^2
 * Zero when line is vertical (no horizontal component). */
float error_vertical(vec2 L_start, vec2 L_end)
{
    float dx = L_end[0] - L_start[0];
    return dx*dx;  /* Squared error */
}
```

#### Tangency Constraints

**Line Tangent to Circle**

```c
/* Error function: e = (|dot(C - L0, N)| - r)^2
 * where C is circle center, L0 is line origin, N is line normal, r is radius.
 * Zero when distance from circle center to line equals radius. */
float error_tangent_line_circle(vec2 L_start, vec2 L_end, vec2 C, float r)
{
    /* Compute line normal */
    vec2 dir = {L_end[0] - L_start[0], L_end[1] - L_start[1]};
    float len = sqrtf(dir[0]*dir[0] + dir[1]*dir[1]);
    vec2 norm = {-dir[1] / len, dir[0] / len};

    /* Distance from circle center to line */
    vec2 dC = {C[0] - L_start[0], C[1] - L_start[1]};
    float dist = fabsf(dC[0]*norm[0] + dC[1]*norm[1]);

    float err = dist - r;
    return err*err;  /* Squared error */
}
```

**Circle Tangent to Circle**

```c
/* Error function: e = (|C1 - C2| - (r1 + r2))^2 for external tangency
 * or e = (|C1 - C2| - |r1 - r2|)^2 for internal tangency.
 * Use external tangency by default. */
float error_tangent_circle_circle(vec2 C1, float r1, vec2 C2, float r2)
{
    float dx = C2[0] - C1[0];
    float dy = C2[1] - C1[1];
    float dist = sqrtf(dx*dx + dy*dy);

    /* External tangency: distance = r1 + r2 */
    float target_dist = r1 + r2;
    float err = dist - target_dist;
    return err*err;  /* Squared error */
}
```

#### Equality Constraints

**Equal Length**

```c
/* Error function: e = (|L1_end - L1_start| - |L2_end - L2_start|)^2
 * Zero when line lengths are equal. */
float error_equal_length(vec2 L1_start, vec2 L1_end, vec2 L2_start, vec2 L2_end)
{
    float dx1 = L1_end[0] - L1_start[0];
    float dy1 = L1_end[1] - L1_start[1];
    float len1 = sqrtf(dx1*dx1 + dy1*dy1);

    float dx2 = L2_end[0] - L2_start[0];
    float dy2 = L2_end[1] - L2_start[1];
    float len2 = sqrtf(dx2*dx2 + dy2*dy2);

    float err = len1 - len2;
    return err*err;  /* Squared error */
}
```

**Equal Radius**

```c
/* Error function: e = (r1 - r2)^2
 * Zero when radii are equal. */
float error_equal_radius(float r1, float r2)
{
    float err = r1 - r2;
    return err*err;  /* Squared error */
}
```

#### Fix Constraints

**Fix Point Position**

```c
/* Error function: e = |p - p_fixed|^2
 * Zero when point is at fixed position. */
float error_fix_point(vec2 p, vec2 p_fixed)
{
    float dx = p[0] - p_fixed[0];
    float dy = p[1] - p_fixed[1];
    return dx*dx + dy*dy;  /* Squared error */
}
```

### Data Flow

#### Constraint Creation Flow

```
User calls cad_constraint_distance_point_point(p1, p2, distance)
  ↓
libcad.c → lc_constraint_create(LC_CONSTRAINT_DISTANCE_POINT_POINT, {p1, p2}, 2, distance)
  ↓
lc_constraint.c:
  1. Validate entity handles (p1, p2 exist and are valid point-like entities)
  2. Create constraint entity via lc_entity_create(LC_ENTITY_TYPE_CONSTRAINT)
  3. Allocate lc_constraint_data_t
  4. Initialize constraint data:
     - constraint_type = LC_CONSTRAINT_DISTANCE_POINT_POINT
     - entities[0] = p1, entities[1] = p2, entities[2] = entities[3] = INVALID
     - value = distance
     - weight = 1.0 (default)
     - error = 0.0 (will be computed on first evaluation)
     - state_flags = 0
  5. Attach constraint data to entity slot
  6. Add constraint to parent sketch's child list
  ↓
lc_undo.c:
  7. Record LC_COMMAND_CREATE_ENTITY with constraint snapshot
  ↓
lc_constraint.c:
  8. Invalidate constraint graph (rebuild needed)
  ↓
lc_document.c:
  9. Mark document dirty
  ↓
Return constraint handle to user
```

#### Constraint Evaluation Flow

```
User calls cad_sketch_get_dof(sketch) or solver invokes evaluation
  ↓
libcad.c → lc_constraint_evaluate_sketch(sketch, &max_error)
  ↓
lc_constraint.c:
  1. Enumerate all constraint entities in sketch (walk child list)
  2. For each constraint:
     a. Get constraint data (lc_constraint_get_data())
     b. Read current geometry entity positions:
        - For point-point: read both point positions via lc_constraint_get_point_position()
        - For line-line angle: read both line endpoints via lc_constraint_get_line_endpoints()
        - etc.
     c. Call appropriate error function based on constraint_type:
        - Distance point-point: error_distance_point_point(p1, p2, value)
        - Angle line-line: error_angle_line_line(L1_start, L1_end, L2_start, L2_end, value)
        - etc.
     d. Store error in constraint_data->error field
     e. Update state_flags:
        - Set LC_CONSTRAINT_STATE_SATISFIED if |error| < tolerance
        - Set LC_CONSTRAINT_STATE_VIOLATED if |error| >= tolerance
     f. Accumulate total_error += error * error * weight
     g. Track max_error = max(max_error, |error|)
  3. Return total_error
  ↓
Return total error and max error to caller
```

#### Constraint Graph Build Flow

```
User triggers DOF analysis (e.g., calls cad_sketch_is_fully_constrained())
  ↓
libcad.c → lc_constraint_get_dof(sketch)
  ↓
lc_constraint.c → lc_constraint_build_graph(sketch)
  ↓
lc_constraint.c:
  1. Check if graph is cached and valid (invalidate_flag == false)
     - If valid, return cached result
  2. Clear graph data structures
  3. Enumerate all geometry entities in sketch:
     - For each entity, create a graph node:
       - entity_handle
       - entity_type (line, circle, point)
       - dof = compute DOF for entity type:
         - Point: 2 DOF (x, y)
         - Line: 4 DOF (start_x, start_y, end_x, end_y)
         - Circle: 3 DOF (center_x, center_y, radius)
       - constrained_dof = 0 (will be computed)
       - flags = 0
  4. Enumerate all constraint entities in sketch:
     - For each constraint, create a graph edge:
       - constraint_handle
       - constraint_type
       - dof_constrained = compute DOF removed by constraint:
         - Distance point-point: 1 DOF (distance fixed)
         - Angle line-line: 1 DOF (angle fixed)
         - Coincident point-point: 2 DOF (both x and y fixed)
         - Horizontal line: 1 DOF (y-direction fixed)
         - Fix point: 2 DOF (position locked)
       - Link edge to affected nodes (entities referenced by constraint)
  5. Compute constrained_dof for each node:
     - For each edge connected to node, add edge.dof_constrained
  6. Compute node flags:
     - If constrained_dof >= dof: set LC_GRAPH_NODE_FULLY_CONSTRAINED
     - If constrained_dof < dof: set LC_GRAPH_NODE_UNDER_CONSTRAINED
     - If constrained_dof > dof: set LC_GRAPH_NODE_OVER_CONSTRAINED
  7. Compute total sketch DOF:
     - total_dof = sum(node.dof) - sum(edge.dof_constrained)
     - Positive: under-constrained
     - Zero: fully constrained
     - Negative: over-constrained
  8. Cache graph and mark valid
  ↓
Return total_dof
```

### Undo/Redo Integration

Constraints are entities, so they integrate naturally with the existing undo system:

**Constraint Creation**
- `lc_entity_create(LC_ENTITY_TYPE_CONSTRAINT)` records `LC_COMMAND_CREATE_ENTITY`
- Undo: `lc_entity_destroy(constraint)` removes constraint
- Redo: Restore constraint entity from snapshot

**Constraint Deletion**
- `lc_constraint_destroy(constraint)` records `LC_COMMAND_DELETE_ENTITY`
- Undo: Restore constraint entity from snapshot
- Redo: `lc_entity_destroy(constraint)` removes constraint again

**Constraint Parameter Modification**
- `lc_constraint_set_value(constraint, new_value)` records `LC_COMMAND_MODIFY_PROPERTY`
- Undo: Set value back to old_value
- Redo: Set value back to new_value

**Invalidation on Undo/Redo**
- After any undo/redo that affects entities or constraints, call `lc_constraint_invalidate_graph(sketch)` to force graph rebuild

### Platform Considerations

**Cross-Platform Compatibility**

1. **C99 Compliance**: All code uses strict C99. No C++ features, no `//` comments.
2. **Math Functions**: Use `<math.h>` (sqrtf, atan2f, fabsf) — available on all platforms.
3. **No External Dependencies**: No solver library yet. Phase 5 will add solver.
4. **Endianness**: Constraint data is stored in entities; serialization uses JSON (text-based, no endianness issues).

**Emscripten Considerations**

1. **No Threads**: DOF analysis and constraint evaluation are single-threaded (fast enough for typical sketches).
2. **Memory**: Constraint graph is ~16 bytes per edge, ~24 bytes per node. Typical sketch (100 constraints) uses ~2KB for graph. Acceptable for web.

**Build System**

Add to CMakeLists.txt:

```cmake
target_sources(libcad PRIVATE
    lib/lc_constraint.c
)
```

No new dependencies. Constraint module uses only:
- stdlib.h (malloc, free)
- math.h (sqrtf, atan2f, fabsf, etc.)
- cglm (vec2 types, already used)
- lc_entity (entity system, already exists)

## Alternatives Considered

### 1. Constraint Storage: Separate Array vs Entity System

**Entity System (CHOSEN)**

Pros:
- Reuses existing handle-based storage (generational indices)
- Constraints are first-class citizens (can be named, selected, hidden, etc.)
- Undo/redo integration is automatic (constraints are entities)
- Document tree includes constraints (easier to serialize)

Cons:
- Entity slots are larger (~80 bytes) than minimal constraint storage (~32 bytes)
- Slight indirection overhead (handle lookup)

**Separate Constraint Array**

Pros:
- Minimal memory overhead (tight packing)
- Faster enumeration (contiguous array)

Cons:
- Duplicate handle system (entity handles + constraint IDs)
- Undo/redo requires separate implementation
- Harder to integrate with document serialization
- Constraints not part of entity tree (orphaned data)

**Verdict**: Entity system integration is cleaner and leverages existing infrastructure.

### 2. Error Function: Squared vs Absolute

**Squared Error (CHOSEN)**

Pros:
- Smooth (differentiable everywhere)
- Matches least-squares minimization (standard for constraint solvers)
- Penalizes large errors more heavily (good for convergence)

Cons:
- Squared units can be confusing (distance^2 vs distance)
- Large errors can cause numerical issues (overflow)

**Absolute Error**

Pros:
- Intuitive units (distance in mm, angle in radians)
- Avoids overflow for large errors

Cons:
- Not differentiable at zero (kink in error function)
- Least-absolute-deviations solvers are more complex

**Verdict**: Squared error is standard for constraint solvers and works well with gradient-based methods.

### 3. Constraint Graph: Adjacency List vs Adjacency Matrix

**Adjacency List (CHOSEN)**

Pros:
- Memory-efficient for sparse graphs (typical sketches have sparse constraints)
- Fast enumeration of constraints for a specific entity
- Easy to add/remove constraints dynamically

Cons:
- Slightly slower lookup (O(k) where k = constraints per entity, typically < 10)

**Adjacency Matrix**

Pros:
- O(1) lookup: "are entities A and B constrained?"
- Simple implementation

Cons:
- Memory explosion: O(n^2) where n = entities (1000 entities = 1MB matrix)
- Wasteful for sparse graphs
- Harder to enumerate constraints for a specific entity

**Verdict**: Adjacency list matches CAD constraint graph structure (sparse, dynamic).

### 4. DOF Analysis: Exact vs Heuristic

**Exact DOF Counting (CHOSEN)**

Pros:
- Accurate detection of over-constrained systems
- Works for simple constraint types (distance, angle, etc.)

Cons:
- Does not detect all redundancy (e.g., three collinear points with distance constraints)
- Complex constraints (tangent, etc.) have subtle DOF behavior

**Heuristic DOF (Graph-Based)**

Pros:
- Faster (no detailed analysis)
- Works for complex constraint types

Cons:
- Can miss over-constrained systems (false negatives)
- Can report false over-constraints (false positives)

**Verdict**: Exact DOF counting is sufficient for Phase 3. Phase 5 solver will detect true redundancy during solving.

## Implementation Notes

### Suggested Implementation Order

**Phase 3A: Constraint Data Structures (1 day)**

1. Update `lc_entity.h` with full `lc_constraint_data_t` definition
2. Create `lib/lc_constraint.h` with constraint type enum and API prototypes
3. Create `lib/lc_constraint.c` with stub implementations
4. Add to CMakeLists.txt
5. Test: Compiles cleanly, stubs callable

**Phase 3B: Constraint Creation (1 day)**

1. Implement `lc_constraint_create()` with entity validation
2. Implement `lc_constraint_destroy()` wrapper
3. Implement `lc_constraint_get_data()` convenience function
4. Implement `lc_constraint_set_value()` and `lc_constraint_set_weight()`
5. Wire up public API stubs in `libcad.c` (call lc_constraint_create from cad_constraint_distance_point_point, etc.)
6. Test: Create constraints via API, verify entity slots allocated correctly

**Phase 3C: Error Functions (2-3 days)**

1. Implement geometry query helpers:
   - `lc_constraint_get_point_position()`
   - `lc_constraint_get_line_endpoints()`
   - `lc_constraint_get_circle_params()`
2. Implement error functions for each constraint type:
   - Distance constraints (point-point, point-line, line-line)
   - Angle constraints (line-line)
   - Coincident constraints (point-point, point-line, point-circle)
   - Parallel/perpendicular (lines)
   - Horizontal/vertical (single line)
   - Tangent (line-circle, circle-circle)
   - Equal (length, radius)
   - Fix (point, length, radius)
3. Implement `lc_constraint_evaluate()` (switch on constraint_type, call error function)
4. Implement `lc_constraint_evaluate_sketch()` (enumerate constraints, accumulate errors)
5. Test: Create constrained sketch, evaluate, verify error values

**Phase 3D: Constraint Graph & DOF Analysis (2-3 days)**

1. Define graph data structures in `libcad_internal.h`
2. Implement `lc_constraint_build_graph()`:
   - Enumerate geometry entities, create nodes with DOF counts
   - Enumerate constraints, create edges with DOF removed
   - Compute constrained_dof for each node
3. Implement `lc_constraint_get_dof()` (sum total DOF)
4. Implement `lc_constraint_is_fully_constrained()` and `lc_constraint_is_over_constrained()`
5. Implement `lc_constraint_get_entity_constraints()` (enumerate constraints affecting entity)
6. Implement `lc_constraint_invalidate_graph()` (clear cache)
7. Test: Create sketch with various constraint configurations, verify DOF counts

**Phase 3E: Public API Integration (1 day)**

1. Wire up all public API stubs in `libcad.c`:
   - `cad_constraint_distance_point_point()` → `lc_constraint_create()`
   - `cad_constraint_angle()` → `lc_constraint_create()`
   - etc.
2. Implement query functions:
   - `cad_constraint_is_satisfied()` → `lc_constraint_is_satisfied()`
   - `cad_constraint_get_error()` → `lc_constraint_get_data()->error`
   - `cad_sketch_get_dof()` → `lc_constraint_get_dof()`
3. Test: Call public API from test harness, verify all functions work

**Phase 3F: Undo/Redo Integration (1 day)**

1. Verify constraint creation/deletion records undo commands (should work automatically)
2. Verify `lc_constraint_set_value()` records property modification command
3. Add `lc_constraint_invalidate_graph()` calls after undo/redo
4. Test: Create constraint, undo, redo; modify value, undo, redo

**Phase 3G: Constraint Visualization (Optional, 1 day)**

1. Update `lc_canvas.c` to render constraints:
   - Distance constraints: draw dimension line with text label
   - Angle constraints: draw arc with angle label
   - Coincident constraints: draw small icon at coincident point
   - Parallel/perpendicular: draw parallel lines icon or right angle icon
2. Highlight violated constraints in red, satisfied constraints in green
3. Test: Create constrained sketch, verify constraints render correctly

**Total Estimated Effort: 8-12 days**

### Known Risks and Open Questions

**Risk: Point References**

Current entity system stores lines, circles, rects. Points are implicit (line endpoints, circle centers). Constraint system needs point handles for point-point distance, fix point, etc.

**Mitigation Options**:
1. Add `LC_ENTITY_TYPE_POINT` geometry entity type (explicit points)
2. Use composite handles: encode "line start point" as `(line_handle, point_index)`
3. Add point extraction API: `lc_constraint_get_point_from_geometry(line_handle, point_index)`

**Recommendation**: Option 3 is simplest for Phase 3. Add explicit point entities in Phase 4 (B-Rep vertices).

**Risk: Constraint Satisfaction Tolerance**

Different constraint types have different natural units (distance in mm, angle in radians). What tolerance should be used for "satisfied" check?

**Mitigation**: Define per-constraint-type tolerances:
- Distance constraints: 1e-6 (sub-micron accuracy)
- Angle constraints: 1e-4 radians (~0.006 degrees)
- Coincident constraints: 1e-8 (squared distance, very tight)

**Recommendation**: Expose tolerance as parameter in `lc_constraint_is_satisfied()`.

**Question: Constraint Ordering for Solver**

Solver (Phase 5) needs to solve constraints in dependency order. Should Phase 3 implement topological sort of constraint graph?

**Recommendation**: Defer to Phase 5. Phase 3 provides graph structure; Phase 5 solver handles ordering.

**Question: Thread Safety**

Should constraint evaluation be thread-safe (for multi-threaded solver)?

**Recommendation**: No. Phase 3 is single-threaded. Phase 5 solver can parallelize constraint evaluation using read-only access (no writes during evaluation).

### Dependencies on Existing Code

**Requires from Phase 2**

- `lc_entity` system (entity creation, handles, tree traversal)
- `lc_document` system (metadata, selection)
- `lc_undo` system (undo/redo commands)
- `LC_ENTITY_TYPE_CONSTRAINT` enum value (already defined in `lc_entity.h`)
- `lc_constraint_data_t` struct (stub exists, needs full implementation)

**Integrates with**

- `lc_entity.c` - constraint creation uses `lc_entity_create(LC_ENTITY_TYPE_CONSTRAINT)`
- `libcad.c` - public API wrappers call `lc_constraint_*` functions
- `lc_undo.c` - constraint modification records property change commands

**No changes required to**

- `lc_draw.c` - rendering (constraint visualization is optional)
- `lc_scene.c` - 3D scene (constraints are 2D sketch-only)
- `lc_gpu.c` - GPU resources (constraints are CPU-side)

### Testing Strategy

**Unit Tests (Manual; test harness planned)**

1. **Constraint Creation**
   - Create each constraint type, verify entity slot allocated
   - Verify constraint data initialized correctly
   - Verify constraint added to sketch child list

2. **Error Functions**
   - Create two points 10 units apart, add distance constraint with target=10, verify error=0
   - Move one point to 12 units away, verify error=2
   - Create two lines at 90 degrees, add perpendicular constraint, verify error=0
   - Rotate one line to 85 degrees, verify error > 0

3. **DOF Analysis**
   - Single unconstrained line: 4 DOF
   - Line with horizontal constraint: 3 DOF
   - Line with horizontal + fix start point: 1 DOF (end point x-position free)
   - Line with horizontal + fix start point + fix length: 0 DOF (fully constrained)
   - Over-constrained: line with horizontal + vertical (impossible, DOF < 0)

4. **Constraint Graph**
   - Create sketch with 10 lines, 20 constraints
   - Build graph, verify node count = 10, edge count = 20
   - Enumerate constraints for one line, verify correct subset returned

5. **Undo/Redo**
   - Create constraint, undo, verify constraint gone
   - Create constraint, undo, redo, verify constraint restored
   - Modify constraint value, undo, verify old value restored

**Integration Tests**

1. **Sketch with Constraints**
   - Create rectangle (4 lines)
   - Add horizontal constraints to top/bottom edges
   - Add vertical constraints to left/right edges
   - Add distance constraint: width = 100
   - Add distance constraint: height = 50
   - Verify DOF = 2 (position free, size/shape constrained)
   - Fix bottom-left corner position
   - Verify DOF = 0 (fully constrained)

2. **Constraint Conflict Detection**
   - Create line
   - Add horizontal constraint
   - Add vertical constraint
   - Verify over-constrained (DOF < 0)
   - Verify both constraints show conflicting state

**Performance Tests**

1. Create 1000 constraints, measure graph build time (should be < 100ms)
2. Evaluate 1000 constraints, measure time (should be < 10ms)
3. Enumerate constraints 10,000 times, measure time (should be < 1ms per call)

## Summary

This architecture provides a comprehensive constraint system for Phase 3:

- **Entity-based storage**: Constraints are first-class entities with handles, undo/redo, serialization
- **Rich constraint types**: Distance, angle, coincident, parallel, perpendicular, horizontal, vertical, tangent, equal, fix
- **Differentiable error functions**: Smooth, squared-error formulations for gradient-based solvers
- **Constraint graph & DOF analysis**: Detects under-constrained, fully-constrained, and over-constrained sketches
- **Undo/redo integration**: Automatic via entity system
- **Solver-ready**: Error functions and graph structure prepare for Phase 5 numerical solver

The implementation is pure C99, cross-platform, and integrates cleanly with Phase 2's entity system. Estimated effort is 8-12 days for a complete, tested implementation.

After Phase 3, libcad will have a full parametric constraint system ready for solver integration (Phase 5), enabling constraint-driven sketch modeling and generative design workflows.
