/***************************************************************
**
** libcad Implementation File
**
** File         :  lc_constraint.c
** Module       :  libcad (constraint system)
** Author       :  SH
** Created      :  2026-01-29 (YYYY-MM-DD)
** License      :  MIT
** Description  :  2D parametric constraint system implementation.
**                 Provides constraint creation, validation, and
**                 error evaluation for sketch entities.
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_constraint.h"
#include "lc_entity.h"
#include "libcad_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/* Default constraint weight */
#define LC_CONSTRAINT_DEFAULT_WEIGHT 1.0f

/* Maximum number of sketches with cached graphs */
#define MAX_CACHED_GRAPHS 16

/* Initial capacity for graph nodes and edges */
#define INITIAL_NODE_CAPACITY 32
#define INITIAL_EDGE_CAPACITY 64

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static bool g_initialized = false;

/* Cached constraint graphs (one per sketch) */
static lc_constraint_graph_t g_graphs[MAX_CACHED_GRAPHS];
static int g_graph_count = 0;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static lc_entity_handle_t lc_constraint_create_internal(
    lc_constraint_type_t type,
    const lc_entity_handle_t *entities,
    int num_entities,
    float value);

static bool lc_constraint_validate_entities(
    lc_constraint_type_t type,
    const lc_entity_handle_t *entities,
    int num_entities);

/* Geometry query helpers */
static bool lc_constraint_get_point_position(lc_entity_handle_t entity, vec2 out_pos);
static bool lc_constraint_get_line_endpoints(lc_entity_handle_t entity, vec2 out_start, vec2 out_end);
static bool lc_constraint_get_circle_params(lc_entity_handle_t entity, vec2 out_center, float *out_radius);

/* Error computation functions */
static float compute_error_distance_point_point(vec2 p1, vec2 p2, float target_distance);
static float compute_error_distance_point_line(vec2 p, vec2 L0, vec2 L1, float target_distance);
static float compute_error_angle_line_line(vec2 L1_start, vec2 L1_end, vec2 L2_start, vec2 L2_end, float target_angle);
static float compute_error_coincident_point_point(vec2 p1, vec2 p2);
static float compute_error_coincident_point_line(vec2 p, vec2 L0, vec2 L1);
static float compute_error_coincident_point_circle(vec2 p, vec2 C, float r);
static float compute_error_parallel(vec2 L1_start, vec2 L1_end, vec2 L2_start, vec2 L2_end);
static float compute_error_perpendicular(vec2 L1_start, vec2 L1_end, vec2 L2_start, vec2 L2_end);
static float compute_error_horizontal(vec2 L_start, vec2 L_end);
static float compute_error_vertical(vec2 L_start, vec2 L_end);
static float compute_error_tangent_line_circle(vec2 L_start, vec2 L_end, vec2 C, float r);
static float compute_error_tangent_circle_circle(vec2 C1, float r1, vec2 C2, float r2);
static float compute_error_equal_length(vec2 L1_start, vec2 L1_end, vec2 L2_start, vec2 L2_end);
static float compute_error_equal_radius(float r1, float r2);
static float compute_error_fix_point(vec2 p, vec2 p_fixed);

/* Graph management helpers */
static lc_constraint_graph_t* find_graph(lc_entity_handle_t sketch);
static lc_constraint_graph_t* create_graph(lc_entity_handle_t sketch);
static void free_graph(lc_constraint_graph_t *graph);
static int compute_entity_dof(lc_entity_type_t type);
static int compute_constraint_dof(lc_constraint_type_t type);
static void add_graph_node(lc_constraint_graph_t *graph, lc_entity_handle_t entity);
static void add_graph_edge(lc_constraint_graph_t *graph, lc_entity_handle_t constraint);
static void analyze_graph_dof(lc_constraint_graph_t *graph);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void lc_constraint_init(void)
{
    if (g_initialized)
    {
        return;
    }

    g_initialized = true;
    printf("[lc_constraint] Constraint system initialized\n");
}

void lc_constraint_shutdown(void)
{
    if (!g_initialized)
    {
        return;
    }

    /* Free all cached graphs */
    int i;
    for (i = 0; i < g_graph_count; i++)
    {
        free_graph(&g_graphs[i]);
    }
    g_graph_count = 0;

    g_initialized = false;
    printf("[lc_constraint] Constraint system shutdown\n");
}

lc_entity_handle_t lc_constraint_create_distance_point_point(
    lc_entity_handle_t point1,
    lc_entity_handle_t point2,
    float distance_value)
{
    lc_entity_handle_t entities[2] = {point1, point2};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_DISTANCE_POINT_POINT,
        entities,
        2,
        distance_value);
}

lc_entity_handle_t lc_constraint_create_distance_point_line(
    lc_entity_handle_t point,
    lc_entity_handle_t line,
    float distance_value)
{
    lc_entity_handle_t entities[2] = {point, line};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_DISTANCE_POINT_LINE,
        entities,
        2,
        distance_value);
}

lc_entity_handle_t lc_constraint_create_angle_line_line(
    lc_entity_handle_t line1,
    lc_entity_handle_t line2,
    float angle_radians)
{
    lc_entity_handle_t entities[2] = {line1, line2};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_ANGLE_LINE_LINE,
        entities,
        2,
        angle_radians);
}

lc_entity_handle_t lc_constraint_create_coincident_point_point(
    lc_entity_handle_t point1,
    lc_entity_handle_t point2)
{
    lc_entity_handle_t entities[2] = {point1, point2};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_COINCIDENT_POINT_POINT,
        entities,
        2,
        0.0f);
}

lc_entity_handle_t lc_constraint_create_coincident_point_line(
    lc_entity_handle_t point,
    lc_entity_handle_t line)
{
    lc_entity_handle_t entities[2] = {point, line};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_COINCIDENT_POINT_LINE,
        entities,
        2,
        0.0f);
}

