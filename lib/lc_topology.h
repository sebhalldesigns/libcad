/***************************************************************
**
** libcad Header File
**
** File         :  lc_topology.h
** Module       :  libcad (topology queries)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Topological query API for B-Rep traversal and
**                 analysis. Provides functions to navigate the
**                 topology hierarchy (solid -> shell -> face ->
**                 loop -> edge -> vertex) and compute geometric
**                 properties.
**
***************************************************************/

#ifndef LC_TOPOLOGY_H
#define LC_TOPOLOGY_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_entity.h"
#include <cglm/cglm.h>
#include <stdbool.h>
#include <stddef.h>

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* Get all shells in a solid.
 * Fills out_handles array with up to max_count handles.
 * Returns actual count (may exceed max_count if more shells exist).
 */
size_t lc_topology_get_shells(lc_entity_handle_t solid,
                               lc_entity_handle_t *out_handles,
                               size_t max_count);

/* Get all faces in a shell.
 * Fills out_handles array with up to max_count handles.
 * Returns actual count (may exceed max_count if more faces exist).
 */
size_t lc_topology_get_faces(lc_entity_handle_t shell,
                              lc_entity_handle_t *out_handles,
                              size_t max_count);

/* Get all loops in a face (outer loop + holes).
 * Fills out_handles array with up to max_count handles.
 * Returns actual count (may exceed max_count if more loops exist).
 */
size_t lc_topology_get_loops(lc_entity_handle_t face,
                              lc_entity_handle_t *out_handles,
                              size_t max_count);

/* Get all edge uses in a loop (ordered, following next_in_loop chain).
 * Fills out_handles array with up to max_count handles.
 * Returns actual count (may exceed max_count if more edge uses exist).
 */
size_t lc_topology_get_edge_uses(lc_entity_handle_t loop,
                                  lc_entity_handle_t *out_handles,
                                  size_t max_count);

/* Get all edges in a loop (ordered, resolving edge uses to parent edges).
 * Fills out_handles array with up to max_count handles.
 * Returns actual count (may exceed max_count if more edges exist).
 */
size_t lc_topology_get_edges(lc_entity_handle_t loop,
                              lc_entity_handle_t *out_handles,
                              size_t max_count);

/* Get vertices of an edge (start and end).
 * Returns true if edge is valid and has vertices, false otherwise.
 */
bool lc_topology_get_edge_vertices(lc_entity_handle_t edge,
                                    lc_entity_handle_t *out_start,
                                    lc_entity_handle_t *out_end);

/* Get vertex position.
 * Returns true if vertex is valid, false otherwise.
 */
bool lc_topology_get_vertex_position(lc_entity_handle_t vertex, vec3 out_pos);

/* Get all faces adjacent to an edge (via edge uses, 0-2 faces).
 * Fills out_handles array with up to max_count handles.
 * Returns actual count (0 for wire edges, 1 for boundary, 2 for interior).
 */
size_t lc_topology_get_edge_faces(lc_entity_handle_t edge,
                                   lc_entity_handle_t *out_handles,
                                   size_t max_count);

/* Count B-Rep elements in solid.
 * Counts vertices, edges, and faces by traversing the topology hierarchy.
 * Any output pointer may be NULL to skip that count.
 */
void lc_topology_count_elements(lc_entity_handle_t solid,
                                 size_t *out_vertices,
                                 size_t *out_edges,
                                 size_t *out_faces);

/* Check if solid satisfies Euler characteristic: V - E + F = 2 for closed manifold.
 * Returns true if the characteristic holds, false otherwise.
 */
bool lc_topology_check_euler(lc_entity_handle_t solid);

/* Compute bounding box of solid from vertex positions.
 * Traverses all vertices in the solid and computes min/max extents.
 */
void lc_topology_compute_bbox(lc_entity_handle_t solid, vec3 out_min, vec3 out_max);

#ifdef __cplusplus
}
#endif

#endif /* LC_TOPOLOGY_H */
