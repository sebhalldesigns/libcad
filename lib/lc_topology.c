/***************************************************************
**
** libcad Implementation File
**
** File         :  lc_topology.c
** Module       :  libcad (topology queries)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Topological query functions for B-Rep navigation
**                 and analysis. Traversal, element counting, Euler
**                 characteristic verification, and bounding box
**                 computation.
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_topology.h"
#include "lc_entity.h"
#include <string.h>
#include <stdio.h>
#include <float.h>

/* MARK: STATIC FUNCTION DEFS */

static size_t get_children_of_type(
    lc_entity_handle_t parent,
    lc_entity_type_t type,
    lc_entity_handle_t *out_handles,
    size_t max_count
);

static bool handle_in_array(
    lc_entity_handle_t handle,
    const lc_entity_handle_t *array,
    size_t count
);

static size_t add_unique_handle(
    lc_entity_handle_t handle,
    lc_entity_handle_t *array,
    size_t current_count,
    size_t max_count
);

/* MARK: PUBLIC FUNCTIONS */

size_t lc_topology_get_shells(
    lc_entity_handle_t solid,
    lc_entity_handle_t *out_shells,
    size_t max_count)
{
    if (!lc_entity_is_valid(solid))
    {
        return 0;
    }

    if (lc_entity_get_type(solid) != LC_ENTITY_TYPE_SOLID)
    {
        return 0;
    }

    return get_children_of_type(solid, LC_ENTITY_TYPE_SHELL, out_shells, max_count);
}

size_t lc_topology_get_faces(
    lc_entity_handle_t shell,
    lc_entity_handle_t *out_faces,
    size_t max_count)
{
    if (!lc_entity_is_valid(shell))
    {
        return 0;
    }

    if (lc_entity_get_type(shell) != LC_ENTITY_TYPE_SHELL)
    {
        return 0;
    }

    return get_children_of_type(shell, LC_ENTITY_TYPE_FACE, out_faces, max_count);
}

size_t lc_topology_get_loops(
    lc_entity_handle_t face,
    lc_entity_handle_t *out_loops,
    size_t max_count)
{
    if (!lc_entity_is_valid(face))
    {
        return 0;
    }

    if (lc_entity_get_type(face) != LC_ENTITY_TYPE_FACE)
    {
        return 0;
    }

    return get_children_of_type(face, LC_ENTITY_TYPE_LOOP, out_loops, max_count);
}

size_t lc_topology_get_edge_uses(
    lc_entity_handle_t loop,
    lc_entity_handle_t *out_edge_uses,
    size_t max_count)
{
    if (!lc_entity_is_valid(loop))
    {
        return 0;
    }

    if (lc_entity_get_type(loop) != LC_ENTITY_TYPE_LOOP)
    {
        return 0;
    }

    /* Get first child of loop entity (should be first edge use) */
    lc_entity_handle_t first_edge_use = lc_entity_get_first_child(loop);
    if (!lc_entity_is_valid(first_edge_use))
    {
        return 0;
    }

    /* Validate first edge use */
    if (lc_entity_get_type(first_edge_use) != LC_ENTITY_TYPE_EDGE_USE)
    {
        return 0;
    }

    /* Follow next_in_loop chain to collect all edge uses */
    size_t count = 0;
    lc_entity_handle_t current = first_edge_use;

    do
    {
        /* Add current to output if space available */
        if (out_edge_uses && count < max_count)
        {
            out_edge_uses[count] = current;
        }
        count++;

        /* Get next edge use in loop */
        const lc_edge_use_data_t *use_data = (const lc_edge_use_data_t*)lc_entity_get_data(current);
        if (!use_data)
        {
            break;
        }

        current = use_data->next_in_loop;

        /* Stop if we've circled back or hit invalid */
        if (!lc_entity_is_valid(current) || current == first_edge_use)
        {
            break;
        }

        /* Safety: prevent infinite loop */
        if (count > 10000)
        {
            break;
        }
    }
    while (current != first_edge_use);

    return count;
}

size_t lc_topology_get_edges(
    lc_entity_handle_t loop,
    lc_entity_handle_t *out_edges,
    size_t max_count)
{
    /* Allocate temp buffer for edge uses */
    lc_entity_handle_t edge_uses[256];
    size_t use_count = lc_topology_get_edge_uses(loop, edge_uses, 256);

    if (use_count == 0)
    {
        return 0;
    }

    /* Collect edges from edge uses */
    size_t edge_count = 0;
    for (size_t i = 0; i < use_count && i < 256; i++)
    {
        const lc_edge_use_data_t *use_data = (const lc_edge_use_data_t*)lc_entity_get_data(edge_uses[i]);
        if (use_data && lc_entity_is_valid(use_data->edge))
        {
            if (out_edges && edge_count < max_count)
            {
                out_edges[edge_count] = use_data->edge;
            }
            edge_count++;
        }
    }

    return edge_count;
}