lc_entity_handle_t lc_constraint_create_coincident_point_circle(
    lc_entity_handle_t point,
    lc_entity_handle_t circle)
{
    lc_entity_handle_t entities[2] = {point, circle};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_COINCIDENT_POINT_CIRCLE,
        entities,
        2,
        0.0f);
}

lc_entity_handle_t lc_constraint_create_parallel(
    lc_entity_handle_t line1,
    lc_entity_handle_t line2)
{
    lc_entity_handle_t entities[2] = {line1, line2};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_PARALLEL,
        entities,
        2,
        0.0f);
}

lc_entity_handle_t lc_constraint_create_perpendicular(
    lc_entity_handle_t line1,
    lc_entity_handle_t line2)
{
    lc_entity_handle_t entities[2] = {line1, line2};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_PERPENDICULAR,
        entities,
        2,
        0.0f);
}

lc_entity_handle_t lc_constraint_create_horizontal(
    lc_entity_handle_t line)
{
    lc_entity_handle_t entities[1] = {line};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_HORIZONTAL,
        entities,
        1,
        0.0f);
}

lc_entity_handle_t lc_constraint_create_vertical(
    lc_entity_handle_t line)
{
    lc_entity_handle_t entities[1] = {line};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_VERTICAL,
        entities,
        1,
        0.0f);
}

lc_entity_handle_t lc_constraint_create_tangent_line_circle(
    lc_entity_handle_t line,
    lc_entity_handle_t circle)
{
    lc_entity_handle_t entities[2] = {line, circle};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_TANGENT_LINE_CIRCLE,
        entities,
        2,
        0.0f);
}

lc_entity_handle_t lc_constraint_create_tangent_circle_circle(
    lc_entity_handle_t circle1,
    lc_entity_handle_t circle2)
{
    lc_entity_handle_t entities[2] = {circle1, circle2};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_TANGENT_CIRCLE_CIRCLE,
        entities,
        2,
        0.0f);
}

lc_entity_handle_t lc_constraint_create_equal_length(
    lc_entity_handle_t line1,
    lc_entity_handle_t line2)
{
    lc_entity_handle_t entities[2] = {line1, line2};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_EQUAL_LENGTH,
        entities,
        2,
        0.0f);
}

lc_entity_handle_t lc_constraint_create_equal_radius(
    lc_entity_handle_t circle1,
    lc_entity_handle_t circle2)
{
    lc_entity_handle_t entities[2] = {circle1, circle2};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_EQUAL_RADIUS,
        entities,
        2,
        0.0f);
}

lc_entity_handle_t lc_constraint_create_fix_point(
    lc_entity_handle_t point)
{
    lc_entity_handle_t entities[1] = {point};
    return lc_constraint_create_internal(
        LC_CONSTRAINT_FIX_POINT,
        entities,
        1,
        0.0f);
}

bool lc_constraint_destroy(lc_entity_handle_t constraint)
{
    if (!g_initialized)
    {
        return false;
    }

    /* Verify constraint is valid */
    if (!lc_entity_is_valid(constraint))
    {
        return false;
    }

    /* Verify it is a constraint entity */
    if (lc_entity_get_type(constraint) != LC_ENTITY_TYPE_CONSTRAINT)
    {
        printf("[lc_constraint] ERROR: Entity is not a constraint\n");
        return false;
    }

    /* Use entity system to destroy */
    return lc_entity_destroy(constraint);
}

