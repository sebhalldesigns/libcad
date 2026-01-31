/***************************************************************
**
** libcad Implementation File
**
** File         :  lc_euler.c
** Module       :  libcad (Euler operators)
** Author       :  SH
** Created      :  2026-01-31 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Implementation of Euler operators for topological
**                 modifications of B-Rep models. Provides MVEF, MEV,
**                 MEF, KEV, KEF, MEKL, and KEML operators that
**                 maintain manifold validity.
**
***************************************************************/

/* MARK: INCLUDES */

#include "lc_euler.h"
#include "lc_entity.h"
#include "lc_geometry.h"
#include "lc_tessellate.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <cglm/cglm.h>

/* MARK: CONSTANTS & MACROS */

/* None */

/* MARK: TYPEDEFS */

/* None */

/* MARK: STATIC VARIABLES */

/* None */

/* MARK: STATIC FUNCTION DEFS */

static lc_entity_handle_t euler_create_vertex(vec3 position);
static lc_entity_handle_t euler_create_edge(lc_entity_handle_t v_start, lc_entity_handle_t v_end);
static lc_entity_handle_t euler_create_edge_use(lc_entity_handle_t edge, lc_entity_handle_t loop, bool forward);
static lc_entity_handle_t euler_create_loop(lc_entity_handle_t face, bool is_outer);
static lc_entity_handle_t euler_create_face(lc_entity_handle_t shell);
static lc_entity_handle_t find_edge_use_for_vertex(lc_entity_handle_t loop, lc_entity_handle_t vertex);
static lc_entity_handle_t edge_use_start_vertex(lc_entity_handle_t edge_use);
static lc_entity_handle_t edge_use_end_vertex(lc_entity_handle_t edge_use);
static void unregister_edge_use(lc_entity_handle_t edge, lc_entity_handle_t edge_use);

/* MARK: PUBLIC FUNCTIONS */

bool lc_euler_mvef(
    lc_entity_handle_t shell,
    vec3 v1_pos,
    vec3 v2_pos,
    lc_entity_handle_t *out_face,
    lc_entity_handle_t *out_edge,
    lc_entity_handle_t *out_v1,
    lc_entity_handle_t *out_v2)
{
    lc_entity_handle_t vertex1, vertex2, edge, face, loop, eu_fwd, eu_rev;
    lc_loop_data_t *loop_data;
    lc_face_data_t *face_data;
    lc_edge_use_data_t *eu_fwd_data, *eu_rev_data;

    /* Validate shell */
    if (!lc_entity_is_valid(shell))
    {
        printf("[lc_euler] MVEF: Invalid shell handle\n");
        return false;
    }

    /* Create vertices */
    vertex1 = euler_create_vertex(v1_pos);
    vertex2 = euler_create_vertex(v2_pos);
    if (vertex1 == LC_ENTITY_INVALID || vertex2 == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MVEF: Failed to create vertices\n");
        return false;
    }

    /* Create edge */
    edge = euler_create_edge(vertex1, vertex2);
    if (edge == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MVEF: Failed to create edge\n");
        lc_entity_destroy(vertex1);
        lc_entity_destroy(vertex2);
        return false;
    }

    /* Create face */
    face = euler_create_face(shell);
    if (face == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MVEF: Failed to create face\n");
        lc_entity_destroy(edge);
        lc_entity_destroy(vertex1);
        lc_entity_destroy(vertex2);
        return false;
    }

    /* Create outer loop */
    loop = euler_create_loop(face, true);
    if (loop == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MVEF: Failed to create loop\n");
        lc_entity_destroy(face);
        lc_entity_destroy(edge);
        lc_entity_destroy(vertex1);
        lc_entity_destroy(vertex2);
        return false;
    }

    /* Create edge uses */
    eu_fwd = euler_create_edge_use(edge, loop, true);
    eu_rev = euler_create_edge_use(edge, loop, false);
    if (eu_fwd == LC_ENTITY_INVALID || eu_rev == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MVEF: Failed to create edge uses\n");
        lc_entity_destroy(loop);
        lc_entity_destroy(face);
        lc_entity_destroy(edge);
        lc_entity_destroy(vertex1);
        lc_entity_destroy(vertex2);
        return false;
    }

    /* Link edge uses in circular chain */
    eu_fwd_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_fwd);
    eu_rev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_rev);

    eu_fwd_data->next_in_loop = eu_rev;
    eu_fwd_data->prev_in_loop = eu_rev;
    eu_rev_data->next_in_loop = eu_fwd;
    eu_rev_data->prev_in_loop = eu_fwd;

    /* Set face's outer loop */
    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    face_data->outer_loop = loop;
    loop_data = (lc_loop_data_t *)lc_entity_get_data(loop);
    loop_data->face = face;

    /* Write output handles */
    if (out_face) *out_face = face;
    if (out_edge) *out_edge = edge;
    if (out_v1) *out_v1 = vertex1;
    if (out_v2) *out_v2 = vertex2;

    return true;
}