bool lc_topology_get_edge_vertices(
    lc_entity_handle_t edge,
    lc_entity_handle_t *out_start,
    lc_entity_handle_t *out_end)
{
    if (!lc_entity_is_valid(edge))
    {
        return false;
    }

    if (lc_entity_get_type(edge) != LC_ENTITY_TYPE_EDGE)
    {
        return false;
    }

    const lc_edge_data_t *data = (const lc_edge_data_t*)lc_entity_get_data(edge);
    if (!data)
    {
        return false;
    }

    if (out_start)
    {
        *out_start = data->vertex_start;
    }

    if (out_end)
    {
        *out_end = data->vertex_end;
    }

    return true;
}

bool lc_topology_get_vertex_position(
    lc_entity_handle_t vertex,
    vec3 out_position)
{
    if (!lc_entity_is_valid(vertex))
    {
        return false;
    }

    if (lc_entity_get_type(vertex) != LC_ENTITY_TYPE_VERTEX)
    {
        return false;
    }

    lc_vertex_data_t *data = (lc_vertex_data_t*)lc_entity_get_data(vertex);
    if (!data)
    {
        return false;
    }

    glm_vec3_copy(data->position, out_position);
    return true;
}

size_t lc_topology_get_edge_faces(
    lc_entity_handle_t edge,
    lc_entity_handle_t *out_faces,
    size_t max_count)
{
    if (!lc_entity_is_valid(edge))
    {
        return 0;
    }

    if (lc_entity_get_type(edge) != LC_ENTITY_TYPE_EDGE)
    {
        return 0;
    }

    /* Use the edge's built-in edge_use list (O(1) lookup) */
    lc_edge_data_t *edge_data = (lc_edge_data_t*)lc_entity_get_data(edge);
    if (!edge_data)
    {
        return 0;
    }

    size_t face_count = 0;
    int i;
    for (i = 0; i < edge_data->edge_use_count && i < 2; i++)
    {
        lc_entity_handle_t eu = edge_data->edge_uses[i];
        if (!lc_entity_is_valid(eu))
        {
            continue;
        }

        lc_edge_use_data_t *eu_data = (lc_edge_use_data_t*)lc_entity_get_data(eu);
        if (!eu_data || !lc_entity_is_valid(eu_data->loop))
        {
            continue;
        }

        lc_loop_data_t *loop_data = (lc_loop_data_t*)lc_entity_get_data(eu_data->loop);
        if (!loop_data || !lc_entity_is_valid(loop_data->face))
        {
            continue;
        }

        if (out_faces && face_count < max_count)
        {
            out_faces[face_count] = loop_data->face;
        }
        face_count++;
    }

    return face_count;
}

void lc_topology_count_elements(
    lc_entity_handle_t solid,
    size_t *out_vertices,
    size_t *out_edges,
    size_t *out_faces)
{
    /* Initialize counts */
    size_t v_count = 0;
    size_t e_count = 0;
    size_t f_count = 0;

    /* Deduplication arrays */
    lc_entity_handle_t unique_vertices[1024];
    lc_entity_handle_t unique_edges[1024];

    if (!lc_entity_is_valid(solid))
    {
        goto done;
    }

    /* Get all shells in solid */
    lc_entity_handle_t shells[16];
    size_t shell_count = lc_topology_get_shells(solid, shells, 16);

    /* For each shell, get faces */
    for (size_t i = 0; i < shell_count && i < 16; i++)
    {
        lc_entity_handle_t faces[256];
        size_t face_count = lc_topology_get_faces(shells[i], faces, 256);

        for (size_t j = 0; j < face_count && j < 256; j++)
        {
            f_count++;

            /* Get loops for this face */
            lc_entity_handle_t loops[16];
            size_t loop_count = lc_topology_get_loops(faces[j], loops, 16);

            for (size_t k = 0; k < loop_count && k < 16; k++)
            {
                /* Get edges for this loop */
                lc_entity_handle_t edges[256];
                size_t edge_count = lc_topology_get_edges(loops[k], edges, 256);

                for (size_t m = 0; m < edge_count && m < 256; m++)
                {
                    /* Add edge to unique set */
                    e_count = add_unique_handle(edges[m], unique_edges, e_count, 1024);

                    /* Get vertices for this edge */
                    lc_entity_handle_t v_start, v_end;
                    if (lc_topology_get_edge_vertices(edges[m], &v_start, &v_end))
                    {
                        v_count = add_unique_handle(v_start, unique_vertices, v_count, 1024);
                        v_count = add_unique_handle(v_end, unique_vertices, v_count, 1024);
                    }
                }
            }
        }
    }

done:
    if (out_vertices)
    {
        *out_vertices = v_count;
    }

    if (out_edges)
    {
        *out_edges = e_count;
    }

    if (out_faces)
    {
        *out_faces = f_count;
    }
}