float lc_constraint_evaluate(lc_entity_handle_t constraint)
{
    if (!g_initialized)
    {
        return 0.0f;
    }

    /* Verify constraint is valid */
    if (!lc_entity_is_valid(constraint))
    {
        return 0.0f;
    }

    /* Verify it is a constraint entity */
    if (lc_entity_get_type(constraint) != LC_ENTITY_TYPE_CONSTRAINT)
    {
        return 0.0f;
    }

    /* Get constraint data */
    lc_constraint_data_t *data = (lc_constraint_data_t*)lc_entity_get_data(constraint);
    if (data == NULL)
    {
        return 0.0f;
    }

    /* Compute error based on constraint type */
    float error = 0.0f;
    vec2 p1, p2, L1_start, L1_end, L2_start, L2_end, C1, C2;
    float r1, r2;

    switch (data->type)
    {
        case LC_CONSTRAINT_DISTANCE_POINT_POINT:
            if (lc_constraint_get_point_position(data->entities[0], p1) &&
                lc_constraint_get_point_position(data->entities[1], p2))
            {
                error = compute_error_distance_point_point(p1, p2, data->value);
            }
            break;

        case LC_CONSTRAINT_DISTANCE_POINT_LINE:
            if (lc_constraint_get_point_position(data->entities[0], p1) &&
                lc_constraint_get_line_endpoints(data->entities[1], L1_start, L1_end))
            {
                error = compute_error_distance_point_line(p1, L1_start, L1_end, data->value);
            }
            break;

        case LC_CONSTRAINT_ANGLE_LINE_LINE:
            if (lc_constraint_get_line_endpoints(data->entities[0], L1_start, L1_end) &&
                lc_constraint_get_line_endpoints(data->entities[1], L2_start, L2_end))
            {
                error = compute_error_angle_line_line(L1_start, L1_end, L2_start, L2_end, data->value);
            }
            break;

        case LC_CONSTRAINT_COINCIDENT_POINT_POINT:
            if (lc_constraint_get_point_position(data->entities[0], p1) &&
                lc_constraint_get_point_position(data->entities[1], p2))
            {
                error = compute_error_coincident_point_point(p1, p2);
            }
            break;

        case LC_CONSTRAINT_COINCIDENT_POINT_LINE:
            if (lc_constraint_get_point_position(data->entities[0], p1) &&
                lc_constraint_get_line_endpoints(data->entities[1], L1_start, L1_end))
            {
                error = compute_error_coincident_point_line(p1, L1_start, L1_end);
            }
            break;

        case LC_CONSTRAINT_COINCIDENT_POINT_CIRCLE:
            if (lc_constraint_get_point_position(data->entities[0], p1) &&
                lc_constraint_get_circle_params(data->entities[1], C1, &r1))
            {
                error = compute_error_coincident_point_circle(p1, C1, r1);
            }
            break;

        case LC_CONSTRAINT_PARALLEL:
            if (lc_constraint_get_line_endpoints(data->entities[0], L1_start, L1_end) &&
                lc_constraint_get_line_endpoints(data->entities[1], L2_start, L2_end))
            {
                error = compute_error_parallel(L1_start, L1_end, L2_start, L2_end);
            }
            break;

        case LC_CONSTRAINT_PERPENDICULAR:
            if (lc_constraint_get_line_endpoints(data->entities[0], L1_start, L1_end) &&
                lc_constraint_get_line_endpoints(data->entities[1], L2_start, L2_end))
            {
                error = compute_error_perpendicular(L1_start, L1_end, L2_start, L2_end);
            }
            break;

        case LC_CONSTRAINT_HORIZONTAL:
            if (lc_constraint_get_line_endpoints(data->entities[0], L1_start, L1_end))
            {
                error = compute_error_horizontal(L1_start, L1_end);
            }
            break;

        case LC_CONSTRAINT_VERTICAL:
            if (lc_constraint_get_line_endpoints(data->entities[0], L1_start, L1_end))
            {
                error = compute_error_vertical(L1_start, L1_end);
            }
            break;

        case LC_CONSTRAINT_TANGENT_LINE_CIRCLE:
            if (lc_constraint_get_line_endpoints(data->entities[0], L1_start, L1_end) &&
                lc_constraint_get_circle_params(data->entities[1], C1, &r1))
            {
                error = compute_error_tangent_line_circle(L1_start, L1_end, C1, r1);
            }
            break;

        case LC_CONSTRAINT_TANGENT_CIRCLE_CIRCLE:
            if (lc_constraint_get_circle_params(data->entities[0], C1, &r1) &&
                lc_constraint_get_circle_params(data->entities[1], C2, &r2))
            {
                error = compute_error_tangent_circle_circle(C1, r1, C2, r2);
            }
            break;

        case LC_CONSTRAINT_EQUAL_LENGTH:
            if (lc_constraint_get_line_endpoints(data->entities[0], L1_start, L1_end) &&
                lc_constraint_get_line_endpoints(data->entities[1], L2_start, L2_end))
            {
                error = compute_error_equal_length(L1_start, L1_end, L2_start, L2_end);
            }
            break;

        case LC_CONSTRAINT_EQUAL_RADIUS:
            if (lc_constraint_get_circle_params(data->entities[0], C1, &r1) &&
                lc_constraint_get_circle_params(data->entities[1], C2, &r2))
            {
                error = compute_error_equal_radius(r1, r2);
            }
            break;

        case LC_CONSTRAINT_FIX_POINT:
            if (lc_constraint_get_point_position(data->entities[0], p1))
            {
                /* For fix constraint, the fixed position is stored in data->value as packed x,y.
                 * But actually we need 2 floats. For now, we'll use a simple approach:
                 * Store the fixed position at constraint creation time.
                 * This requires extending the constraint data structure later.
                 * For Phase 3C, we'll just evaluate to 0 (needs proper design). */
                error = 0.0f;  /* TODO: Implement proper fix constraint storage */
            }
            break;

        default:
            printf("[lc_constraint] ERROR: Unknown constraint type %d\n", data->type);
            error = 0.0f;
            break;
    }

    /* Update constraint data */
    data->error = error;

    /* Mark as satisfied if error is below tolerance (1e-6) */
    float tolerance = 1e-6f;
    if (fabsf(error) < tolerance)
    {
        data->flags |= LC_CONSTRAINT_FLAG_SATISFIED;
    }
    else
    {
        data->flags &= ~LC_CONSTRAINT_FLAG_SATISFIED;
    }

    return error;
}

float lc_constraint_get_error(lc_entity_handle_t constraint)
{
    if (!g_initialized)
    {
        return 0.0f;
    }

    /* Verify constraint is valid */
    if (!lc_entity_is_valid(constraint))
    {
        return 0.0f;
    }

    /* Verify it is a constraint entity */
    if (lc_entity_get_type(constraint) != LC_ENTITY_TYPE_CONSTRAINT)
    {
        return 0.0f;
    }

    /* Get constraint data */
    lc_constraint_data_t *data = (lc_constraint_data_t*)lc_entity_get_data(constraint);
    if (data == NULL)
    {
        return 0.0f;
    }

    return data->error;
}