bool lc_euler_mev(
    lc_entity_handle_t loop,
    lc_entity_handle_t existing_vertex,
    vec3 new_pos,
    lc_entity_handle_t *out_vertex,
    lc_entity_handle_t *out_edge)
{
    lc_entity_handle_t new_vertex, new_edge, eu_at, eu_prev, eu_fwd, eu_rev;
    lc_edge_use_data_t *eu_at_data, *eu_prev_data, *eu_fwd_data, *eu_rev_data;
    lc_loop_data_t *loop_data;
    lc_face_data_t *face_data;
    lc_entity_handle_t face;

    /* Validate loop and existing vertex */
    if (!lc_entity_is_valid(loop) || !lc_entity_is_valid(existing_vertex))
    {
        printf("[lc_euler] MEV: Invalid loop or vertex handle\n");
        return false;
    }

    /* Find edge use starting at existing vertex */
    eu_at = find_edge_use_for_vertex(loop, existing_vertex);
    if (eu_at == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MEV: Vertex not found in loop\n");
        return false;
    }

    /* Create new vertex */
    new_vertex = euler_create_vertex(new_pos);
    if (new_vertex == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MEV: Failed to create vertex\n");
        return false;
    }

    /* Create new edge */
    new_edge = euler_create_edge(existing_vertex, new_vertex);
    if (new_edge == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MEV: Failed to create edge\n");
        lc_entity_destroy(new_vertex);
        return false;
    }

    /* Create edge uses */
    eu_fwd = euler_create_edge_use(new_edge, loop, true);
    eu_rev = euler_create_edge_use(new_edge, loop, false);
    if (eu_fwd == LC_ENTITY_INVALID || eu_rev == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MEV: Failed to create edge uses\n");
        lc_entity_destroy(new_edge);
        lc_entity_destroy(new_vertex);
        return false;
    }

    /* Get edge use data */
    eu_at_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_at);
    eu_prev = eu_at_data->prev_in_loop;
    eu_prev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_prev);
    eu_fwd_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_fwd);
    eu_rev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_rev);

    /* Splice into loop: eu_prev -> eu_fwd -> eu_rev -> eu_at */
    eu_prev_data->next_in_loop = eu_fwd;
    eu_fwd_data->prev_in_loop = eu_prev;
    eu_fwd_data->next_in_loop = eu_rev;
    eu_rev_data->prev_in_loop = eu_fwd;
    eu_rev_data->next_in_loop = eu_at;
    eu_at_data->prev_in_loop = eu_rev;

    /* Invalidate face tessellation */
    loop_data = (lc_loop_data_t *)lc_entity_get_data(loop);
    face = loop_data->face;
    if (lc_entity_is_valid(face))
    {
        face_data = (lc_face_data_t *)lc_entity_get_data(face);
        face_data->mesh_dirty = true;
        lc_tessellate_invalidate(face);
    }

    /* Write output handles */
    if (out_vertex) *out_vertex = new_vertex;
    if (out_edge) *out_edge = new_edge;

    return true;
}

