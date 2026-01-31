/***************************************************************
**
** libcad Header File
**
** File         :  lc_euler.h
** Module       :  libcad (Euler operators)
** Author       :  SH
** Created      :  2026-01-31 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Euler operators for topologically valid B-Rep
**                 solid modeling. Implements MVEF, MEV, MEF, KEV,
**                 KEF, MEKL, KEML operators that maintain the
**                 Euler characteristic (V - E + F = 2).
**
***************************************************************/

#ifndef LC_EULER_H
#define LC_EULER_H

#ifdef __cplusplus
extern "C" {
#endif

/* MARK: INCLUDES */

#include "lc_entity.h"
#include <cglm/cglm.h>
#include <stdbool.h>

/* MARK: PUBLIC FUNCTIONS */

/*
 * MVEF: Make-Vertex-Edge-Face
 *
 * Bootstrap a new solid face in a shell. Creates the first topological
 * elements: 2 vertices, 1 edge, 2 edge uses, 1 loop, 1 face.
 *
 * This is typically the first operation when creating a new solid from scratch.
 * The edge connects v1 to v2, and both edge uses are on the new loop.
 *
 * Parameters:
 *   shell   - Parent shell entity (must be ENTITY_SHELL type)
 *   v1_pos  - Position of first vertex
 *   v2_pos  - Position of second vertex
 *   out_face   - Optional output: handle to created face
 *   out_edge   - Optional output: handle to created edge
 *   out_v1     - Optional output: handle to first vertex
 *   out_v2     - Optional output: handle to second vertex
 *
 * Returns: true on success, false on failure (invalid shell, allocation failure)
 */
bool lc_euler_mvef(lc_entity_handle_t shell, vec3 v1_pos, vec3 v2_pos,
                   lc_entity_handle_t *out_face, lc_entity_handle_t *out_edge,
                   lc_entity_handle_t *out_v1, lc_entity_handle_t *out_v2);

/*
 * MEV: Make-Edge-Vertex
 *
 * Add a new vertex and edge to an existing loop vertex. Creates 1 vertex,
 * 1 edge, and 2 edge uses that are spliced into the loop at existing_vertex.
 *
 * This extends a loop by adding a new edge emanating from an existing vertex.
 * The loop traversal order is preserved.
 *
 * Parameters:
 *   loop            - Loop to extend (must be ENTITY_LOOP type)
 *   existing_vertex - Vertex in the loop to attach new edge to
 *   new_pos         - Position of new vertex
 *   out_vertex      - Optional output: handle to created vertex
 *   out_edge        - Optional output: handle to created edge
 *
 * Returns: true on success, false on failure (invalid inputs, vertex not in loop)
 */
bool lc_euler_mev(lc_entity_handle_t loop, lc_entity_handle_t existing_vertex,
                  vec3 new_pos, lc_entity_handle_t *out_vertex,
                  lc_entity_handle_t *out_edge);

/*
 * MEF: Make-Edge-Face
 *
 * Split a face by connecting two vertices on the same loop. Creates 1 edge,
 * 2 edge uses, 1 new loop, and 1 new face. The original face and loop are
 * split by the new edge.
 *
 * This is the primary operation for subdividing faces during modeling.
 * Both vertices must be on the same loop of the face.
 *
 * Parameters:
 *   face     - Face to split (must be ENTITY_FACE type)
 *   v1       - First vertex on face's loop
 *   v2       - Second vertex on same loop
 *   out_face - Optional output: handle to new face
 *   out_edge - Optional output: handle to created edge
 *
 * Returns: true on success, false on failure (vertices not on same loop)
 */
bool lc_euler_mef(lc_entity_handle_t face, lc_entity_handle_t v1,
                  lc_entity_handle_t v2, lc_entity_handle_t *out_face,
                  lc_entity_handle_t *out_edge);

/*
 * KEV: Kill-Edge-Vertex
 *
 * Inverse of MEV. Removes an edge and one of its endpoint vertices.
 * The vertex_to_kill must be an endpoint of the edge. The other vertex
 * remains in the loop.
 *
 * This operation reduces the complexity of a loop by removing a vertex.
 *
 * Parameters:
 *   edge           - Edge to remove (must be ENTITY_EDGE type)
 *   vertex_to_kill - Endpoint vertex to remove (must be ENTITY_VERTEX type)
 *
 * Returns: true on success, false on failure (vertex not on edge, edge has >2 uses)
 */
bool lc_euler_kev(lc_entity_handle_t edge, lc_entity_handle_t vertex_to_kill);

/*
 * KEF: Kill-Edge-Face
 *
 * Inverse of MEF. Removes an edge and merges two adjacent faces into one.
 * The edge must separate exactly two faces. One face is deleted, the other
 * is expanded to include both loops.
 *
 * This operation simplifies a solid by merging coplanar or adjacent faces.
 *
 * Parameters:
 *   edge - Edge to remove (must separate two faces)
 *
 * Returns: true on success, false on failure (edge doesn't separate 2 faces)
 */
bool lc_euler_kef(lc_entity_handle_t edge);

/*
 * MEKL: Make-Edge-Kill-Loop
 *
 * Connect two vertices on different loops of the same face. Creates 1 edge
 * and merges the 2 loops into 1. This operation creates an inner loop (hole)
 * in a face or connects an inner loop to the outer boundary.
 *
 * The two vertices must be on different loops of the same face.
 *
 * Parameters:
 *   face     - Face containing both loops
 *   v1       - Vertex on first loop
 *   v2       - Vertex on second loop (different from v1's loop)
 *   out_edge - Optional output: handle to created edge
 *
 * Returns: true on success, false on failure (vertices on same loop)
 */
bool lc_euler_mekl(lc_entity_handle_t face, lc_entity_handle_t v1,
                   lc_entity_handle_t v2, lc_entity_handle_t *out_edge);

/*
 * KEML: Kill-Edge-Make-Loop
 *
 * Inverse of MEKL. Removes an edge and splits a loop into two loops.
 * This creates a hole in a face or separates an inner loop from the outer boundary.
 *
 * The edge must be on a loop with at least 4 edges (to split into 2 valid loops).
 *
 * Parameters:
 *   edge     - Edge to remove
 *   out_loop - Optional output: handle to new loop
 *
 * Returns: true on success, false on failure (loop too small to split)
 */
bool lc_euler_keml(lc_entity_handle_t edge, lc_entity_handle_t *out_loop);

#ifdef __cplusplus
}
#endif

#endif /* LC_EULER_H */
