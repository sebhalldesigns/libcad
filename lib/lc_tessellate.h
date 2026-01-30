/***************************************************************
**
** libcad Header File
**
** File         :  lc_tessellate.h
** Module       :  libcad (tessellation system)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Triangle mesh tessellation for B-Rep faces.
**                 Converts B-Rep faces to renderable triangle meshes
**                 with vertex normals. Provides caching to avoid
**                 redundant tessellation.
**
***************************************************************/

#ifndef LC_TESSELLATE_H
#define LC_TESSELLATE_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_entity.h"
#include <stdbool.h>
#include <stddef.h>

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* Mesh vertex structure with position and normal. */
typedef struct lc_mesh_vertex_t
{
    float position[3];  /* Vertex position in world coordinates */
    float normal[3];    /* Vertex normal (unit vector) */
} lc_mesh_vertex_t;

/* Triangle mesh structure with vertices and indices. */
typedef struct lc_mesh_t
{
    lc_mesh_vertex_t *vertices;  /* Vertex array */
    uint32_t vertex_count;        /* Number of vertices */
    uint32_t *indices;            /* Index array (3 per triangle) */
    uint32_t index_count;         /* Number of indices (3 * triangle_count) */
} lc_mesh_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* Tessellate a single face into a triangle mesh.
 * The mesh is cached in the face's face_data->mesh_data field.
 * Returns true on success, false if face is invalid or tessellation fails.
 */
bool lc_tessellate_face(lc_entity_handle_t face);

/* Tessellate all faces in a solid.
 * Traverses all shells and faces in the solid and tessellates each face.
 * Returns true on success, false if solid is invalid or any face fails.
 */
bool lc_tessellate_solid(lc_entity_handle_t solid);

/* Get the cached mesh for a face.
 * Returns pointer to the cached mesh, or NULL if face is invalid or not tessellated.
 * The returned pointer is valid until the face is re-tessellated or destroyed.
 */
const lc_mesh_t* lc_tessellate_get_mesh(lc_entity_handle_t face);

/* Mark a face's mesh as dirty (needs re-tessellation).
 * Does not free the mesh data; call lc_tessellate_free_mesh for that.
 * Returns true if face is valid, false otherwise.
 */
bool lc_tessellate_invalidate(lc_entity_handle_t face);

/* Free cached mesh data for a face.
 * Sets mesh_data to NULL and mesh_dirty to true.
 * Returns true if face is valid, false otherwise.
 */
bool lc_tessellate_free_mesh(lc_entity_handle_t face);

#ifdef __cplusplus
}
#endif

#endif /* LC_TESSELLATE_H */
