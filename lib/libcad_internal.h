/***************************************************************
**
** libcad Header File
**
** File         :  libcad_internal.h
** Module       :  libcad (internal)
** Author       :  SH
** Created      :  2026-01-28 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Shared internal types for libcad modules.
**                 Not part of the public API.
**
***************************************************************/

#ifndef LIBCAD_INTERNAL_H
#define LIBCAD_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <cglm/cglm.h>
#include <stdint.h>
#include <stdbool.h>
#include "lc_entity.h"

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* Rendering context passed between modules during a frame.
 * Contains the view-projection transform, viewport dimensions,
 * and canvas plane parameters needed by all render functions. */
typedef struct {
    mat4 view_projection;       /* world -> clip space */
    mat4 view_projection_inv;   /* clip space -> world (for unprojection) */
    int viewport_width;
    int viewport_height;

    /* canvas (2D sketch plane) parameters */
    vec2 canvas_origin;         /* world position of 2D plane origin */
    vec3 canvas_normal;         /* plane normal (typically z-axis) */
    float canvas_zoom;          /* current zoom level */
} lc_render_context_t;

/* Constraint graph data structures for DOF analysis.
 * Graph is built on-demand and cached per sketch. */

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
#define LC_GRAPH_NODE_FIXED             (1 << 0)  /* Node is fixed by constraint */
#define LC_GRAPH_NODE_UNDER_CONSTRAINED (1 << 1)  /* Node has fewer constraints than DOF */
#define LC_GRAPH_NODE_FULLY_CONSTRAINED (1 << 2)  /* Node is fully constrained (DOF == constrained_dof) */
#define LC_GRAPH_NODE_OVER_CONSTRAINED  (1 << 3)  /* Node has more constraints than DOF */

/* Constraint graph for a sketch.
 * Stores nodes (entities) and edges (constraints) for DOF analysis. */
typedef struct lc_constraint_graph_t
{
    lc_entity_handle_t sketch;          /* Sketch this graph belongs to */

    lc_constraint_graph_node_t *nodes;  /* Dynamic array of nodes */
    int node_count;
    int node_capacity;

    lc_constraint_graph_edge_t *edges;  /* Dynamic array of edges */
    int edge_count;
    int edge_capacity;

    int total_dof;                      /* Total degrees of freedom */
    int total_constrained_dof;          /* Total DOF constrained */

    bool valid;                         /* Graph is up-to-date */
} lc_constraint_graph_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/


#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_INTERNAL_H */