bool lc_constraint_build_graph(lc_entity_handle_t sketch)
{
    if (!g_initialized)
    {
        return false;
    }

    /* Verify sketch is valid */
    if (!lc_entity_is_valid(sketch))
    {
        return false;
    }

    if (lc_entity_get_type(sketch) != LC_ENTITY_TYPE_SKETCH)
    {
        printf("[lc_constraint] ERROR: Entity is not a sketch\n");
        return false;
    }

    /* Find or create graph for this sketch */
    lc_constraint_graph_t *graph = find_graph(sketch);
    if (graph == NULL)
    {
        graph = create_graph(sketch);
        if (graph == NULL)
        {
            printf("[lc_constraint] ERROR: Failed to create graph\n");
            return false;
        }
    }

    /* If graph is valid, no rebuild needed */
    if (graph->valid)
    {
        return true;
    }

    /* Reset graph */
    graph->node_count = 0;
    graph->edge_count = 0;
    graph->total_dof = 0;
    graph->total_constrained_dof = 0;

    /* Enumerate all entities in sketch and create nodes */
    lc_entity_handle_t child = lc_entity_get_first_child(sketch);
    while (child != LC_ENTITY_INVALID)
    {
        lc_entity_type_t type = lc_entity_get_type(child);

        /* Add geometry entities as nodes */
        if (type == LC_ENTITY_TYPE_GEOMETRY_LINE ||
            type == LC_ENTITY_TYPE_GEOMETRY_CIRCLE ||
            type == LC_ENTITY_TYPE_GEOMETRY_RECT)
        {
            add_graph_node(graph, child);
        }

        /* Add constraints as edges */
        if (type == LC_ENTITY_TYPE_CONSTRAINT)
        {
            add_graph_edge(graph, child);
        }

        child = lc_entity_get_next_sibling(child);
    }

    /* Analyze DOF */
    analyze_graph_dof(graph);

    /* Mark graph as valid */
    graph->valid = true;

    return true;
}

void lc_constraint_invalidate_graph(lc_entity_handle_t sketch)
{
    if (!g_initialized)
    {
        return;
    }

    lc_constraint_graph_t *graph = find_graph(sketch);
    if (graph != NULL)
    {
        graph->valid = false;
    }
}

int lc_constraint_get_dof(lc_entity_handle_t sketch)
{
    if (!g_initialized)
    {
        return -1;
    }

    /* Build graph if needed */
    if (!lc_constraint_build_graph(sketch))
    {
        return -1;
    }

    lc_constraint_graph_t *graph = find_graph(sketch);
    if (graph == NULL)
    {
        return -1;
    }

    return graph->total_dof - graph->total_constrained_dof;
}

bool lc_constraint_is_fully_constrained(lc_entity_handle_t sketch)
{
    int dof = lc_constraint_get_dof(sketch);
    return dof == 0;
}