bool lc_euler_mef(
    lc_entity_handle_t face,
    lc_entity_handle_t v1,
    lc_entity_handle_t v2,
    lc_entity_handle_t *out_face,
    lc_entity_handle_t *out_edge)
{
    lc_entity_handle_t old_loop, eu1, eu2, eu1_prev, eu2_prev;
    lc_entity_handle_t new_edge, new_face, new_loop, eu_new_fwd, eu_new_rev;
    lc_entity_handle_t shell, current_eu;
    lc_face_data_t *face_data, *new_face_data;
    lc_edge_use_data_t *eu1_data, *eu2_data, *eu1_prev_data, *eu2_prev_data;
    lc_edge_use_data_t *eu_new_fwd_data, *eu_new_rev_data, *current_eu_data;
    lc_loop_data_t *new_loop_data;

    /* Validate face */
    if (!lc_entity_is_valid(face))
    {
        printf("[lc_euler] MEF: Invalid face handle\n");
        return false;
    }

    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    old_loop = face_data->outer_loop;

    /* Find edge uses */
    eu1 = find_edge_use_for_vertex(old_loop, v1);
    eu2 = find_edge_use_for_vertex(old_loop, v2);
    if (eu1 == LC_ENTITY_INVALID || eu2 == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MEF: Vertices not found in loop\n");
        return false;
    }

    /* Create new edge */
    new_edge = euler_create_edge(v1, v2);
    if (new_edge == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MEF: Failed to create edge\n");
        return false;
    }

    /* Get shell from face's parent */
    shell = lc_entity_get_parent(face);

    /* Create new face */
    new_face = euler_create_face(shell);
    if (new_face == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MEF: Failed to create face\n");
        lc_entity_destroy(new_edge);
        return false;
    }

    /* Create new loop */
    new_loop = euler_create_loop(new_face, true);
    if (new_loop == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MEF: Failed to create loop\n");
        lc_entity_destroy(new_face);
        lc_entity_destroy(new_edge);
        return false;
    }

    /* Create edge uses */
    eu_new_fwd = euler_create_edge_use(new_edge, new_loop, true);
    eu_new_rev = euler_create_edge_use(new_edge, old_loop, false);
    if (eu_new_fwd == LC_ENTITY_INVALID || eu_new_rev == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MEF: Failed to create edge uses\n");
        lc_entity_destroy(new_loop);
        lc_entity_destroy(new_face);
        lc_entity_destroy(new_edge);
        return false;
    }

    /* Get edge use data */
    eu1_data = (lc_edge_use_data_t *)lc_entity_get_data(eu1);
    eu2_data = (lc_edge_use_data_t *)lc_entity_get_data(eu2);
    eu1_prev = eu1_data->prev_in_loop;
    eu2_prev = eu2_data->prev_in_loop;
    eu1_prev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu1_prev);
    eu2_prev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu2_prev);
    eu_new_fwd_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_new_fwd);
    eu_new_rev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_new_rev);

    /* Splice new loop: eu_new_fwd -> eu1 -> ... -> eu2_prev -> eu_new_fwd */
    eu_new_fwd_data->next_in_loop = eu1;
    eu1_data->prev_in_loop = eu_new_fwd;
    eu2_prev_data->next_in_loop = eu_new_fwd;
    eu_new_fwd_data->prev_in_loop = eu2_prev;

    /* Splice old loop: eu_new_rev -> eu2 -> ... -> eu1_prev -> eu_new_rev */
    eu_new_rev_data->next_in_loop = eu2;
    eu2_data->prev_in_loop = eu_new_rev;
    eu1_prev_data->next_in_loop = eu_new_rev;
    eu_new_rev_data->prev_in_loop = eu1_prev;

    /* Move edge uses from old loop to new loop */
    current_eu = eu1;
    while (1)
    {
        current_eu_data = (lc_edge_use_data_t *)lc_entity_get_data(current_eu);
        current_eu_data->loop = new_loop;
        lc_entity_remove_child(old_loop, current_eu);
        lc_entity_add_child(new_loop, current_eu);

        if (current_eu == eu2_prev)
            break;

        current_eu = current_eu_data->next_in_loop;
    }

    /* Set new face's outer loop */
    new_face_data = (lc_face_data_t *)lc_entity_get_data(new_face);
    new_face_data->outer_loop = new_loop;
    new_loop_data = (lc_loop_data_t *)lc_entity_get_data(new_loop);
    new_loop_data->face = new_face;

    /* Invalidate tessellation */
    face_data->mesh_dirty = true;
    new_face_data->mesh_dirty = true;
    lc_tessellate_invalidate(face);
    lc_tessellate_invalidate(new_face);

    /* Write output handles */
    if (out_face) *out_face = new_face;
    if (out_edge) *out_edge = new_edge;

    return true;
}

