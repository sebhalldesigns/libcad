/***************************************************************
**
** libcad Implementation File
**
** File         :  lc_brep.c
** Module       :  libcad (B-Rep construction)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  B-Rep solid primitive construction (box, cylinder,
**                 sphere). Constructs topologically valid solids with
**                 vertices, edges, loops, faces, shells. Integrates
**                 with entity system and geometry kernel.
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_brep.h"
#include "lc_topology.h"
#include "lc_geometry.h"
#include "lc_entity.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <cglm/cglm.h>

/* MARK: CONSTANTS & MACROS */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_EDGES 2048
#define MAX_VERTICES 1024
#define MAX_ENTITIES 8192

/* MARK: STATIC VARIABLES */

static bool g_brep_initialized = false;

/* MARK: STATIC FUNCTION DEFS */

static lc_entity_handle_t create_vertex(vec3 position);
static lc_entity_handle_t create_edge(lc_entity_handle_t v_start, lc_entity_handle_t v_end, lc_curve_handle_t curve, float u_start, float u_end);
static lc_entity_handle_t create_edge_use(lc_entity_handle_t edge, lc_entity_handle_t loop, bool forward);
static lc_entity_handle_t create_loop(lc_entity_handle_t face, bool is_outer);
static lc_entity_handle_t create_face(lc_surface_handle_t surface, bool forward);
static lc_entity_handle_t create_shell(bool is_closed);
static lc_entity_handle_t create_solid_entity(void);
static void link_edge_uses_in_loop(lc_entity_handle_t *edge_uses, int count);
static lc_entity_handle_t find_or_create_edge(lc_entity_handle_t v1, lc_entity_handle_t v2, lc_entity_handle_t *edges, int *edge_count, int max_edges, lc_entity_handle_t *vertices);

/* MARK: PUBLIC FUNCTIONS */

void lc_brep_init(void)
{
    g_brep_initialized = true;
    printf("[lc_brep] B-Rep construction module initialized\n");
}

void lc_brep_shutdown(void)
{
    g_brep_initialized = false;
    printf("[lc_brep] B-Rep construction module shutdown\n");
}