bool lc_constraint_is_over_constrained(lc_entity_handle_t sketch)
{
    int dof = lc_constraint_get_dof(sketch);
    return dof < 0;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static lc_entity_handle_t lc_constraint_create_internal(
    lc_constraint_type_t type,
    const lc_entity_handle_t *entities,
    int num_entities,
    float value)
{
    if (!g_initialized)
    {
        printf("[lc_constraint] ERROR: Constraint system not initialized\n");
        return LC_ENTITY_INVALID;
    }

    /* Validate entity handles */
    if (!lc_constraint_validate_entities(type, entities, num_entities))
    {
        printf("[lc_constraint] ERROR: Invalid entities for constraint type %d\n", type);
        return LC_ENTITY_INVALID;
    }

    /* Create constraint entity */
    lc_entity_handle_t handle = lc_entity_create(LC_ENTITY_TYPE_CONSTRAINT);
    if (handle == LC_ENTITY_INVALID)
    {
        printf("[lc_constraint] ERROR: Failed to create constraint entity\n");
        return LC_ENTITY_INVALID;
    }

    /* Allocate constraint data */
    lc_constraint_data_t *data = (lc_constraint_data_t*)malloc(sizeof(lc_constraint_data_t));
    if (data == NULL)
    {
        lc_entity_destroy(handle);
        printf("[lc_constraint] ERROR: Failed to allocate constraint data\n");
        return LC_ENTITY_INVALID;
    }

    /* Initialize constraint data */
    memset(data, 0, sizeof(lc_constraint_data_t));
    data->type = type;
    data->value = value;
    data->weight = LC_CONSTRAINT_DEFAULT_WEIGHT;
    data->error = 0.0f;
    data->flags = 0;

    /* Copy entity references */
    int i;
    for (i = 0; i < num_entities && i < 4; i++)
    {
        data->entities[i] = entities[i];
    }
    for (; i < 4; i++)
    {
        data->entities[i] = LC_ENTITY_INVALID;
    }

    /* Attach to entity */
    if (!lc_entity_set_data(handle, data))
    {
        free(data);
        lc_entity_destroy(handle);
        printf("[lc_constraint] ERROR: Failed to set constraint data\n");
        return LC_ENTITY_INVALID;
    }

    /* TODO: Add constraint to parent sketch's child list.
     * For Phase 3A-3B, constraints are created but not attached to sketches yet.
     * Phase 3C will implement proper parent-child relationships. */

    return handle;
}

static bool lc_constraint_validate_entities(
    lc_constraint_type_t type,
    const lc_entity_handle_t *entities,
    int num_entities)
{
    if (entities == NULL || num_entities <= 0)
    {
        return false;
    }

    /* Verify all entity handles are valid */
    int i;
    for (i = 0; i < num_entities; i++)
    {
        if (!lc_entity_is_valid(entities[i]))
        {
            printf("[lc_constraint] ERROR: Invalid entity handle at index %d\n", i);
            return false;
        }
    }

    /* Type-specific validation */
    /* For Phase 3A-3B, we perform basic validation.
     * Phase 3C will add detailed entity type checking
     * (e.g., verify line entities are actually lines). */

    switch (type)
    {
        case LC_CONSTRAINT_DISTANCE_POINT_POINT:
        case LC_CONSTRAINT_DISTANCE_POINT_LINE:
        case LC_CONSTRAINT_ANGLE_LINE_LINE:
        case LC_CONSTRAINT_COINCIDENT_POINT_POINT:
        case LC_CONSTRAINT_COINCIDENT_POINT_LINE:
        case LC_CONSTRAINT_COINCIDENT_POINT_CIRCLE:
        case LC_CONSTRAINT_PARALLEL:
        case LC_CONSTRAINT_PERPENDICULAR:
        case LC_CONSTRAINT_TANGENT_LINE_CIRCLE:
        case LC_CONSTRAINT_TANGENT_CIRCLE_CIRCLE:
        case LC_CONSTRAINT_EQUAL_LENGTH:
        case LC_CONSTRAINT_EQUAL_RADIUS:
            /* Require 2 entities */
            if (num_entities != 2)
            {
                printf("[lc_constraint] ERROR: Expected 2 entities for constraint type %d\n", type);
                return false;
            }
            break;

        case LC_CONSTRAINT_HORIZONTAL:
        case LC_CONSTRAINT_VERTICAL:
        case LC_CONSTRAINT_FIX_POINT:
            /* Require 1 entity */
            if (num_entities != 1)
            {
                printf("[lc_constraint] ERROR: Expected 1 entity for constraint type %d\n", type);
                return false;
            }
            break;

        default:
            printf("[lc_constraint] ERROR: Unknown constraint type %d\n", type);
            return false;
    }

    return true;
}

/***************************************************************
** Geometry Query Helpers
***************************************************************/

/* Get point position from an entity.
 * Points can be extracted from line endpoints or circle centers.
 * Returns true if successful, false otherwise. */
static bool lc_constraint_get_point_position(lc_entity_handle_t entity, vec2 out_pos)
{
    if (!lc_entity_is_valid(entity))
    {
        return false;
    }

    lc_entity_type_t type = lc_entity_get_type(entity);

    /* For now, treat lines and circles as "point" sources */
    if (type == LC_ENTITY_TYPE_GEOMETRY_LINE)
    {
        lc_geometry_line_data_t *line = (lc_geometry_line_data_t*)lc_entity_get_data(entity);
        if (line == NULL)
        {
            return false;
        }
        /* Return start point (could be parametrized later) */
        out_pos[0] = line->start[0];
        out_pos[1] = line->start[1];
        return true;
    }
    else if (type == LC_ENTITY_TYPE_GEOMETRY_CIRCLE)
    {
        lc_geometry_circle_data_t *circle = (lc_geometry_circle_data_t*)lc_entity_get_data(entity);
        if (circle == NULL)
        {
            return false;
        }
        /* Return center point */
        out_pos[0] = circle->center[0];
        out_pos[1] = circle->center[1];
        return true;
    }

    return false;
}

/* Get line endpoints from an entity.
 * Returns true if entity is a line, false otherwise. */
static bool lc_constraint_get_line_endpoints(lc_entity_handle_t entity, vec2 out_start, vec2 out_end)
{
    if (!lc_entity_is_valid(entity))
    {
        return false;
    }

    if (lc_entity_get_type(entity) != LC_ENTITY_TYPE_GEOMETRY_LINE)
    {
        return false;
    }

    lc_geometry_line_data_t *line = (lc_geometry_line_data_t*)lc_entity_get_data(entity);
    if (line == NULL)
    {
        return false;
    }

    out_start[0] = line->start[0];
    out_start[1] = line->start[1];
    out_end[0] = line->end[0];
    out_end[1] = line->end[1];

    return true;
}

/* Get circle parameters from an entity.
 * Returns true if entity is a circle, false otherwise. */
static bool lc_constraint_get_circle_params(lc_entity_handle_t entity, vec2 out_center, float *out_radius)
{
    if (!lc_entity_is_valid(entity))
    {
        return false;
    }

    if (lc_entity_get_type(entity) != LC_ENTITY_TYPE_GEOMETRY_CIRCLE)
    {
        return false;
    }

    lc_geometry_circle_data_t *circle = (lc_geometry_circle_data_t*)lc_entity_get_data(entity);
    if (circle == NULL)
    {
        return false;
    }

    out_center[0] = circle->center[0];
    out_center[1] = circle->center[1];
    *out_radius = circle->radius;

    return true;
}

/***************************************************************
** Error Computation Functions
***************************************************************/

/* Distance point-to-point error.
 * Error = |p1 - p2| - target_distance
 * Zero when distance equals target. */
static float compute_error_distance_point_point(vec2 p1, vec2 p2, float target_distance)
{
    float dx = p2[0] - p1[0];
    float dy = p2[1] - p1[1];
    float actual_distance = sqrtf(dx*dx + dy*dy);
    float error = actual_distance - target_distance;
    return error * error;  /* Squared error for smoothness */
}

/* Distance point-to-line error.
 * Error = |perpendicular_distance| - target_distance
 * Zero when perpendicular distance equals target. */
static float compute_error_distance_point_line(vec2 p, vec2 L0, vec2 L1, float target_distance)
{
    /* Compute line direction and normal */
    vec2 dir;
    dir[0] = L1[0] - L0[0];
    dir[1] = L1[1] - L0[1];
    float len = sqrtf(dir[0]*dir[0] + dir[1]*dir[1]);

    if (len < 1e-9f)
    {
        return 0.0f;  /* Degenerate line */
    }

    vec2 norm;
    norm[0] = -dir[1] / len;  /* Perpendicular */
    norm[1] = dir[0] / len;

    /* Compute signed distance */
    vec2 dp;
    dp[0] = p[0] - L0[0];
    dp[1] = p[1] - L0[1];
    float actual_distance = dp[0]*norm[0] + dp[1]*norm[1];

    float error = fabsf(actual_distance) - target_distance;
    return error * error;  /* Squared error */
}

/* Angle line-to-line error.
 * Error = angle(dir1, dir2) - target_angle
 * Zero when angle equals target. */
static float compute_error_angle_line_line(vec2 L1_start, vec2 L1_end, vec2 L2_start, vec2 L2_end, float target_angle)
{
    /* Compute direction vectors */
    vec2 dir1, dir2;
    dir1[0] = L1_end[0] - L1_start[0];
    dir1[1] = L1_end[1] - L1_start[1];
    dir2[0] = L2_end[0] - L2_start[0];
    dir2[1] = L2_end[1] - L2_start[1];

    /* Normalize */
    float len1 = sqrtf(dir1[0]*dir1[0] + dir1[1]*dir1[1]);
    float len2 = sqrtf(dir2[0]*dir2[0] + dir2[1]*dir2[1]);

    if (len1 < 1e-9f || len2 < 1e-9f)
    {
        return 0.0f;  /* Degenerate line */
    }

    dir1[0] /= len1;
    dir1[1] /= len1;
    dir2[0] /= len2;
    dir2[1] /= len2;

    /* Compute angle using atan2 for full range */
    float dot = dir1[0]*dir2[0] + dir1[1]*dir2[1];
    float cross = dir1[0]*dir2[1] - dir1[1]*dir2[0];
    float actual_angle = atan2f(cross, dot);

    float error = actual_angle - target_angle;
    return error * error;  /* Squared error */
}

/* Coincident point-on-point error.
 * Error = |p1 - p2|^2
 * Zero when points coincide. */
static float compute_error_coincident_point_point(vec2 p1, vec2 p2)
{
    float dx = p2[0] - p1[0];
    float dy = p2[1] - p1[1];
    return dx*dx + dy*dy;  /* Squared distance */
}

/* Coincident point-on-line error.
 * Error = (perpendicular_distance)^2
 * Zero when point lies on line. */
static float compute_error_coincident_point_line(vec2 p, vec2 L0, vec2 L1)
{
    /* Compute line normal */
    vec2 dir;
    dir[0] = L1[0] - L0[0];
    dir[1] = L1[1] - L0[1];
    float len = sqrtf(dir[0]*dir[0] + dir[1]*dir[1]);

    if (len < 1e-9f)
    {
        return 0.0f;  /* Degenerate line */
    }

    vec2 norm;
    norm[0] = -dir[1] / len;
    norm[1] = dir[0] / len;

    /* Compute signed distance */
    vec2 dp;
    dp[0] = p[0] - L0[0];
    dp[1] = p[1] - L0[1];
    float d = dp[0]*norm[0] + dp[1]*norm[1];

    return d*d;  /* Squared error */
}

/* Coincident point-on-circle error.
 * Error = (|p - C| - r)^2
 * Zero when point lies on circle. */
static float compute_error_coincident_point_circle(vec2 p, vec2 C, float r)
{
    float dx = p[0] - C[0];
    float dy = p[1] - C[1];
    float dist = sqrtf(dx*dx + dy*dy);
    float error = dist - r;
    return error*error;  /* Squared error */
}

/* Parallel lines error.
 * Error = |cross(dir1, dir2)|^2
 * Zero when directions are parallel. */
static float compute_error_parallel(vec2 L1_start, vec2 L1_end, vec2 L2_start, vec2 L2_end)
{
    vec2 dir1, dir2;
    dir1[0] = L1_end[0] - L1_start[0];
    dir1[1] = L1_end[1] - L1_start[1];
    dir2[0] = L2_end[0] - L2_start[0];
    dir2[1] = L2_end[1] - L2_start[1];

    /* Normalize */
    float len1 = sqrtf(dir1[0]*dir1[0] + dir1[1]*dir1[1]);
    float len2 = sqrtf(dir2[0]*dir2[0] + dir2[1]*dir2[1]);

    if (len1 < 1e-9f || len2 < 1e-9f)
    {
        return 0.0f;  /* Degenerate line */
    }

    dir1[0] /= len1;
    dir1[1] /= len1;
    dir2[0] /= len2;
    dir2[1] /= len2;

    /* Cross product in 2D */
    float cross = dir1[0]*dir2[1] - dir1[1]*dir2[0];
    return cross*cross;  /* Squared error */
}

/* Perpendicular lines error.
 * Error = dot(dir1, dir2)^2
 * Zero when directions are perpendicular. */
static float compute_error_perpendicular(vec2 L1_start, vec2 L1_end, vec2 L2_start, vec2 L2_end)
{
    vec2 dir1, dir2;
    dir1[0] = L1_end[0] - L1_start[0];
    dir1[1] = L1_end[1] - L1_start[1];
    dir2[0] = L2_end[0] - L2_start[0];
    dir2[1] = L2_end[1] - L2_start[1];

    /* Normalize */
    float len1 = sqrtf(dir1[0]*dir1[0] + dir1[1]*dir1[1]);
    float len2 = sqrtf(dir2[0]*dir2[0] + dir2[1]*dir2[1]);

    if (len1 < 1e-9f || len2 < 1e-9f)
    {
        return 0.0f;  /* Degenerate line */
    }

    dir1[0] /= len1;
    dir1[1] /= len1;
    dir2[0] /= len2;
    dir2[1] /= len2;

    /* Dot product */
    float dot = dir1[0]*dir2[0] + dir1[1]*dir2[1];
    return dot*dot;  /* Squared error */
}

/* Horizontal line error.
 * Error = (L_end.y - L_start.y)^2
 * Zero when line is horizontal. */
static float compute_error_horizontal(vec2 L_start, vec2 L_end)
{
    float dy = L_end[1] - L_start[1];
    return dy*dy;  /* Squared error */
}

/* Vertical line error.
 * Error = (L_end.x - L_start.x)^2
 * Zero when line is vertical. */
static float compute_error_vertical(vec2 L_start, vec2 L_end)
{
    float dx = L_end[0] - L_start[0];
    return dx*dx;  /* Squared error */
}

/* Line tangent to circle error.
 * Error = (distance_from_center_to_line - radius)^2
 * Zero when line is tangent to circle. */
static float compute_error_tangent_line_circle(vec2 L_start, vec2 L_end, vec2 C, float r)
{
    /* Compute line normal */
    vec2 dir;
    dir[0] = L_end[0] - L_start[0];
    dir[1] = L_end[1] - L_start[1];
    float len = sqrtf(dir[0]*dir[0] + dir[1]*dir[1]);

    if (len < 1e-9f)
    {
        return 0.0f;  /* Degenerate line */
    }

    vec2 norm;
    norm[0] = -dir[1] / len;
    norm[1] = dir[0] / len;

    /* Distance from circle center to line */
    vec2 dC;
    dC[0] = C[0] - L_start[0];
    dC[1] = C[1] - L_start[1];
    float dist = fabsf(dC[0]*norm[0] + dC[1]*norm[1]);

    float error = dist - r;
    return error*error;  /* Squared error */
}

/* Circle tangent to circle error.
 * Error = (|C1 - C2| - (r1 + r2))^2 for external tangency.
 * Zero when circles are externally tangent. */
static float compute_error_tangent_circle_circle(vec2 C1, float r1, vec2 C2, float r2)
{
    float dx = C2[0] - C1[0];
    float dy = C2[1] - C1[1];
    float dist = sqrtf(dx*dx + dy*dy);

    /* External tangency: distance = r1 + r2 */
    float target_dist = r1 + r2;
    float error = dist - target_dist;
    return error*error;  /* Squared error */
}

/* Equal length error.
 * Error = (|L1| - |L2|)^2
 * Zero when line lengths are equal. */
static float compute_error_equal_length(vec2 L1_start, vec2 L1_end, vec2 L2_start, vec2 L2_end)
{
    float dx1 = L1_end[0] - L1_start[0];
    float dy1 = L1_end[1] - L1_start[1];
    float len1 = sqrtf(dx1*dx1 + dy1*dy1);

    float dx2 = L2_end[0] - L2_start[0];
    float dy2 = L2_end[1] - L2_start[1];
    float len2 = sqrtf(dx2*dx2 + dy2*dy2);

    float error = len1 - len2;
    return error*error;  /* Squared error */
}

/* Equal radius error.
 * Error = (r1 - r2)^2
 * Zero when radii are equal. */
static float compute_error_equal_radius(float r1, float r2)
{
    float error = r1 - r2;
    return error*error;  /* Squared error */
}

/* Fix point error.
 * Error = |p - p_fixed|^2
 * Zero when point is at fixed position. */
static float compute_error_fix_point(vec2 p, vec2 p_fixed)
{
    float dx = p[0] - p_fixed[0];
    float dy = p[1] - p_fixed[1];
    return dx*dx + dy*dy;  /* Squared error */
}

/***************************************************************
** Graph Management Helpers
***************************************************************/

/* Find cached graph for a sketch.
 * Returns graph pointer or NULL if not found. */
static lc_constraint_graph_t* find_graph(lc_entity_handle_t sketch)
{
    int i;
    for (i = 0; i < g_graph_count; i++)
    {
        if (g_graphs[i].sketch == sketch)
        {
            return &g_graphs[i];
        }
    }
    return NULL;
}

/* Create new graph for a sketch.
 * Returns graph pointer or NULL if cache is full. */
static lc_constraint_graph_t* create_graph(lc_entity_handle_t sketch)
{
    if (g_graph_count >= MAX_CACHED_GRAPHS)
    {
        printf("[lc_constraint] WARNING: Graph cache full, evicting oldest\n");
        /* Evict first graph (simple LRU) */
        free_graph(&g_graphs[0]);
        /* Shift array left */
        int i;
        for (i = 0; i < g_graph_count - 1; i++)
        {
            g_graphs[i] = g_graphs[i + 1];
        }
        g_graph_count--;
    }

    lc_constraint_graph_t *graph = &g_graphs[g_graph_count];
    g_graph_count++;

    /* Initialize graph */
    graph->sketch = sketch;
    graph->nodes = (lc_constraint_graph_node_t*)malloc(INITIAL_NODE_CAPACITY * sizeof(lc_constraint_graph_node_t));
    graph->node_count = 0;
    graph->node_capacity = INITIAL_NODE_CAPACITY;
    graph->edges = (lc_constraint_graph_edge_t*)malloc(INITIAL_EDGE_CAPACITY * sizeof(lc_constraint_graph_edge_t));
    graph->edge_count = 0;
    graph->edge_capacity = INITIAL_EDGE_CAPACITY;
    graph->total_dof = 0;
    graph->total_constrained_dof = 0;
    graph->valid = false;

    if (graph->nodes == NULL || graph->edges == NULL)
    {
        printf("[lc_constraint] ERROR: Failed to allocate graph memory\n");
        free_graph(graph);
        g_graph_count--;
        return NULL;
    }

    return graph;
}

/* Free graph memory. */
static void free_graph(lc_constraint_graph_t *graph)
{
    if (graph->nodes != NULL)
    {
        free(graph->nodes);
        graph->nodes = NULL;
    }
    if (graph->edges != NULL)
    {
        free(graph->edges);
        graph->edges = NULL;
    }
    graph->node_count = 0;
    graph->node_capacity = 0;
    graph->edge_count = 0;
    graph->edge_capacity = 0;
}

/* Compute DOF for an entity type.
 * Returns number of degrees of freedom. */
static int compute_entity_dof(lc_entity_type_t type)
{
    switch (type)
    {
        case LC_ENTITY_TYPE_GEOMETRY_LINE:
            return 4;  /* start_x, start_y, end_x, end_y */

        case LC_ENTITY_TYPE_GEOMETRY_CIRCLE:
            return 3;  /* center_x, center_y, radius */

        case LC_ENTITY_TYPE_GEOMETRY_RECT:
            return 4;  /* min_x, min_y, max_x, max_y */

        default:
            return 0;  /* Unknown entity type */
    }
}

/* Compute DOF removed by a constraint type.
 * Returns number of degrees of freedom constrained. */
static int compute_constraint_dof(lc_constraint_type_t type)
{
    switch (type)
    {
        case LC_CONSTRAINT_DISTANCE_POINT_POINT:
        case LC_CONSTRAINT_DISTANCE_POINT_LINE:
        case LC_CONSTRAINT_ANGLE_LINE_LINE:
        case LC_CONSTRAINT_PARALLEL:
        case LC_CONSTRAINT_PERPENDICULAR:
        case LC_CONSTRAINT_HORIZONTAL:
        case LC_CONSTRAINT_VERTICAL:
        case LC_CONSTRAINT_TANGENT_LINE_CIRCLE:
        case LC_CONSTRAINT_TANGENT_CIRCLE_CIRCLE:
        case LC_CONSTRAINT_EQUAL_LENGTH:
        case LC_CONSTRAINT_EQUAL_RADIUS:
            return 1;  /* Single equation, removes 1 DOF */

        case LC_CONSTRAINT_COINCIDENT_POINT_POINT:
        case LC_CONSTRAINT_COINCIDENT_POINT_LINE:
        case LC_CONSTRAINT_COINCIDENT_POINT_CIRCLE:
        case LC_CONSTRAINT_FIX_POINT:
            return 2;  /* Two equations (x and y), removes 2 DOF */

        default:
            return 0;  /* Unknown constraint type */
    }
}

/* Add a node to the graph. */
static void add_graph_node(lc_constraint_graph_t *graph, lc_entity_handle_t entity)
{
    /* Grow array if needed */
    if (graph->node_count >= graph->node_capacity)
    {
        int new_capacity = graph->node_capacity * 2;
        lc_constraint_graph_node_t *new_nodes = (lc_constraint_graph_node_t*)realloc(
            graph->nodes,
            new_capacity * sizeof(lc_constraint_graph_node_t));
        if (new_nodes == NULL)
        {
            printf("[lc_constraint] ERROR: Failed to grow node array\n");
            return;
        }
        graph->nodes = new_nodes;
        graph->node_capacity = new_capacity;
    }

    /* Add node */
    lc_constraint_graph_node_t *node = &graph->nodes[graph->node_count];
    node->entity = entity;
    node->entity_type = lc_entity_get_type(entity);
    node->dof = compute_entity_dof(node->entity_type);
    node->constrained_dof = 0;  /* Will be computed later */
    node->flags = 0;

    graph->node_count++;
}

/* Add an edge to the graph. */
static void add_graph_edge(lc_constraint_graph_t *graph, lc_entity_handle_t constraint)
{
    /* Grow array if needed */
    if (graph->edge_count >= graph->edge_capacity)
    {
        int new_capacity = graph->edge_capacity * 2;
        lc_constraint_graph_edge_t *new_edges = (lc_constraint_graph_edge_t*)realloc(
            graph->edges,
            new_capacity * sizeof(lc_constraint_graph_edge_t));
        if (new_edges == NULL)
        {
            printf("[lc_constraint] ERROR: Failed to grow edge array\n");
            return;
        }
        graph->edges = new_edges;
        graph->edge_capacity = new_capacity;
    }

    /* Get constraint data */
    lc_constraint_data_t *data = (lc_constraint_data_t*)lc_entity_get_data(constraint);
    if (data == NULL)
    {
        return;
    }

    /* Add edge */
    lc_constraint_graph_edge_t *edge = &graph->edges[graph->edge_count];
    edge->constraint = constraint;
    edge->type = data->type;
    edge->dof_constrained = compute_constraint_dof(data->type);

    graph->edge_count++;
}

/* Analyze graph DOF and mark node flags. */
static void analyze_graph_dof(lc_constraint_graph_t *graph)
{
    int i, j;

    /* Compute total DOF from all nodes */
    graph->total_dof = 0;
    for (i = 0; i < graph->node_count; i++)
    {
        graph->total_dof += graph->nodes[i].dof;
    }

    /* Compute total constrained DOF from all edges */
    graph->total_constrained_dof = 0;
    for (i = 0; i < graph->edge_count; i++)
    {
        graph->total_constrained_dof += graph->edges[i].dof_constrained;
    }

    /* For each node, compute how many DOF are constrained by constraints referencing it */
    for (i = 0; i < graph->node_count; i++)
    {
        lc_constraint_graph_node_t *node = &graph->nodes[i];
        node->constrained_dof = 0;

        /* Check all edges */
        for (j = 0; j < graph->edge_count; j++)
        {
            lc_constraint_graph_edge_t *edge = &graph->edges[j];

            /* Get constraint data to check if it references this node */
            lc_constraint_data_t *data = (lc_constraint_data_t*)lc_entity_get_data(edge->constraint);
            if (data == NULL)
            {
                continue;
            }

            /* Check if any entity in the constraint matches this node */
            int k;
            bool references_node = false;
            for (k = 0; k < 4; k++)
            {
                if (data->entities[k] == node->entity)
                {
                    references_node = true;
                    break;
                }
            }

            if (references_node)
            {
                node->constrained_dof += edge->dof_constrained;
            }
        }

        /* Set node flags based on DOF analysis */
        if (node->constrained_dof < node->dof)
        {
            node->flags |= LC_GRAPH_NODE_UNDER_CONSTRAINED;
        }
        else if (node->constrained_dof == node->dof)
        {
            node->flags |= LC_GRAPH_NODE_FULLY_CONSTRAINED;
        }
        else if (node->constrained_dof > node->dof)
        {
            node->flags |= LC_GRAPH_NODE_OVER_CONSTRAINED;
        }
    }
}