bool lc_topology_check_euler(lc_entity_handle_t solid)
{
    size_t v_count = 0;
    size_t e_count = 0;
    size_t f_count = 0;

    lc_topology_count_elements(solid, &v_count, &e_count, &f_count);

    /* Euler characteristic: V - E + F = 2 for a simple polyhedron */
    int euler = (int)v_count - (int)e_count + (int)f_count;

    return (euler == 2);
}

void lc_topology_compute_bbox(
    lc_entity_handle_t solid,
    vec3 out_min,
    vec3 out_max)
{
    /* Initialize to invalid range */
    out_min[0] = FLT_MAX;
    out_min[1] = FLT_MAX;
    out_min[2] = FLT_MAX;

    out_max[0] = -FLT_MAX;
    out_max[1] = -FLT_MAX;
    out_max[2] = -FLT_MAX;

    if (!lc_entity_is_valid(solid))
    {
        return;
    }

    /* Deduplication array for vertices */
    lc_entity_handle_t unique_vertices[1024];
    size_t v_count = 0;

    /* Get all shells in solid */
    lc_entity_handle_t shells[16];
    size_t shell_count = lc_topology_get_shells(solid, shells, 16);

    /* For each shell, get faces */
    for (size_t i = 0; i < shell_count && i < 16; i++)
    {
        lc_entity_handle_t faces[256];
        size_t face_count = lc_topology_get_faces(shells[i], faces, 256);

        for (size_t j = 0; j < face_count && j < 256; j++)
        {
            /* Get loops for this face */
            lc_entity_handle_t loops[16];
            size_t loop_count = lc_topology_get_loops(faces[j], loops, 16);

            for (size_t k = 0; k < loop_count && k < 16; k++)
            {
                /* Get edges for this loop */
                lc_entity_handle_t edges[256];
                size_t edge_count = lc_topology_get_edges(loops[k], edges, 256);

                for (size_t m = 0; m < edge_count && m < 256; m++)
                {
                    /* Get vertices for this edge */
                    lc_entity_handle_t v_start, v_end;
                    if (lc_topology_get_edge_vertices(edges[m], &v_start, &v_end))
                    {
                        /* Process start vertex if unique */
                        if (!handle_in_array(v_start, unique_vertices, v_count))
                        {
                            if (v_count < 1024)
                            {
                                unique_vertices[v_count++] = v_start;
                            }

                            vec3 pos;
                            if (lc_topology_get_vertex_position(v_start, pos))
                            {
                                glm_vec3_minv(out_min, pos, out_min);
                                glm_vec3_maxv(out_max, pos, out_max);
                            }
                        }

                        /* Process end vertex if unique */
                        if (!handle_in_array(v_end, unique_vertices, v_count))
                        {
                            if (v_count < 1024)
                            {
                                unique_vertices[v_count++] = v_end;
                            }

                            vec3 pos;
                            if (lc_topology_get_vertex_position(v_end, pos))
                            {
                                glm_vec3_minv(out_min, pos, out_min);
                                glm_vec3_maxv(out_max, pos, out_max);
                            }
                        }
                    }
                }
            }
        }
    }
}

/* MARK: STATIC FUNCTIONS */

static size_t get_children_of_type(
    lc_entity_handle_t parent,
    lc_entity_type_t type,
    lc_entity_handle_t *out_handles,
    size_t max_count)
{
    size_t count = 0;
    lc_entity_handle_t child = lc_entity_get_first_child(parent);

    while (lc_entity_is_valid(child))
    {
        if (lc_entity_get_type(child) == type)
        {
            if (out_handles && count < max_count)
            {
                out_handles[count] = child;
            }
            count++;
        }

        child = lc_entity_get_next_sibling(child);

        /* Safety: prevent infinite loop */
        if (count > 10000)
        {
            break;
        }
    }

    return count;
}

static bool handle_in_array(
    lc_entity_handle_t handle,
    const lc_entity_handle_t *array,
    size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        if (array[i] == handle)
        {
            return true;
        }
    }
    return false;
}

static size_t add_unique_handle(
    lc_entity_handle_t handle,
    lc_entity_handle_t *array,
    size_t current_count,
    size_t max_count)
{
    /* Check if already in array */
    if (handle_in_array(handle, array, current_count))
    {
        return current_count;
    }

    /* Add if space available */
    if (current_count < max_count)
    {
        array[current_count] = handle;
        return current_count + 1;
    }

    /* Array full, can't add */
    return current_count;
}
