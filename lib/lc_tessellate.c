/***************************************************************
**
** libcad Implementation File
**
** File         :  lc_tessellate.c
** Module       :  libcad (tessellation system)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Triangle mesh tessellation for B-Rep faces.
**                 Implements simple ear-clipping tessellation for
**                 planar faces. Non-planar faces (cylinders, spheres)
**                 will be added in future phases.
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_tessellate.h"
#include "lc_topology.h"
#include "lc_geometry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cglm/cglm.h>

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static bool tessellate_planar_face(lc_entity_handle_t face, lc_mesh_t *out_mesh);
static void compute_face_normal(lc_entity_handle_t face, vec3 out_normal);
static void free_mesh_internal(lc_mesh_t *mesh);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

bool lc_tessellate_face(lc_entity_handle_t face)
{
    /* Validate face entity */
    if (!lc_entity_is_valid(face))
    {
        return false;
    }

    if (lc_entity_get_type(face) != LC_ENTITY_TYPE_FACE)
    {
        return false;
    }

    /* Get face data */
    lc_face_data_t *face_data = (lc_face_data_t*)lc_entity_get_data(face);
    if (face_data == NULL)
    {
        return false;
    }

    /* Free existing mesh if present */
    if (face_data->mesh_data != NULL)
    {
        lc_mesh_t *old_mesh = (lc_mesh_t*)face_data->mesh_data;
        free_mesh_internal(old_mesh);
        free(old_mesh);
        face_data->mesh_data = NULL;
    }

    /* Allocate new mesh structure */
    lc_mesh_t *mesh = (lc_mesh_t*)malloc(sizeof(lc_mesh_t));
    if (mesh == NULL)
    {
        return false;
    }
    memset(mesh, 0, sizeof(lc_mesh_t));

    /* Tessellate based on surface type */
    /* For Phase 4E, only planar faces are supported */
    bool success = tessellate_planar_face(face, mesh);

    if (success)
    {
        /* Store mesh in face data */
        face_data->mesh_data = mesh;
        face_data->mesh_vertex_count = mesh->vertex_count;
        face_data->mesh_triangle_count = mesh->index_count / 3;
        face_data->mesh_dirty = false;
    }
    else
    {
        /* Cleanup on failure */
        free(mesh);
    }

    return success;
}

bool lc_tessellate_solid(lc_entity_handle_t solid)
{
    /* Validate solid entity */
    if (!lc_entity_is_valid(solid))
    {
        return false;
    }

    if (lc_entity_get_type(solid) != LC_ENTITY_TYPE_SOLID)
    {
        return false;
    }

    /* Get all shells in solid */
    lc_entity_handle_t shells[16];
    size_t shell_count = lc_topology_get_shells(solid, shells, 16);

    /* Iterate all shells */
    size_t i;
    for (i = 0; i < shell_count; i++)
    {
        /* Get all faces in shell */
        lc_entity_handle_t faces[64];
        size_t face_count = lc_topology_get_faces(shells[i], faces, 64);

        /* Tessellate each face */
        size_t j;
        for (j = 0; j < face_count; j++)
        {
            if (!lc_tessellate_face(faces[j]))
            {
                return false;
            }
        }
    }

    return true;
}

const lc_mesh_t* lc_tessellate_get_mesh(lc_entity_handle_t face)
{
    /* Validate face entity */
    if (!lc_entity_is_valid(face))
    {
        return NULL;
    }

    if (lc_entity_get_type(face) != LC_ENTITY_TYPE_FACE)
    {
        return NULL;
    }

    /* Get face data */
    lc_face_data_t *face_data = (lc_face_data_t*)lc_entity_get_data(face);
    if (face_data == NULL)
    {
        return NULL;
    }

    return (const lc_mesh_t*)face_data->mesh_data;
}