bool lc_euler_kev(lc_entity_handle_t edge, lc_entity_handle_t vertex_to_kill)
{
    lc_edge_data_t *edge_data;
    lc_entity_handle_t v_start, v_end, surviving_vertex;
    lc_entity_handle_t eu_to, eu_from, eu_prev, eu_next;
    lc_edge_use_data_t *eu_to_data, *eu_from_data, *eu_prev_data, *eu_next_data;
    lc_entity_handle_t loop, face;
    lc_loop_data_t *loop_data;
    lc_face_data_t *face_data;

    /* Validate edge and vertex */
    if (!lc_entity_is_valid(edge) || !lc_entity_is_valid(vertex_to_kill))
    {
        printf("[lc_euler] KEV: Invalid edge or vertex handle\n");
        return false;
    }

    edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);
    v_start = edge_data->vertex_start;
    v_end = edge_data->vertex_end;

    /* Determine surviving vertex */
    if (vertex_to_kill == v_start)
    {
        surviving_vertex = v_end;
    }
    else if (vertex_to_kill == v_end)
    {
        surviving_vertex = v_start;
    }
    else
    {
        printf("[lc_euler] KEV: Vertex is not an endpoint of edge\n");
        return false;
    }

    /* Check edge has exactly 2 edge uses */
    if (edge_data->edge_use_count != 2)
    {
        printf("[lc_euler] KEV: Edge must have exactly 2 edge uses\n");
        return false;
    }

    /* Get edge uses */
    eu_to = edge_data->edge_uses[0];
    eu_from = edge_data->edge_uses[1];
    eu_to_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_to);
    eu_from_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_from);

    /* Determine which goes to vertex_to_kill */
    if (edge_use_end_vertex(eu_to) != vertex_to_kill)
    {
        lc_entity_handle_t temp = eu_to;
        lc_edge_use_data_t *temp_data = eu_to_data;
        eu_to = eu_from;
        eu_to_data = eu_from_data;
        eu_from = temp;
        eu_from_data = temp_data;
    }

    /* Get loop and unsplice */
    loop = eu_to_data->loop;
    eu_prev = eu_to_data->prev_in_loop;
    eu_next = eu_from_data->next_in_loop;
    eu_prev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_prev);
    eu_next_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_next);

    eu_prev_data->next_in_loop = eu_next;
    eu_next_data->prev_in_loop = eu_prev;

    /* Get face and invalidate tessellation */
    loop_data = (lc_loop_data_t *)lc_entity_get_data(loop);
    face = loop_data->face;
    if (lc_entity_is_valid(face))
    {
        face_data = (lc_face_data_t *)lc_entity_get_data(face);
        face_data->mesh_dirty = true;
        lc_tessellate_invalidate(face);
    }

    /* Unregister edge uses and remove from loop */
    unregister_edge_use(edge, eu_to);
    unregister_edge_use(edge, eu_from);
    lc_entity_remove_child(loop, eu_to);
    lc_entity_remove_child(loop, eu_from);

    /* Destroy entities (lc_entity_destroy frees data internally) */
    lc_entity_destroy(eu_to);
    lc_entity_destroy(eu_from);
    lc_entity_destroy(edge);
    lc_entity_destroy(vertex_to_kill);

    return true;
}

bool lc_euler_kef(lc_entity_handle_t edge)
{
    lc_edge_data_t *edge_data;
    lc_entity_handle_t eu1, eu2, loop1, loop2, face1, face2, surviving_face, killed_face;
    lc_entity_handle_t eu1_prev, eu1_next, eu2_prev, eu2_next, current_eu;
    lc_edge_use_data_t *eu1_data, *eu2_data, *eu1_prev_data, *eu1_next_data;
    lc_edge_use_data_t *eu2_prev_data, *eu2_next_data, *current_eu_data;
    lc_loop_data_t *loop1_data, *loop2_data;
    lc_face_data_t *face1_data;

    /* Validate edge */
    if (!lc_entity_is_valid(edge))
    {
        printf("[lc_euler] KEF: Invalid edge handle\n");
        return false;
    }

    edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);

    /* Check edge has exactly 2 edge uses */
    if (edge_data->edge_use_count != 2)
    {
        printf("[lc_euler] KEF: Edge must have exactly 2 edge uses\n");
        return false;
    }

    /* Get edge uses and their loops */
    eu1 = edge_data->edge_uses[0];
    eu2 = edge_data->edge_uses[1];
    eu1_data = (lc_edge_use_data_t *)lc_entity_get_data(eu1);
    eu2_data = (lc_edge_use_data_t *)lc_entity_get_data(eu2);
    loop1 = eu1_data->loop;
    loop2 = eu2_data->loop;

    /* Check they are in different loops */
    if (loop1 == loop2)
    {
        printf("[lc_euler] KEF: Edge uses must be in different loops\n");
        return false;
    }

    /* Get faces */
    loop1_data = (lc_loop_data_t *)lc_entity_get_data(loop1);
    loop2_data = (lc_loop_data_t *)lc_entity_get_data(loop2);
    face1 = loop1_data->face;
    face2 = loop2_data->face;

    if (face1 == face2)
    {
        printf("[lc_euler] KEF: Edge uses must be in different faces\n");
        return false;
    }

    /* Face1 survives, face2 is killed */
    surviving_face = face1;
    killed_face = face2;

    /* Get splice points */
    eu1_prev = eu1_data->prev_in_loop;
    eu1_next = eu1_data->next_in_loop;
    eu2_prev = eu2_data->prev_in_loop;
    eu2_next = eu2_data->next_in_loop;

    eu1_prev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu1_prev);
    eu1_next_data = (lc_edge_use_data_t *)lc_entity_get_data(eu1_next);
    eu2_prev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu2_prev);
    eu2_next_data = (lc_edge_use_data_t *)lc_entity_get_data(eu2_next);

    /* Splice loops */
    eu1_prev_data->next_in_loop = eu2_next;
    eu2_next_data->prev_in_loop = eu1_prev;
    eu2_prev_data->next_in_loop = eu1_next;
    eu1_next_data->prev_in_loop = eu2_prev;

    /* Unregister edge uses before moving (eu1 is in loop1, eu2 is in loop2) */
    unregister_edge_use(edge, eu1);
    unregister_edge_use(edge, eu2);
    lc_entity_remove_child(loop1, eu1);
    lc_entity_remove_child(loop2, eu2);

    /* Move edge uses from loop2 to loop1 (walk from eu2_next to eu2_prev) */
    current_eu = eu2_next;
    while (1)
    {
        current_eu_data = (lc_edge_use_data_t *)lc_entity_get_data(current_eu);
        current_eu_data->loop = loop1;
        lc_entity_remove_child(loop2, current_eu);
        lc_entity_add_child(loop1, current_eu);

        if (current_eu == eu2_prev)
        {
            break;
        }

        current_eu = current_eu_data->next_in_loop;
    }

    /* Invalidate surviving face tessellation */
    face1_data = (lc_face_data_t *)lc_entity_get_data(surviving_face);
    face1_data->mesh_dirty = true;
    lc_tessellate_invalidate(surviving_face);

    /* Destroy entities (lc_entity_destroy frees data internally) */
    lc_entity_destroy(eu1);
    lc_entity_destroy(eu2);
    lc_entity_destroy(edge);
    lc_entity_destroy(loop2);
    lc_entity_destroy(killed_face);

    return true;
}

