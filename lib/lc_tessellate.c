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
#include <math.h>
#include <cglm/cglm.h>

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static bool tessellate_planar_face(lc_entity_handle_t face, lc_mesh_t *out_mesh);
static bool tessellate_cylinder_face(lc_entity_handle_t face, lc_mesh_t *out_mesh);
static bool tessellate_sphere_face(lc_entity_handle_t face, lc_mesh_t *out_mesh);
static void compute_face_normal(lc_entity_handle_t face, vec3 out_normal);
static void free_mesh_internal(lc_mesh_t *mesh);

/* Number of subdivisions per edge for curved face tessellation */
#define CURVED_SUBDIVISIONS 4

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
    lc_surface_type_t surface_type = lc_geometry_get_surface_type(face_data->surface);
    bool success;

    switch (surface_type)
    {
        case LC_SURFACE_CYLINDER:
            success = tessellate_cylinder_face(face, mesh);
            break;
        case LC_SURFACE_SPHERE:
            success = tessellate_sphere_face(face, mesh);
            break;
        default:
            success = tessellate_planar_face(face, mesh);
            break;
    }

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

static bool tessellate_cylinder_face(lc_entity_handle_t face, lc_mesh_t *out_mesh)
{
    /* Get face data */
    lc_face_data_t *face_data = (lc_face_data_t *)lc_entity_get_data(face);
    if (face_data == NULL || face_data->outer_loop == LC_ENTITY_INVALID)
    {
        return false;
    }

    /* Get edge uses from outer loop */
    lc_entity_handle_t edge_uses_local[64];
    size_t edge_use_count = lc_topology_get_edge_uses(face_data->outer_loop, edge_uses_local, 64);

    if (edge_use_count != 4)
    {
        return tessellate_planar_face(face, out_mesh);
    }

    /* Get cylinder surface data */
    lc_surface_handle_t surf = face_data->surface;
    lc_surface_cylinder_t cyl_data;
    if (!lc_geometry_get_cylinder_data(surf, &cyl_data))
    {
        return tessellate_planar_face(face, out_mesh);
    }

    /* Compute angle and axial position for all 4 corners */
    float corner_angles[4];
    float corner_axials[4];
    size_t ci;
    for (ci = 0; ci < 4; ci++)
    {
        lc_edge_use_data_t *eu_data = (lc_edge_use_data_t *)lc_entity_get_data(edge_uses_local[ci]);
        if (eu_data == NULL)
        {
            return false;
        }
        lc_edge_data_t *e_data = (lc_edge_data_t *)lc_entity_get_data(eu_data->edge);
        if (e_data == NULL)
        {
            return false;
        }
        lc_entity_handle_t vh = eu_data->forward ? e_data->vertex_start : e_data->vertex_end;
        vec3 pos;
        if (!lc_topology_get_vertex_position(vh, pos))
        {
            return false;
        }

        vec3 diff;
        glm_vec3_sub(pos, cyl_data.origin, diff);
        corner_axials[ci] = glm_vec3_dot(diff, cyl_data.axis);

        vec3 axial_comp, radial;
        glm_vec3_scale(cyl_data.axis, corner_axials[ci], axial_comp);
        glm_vec3_sub(diff, axial_comp, radial);
        corner_angles[ci] = atan2f(glm_vec3_dot(radial, cyl_data.y_axis),
                                   glm_vec3_dot(radial, cyl_data.x_axis));
    }

    /* Identify which pair of consecutive corners spans the arc (different angles)
     * and which pair shares the same angle (axial edges).
     * Walk the 4 edges and find one where the angle changes. */
    int arc_start = -1;
    int ei;
    for (ei = 0; ei < 4; ei++)
    {
        int next = (ei + 1) % 4;
        float angle_diff = fabsf(corner_angles[next] - corner_angles[ei]);
        /* Normalize angle diff */
        if (angle_diff > (float)M_PI)
        {
            angle_diff = 2.0f * (float)M_PI - angle_diff;
        }
        if (angle_diff > 0.01f)
        {
            arc_start = ei;
            break;
        }
    }

    if (arc_start < 0)
    {
        return tessellate_planar_face(face, out_mesh);
    }

    /* The quad corners in order starting from arc_start are:
     * arc_start: bottom-left (angle_a, axial_bot)
     * arc_start+1: bottom-right (angle_b, axial_bot)
     * arc_start+2: top-right (angle_b, axial_top)
     * arc_start+3: top-left (angle_a, axial_top) */
    int i0 = arc_start;
    int i1 = (arc_start + 1) % 4;
    int i2 = (arc_start + 2) % 4;
    int i3 = (arc_start + 3) % 4;

    float angle_a = corner_angles[i0];
    float angle_b = corner_angles[i1];
    float axial_bot = corner_axials[i0];
    float axial_top = corner_axials[i3];

    /* Handle angle wrapping */
    if (angle_b < angle_a)
    {
        angle_b += 2.0f * (float)M_PI;
    }

    /* Avoid zero-span arcs */
    if (fabsf(angle_b - angle_a) < 0.001f)
    {
        return tessellate_planar_face(face, out_mesh);
    }

    int subdiv = CURVED_SUBDIVISIONS;
    int cols = subdiv + 1;
    int vert_count = cols * 2;
    int tri_count = subdiv * 2;

    lc_mesh_vertex_t *vertices = (lc_mesh_vertex_t *)malloc(sizeof(lc_mesh_vertex_t) * vert_count);
    if (vertices == NULL)
    {
        return false;
    }

    uint32_t *indices = (uint32_t *)malloc(sizeof(uint32_t) * tri_count * 3);
    if (indices == NULL)
    {
        free(vertices);
        return false;
    }

    int col;
    for (col = 0; col <= subdiv; col++)
    {
        float t = (float)col / (float)subdiv;
        float angle = angle_a + t * (angle_b - angle_a);

        /* Bottom vertex */
        vec3 eval_bot;
        lc_geometry_eval_surface(surf, angle, axial_bot, eval_bot);
        vertices[col].position[0] = eval_bot[0];
        vertices[col].position[1] = eval_bot[1];
        vertices[col].position[2] = eval_bot[2];

        vec3 normal_bot;
        lc_geometry_eval_surface_normal(surf, angle, axial_bot, normal_bot);
        vertices[col].normal[0] = normal_bot[0];
        vertices[col].normal[1] = normal_bot[1];
        vertices[col].normal[2] = normal_bot[2];

        /* Top vertex */
        int top_idx = cols + col;
        vec3 eval_top;
        lc_geometry_eval_surface(surf, angle, axial_top, eval_top);
        vertices[top_idx].position[0] = eval_top[0];
        vertices[top_idx].position[1] = eval_top[1];
        vertices[top_idx].position[2] = eval_top[2];

        vec3 normal_top;
        lc_geometry_eval_surface_normal(surf, angle, axial_top, normal_top);
        vertices[top_idx].normal[0] = normal_top[0];
        vertices[top_idx].normal[1] = normal_top[1];
        vertices[top_idx].normal[2] = normal_top[2];
    }

    /* Generate triangle indices */
    int idx = 0;
    for (col = 0; col < subdiv; col++)
    {
        int bl = col;
        int br = col + 1;
        int tl = cols + col;
        int tr = cols + col + 1;

        indices[idx++] = (uint32_t)bl;
        indices[idx++] = (uint32_t)br;
        indices[idx++] = (uint32_t)tr;

        indices[idx++] = (uint32_t)bl;
        indices[idx++] = (uint32_t)tr;
        indices[idx++] = (uint32_t)tl;
    }

    out_mesh->vertices = vertices;
    out_mesh->vertex_count = (uint32_t)vert_count;
    out_mesh->indices = indices;
    out_mesh->index_count = (uint32_t)(tri_count * 3);

    return true;
}