lc_entity_handle_t lc_brep_create_box(vec3 origin, vec3 dimensions)
{
    if (!g_brep_initialized)
    {
        printf("[lc_brep] ERROR: lc_brep_create_box() called before lc_brep_init()\n");
        return LC_ENTITY_INVALID;
    }

    /* Create solid entity */
    lc_entity_handle_t solid = create_solid_entity();
    if (solid == LC_ENTITY_INVALID)
    {
        printf("[lc_brep] ERROR: Failed to create solid entity\n");
        return LC_ENTITY_INVALID;
    }

    /* Create shell entity */
    lc_entity_handle_t shell = create_shell(true);
    if (shell == LC_ENTITY_INVALID)
    {
        printf("[lc_brep] ERROR: Failed to create shell entity\n");
        lc_entity_destroy(solid);
        return LC_ENTITY_INVALID;
    }
    lc_entity_add_child(solid, shell);

    /* Compute 8 vertex positions */
    vec3 v[8];
    float ox = origin[0];
    float oy = origin[1];
    float oz = origin[2];
    float dx = dimensions[0];
    float dy = dimensions[1];
    float dz = dimensions[2];

    glm_vec3_copy((vec3){ox, oy, oz}, v[0]);
    glm_vec3_copy((vec3){ox + dx, oy, oz}, v[1]);
    glm_vec3_copy((vec3){ox + dx, oy + dy, oz}, v[2]);
    glm_vec3_copy((vec3){ox, oy + dy, oz}, v[3]);
    glm_vec3_copy((vec3){ox, oy, oz + dz}, v[4]);
    glm_vec3_copy((vec3){ox + dx, oy, oz + dz}, v[5]);
    glm_vec3_copy((vec3){ox + dx, oy + dy, oz + dz}, v[6]);
    glm_vec3_copy((vec3){ox, oy + dy, oz + dz}, v[7]);

    /* Create 8 vertex entities */
    lc_entity_handle_t vertices[8];
    int i;
    for (i = 0; i < 8; i++)
    {
        vertices[i] = create_vertex(v[i]);
        if (vertices[i] == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create vertex %d\n", i);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }
    }

    /* Edge storage for sharing between faces */
    lc_entity_handle_t edges[MAX_EDGES];
    int edge_count = 0;

    /* Define 6 faces with their vertex indices (CCW from outside) */
    int face_indices[6][4] = {
        {0, 3, 2, 1}, /* Bottom (z=0), normal -Z */
        {4, 5, 6, 7}, /* Top (z=dz), normal +Z */
        {0, 1, 5, 4}, /* Front (y=0), normal -Y */
        {2, 3, 7, 6}, /* Back (y=dy), normal +Y */
        {0, 4, 7, 3}, /* Left (x=0), normal -X */
        {1, 2, 6, 5}  /* Right (x=dx), normal +X */
    };

    /* Face normals */
    vec3 face_normals[6] = {
        {0.0f, 0.0f, -1.0f}, /* Bottom */
        {0.0f, 0.0f, 1.0f},  /* Top */
        {0.0f, -1.0f, 0.0f}, /* Front */
        {0.0f, 1.0f, 0.0f},  /* Back */
        {-1.0f, 0.0f, 0.0f}, /* Left */
        {1.0f, 0.0f, 0.0f}   /* Right */
    };

    /* Create each face */
    int face_idx;
    for (face_idx = 0; face_idx < 6; face_idx++)
    {
        int *vidx = face_indices[face_idx];

        /* Create plane surface for this face */
        vec3 face_origin;
        glm_vec3_copy(v[vidx[0]], face_origin);

        /* Compute two perpendicular axes in the plane */
        vec3 edge1, edge2;
        glm_vec3_sub(v[vidx[1]], v[vidx[0]], edge1);
        glm_vec3_normalize(edge1);

        glm_vec3_sub(v[vidx[3]], v[vidx[0]], edge2);
        glm_vec3_normalize(edge2);

        lc_surface_handle_t surface = lc_geometry_create_plane(face_origin, edge1, edge2);
        if (surface == LC_SURFACE_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create plane surface for face %d\n", face_idx);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        /* Create face entity */
        lc_entity_handle_t face = create_face(surface, true);
        if (face == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create face entity %d\n", face_idx);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        /* Create loop entity */
        lc_entity_handle_t loop = create_loop(face, true);
        if (loop == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create loop for face %d\n", face_idx);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        /* Add loop as child of face */
        lc_entity_add_child(face, loop);

        /* Set face's outer loop */
        lc_face_data_t *face_data = (lc_face_data_t *)lc_entity_get_data(face);
        if (face_data)
        {
            face_data->outer_loop = loop;
        }

        /* Create edge uses for this face (4 edges) */
        lc_entity_handle_t edge_uses[4];
        int edge_idx;
        for (edge_idx = 0; edge_idx < 4; edge_idx++)
        {
            int v_start_idx = vidx[edge_idx];
            int v_end_idx = vidx[(edge_idx + 1) % 4];
            lc_entity_handle_t v_start = vertices[v_start_idx];
            lc_entity_handle_t v_end = vertices[v_end_idx];

            /* Find or create edge */
            lc_entity_handle_t edge = find_or_create_edge(v_start, v_end, edges, &edge_count, MAX_EDGES, vertices);
            if (edge == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to find/create edge for face %d edge %d\n", face_idx, edge_idx);
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            /* Determine forward direction */
            lc_edge_data_t *edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);
            bool forward = (edge_data->vertex_start == v_start);

            /* Create edge use */
            lc_entity_handle_t edge_use = create_edge_use(edge, loop, forward);
            if (edge_use == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to create edge use\n");
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            edge_uses[edge_idx] = edge_use;
        }

        /* Link edge uses into circular chain */
        link_edge_uses_in_loop(edge_uses, 4);

        /* Add face as child of shell */
        lc_entity_add_child(shell, face);
    }

    /* Set solid's bounding box */
    lc_solid_data_t *solid_data = (lc_solid_data_t *)lc_entity_get_data(solid);
    if (solid_data)
    {
        glm_vec3_copy(origin, solid_data->bbox_min);
        glm_vec3_add(origin, dimensions, solid_data->bbox_max);
        solid_data->bbox_dirty = false;
    }

    return solid;
}

lc_entity_handle_t lc_brep_create_cylinder(vec3 base_center, vec3 axis, float radius, float height, int segments)
{
    if (!g_brep_initialized)
    {
        printf("[lc_brep] ERROR: lc_brep_create_cylinder() called before lc_brep_init()\n");
        return LC_ENTITY_INVALID;
    }

    /* Validate parameters */
    if (segments < 3)
    {
        printf("[lc_brep] ERROR: Cylinder segments must be >= 3 (got %d)\n", segments);
        return LC_ENTITY_INVALID;
    }
    if (radius <= 0.0f)
    {
        printf("[lc_brep] ERROR: Cylinder radius must be > 0 (got %f)\n", radius);
        return LC_ENTITY_INVALID;
    }
    if (height <= 0.0f)
    {
        printf("[lc_brep] ERROR: Cylinder height must be > 0 (got %f)\n", height);
        return LC_ENTITY_INVALID;
    }

    /* Create solid entity */
    lc_entity_handle_t solid = create_solid_entity();
    if (solid == LC_ENTITY_INVALID)
    {
        printf("[lc_brep] ERROR: Failed to create solid entity\n");
        return LC_ENTITY_INVALID;
    }

    /* Create shell entity */
    lc_entity_handle_t shell = create_shell(true);
    if (shell == LC_ENTITY_INVALID)
    {
        printf("[lc_brep] ERROR: Failed to create shell entity\n");
        lc_entity_destroy(solid);
        return LC_ENTITY_INVALID;
    }
    lc_entity_add_child(solid, shell);

    /* Normalize the axis */
    vec3 axis_normalized;
    glm_vec3_copy(axis, axis_normalized);
    glm_vec3_normalize(axis_normalized);

    /* Compute two perpendicular vectors to the axis */
    vec3 up = {0.0f, 0.0f, 1.0f};
    if (fabsf(glm_vec3_dot(axis_normalized, up)) > 0.99f)
    {
        glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, up);
    }
    vec3 u_axis, v_axis;
    glm_vec3_cross(axis_normalized, up, u_axis);
    glm_vec3_normalize(u_axis);
    glm_vec3_cross(axis_normalized, u_axis, v_axis);
    glm_vec3_normalize(v_axis);

    /* Compute top center */
    vec3 top_center;
    glm_vec3_scale(axis_normalized, height, top_center);
    glm_vec3_add(base_center, top_center, top_center);

    /* Create vertices: bottom ring + top ring */
    lc_entity_handle_t vertices[MAX_VERTICES];
    int vertex_count = 0;
    int i;

    /* Bottom ring vertices */
    for (i = 0; i < segments; i++)
    {
        float theta = 2.0f * M_PI * i / segments;
        float cos_theta = cosf(theta);
        float sin_theta = sinf(theta);

        vec3 offset;
        vec3 u_scaled, v_scaled;
        glm_vec3_scale(u_axis, radius * cos_theta, u_scaled);
        glm_vec3_scale(v_axis, radius * sin_theta, v_scaled);
        glm_vec3_add(u_scaled, v_scaled, offset);

        vec3 position;
        glm_vec3_add(base_center, offset, position);

        vertices[vertex_count] = create_vertex(position);
        if (vertices[vertex_count] == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create bottom vertex %d\n", i);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }
        vertex_count++;
    }

    /* Top ring vertices */
    for (i = 0; i < segments; i++)
    {
        float theta = 2.0f * M_PI * i / segments;
        float cos_theta = cosf(theta);
        float sin_theta = sinf(theta);

        vec3 offset;
        vec3 u_scaled, v_scaled;
        glm_vec3_scale(u_axis, radius * cos_theta, u_scaled);
        glm_vec3_scale(v_axis, radius * sin_theta, v_scaled);
        glm_vec3_add(u_scaled, v_scaled, offset);

        vec3 position;
        glm_vec3_add(top_center, offset, position);

        vertices[vertex_count] = create_vertex(position);
        if (vertices[vertex_count] == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create top vertex %d\n", i);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }
        vertex_count++;
    }

    /* Edge storage for sharing between faces */
    lc_entity_handle_t edges[MAX_EDGES];
    int edge_count = 0;

    /* Create side faces (one quad per segment) */
    int seg;
    for (seg = 0; seg < segments; seg++)
    {
        int bottom_i = seg;
        int bottom_j = (seg + 1) % segments;
        int top_i = segments + seg;
        int top_j = segments + ((seg + 1) % segments);

        /* Face vertices in CCW order from outside: bottom[i], bottom[j], top[j], top[i] */
        int vidx[4] = {bottom_i, bottom_j, top_j, top_i};

        /* Create plane surface for this face */
        lc_vertex_data_t *v0_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[vidx[0]]);
        lc_vertex_data_t *v1_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[vidx[1]]);
        lc_vertex_data_t *v3_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[vidx[3]]);

        vec3 edge1, edge2;
        glm_vec3_sub(v1_data->position, v0_data->position, edge1);
        glm_vec3_normalize(edge1);
        glm_vec3_sub(v3_data->position, v0_data->position, edge2);
        glm_vec3_normalize(edge2);

        lc_surface_handle_t surface = lc_geometry_create_plane(v0_data->position, edge1, edge2);
        if (surface == LC_SURFACE_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create plane surface for side face %d\n", seg);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        /* Create face entity */
        lc_entity_handle_t face = create_face(surface, true);
        if (face == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create side face entity %d\n", seg);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        /* Create loop entity */
        lc_entity_handle_t loop = create_loop(face, true);
        if (loop == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create loop for side face %d\n", seg);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_add_child(face, loop);

        lc_face_data_t *face_data = (lc_face_data_t *)lc_entity_get_data(face);
        if (face_data)
        {
            face_data->outer_loop = loop;
        }

        /* Create edge uses for this face (4 edges) */
        lc_entity_handle_t edge_uses[4];
        int edge_idx;
        for (edge_idx = 0; edge_idx < 4; edge_idx++)
        {
            int v_start_idx = vidx[edge_idx];
            int v_end_idx = vidx[(edge_idx + 1) % 4];
            lc_entity_handle_t v_start = vertices[v_start_idx];
            lc_entity_handle_t v_end = vertices[v_end_idx];

            lc_entity_handle_t edge = find_or_create_edge(v_start, v_end, edges, &edge_count, MAX_EDGES, vertices);
            if (edge == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to find/create edge for side face %d edge %d\n", seg, edge_idx);
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            lc_edge_data_t *edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);
            bool forward = (edge_data->vertex_start == v_start);

            lc_entity_handle_t edge_use = create_edge_use(edge, loop, forward);
            if (edge_use == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to create edge use\n");
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            edge_uses[edge_idx] = edge_use;
        }

        link_edge_uses_in_loop(edge_uses, 4);
        lc_entity_add_child(shell, face);
    }

    /* Create bottom cap face (reversed winding for outward normal pointing down) */
    {
        lc_vertex_data_t *v0_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[0]);

        vec3 normal;
        glm_vec3_negate_to(axis_normalized, normal);

        lc_surface_handle_t surface = lc_geometry_create_plane(v0_data->position, u_axis, v_axis);
        if (surface == LC_SURFACE_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create plane surface for bottom cap\n");
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_handle_t face = create_face(surface, true);
        if (face == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create bottom cap face\n");
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_handle_t loop = create_loop(face, true);
        if (loop == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create loop for bottom cap\n");
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_add_child(face, loop);

        lc_face_data_t *face_data = (lc_face_data_t *)lc_entity_get_data(face);
        if (face_data)
        {
            face_data->outer_loop = loop;
        }

        /* Create edge uses for bottom cap (reversed order) */
        lc_entity_handle_t edge_uses[MAX_EDGES];
        for (i = 0; i < segments; i++)
        {
            int v_start_idx = (segments - i) % segments;
            int v_end_idx = (segments - i - 1 + segments) % segments;
            lc_entity_handle_t v_start = vertices[v_start_idx];
            lc_entity_handle_t v_end = vertices[v_end_idx];

            lc_entity_handle_t edge = find_or_create_edge(v_start, v_end, edges, &edge_count, MAX_EDGES, vertices);
            if (edge == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to find/create edge for bottom cap edge %d\n", i);
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            lc_edge_data_t *edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);
            bool forward = (edge_data->vertex_start == v_start);

            lc_entity_handle_t edge_use = create_edge_use(edge, loop, forward);
            if (edge_use == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to create edge use for bottom cap\n");
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            edge_uses[i] = edge_use;
        }

        link_edge_uses_in_loop(edge_uses, segments);
        lc_entity_add_child(shell, face);
    }

    /* Create top cap face */
    {
        lc_vertex_data_t *v0_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[segments]);

        lc_surface_handle_t surface = lc_geometry_create_plane(v0_data->position, u_axis, v_axis);
        if (surface == LC_SURFACE_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create plane surface for top cap\n");
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_handle_t face = create_face(surface, true);
        if (face == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create top cap face\n");
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_handle_t loop = create_loop(face, true);
        if (loop == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create loop for top cap\n");
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_add_child(face, loop);

        lc_face_data_t *face_data = (lc_face_data_t *)lc_entity_get_data(face);
        if (face_data)
        {
            face_data->outer_loop = loop;
        }

        /* Create edge uses for top cap */
        lc_entity_handle_t edge_uses[MAX_EDGES];
        for (i = 0; i < segments; i++)
        {
            int v_start_idx = segments + i;
            int v_end_idx = segments + ((i + 1) % segments);
            lc_entity_handle_t v_start = vertices[v_start_idx];
            lc_entity_handle_t v_end = vertices[v_end_idx];

            lc_entity_handle_t edge = find_or_create_edge(v_start, v_end, edges, &edge_count, MAX_EDGES, vertices);
            if (edge == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to find/create edge for top cap edge %d\n", i);
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            lc_edge_data_t *edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);
            bool forward = (edge_data->vertex_start == v_start);

            lc_entity_handle_t edge_use = create_edge_use(edge, loop, forward);
            if (edge_use == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to create edge use for top cap\n");
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            edge_uses[i] = edge_use;
        }

        link_edge_uses_in_loop(edge_uses, segments);
        lc_entity_add_child(shell, face);
    }

    /* Set solid's bounding box */
    lc_solid_data_t *solid_data = (lc_solid_data_t *)lc_entity_get_data(solid);
    if (solid_data)
    {
        /* Bounding box is cylinder extents */
        glm_vec3_copy(base_center, solid_data->bbox_min);
        solid_data->bbox_min[0] -= radius;
        solid_data->bbox_min[1] -= radius;
        solid_data->bbox_min[2] -= radius;

        glm_vec3_copy(top_center, solid_data->bbox_max);
        solid_data->bbox_max[0] += radius;
        solid_data->bbox_max[1] += radius;
        solid_data->bbox_max[2] += radius;

        /* Ensure min < max for each axis */
        for (i = 0; i < 3; i++)
        {
            if (solid_data->bbox_min[i] > solid_data->bbox_max[i])
            {
                float temp = solid_data->bbox_min[i];
                solid_data->bbox_min[i] = solid_data->bbox_max[i];
                solid_data->bbox_max[i] = temp;
            }
        }

        solid_data->bbox_dirty = false;
    }

    return solid;
}

lc_entity_handle_t lc_brep_create_sphere(vec3 center, float radius, int u_segments, int v_segments)
{
    if (!g_brep_initialized)
    {
        printf("[lc_brep] ERROR: lc_brep_create_sphere() called before lc_brep_init()\n");
        return LC_ENTITY_INVALID;
    }

    /* Validate parameters */
    if (u_segments < 3)
    {
        printf("[lc_brep] ERROR: Sphere u_segments must be >= 3 (got %d)\n", u_segments);
        return LC_ENTITY_INVALID;
    }
    if (v_segments < 2)
    {
        printf("[lc_brep] ERROR: Sphere v_segments must be >= 2 (got %d)\n", v_segments);
        return LC_ENTITY_INVALID;
    }
    if (radius <= 0.0f)
    {
        printf("[lc_brep] ERROR: Sphere radius must be > 0 (got %f)\n", radius);
        return LC_ENTITY_INVALID;
    }

    /* Create solid entity */
    lc_entity_handle_t solid = create_solid_entity();
    if (solid == LC_ENTITY_INVALID)
    {
        printf("[lc_brep] ERROR: Failed to create solid entity\n");
        return LC_ENTITY_INVALID;
    }

    /* Create shell entity */
    lc_entity_handle_t shell = create_shell(true);
    if (shell == LC_ENTITY_INVALID)
    {
        printf("[lc_brep] ERROR: Failed to create shell entity\n");
        lc_entity_destroy(solid);
        return LC_ENTITY_INVALID;
    }
    lc_entity_add_child(solid, shell);

    /* Create vertices: north pole + rings + south pole */
    lc_entity_handle_t vertices[MAX_VERTICES];
    int vertex_count = 0;

    /* North pole (phi = 0, pointing up +Z) */
    vec3 north_pos;
    glm_vec3_copy(center, north_pos);
    north_pos[2] += radius;
    vertices[vertex_count] = create_vertex(north_pos);
    if (vertices[vertex_count] == LC_ENTITY_INVALID)
    {
        printf("[lc_brep] ERROR: Failed to create north pole vertex\n");
        lc_entity_destroy(solid);
        return LC_ENTITY_INVALID;
    }
    int north_pole_idx = vertex_count;
    vertex_count++;

    /* Latitude rings (j = 1 to v_segments-1) */
    int j, i;
    for (j = 1; j < v_segments; j++)
    {
        float phi = M_PI * j / v_segments;
        float sin_phi = sinf(phi);
        float cos_phi = cosf(phi);

        for (i = 0; i < u_segments; i++)
        {
            float theta = 2.0f * M_PI * i / u_segments;
            float cos_theta = cosf(theta);
            float sin_theta = sinf(theta);

            vec3 position;
            position[0] = center[0] + radius * sin_phi * cos_theta;
            position[1] = center[1] + radius * sin_phi * sin_theta;
            position[2] = center[2] + radius * cos_phi;

            vertices[vertex_count] = create_vertex(position);
            if (vertices[vertex_count] == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to create ring vertex (j=%d, i=%d)\n", j, i);
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }
            vertex_count++;
        }
    }

    /* South pole (phi = π, pointing down -Z) */
    vec3 south_pos;
    glm_vec3_copy(center, south_pos);
    south_pos[2] -= radius;
    vertices[vertex_count] = create_vertex(south_pos);
    if (vertices[vertex_count] == LC_ENTITY_INVALID)
    {
        printf("[lc_brep] ERROR: Failed to create south pole vertex\n");
        lc_entity_destroy(solid);
        return LC_ENTITY_INVALID;
    }
    int south_pole_idx = vertex_count;
    vertex_count++;

    /* Edge storage for sharing between faces */
    lc_entity_handle_t edges[MAX_EDGES];
    int edge_count = 0;

    /* Create north pole triangle fan faces */
    for (i = 0; i < u_segments; i++)
    {
        int ring0_i = 1 + i;
        int ring0_j = 1 + ((i + 1) % u_segments);

        /* Triangle: north_pole, ring0_i, ring0_j */
        int vidx[3] = {north_pole_idx, ring0_i, ring0_j};

        /* Create plane surface */
        lc_vertex_data_t *v0_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[vidx[0]]);
        lc_vertex_data_t *v1_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[vidx[1]]);
        lc_vertex_data_t *v2_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[vidx[2]]);

        vec3 edge1, edge2;
        glm_vec3_sub(v1_data->position, v0_data->position, edge1);
        glm_vec3_normalize(edge1);
        glm_vec3_sub(v2_data->position, v0_data->position, edge2);
        glm_vec3_normalize(edge2);

        lc_surface_handle_t surface = lc_geometry_create_plane(v0_data->position, edge1, edge2);
        if (surface == LC_SURFACE_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create plane surface for north pole face %d\n", i);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_handle_t face = create_face(surface, true);
        if (face == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create north pole face %d\n", i);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_handle_t loop = create_loop(face, true);
        if (loop == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create loop for north pole face %d\n", i);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_add_child(face, loop);

        lc_face_data_t *face_data = (lc_face_data_t *)lc_entity_get_data(face);
        if (face_data)
        {
            face_data->outer_loop = loop;
        }

        /* Create edge uses (3 edges for triangle) */
        lc_entity_handle_t edge_uses[3];
        int edge_idx;
        for (edge_idx = 0; edge_idx < 3; edge_idx++)
        {
            int v_start_idx = vidx[edge_idx];
            int v_end_idx = vidx[(edge_idx + 1) % 3];
            lc_entity_handle_t v_start = vertices[v_start_idx];
            lc_entity_handle_t v_end = vertices[v_end_idx];

            lc_entity_handle_t edge = find_or_create_edge(v_start, v_end, edges, &edge_count, MAX_EDGES, vertices);
            if (edge == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to find/create edge for north pole face %d edge %d\n", i, edge_idx);
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            lc_edge_data_t *edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);
            bool forward = (edge_data->vertex_start == v_start);

            lc_entity_handle_t edge_use = create_edge_use(edge, loop, forward);
            if (edge_use == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to create edge use for north pole\n");
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            edge_uses[edge_idx] = edge_use;
        }

        link_edge_uses_in_loop(edge_uses, 3);
        lc_entity_add_child(shell, face);
    }

    /* Create middle quad strip faces (between rings) */
    for (j = 1; j < v_segments - 1; j++)
    {
        int ring_base = 1 + (j - 1) * u_segments;
        int next_ring_base = 1 + j * u_segments;

        for (i = 0; i < u_segments; i++)
        {
            int ring_i = ring_base + i;
            int ring_j = ring_base + ((i + 1) % u_segments);
            int next_ring_i = next_ring_base + i;
            int next_ring_j = next_ring_base + ((i + 1) % u_segments);

            /* Quad: ring_i, ring_j, next_ring_j, next_ring_i */
            int vidx[4] = {ring_i, ring_j, next_ring_j, next_ring_i};

            /* Create plane surface */
            lc_vertex_data_t *v0_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[vidx[0]]);
            lc_vertex_data_t *v1_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[vidx[1]]);
            lc_vertex_data_t *v3_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[vidx[3]]);

            vec3 edge1, edge2;
            glm_vec3_sub(v1_data->position, v0_data->position, edge1);
            glm_vec3_normalize(edge1);
            glm_vec3_sub(v3_data->position, v0_data->position, edge2);
            glm_vec3_normalize(edge2);

            lc_surface_handle_t surface = lc_geometry_create_plane(v0_data->position, edge1, edge2);
            if (surface == LC_SURFACE_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to create plane surface for middle face (j=%d, i=%d)\n", j, i);
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            lc_entity_handle_t face = create_face(surface, true);
            if (face == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to create middle face (j=%d, i=%d)\n", j, i);
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            lc_entity_handle_t loop = create_loop(face, true);
            if (loop == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to create loop for middle face (j=%d, i=%d)\n", j, i);
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            lc_entity_add_child(face, loop);

            lc_face_data_t *face_data = (lc_face_data_t *)lc_entity_get_data(face);
            if (face_data)
            {
                face_data->outer_loop = loop;
            }

            /* Create edge uses (4 edges for quad) */
            lc_entity_handle_t edge_uses[4];
            int edge_idx;
            for (edge_idx = 0; edge_idx < 4; edge_idx++)
            {
                int v_start_idx = vidx[edge_idx];
                int v_end_idx = vidx[(edge_idx + 1) % 4];
                lc_entity_handle_t v_start = vertices[v_start_idx];
                lc_entity_handle_t v_end = vertices[v_end_idx];

                lc_entity_handle_t edge = find_or_create_edge(v_start, v_end, edges, &edge_count, MAX_EDGES, vertices);
                if (edge == LC_ENTITY_INVALID)
                {
                    printf("[lc_brep] ERROR: Failed to find/create edge for middle face (j=%d, i=%d, edge=%d)\n", j, i, edge_idx);
                    lc_entity_destroy(solid);
                    return LC_ENTITY_INVALID;
                }

                lc_edge_data_t *edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);
                bool forward = (edge_data->vertex_start == v_start);

                lc_entity_handle_t edge_use = create_edge_use(edge, loop, forward);
                if (edge_use == LC_ENTITY_INVALID)
                {
                    printf("[lc_brep] ERROR: Failed to create edge use for middle face\n");
                    lc_entity_destroy(solid);
                    return LC_ENTITY_INVALID;
                }

                edge_uses[edge_idx] = edge_use;
            }

            link_edge_uses_in_loop(edge_uses, 4);
            lc_entity_add_child(shell, face);
        }
    }

    /* Create south pole triangle fan faces */
    int last_ring_base = 1 + (v_segments - 2) * u_segments;
    for (i = 0; i < u_segments; i++)
    {
        int ring_i = last_ring_base + i;
        int ring_j = last_ring_base + ((i + 1) % u_segments);

        /* Triangle: ring_i, south_pole, ring_j (reversed winding for outward normal) */
        int vidx[3] = {ring_i, south_pole_idx, ring_j};

        /* Create plane surface */
        lc_vertex_data_t *v0_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[vidx[0]]);
        lc_vertex_data_t *v1_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[vidx[1]]);
        lc_vertex_data_t *v2_data = (lc_vertex_data_t *)lc_entity_get_data(vertices[vidx[2]]);

        vec3 edge1, edge2;
        glm_vec3_sub(v1_data->position, v0_data->position, edge1);
        glm_vec3_normalize(edge1);
        glm_vec3_sub(v2_data->position, v0_data->position, edge2);
        glm_vec3_normalize(edge2);

        lc_surface_handle_t surface = lc_geometry_create_plane(v0_data->position, edge1, edge2);
        if (surface == LC_SURFACE_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create plane surface for south pole face %d\n", i);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_handle_t face = create_face(surface, true);
        if (face == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create south pole face %d\n", i);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_handle_t loop = create_loop(face, true);
        if (loop == LC_ENTITY_INVALID)
        {
            printf("[lc_brep] ERROR: Failed to create loop for south pole face %d\n", i);
            lc_entity_destroy(solid);
            return LC_ENTITY_INVALID;
        }

        lc_entity_add_child(face, loop);

        lc_face_data_t *face_data = (lc_face_data_t *)lc_entity_get_data(face);
        if (face_data)
        {
            face_data->outer_loop = loop;
        }

        /* Create edge uses (3 edges for triangle) */
        lc_entity_handle_t edge_uses[3];
        int edge_idx;
        for (edge_idx = 0; edge_idx < 3; edge_idx++)
        {
            int v_start_idx = vidx[edge_idx];
            int v_end_idx = vidx[(edge_idx + 1) % 3];
            lc_entity_handle_t v_start = vertices[v_start_idx];
            lc_entity_handle_t v_end = vertices[v_end_idx];

            lc_entity_handle_t edge = find_or_create_edge(v_start, v_end, edges, &edge_count, MAX_EDGES, vertices);
            if (edge == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to find/create edge for south pole face %d edge %d\n", i, edge_idx);
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            lc_edge_data_t *edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);
            bool forward = (edge_data->vertex_start == v_start);

            lc_entity_handle_t edge_use = create_edge_use(edge, loop, forward);
            if (edge_use == LC_ENTITY_INVALID)
            {
                printf("[lc_brep] ERROR: Failed to create edge use for south pole\n");
                lc_entity_destroy(solid);
                return LC_ENTITY_INVALID;
            }

            edge_uses[edge_idx] = edge_use;
        }

        link_edge_uses_in_loop(edge_uses, 3);
        lc_entity_add_child(shell, face);
    }

    /* Set solid's bounding box */
    lc_solid_data_t *solid_data = (lc_solid_data_t *)lc_entity_get_data(solid);
    if (solid_data)
    {
        glm_vec3_copy(center, solid_data->bbox_min);
        solid_data->bbox_min[0] -= radius;
        solid_data->bbox_min[1] -= radius;
        solid_data->bbox_min[2] -= radius;

        glm_vec3_copy(center, solid_data->bbox_max);
        solid_data->bbox_max[0] += radius;
        solid_data->bbox_max[1] += radius;
        solid_data->bbox_max[2] += radius;

        solid_data->bbox_dirty = false;
    }

    return solid;
}

bool lc_brep_validate_solid(lc_entity_handle_t solid)
{
    if (!lc_entity_is_valid(solid))
    {
        printf("[lc_brep] ERROR: Invalid solid handle\n");
        return false;
    }

    if (lc_entity_get_type(solid) != LC_ENTITY_TYPE_SOLID)
    {
        printf("[lc_brep] ERROR: Entity is not a solid\n");
        return false;
    }

    /* Use topology module to check Euler characteristic */
    if (!lc_topology_check_euler(solid))
    {
        printf("[lc_brep] ERROR: Solid fails Euler characteristic check\n");
        return false;
    }

    return true;
}

bool lc_brep_destroy_solid(lc_entity_handle_t solid)
{
    if (!lc_entity_is_valid(solid))
    {
        printf("[lc_brep] ERROR: Invalid solid handle\n");
        return false;
    }

    if (lc_entity_get_type(solid) != LC_ENTITY_TYPE_SOLID)
    {
        printf("[lc_brep] ERROR: Entity is not a solid\n");
        return false;
    }

    /* Collect all entities in the tree */
    lc_entity_handle_t entities[MAX_ENTITIES];
    int entity_count = 0;

    /* Simple recursive walk to collect all descendants */
    entities[entity_count++] = solid;

    int i;
    for (i = 0; i < entity_count && i < MAX_ENTITIES; i++)
    {
        lc_entity_handle_t current = entities[i];
        lc_entity_handle_t child = lc_entity_get_first_child(current);

        while (child != LC_ENTITY_INVALID && entity_count < MAX_ENTITIES)
        {
            entities[entity_count++] = child;
            child = lc_entity_get_next_sibling(child);
        }
    }

    /* Destroy all entities in reverse order (leaves first) */
    for (i = entity_count - 1; i >= 0; i--)
    {
        lc_entity_destroy(entities[i]);
    }

    return true;
}

/* MARK: STATIC FUNCTIONS */

static lc_entity_handle_t create_vertex(vec3 position)
{
    lc_entity_handle_t vertex = lc_entity_create(LC_ENTITY_TYPE_VERTEX);
    if (vertex == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    lc_vertex_data_t *data = (lc_vertex_data_t *)malloc(sizeof(lc_vertex_data_t));
    if (!data)
    {
        lc_entity_destroy(vertex);
        return LC_ENTITY_INVALID;
    }

    memset(data, 0, sizeof(lc_vertex_data_t));
    glm_vec3_copy(position, data->position);

    lc_entity_set_data(vertex, data);
    return vertex;
}

static lc_entity_handle_t create_edge(lc_entity_handle_t v_start, lc_entity_handle_t v_end, lc_curve_handle_t curve, float u_start, float u_end)
{
    lc_entity_handle_t edge = lc_entity_create(LC_ENTITY_TYPE_EDGE);
    if (edge == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    lc_edge_data_t *data = (lc_edge_data_t *)malloc(sizeof(lc_edge_data_t));
    if (!data)
    {
        lc_entity_destroy(edge);
        return LC_ENTITY_INVALID;
    }

    memset(data, 0, sizeof(lc_edge_data_t));
    data->vertex_start = v_start;
    data->vertex_end = v_end;
    data->curve = curve;
    data->u_start = u_start;
    data->u_end = u_end;

    lc_entity_set_data(edge, data);
    return edge;
}

static lc_entity_handle_t create_edge_use(lc_entity_handle_t edge, lc_entity_handle_t loop, bool forward)
{
    lc_entity_handle_t edge_use = lc_entity_create(LC_ENTITY_TYPE_EDGE_USE);
    if (edge_use == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    lc_edge_use_data_t *data = (lc_edge_use_data_t *)malloc(sizeof(lc_edge_use_data_t));
    if (!data)
    {
        lc_entity_destroy(edge_use);
        return LC_ENTITY_INVALID;
    }

    memset(data, 0, sizeof(lc_edge_use_data_t));
    data->edge = edge;
    data->loop = loop;
    data->forward = forward;
    data->next_in_loop = LC_ENTITY_INVALID;
    data->prev_in_loop = LC_ENTITY_INVALID;

    lc_entity_set_data(edge_use, data);

    /* Add edge use as child of the loop (for tree traversal) */
    lc_entity_add_child(loop, edge_use);

    /* Register this edge use with the parent edge */
    lc_edge_data_t *edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);
    if (edge_data && edge_data->edge_use_count < 2)
    {
        edge_data->edge_uses[edge_data->edge_use_count] = edge_use;
        edge_data->edge_use_count++;
    }

    return edge_use;
}

static lc_entity_handle_t create_loop(lc_entity_handle_t face, bool is_outer)
{
    lc_entity_handle_t loop = lc_entity_create(LC_ENTITY_TYPE_LOOP);
    if (loop == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    lc_loop_data_t *data = (lc_loop_data_t *)malloc(sizeof(lc_loop_data_t));
    if (!data)
    {
        lc_entity_destroy(loop);
        return LC_ENTITY_INVALID;
    }

    memset(data, 0, sizeof(lc_loop_data_t));
    data->face = face;
    data->is_outer = is_outer;

    lc_entity_set_data(loop, data);
    return loop;
}

static lc_entity_handle_t create_face(lc_surface_handle_t surface, bool forward)
{
    lc_entity_handle_t face = lc_entity_create(LC_ENTITY_TYPE_FACE);
    if (face == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    lc_face_data_t *data = (lc_face_data_t *)malloc(sizeof(lc_face_data_t));
    if (!data)
    {
        lc_entity_destroy(face);
        return LC_ENTITY_INVALID;
    }

    memset(data, 0, sizeof(lc_face_data_t));
    data->surface = surface;
    data->outer_loop = LC_ENTITY_INVALID;
    data->forward = forward;
    data->mesh_dirty = true;
    data->mesh_data = NULL;

    lc_entity_set_data(face, data);
    return face;
}

static lc_entity_handle_t create_shell(bool is_closed)
{
    lc_entity_handle_t shell = lc_entity_create(LC_ENTITY_TYPE_SHELL);
    if (shell == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    lc_shell_data_t *data = (lc_shell_data_t *)malloc(sizeof(lc_shell_data_t));
    if (!data)
    {
        lc_entity_destroy(shell);
        return LC_ENTITY_INVALID;
    }

    memset(data, 0, sizeof(lc_shell_data_t));
    data->is_closed = is_closed;

    lc_entity_set_data(shell, data);
    return shell;
}

static lc_entity_handle_t create_solid_entity(void)
{
    lc_entity_handle_t solid = lc_entity_create(LC_ENTITY_TYPE_SOLID);
    if (solid == LC_ENTITY_INVALID)
    {
        return LC_ENTITY_INVALID;
    }

    lc_solid_data_t *data = (lc_solid_data_t *)malloc(sizeof(lc_solid_data_t));
    if (!data)
    {
        lc_entity_destroy(solid);
        return LC_ENTITY_INVALID;
    }

    memset(data, 0, sizeof(lc_solid_data_t));
    data->bbox_dirty = true;
    glm_vec3_zero(data->bbox_min);
    glm_vec3_zero(data->bbox_max);

    lc_entity_set_data(solid, data);
    return solid;
}

static void link_edge_uses_in_loop(lc_entity_handle_t *edge_uses, int count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        lc_edge_use_data_t *data = (lc_edge_use_data_t *)lc_entity_get_data(edge_uses[i]);
        if (data)
        {
            data->next_in_loop = edge_uses[(i + 1) % count];
            data->prev_in_loop = edge_uses[(i - 1 + count) % count];
        }
    }
}

static lc_entity_handle_t find_or_create_edge(lc_entity_handle_t v1, lc_entity_handle_t v2, lc_entity_handle_t *edges, int *edge_count, int max_edges, lc_entity_handle_t *vertices)
{
    /* Check if edge already exists between v1 and v2 (or v2 and v1) */
    int i;
    for (i = 0; i < *edge_count; i++)
    {
        lc_edge_data_t *edge_data = (lc_edge_data_t *)lc_entity_get_data(edges[i]);
        if (edge_data)
        {
            if ((edge_data->vertex_start == v1 && edge_data->vertex_end == v2) ||
                (edge_data->vertex_start == v2 && edge_data->vertex_end == v1))
            {
                return edges[i];
            }
        }
    }

    /* Edge not found, create new edge */
    if (*edge_count >= max_edges)
    {
        printf("[lc_brep] ERROR: Maximum edge count exceeded\n");
        return LC_ENTITY_INVALID;
    }

    /* Get vertex positions */
    lc_vertex_data_t *v1_data = (lc_vertex_data_t *)lc_entity_get_data(v1);
    lc_vertex_data_t *v2_data = (lc_vertex_data_t *)lc_entity_get_data(v2);

    if (!v1_data || !v2_data)
    {
        printf("[lc_brep] ERROR: Invalid vertex data\n");
        return LC_ENTITY_INVALID;
    }

    /* Compute direction and length */
    vec3 direction;
    glm_vec3_sub(v2_data->position, v1_data->position, direction);
    float length = glm_vec3_norm(direction);
    glm_vec3_normalize(direction);

    /* Create line curve */
    lc_curve_handle_t curve = lc_geometry_create_line(v1_data->position, direction, length);
    if (curve == LC_CURVE_INVALID)
    {
        printf("[lc_brep] ERROR: Failed to create line curve\n");
        return LC_ENTITY_INVALID;
    }

    /* Create edge */
    lc_entity_handle_t edge = create_edge(v1, v2, curve, 0.0f, length);
    if (edge == LC_ENTITY_INVALID)
    {
        printf("[lc_brep] ERROR: Failed to create edge\n");
        return LC_ENTITY_INVALID;
    }

    /* Add to edges array */
    edges[*edge_count] = edge;
    (*edge_count)++;

    return edge;
}