bool lc_euler_mekl(
    lc_entity_handle_t face,
    lc_entity_handle_t v1,
    lc_entity_handle_t v2,
    lc_entity_handle_t *out_edge)
{
    lc_entity_handle_t loop1, loop2, eu_v1, eu_v2, new_edge, eu_fwd, eu_rev;
    lc_entity_handle_t eu_v1_prev, eu_v2_prev, current_eu;
    lc_edge_use_data_t *eu_v1_data, *eu_v2_data, *eu_v1_prev_data, *eu_v2_prev_data;
    lc_edge_use_data_t *eu_fwd_data, *eu_rev_data, *current_eu_data;
    lc_face_data_t *face_data;
    lc_entity_handle_t child;
    bool found_v1 = false, found_v2 = false;

    /* Validate face */
    if (!lc_entity_is_valid(face))
    {
        printf("[lc_euler] MEKL: Invalid face handle\n");
        return false;
    }

    /* Find which loops contain v1 and v2 */
    loop1 = LC_ENTITY_INVALID;
    loop2 = LC_ENTITY_INVALID;

    child = lc_entity_get_first_child(face);
    while (lc_entity_is_valid(child))
    {
        if (lc_entity_get_type(child) == LC_ENTITY_TYPE_LOOP)
        {
            lc_entity_handle_t test_eu = find_edge_use_for_vertex(child, v1);
            if (test_eu != LC_ENTITY_INVALID)
            {
                loop1 = child;
                found_v1 = true;
            }

            test_eu = find_edge_use_for_vertex(child, v2);
            if (test_eu != LC_ENTITY_INVALID)
            {
                loop2 = child;
                found_v2 = true;
            }
        }
        child = lc_entity_get_next_sibling(child);
    }

    if (!found_v1 || !found_v2)
    {
        printf("[lc_euler] MEKL: Vertices not found in face loops\n");
        return false;
    }

    if (loop1 == loop2)
    {
        printf("[lc_euler] MEKL: Vertices must be in different loops\n");
        return false;
    }

    /* Find edge uses */
    eu_v1 = find_edge_use_for_vertex(loop1, v1);
    eu_v2 = find_edge_use_for_vertex(loop2, v2);

    /* Create new edge */
    new_edge = euler_create_edge(v1, v2);
    if (new_edge == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MEKL: Failed to create edge\n");
        return false;
    }

    /* Create edge uses */
    eu_fwd = euler_create_edge_use(new_edge, loop1, true);
    eu_rev = euler_create_edge_use(new_edge, loop1, false);
    if (eu_fwd == LC_ENTITY_INVALID || eu_rev == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] MEKL: Failed to create edge uses\n");
        lc_entity_destroy(new_edge);
        return false;
    }

    /* Get edge use data */
    eu_v1_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_v1);
    eu_v2_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_v2);
    eu_v1_prev = eu_v1_data->prev_in_loop;
    eu_v2_prev = eu_v2_data->prev_in_loop;
    eu_v1_prev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_v1_prev);
    eu_v2_prev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_v2_prev);
    eu_fwd_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_fwd);
    eu_rev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu_rev);

    /* Splice: eu_v1_prev -> eu_fwd -> eu_v2 -> ... -> eu_v2_prev -> eu_rev -> eu_v1 -> ... */
    eu_v1_prev_data->next_in_loop = eu_fwd;
    eu_fwd_data->prev_in_loop = eu_v1_prev;
    eu_fwd_data->next_in_loop = eu_v2;
    eu_v2_data->prev_in_loop = eu_fwd;
    eu_v2_prev_data->next_in_loop = eu_rev;
    eu_rev_data->prev_in_loop = eu_v2_prev;
    eu_rev_data->next_in_loop = eu_v1;
    eu_v1_data->prev_in_loop = eu_rev;

    /* Move edge uses from loop2 to loop1 */
    current_eu = eu_v2;
    while (1)
    {
        current_eu_data = (lc_edge_use_data_t *)lc_entity_get_data(current_eu);
        current_eu_data->loop = loop1;
        lc_entity_remove_child(loop2, current_eu);
        lc_entity_add_child(loop1, current_eu);

        if (current_eu == eu_v2_prev)
            break;

        current_eu = current_eu_data->next_in_loop;
    }

    /* Destroy loop2 (lc_entity_destroy frees data internally) */
    lc_entity_destroy(loop2);

    /* Invalidate tessellation */
    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    face_data->mesh_dirty = true;
    lc_tessellate_invalidate(face);

    /* Write output handle */
    if (out_edge) *out_edge = new_edge;

    return true;
}