static bool tessellate_sphere_face(lc_entity_handle_t face, lc_mesh_t *out_mesh)
{
    /* Get face data */
    lc_face_data_t *face_data = (lc_face_data_t *)lc_entity_get_data(face);
    if (face_data == NULL || face_data->outer_loop == LC_ENTITY_INVALID)
    {
        return false;
    }

    /* Get edge uses from outer loop */
    lc_entity_handle_t edge_uses_arr[64];
    size_t edge_use_count = lc_topology_get_edge_uses(face_data->outer_loop, edge_uses_arr, 64);

    if (edge_use_count < 3)
    {
        return false;
    }

    /* Get sphere surface data */
    lc_surface_handle_t surf = face_data->surface;
    lc_surface_sphere_t sph_data;
    if (!lc_geometry_get_sphere_data(surf, &sph_data))
    {
        return tessellate_planar_face(face, out_mesh);
    }

    /* Collect corner positions */
    vec3 *corners = (vec3 *)malloc(sizeof(vec3) * edge_use_count);
    if (corners == NULL)
    {
        return false;
    }

    size_t eci;
    for (eci = 0; eci < edge_use_count; eci++)
    {
        lc_edge_use_data_t *eu_data = (lc_edge_use_data_t *)lc_entity_get_data(edge_uses_arr[eci]);
        if (eu_data == NULL)
        {
            free(corners);
            return false;
        }
        lc_edge_data_t *e_data = (lc_edge_data_t *)lc_entity_get_data(eu_data->edge);
        if (e_data == NULL)
        {
            free(corners);
            return false;
        }
        lc_entity_handle_t vh = eu_data->forward ? e_data->vertex_start : e_data->vertex_end;
        if (!lc_topology_get_vertex_position(vh, corners[eci]))
        {
            free(corners);
            return false;
        }
    }

    if (edge_use_count == 3)
    {
        /* Triangle face (pole fans): subdivide and project onto sphere */
        int subdiv = CURVED_SUBDIVISIONS;
        int total_verts = (subdiv + 1) * (subdiv + 2) / 2;
        int total_tris = subdiv * subdiv;

        lc_mesh_vertex_t *vertices = (lc_mesh_vertex_t *)malloc(sizeof(lc_mesh_vertex_t) * total_verts);
        uint32_t *indices = (uint32_t *)malloc(sizeof(uint32_t) * total_tris * 3);
        if (vertices == NULL || indices == NULL)
        {
            free(vertices);
            free(indices);
            free(corners);
            return false;
        }

        /* Generate vertices using barycentric coordinates */
        int vert_idx = 0;
        int row, col;
        for (row = 0; row <= subdiv; row++)
        {
            for (col = 0; col <= subdiv - row; col++)
            {
                float u = (float)col / (float)subdiv;
                float v = (float)row / (float)subdiv;
                float w = 1.0f - u - v;

                vec3 pos;
                vec3 tmp;
                glm_vec3_scale(corners[0], w, pos);
                glm_vec3_scale(corners[1], u, tmp);
                glm_vec3_add(pos, tmp, pos);
                glm_vec3_scale(corners[2], v, tmp);
                glm_vec3_add(pos, tmp, pos);

                /* Project onto sphere */
                vec3 dir;
                glm_vec3_sub(pos, sph_data.center, dir);
                float len = glm_vec3_norm(dir);
                if (len > 1e-6f)
                {
                    glm_vec3_scale(dir, sph_data.radius / len, dir);
                }
                vec3 sphere_pos;
                glm_vec3_add(sph_data.center, dir, sphere_pos);

                vertices[vert_idx].position[0] = sphere_pos[0];
                vertices[vert_idx].position[1] = sphere_pos[1];
                vertices[vert_idx].position[2] = sphere_pos[2];

                vec3 normal;
                glm_vec3_sub(sphere_pos, sph_data.center, normal);
                glm_vec3_normalize(normal);
                vertices[vert_idx].normal[0] = normal[0];
                vertices[vert_idx].normal[1] = normal[1];
                vertices[vert_idx].normal[2] = normal[2];

                vert_idx++;
            }
        }

        /* Generate triangle indices */
        int tri_idx = 0;
        int row_start = 0;
        for (row = 0; row < subdiv; row++)
        {
            int row_len = subdiv - row + 1;
            int next_row_start = row_start + row_len;

            for (col = 0; col < row_len - 1; col++)
            {
                indices[tri_idx * 3 + 0] = (uint32_t)(row_start + col);
                indices[tri_idx * 3 + 1] = (uint32_t)(row_start + col + 1);
                indices[tri_idx * 3 + 2] = (uint32_t)(next_row_start + col);
                tri_idx++;

                if (col < row_len - 2)
                {
                    indices[tri_idx * 3 + 0] = (uint32_t)(row_start + col + 1);
                    indices[tri_idx * 3 + 1] = (uint32_t)(next_row_start + col + 1);
                    indices[tri_idx * 3 + 2] = (uint32_t)(next_row_start + col);
                    tri_idx++;
                }
            }
            row_start = next_row_start;
        }

        out_mesh->vertices = vertices;
        out_mesh->vertex_count = (uint32_t)total_verts;
        out_mesh->indices = indices;
        out_mesh->index_count = (uint32_t)(tri_idx * 3);

        free(corners);
        return true;
    }
    else if (edge_use_count == 4)
    {
        /* Quad face: bilinear interpolation + sphere projection */
        int subdiv = CURVED_SUBDIVISIONS;
        int rows = subdiv + 1;
        int num_cols = subdiv + 1;
        int total_verts = rows * num_cols;
        int total_tris = subdiv * subdiv * 2;

        lc_mesh_vertex_t *vertices = (lc_mesh_vertex_t *)malloc(sizeof(lc_mesh_vertex_t) * total_verts);
        uint32_t *indices = (uint32_t *)malloc(sizeof(uint32_t) * total_tris * 3);
        if (vertices == NULL || indices == NULL)
        {
            free(vertices);
            free(indices);
            free(corners);
            return false;
        }

        int row, col;
        for (row = 0; row < rows; row++)
        {
            float v = (float)row / (float)subdiv;
            for (col = 0; col < num_cols; col++)
            {
                float u = (float)col / (float)subdiv;

                float w00 = (1.0f - u) * (1.0f - v);
                float w10 = u * (1.0f - v);
                float w11 = u * v;
                float w01 = (1.0f - u) * v;

                vec3 pos;
                vec3 tmp;
                glm_vec3_scale(corners[0], w00, pos);
                glm_vec3_scale(corners[1], w10, tmp);
                glm_vec3_add(pos, tmp, pos);
                glm_vec3_scale(corners[2], w11, tmp);
                glm_vec3_add(pos, tmp, pos);
                glm_vec3_scale(corners[3], w01, tmp);
                glm_vec3_add(pos, tmp, pos);

                /* Project onto sphere */
                vec3 dir;
                glm_vec3_sub(pos, sph_data.center, dir);
                float len = glm_vec3_norm(dir);
                if (len > 1e-6f)
                {
                    glm_vec3_scale(dir, sph_data.radius / len, dir);
                }
                vec3 sphere_pos;
                glm_vec3_add(sph_data.center, dir, sphere_pos);

                int vidx = row * num_cols + col;
                vertices[vidx].position[0] = sphere_pos[0];
                vertices[vidx].position[1] = sphere_pos[1];
                vertices[vidx].position[2] = sphere_pos[2];

                vec3 normal;
                glm_vec3_sub(sphere_pos, sph_data.center, normal);
                glm_vec3_normalize(normal);
                vertices[vidx].normal[0] = normal[0];
                vertices[vidx].normal[1] = normal[1];
                vertices[vidx].normal[2] = normal[2];
            }
        }

        int tri_idx = 0;
        for (row = 0; row < subdiv; row++)
        {
            for (col = 0; col < subdiv; col++)
            {
                int bl = row * num_cols + col;
                int br = row * num_cols + col + 1;
                int tl = (row + 1) * num_cols + col;
                int tr = (row + 1) * num_cols + col + 1;

                indices[tri_idx * 3 + 0] = (uint32_t)bl;
                indices[tri_idx * 3 + 1] = (uint32_t)br;
                indices[tri_idx * 3 + 2] = (uint32_t)tr;
                tri_idx++;

                indices[tri_idx * 3 + 0] = (uint32_t)bl;
                indices[tri_idx * 3 + 1] = (uint32_t)tr;
                indices[tri_idx * 3 + 2] = (uint32_t)tl;
                tri_idx++;
            }
        }

        out_mesh->vertices = vertices;
        out_mesh->vertex_count = (uint32_t)total_verts;
        out_mesh->indices = indices;
        out_mesh->index_count = (uint32_t)(tri_idx * 3);

        free(corners);
        return true;
    }

    free(corners);
    return tessellate_planar_face(face, out_mesh);
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
