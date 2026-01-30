/***************************************************************
**
** libcad Header File
**
** File         :  lc_brep.h
** Module       :  libcad (B-Rep construction)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  B-Rep (Boundary Representation) construction API.
**                 Provides functions to create and manipulate solid
**                 geometry using topological entities (vertex, edge,
**                 loop, face, shell, solid).
**
***************************************************************/

#ifndef LC_BREP_H
#define LC_BREP_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_entity.h"
#include <cglm/cglm.h>
#include <stdbool.h>

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* Initialize B-Rep system. Call once at startup. */
void lc_brep_init(void);

/* Shutdown B-Rep system. */
void lc_brep_shutdown(void);

/*
 * Create a box solid (rectangular prism).
 * origin: box corner at minimum x,y,z
 * dimensions: width (x), height (y), depth (z)
 * Creates: 8 vertices, 12 edges, 6 faces, 6 loops, 24 edge uses, 1 shell, 1 solid
 * Returns solid entity handle on success, LC_ENTITY_INVALID on failure.
 */
lc_entity_handle_t lc_brep_create_box(vec3 origin, vec3 dimensions);

/*
 * Create a cylinder solid.
 * base_center: center of bottom circle
 * axis: cylinder axis direction (will be normalized)
 * radius: cylinder radius
 * height: cylinder height along axis
 * segments: number of segments for circle approximation (min 3)
 * Returns solid entity handle on success, LC_ENTITY_INVALID on failure.
 */
lc_entity_handle_t lc_brep_create_cylinder(vec3 base_center, vec3 axis, float radius, float height, int segments);

/*
 * Create a sphere solid.
 * center: sphere center
 * radius: sphere radius
 * u_segments: number of longitudinal segments (min 3)
 * v_segments: number of latitudinal segments (min 2)
 * Returns solid entity handle on success, LC_ENTITY_INVALID on failure.
 */
lc_entity_handle_t lc_brep_create_sphere(vec3 center, float radius, int u_segments, int v_segments);

/*
 * Validate a solid: check manifold, closed shells, consistent orientations.
 * Returns true if solid is valid.
 */
bool lc_brep_validate_solid(lc_entity_handle_t solid);

/*
 * Destroy a solid and all its child topology entities (shell, faces, loops, edges, vertices, edge uses).
 * Returns true on success.
 */
bool lc_brep_destroy_solid(lc_entity_handle_t solid);

#ifdef __cplusplus
}
#endif

#endif /* LC_BREP_H */