bool lc_euler_keml(lc_entity_handle_t edge, lc_entity_handle_t *out_loop)
{
    lc_edge_data_t *edge_data;
    lc_entity_handle_t eu1, eu2, loop, face, new_loop;
    lc_entity_handle_t eu1_prev, eu1_next, eu2_prev, eu2_next, current_eu, stop_eu;
    lc_edge_use_data_t *eu1_data, *eu2_data, *eu1_prev_data, *eu1_next_data;
    lc_edge_use_data_t *eu2_prev_data, *eu2_next_data, *current_eu_data;
    lc_loop_data_t *loop_data, *new_loop_data;
    lc_face_data_t *face_data;

    /* Validate edge */
    if (!lc_entity_is_valid(edge))
    {
        printf("[lc_euler] KEML: Invalid edge handle\n");
        return false;
    }

    edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);

    /* Check edge has exactly 2 edge uses */
    if (edge_data->edge_use_count != 2)
    {
        printf("[lc_euler] KEML: Edge must have exactly 2 edge uses\n");
        return false;
    }

    /* Get edge uses */
    eu1 = edge_data->edge_uses[0];
    eu2 = edge_data->edge_uses[1];
    eu1_data = (lc_edge_use_data_t *)lc_entity_get_data(eu1);
    eu2_data = (lc_edge_use_data_t *)lc_entity_get_data(eu2);

    /* Check they are in the same loop */
    loop = eu1_data->loop;
    if (loop != eu2_data->loop)
    {
        printf("[lc_euler] KEML: Edge uses must be in the same loop\n");
        return false;
    }

    /* Get face */
    loop_data = (lc_loop_data_t *)lc_entity_get_data(loop);
    face = loop_data->face;

    /* Create new inner loop */
    new_loop = euler_create_loop(face, false);
    if (new_loop == LC_ENTITY_INVALID)
    {
        printf("[lc_euler] KEML: Failed to create loop\n");
        return false;
    }

    /* Get splice points */
    eu1_prev = eu1_data->prev_in_loop;
    eu1_next = eu1_data->next_in_loop;
    eu2_prev = eu2_data->prev_in_loop;
    eu2_next = eu2_data->next_in_loop;

    eu1_prev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu1_prev);
    eu1_next_data = (lc_edge_use_data_t *)lc_entity_get_data(eu1_next);
    eu2_prev_data = (lc_edge_use_data_t *)lc_entity_get_data(eu2_prev);
    eu2_next_data = (lc_edge_use_data_t *)lc_entity_get_data(eu2_next);

    /* Unsplice */
    eu1_prev_data->next_in_loop = eu2_next;
    eu2_next_data->prev_in_loop = eu1_prev;
    eu2_prev_data->next_in_loop = eu1_next;
    eu1_next_data->prev_in_loop = eu2_prev;

    /* Move second chain to new loop (from eu1_next to eu2_prev) */
    current_eu = eu1_next;
    stop_eu = eu2_prev;
    while (1)
    {
        current_eu_data = (lc_edge_use_data_t *)lc_entity_get_data(current_eu);
        current_eu_data->loop = new_loop;
        lc_entity_remove_child(loop, current_eu);
        lc_entity_add_child(new_loop, current_eu);

        if (current_eu == stop_eu)
            break;

        current_eu = current_eu_data->next_in_loop;
    }

    /* Unregister edge uses */
    unregister_edge_use(edge, eu1);
    unregister_edge_use(edge, eu2);
    lc_entity_remove_child(loop, eu1);
    lc_entity_remove_child(loop, eu2);

    /* Set new loop's face */
    new_loop_data = (lc_loop_data_t *)lc_entity_get_data(new_loop);
    new_loop_data->face = face;

    /* Invalidate tessellation */
    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    face_data->mesh_dirty = true;
    lc_tessellate_invalidate(face);

    /* Destroy entities (lc_entity_destroy frees data internally) */
    lc_entity_destroy(eu1);
    lc_entity_destroy(eu2);
    lc_entity_destroy(edge);

    /* Write output handle */
    if (out_loop) *out_loop = new_loop;

    return true;
}