bool lc_tessellate_invalidate(lc_entity_handle_t face)
{
    /* Validate face entity */
    if (!lc_entity_is_valid(face))
    {
        return false;
    }

    if (lc_entity_get_type(face) != LC_ENTITY_TYPE_FACE)
    {
        return false;
    }

    /* Get face data */
    lc_face_data_t *face_data = (lc_face_data_t*)lc_entity_get_data(face);
    if (face_data == NULL)
    {
        return false;
    }

    /* Mark mesh as dirty */
    face_data->mesh_dirty = true;

    return true;
}

bool lc_tessellate_free_mesh(lc_entity_handle_t face)
{
    /* Validate face entity */
    if (!lc_entity_is_valid(face))
    {
        return false;
    }

    if (lc_entity_get_type(face) != LC_ENTITY_TYPE_FACE)
    {
        return false;
    }

    /* Get face data */
    lc_face_data_t *face_data = (lc_face_data_t*)lc_entity_get_data(face);
    if (face_data == NULL)
    {
        return false;
    }

    /* Free mesh data if present */
    if (face_data->mesh_data != NULL)
    {
        lc_mesh_t *mesh = (lc_mesh_t*)face_data->mesh_data;
        free_mesh_internal(mesh);
        free(mesh);
        face_data->mesh_data = NULL;
    }

    /* Reset mesh fields */
    face_data->mesh_vertex_count = 0;
    face_data->mesh_triangle_count = 0;
    face_data->mesh_dirty = true;

    return true;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static bool tessellate_planar_face(lc_entity_handle_t face, lc_mesh_t *out_mesh)
{
    /* Get face data */
    lc_face_data_t *face_data = (lc_face_data_t*)lc_entity_get_data(face);
    if (face_data == NULL || face_data->outer_loop == LC_ENTITY_INVALID)
    {
        return false;
    }

    /* Get edge uses from outer loop */
    lc_entity_handle_t edge_uses[64];
    size_t edge_use_count = lc_topology_get_edge_uses(face_data->outer_loop, edge_uses, 64);

    if (edge_use_count < 3)
    {
        /* Degenerate face */
        return false;
    }

    /* Allocate vertex array (one vertex per edge use) */
    lc_mesh_vertex_t *vertices = (lc_mesh_vertex_t*)malloc(sizeof(lc_mesh_vertex_t) * edge_use_count);
    if (vertices == NULL)
    {
        return false;
    }

    /* Compute face normal */
    vec3 face_normal;
    compute_face_normal(face, face_normal);

    /* Collect vertex positions in loop order */
    size_t i;
    for (i = 0; i < edge_use_count; i++)
    {
        /* Get edge use data */
        lc_edge_use_data_t *edge_use_data = (lc_edge_use_data_t*)lc_entity_get_data(edge_uses[i]);
        if (edge_use_data == NULL)
        {
            free(vertices);
            return false;
        }

        /* Get edge data */
        lc_edge_data_t *edge_data = (lc_edge_data_t*)lc_entity_get_data(edge_use_data->edge);
        if (edge_data == NULL)
        {
            free(vertices);
            return false;
        }

        /* Get start vertex based on edge use orientation */
        lc_entity_handle_t vertex_handle;
        if (edge_use_data->forward)
        {
            /* Forward: use edge start vertex */
            vertex_handle = edge_data->vertex_start;
        }
        else
        {
            /* Reverse: use edge end vertex */
            vertex_handle = edge_data->vertex_end;
        }

        /* Get vertex position */
        vec3 pos;
        if (!lc_topology_get_vertex_position(vertex_handle, pos))
        {
            free(vertices);
            return false;
        }

        /* Store vertex position and normal */
        vertices[i].position[0] = pos[0];
        vertices[i].position[1] = pos[1];
        vertices[i].position[2] = pos[2];
        vertices[i].normal[0] = face_normal[0];
        vertices[i].normal[1] = face_normal[1];
        vertices[i].normal[2] = face_normal[2];
    }

    /* Triangulate using simple fan from first vertex */
    /* This works for convex planar faces (sufficient for box faces) */
    uint32_t triangle_count = (uint32_t)(edge_use_count - 2);
    uint32_t index_count = triangle_count * 3;

    uint32_t *indices = (uint32_t*)malloc(sizeof(uint32_t) * index_count);
    if (indices == NULL)
    {
        free(vertices);
        return false;
    }

    /* Generate triangle fan indices */
    for (i = 0; i < triangle_count; i++)
    {
        indices[i * 3 + 0] = 0;               /* First vertex (fan origin) */
        indices[i * 3 + 1] = (uint32_t)(i + 1);
        indices[i * 3 + 2] = (uint32_t)(i + 2);
    }

    /* Store mesh data */
    out_mesh->vertices = vertices;
    out_mesh->vertex_count = (uint32_t)edge_use_count;
    out_mesh->indices = indices;
    out_mesh->index_count = index_count;

    return true;
}

static void compute_face_normal(lc_entity_handle_t face, vec3 out_normal)
{
    /* Get face data */
    lc_face_data_t *face_data = (lc_face_data_t*)lc_entity_get_data(face);
    if (face_data == NULL || face_data->outer_loop == LC_ENTITY_INVALID)
    {
        /* Default normal if computation fails */
        out_normal[0] = 0.0f;
        out_normal[1] = 0.0f;
        out_normal[2] = 1.0f;
        return;
    }

    /* Get first 3 edge uses to compute normal from cross product */
    lc_entity_handle_t edge_uses[3];
    size_t edge_use_count = lc_topology_get_edge_uses(face_data->outer_loop, edge_uses, 3);

    if (edge_use_count < 3)
    {
        /* Default normal for degenerate face */
        out_normal[0] = 0.0f;
        out_normal[1] = 0.0f;
        out_normal[2] = 1.0f;
        return;
    }

    /* Get positions of first 3 vertices */
    vec3 p0, p1, p2;
    size_t i;
    vec3 positions[3];

    for (i = 0; i < 3; i++)
    {
        /* Get edge use data */
        lc_edge_use_data_t *edge_use_data = (lc_edge_use_data_t*)lc_entity_get_data(edge_uses[i]);
        if (edge_use_data == NULL)
        {
            out_normal[0] = 0.0f;
            out_normal[1] = 0.0f;
            out_normal[2] = 1.0f;
            return;
        }

        /* Get edge data */
        lc_edge_data_t *edge_data = (lc_edge_data_t*)lc_entity_get_data(edge_use_data->edge);
        if (edge_data == NULL)
        {
            out_normal[0] = 0.0f;
            out_normal[1] = 0.0f;
            out_normal[2] = 1.0f;
            return;
        }

        /* Get start vertex based on orientation */
        lc_entity_handle_t vertex_handle;
        if (edge_use_data->forward)
        {
            vertex_handle = edge_data->vertex_start;
        }
        else
        {
            vertex_handle = edge_data->vertex_end;
        }

        /* Get vertex position */
        if (!lc_topology_get_vertex_position(vertex_handle, positions[i]))
        {
            out_normal[0] = 0.0f;
            out_normal[1] = 0.0f;
            out_normal[2] = 1.0f;
            return;
        }
    }

    glm_vec3_copy(positions[0], p0);
    glm_vec3_copy(positions[1], p1);
    glm_vec3_copy(positions[2], p2);

    /* Compute edge vectors */
    vec3 edge1, edge2;
    glm_vec3_sub(p1, p0, edge1);
    glm_vec3_sub(p2, p0, edge2);

    /* Compute cross product (normal) */
    vec3 normal;
    glm_vec3_cross(edge1, edge2, normal);

    /* Normalize */
    glm_vec3_normalize(normal);

    /* Check face orientation and negate if necessary */
    if (!face_data->forward)
    {
        glm_vec3_negate(normal);
    }

    glm_vec3_copy(normal, out_normal);
}

static void free_mesh_internal(lc_mesh_t *mesh)
{
    if (mesh == NULL)
    {
        return;
    }

    if (mesh->vertices != NULL)
    {
        free(mesh->vertices);
        mesh->vertices = NULL;
    }

    if (mesh->indices != NULL)
    {
        free(mesh->indices);
        mesh->indices = NULL;
    }

    mesh->vertex_count = 0;
    mesh->index_count = 0;
}