/* MARK: STATIC FUNCTIONS */

static lc_entity_handle_t euler_create_vertex(vec3 position)
{
    lc_entity_handle_t vertex;
    lc_vertex_data_t *vertex_data;

    vertex = lc_entity_create(LC_ENTITY_TYPE_VERTEX);
    if (vertex == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    vertex_data = (lc_vertex_data_t *)malloc(sizeof(lc_vertex_data_t));
    if (!vertex_data)
    {
        lc_entity_destroy(vertex);
        return LC_ENTITY_INVALID;
    }

    glm_vec3_copy(position, vertex_data->position);
    lc_entity_set_data(vertex, vertex_data);

    return vertex;
}

static lc_entity_handle_t euler_create_edge(lc_entity_handle_t v_start, lc_entity_handle_t v_end)
{
    lc_entity_handle_t edge;
    lc_edge_data_t *edge_data;
    lc_vertex_data_t *v_start_data, *v_end_data;
    vec3 direction, origin;
    float length;

    edge = lc_entity_create(LC_ENTITY_TYPE_EDGE);
    if (edge == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    edge_data = (lc_edge_data_t *)malloc(sizeof(lc_edge_data_t));
    if (!edge_data)
    {
        lc_entity_destroy(edge);
        return LC_ENTITY_INVALID;
    }

    /* Get vertex positions */
    v_start_data = (lc_vertex_data_t *)lc_entity_get_data(v_start);
    v_end_data = (lc_vertex_data_t *)lc_entity_get_data(v_end);

    /* Compute line curve */
    glm_vec3_copy(v_start_data->position, origin);
    glm_vec3_sub(v_end_data->position, v_start_data->position, direction);
    length = glm_vec3_norm(direction);
    glm_vec3_normalize(direction);

    edge_data->vertex_start = v_start;
    edge_data->vertex_end = v_end;
    edge_data->curve = lc_geometry_create_line(origin, direction, length);
    edge_data->u_start = 0.0f;
    edge_data->u_end = 1.0f;
    edge_data->edge_use_count = 0;

    lc_entity_set_data(edge, edge_data);

    return edge;
}

static lc_entity_handle_t euler_create_edge_use(lc_entity_handle_t edge, lc_entity_handle_t loop, bool forward)
{
    lc_entity_handle_t edge_use;
    lc_edge_use_data_t *edge_use_data;
    lc_edge_data_t *edge_data;

    edge_use = lc_entity_create(LC_ENTITY_TYPE_EDGE_USE);
    if (edge_use == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    edge_use_data = (lc_edge_use_data_t *)malloc(sizeof(lc_edge_use_data_t));
    if (!edge_use_data)
    {
        lc_entity_destroy(edge_use);
        return LC_ENTITY_INVALID;
    }

    edge_use_data->edge = edge;
    edge_use_data->loop = loop;
    edge_use_data->next_in_loop = LC_ENTITY_INVALID;
    edge_use_data->prev_in_loop = LC_ENTITY_INVALID;
    edge_use_data->forward = forward;

    lc_entity_set_data(edge_use, edge_use_data);

    /* Register with edge */
    edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);
    if (edge_data->edge_use_count < 2)
    {
        edge_data->edge_uses[edge_data->edge_use_count] = edge_use;
        edge_data->edge_use_count++;
    }
    else
    {
        printf("[lc_euler] Warning: Edge already has 2 edge uses\n");
    }

    /* Add as child of loop */
    lc_entity_add_child(loop, edge_use);

    return edge_use;
}

static lc_entity_handle_t euler_create_loop(lc_entity_handle_t face, bool is_outer)
{
    lc_entity_handle_t loop;
    lc_loop_data_t *loop_data;

    loop = lc_entity_create(LC_ENTITY_TYPE_LOOP);
    if (loop == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    loop_data = (lc_loop_data_t *)malloc(sizeof(lc_loop_data_t));
    if (!loop_data)
    {
        lc_entity_destroy(loop);
        return LC_ENTITY_INVALID;
    }

    loop_data->face = face;
    loop_data->is_outer = is_outer;

    lc_entity_set_data(loop, loop_data);

    /* Add as child of face */
    lc_entity_add_child(face, loop);

    return loop;
}

static lc_entity_handle_t euler_create_face(lc_entity_handle_t shell)
{
    lc_entity_handle_t face;
    lc_face_data_t *face_data;
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};
    vec3 y_axis = {0.0f, 1.0f, 0.0f};

    face = lc_entity_create(LC_ENTITY_TYPE_FACE);
    if (face == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    face_data = (lc_face_data_t *)malloc(sizeof(lc_face_data_t));
    if (!face_data)
    {
        lc_entity_destroy(face);
        return LC_ENTITY_INVALID;
    }

    face_data->surface = lc_geometry_create_plane(origin, x_axis, y_axis);
    face_data->outer_loop = LC_ENTITY_INVALID;
    face_data->forward = true;
    face_data->mesh_vertex_count = 0;
    face_data->mesh_triangle_count = 0;
    face_data->mesh_data = NULL;
    face_data->mesh_dirty = true;

    lc_entity_set_data(face, face_data);

    /* Add as child of shell */
    lc_entity_add_child(shell, face);

    return face;
}

static lc_entity_handle_t find_edge_use_for_vertex(lc_entity_handle_t loop, lc_entity_handle_t vertex)
{
    lc_entity_handle_t child, first_eu, current_eu, start_vertex;
    lc_edge_use_data_t *current_eu_data;
    int count;

    /* Find first edge use in loop */
    first_eu = LC_ENTITY_INVALID;
    child = lc_entity_get_first_child(loop);
    while (lc_entity_is_valid(child))
    {
        if (lc_entity_get_type(child) == LC_ENTITY_TYPE_EDGE_USE)
        {
            first_eu = child;
            break;
        }
        child = lc_entity_get_next_sibling(child);
    }

    if (first_eu == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    /* Walk the loop */
    current_eu = first_eu;
    count = 0;
    while (count < 1000)
    {
        start_vertex = edge_use_start_vertex(current_eu);
        if (start_vertex == vertex)
        {
            return current_eu;
        }

        current_eu_data = (lc_edge_use_data_t *)lc_entity_get_data(current_eu);
        current_eu = current_eu_data->next_in_loop;

        if (current_eu == first_eu)
        {
            break;
        }

        count++;
    }

    return LC_ENTITY_INVALID;
}

static lc_entity_handle_t edge_use_start_vertex(lc_entity_handle_t edge_use)
{
    lc_edge_use_data_t *edge_use_data;
    lc_edge_data_t *edge_data;

    edge_use_data = (lc_edge_use_data_t *)lc_entity_get_data(edge_use);
    edge_data = (lc_edge_data_t *)lc_entity_get_data(edge_use_data->edge);

    if (edge_use_data->forward)
    {
        return edge_data->vertex_start;
    }
    else
    {
        return edge_data->vertex_end;
    }
}

static lc_entity_handle_t edge_use_end_vertex(lc_entity_handle_t edge_use)
{
    lc_edge_use_data_t *edge_use_data;
    lc_edge_data_t *edge_data;

    edge_use_data = (lc_edge_use_data_t *)lc_entity_get_data(edge_use);
    edge_data = (lc_edge_data_t *)lc_entity_get_data(edge_use_data->edge);

    if (edge_use_data->forward)
    {
        return edge_data->vertex_end;
    }
    else
    {
        return edge_data->vertex_start;
    }
}

static void unregister_edge_use(lc_entity_handle_t edge, lc_entity_handle_t edge_use)
{
    lc_edge_data_t *edge_data;
    int i, found_index;

    edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);
    found_index = -1;

    /* Find the edge use in the array */
    for (i = 0; i < edge_data->edge_use_count; i++)
    {
        if (edge_data->edge_uses[i] == edge_use)
        {
            found_index = i;
            break;
        }
    }

    if (found_index == -1)
    {
        return;
    }

    /* Shift remaining elements */
    for (i = found_index; i < edge_data->edge_use_count - 1; i++)
    {
        edge_data->edge_uses[i] = edge_data->edge_uses[i + 1];
    }

    edge_data->edge_use_count--;
}
