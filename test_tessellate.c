/***************************************************************
**
** libcad Test File
**
** File         :  test_tessellate.c
** Module       :  libcad (tessellation test)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Test suite for B-Rep face tessellation. Tests mesh
**                 generation, face normals, mesh invalidation, and
**                 mesh cleanup.
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lib/lc_entity.h"
#include "lib/lc_geometry.h"
#include "lib/lc_brep.h"
#include "lib/lc_topology.h"
#include "lib/lc_tessellate.h"
#include "lib/lc_undo.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Custom assert that prints and exits instead of opening a dialog */
#define TEST_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "ASSERTION FAILED: %s (line %d)\n", #expr, __LINE__); \
            fflush(stderr); \
            exit(1); \
        } \
    } while(0)

/***************************************************************
** MARK: TEST FUNCTIONS
***************************************************************/

static void test_box_tessellation(void)
{
    printf("TEST: Box Tessellation\n");

    /* Create a unit box */
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 dims = {1.0f, 1.0f, 1.0f};
    lc_entity_handle_t solid = lc_brep_create_box(origin, dims);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Tessellate the solid */
    bool success = lc_tessellate_solid(solid);
    TEST_ASSERT(success);

    /* Get all faces and verify they have meshes */
    lc_entity_handle_t shells[8];
    size_t shell_count = lc_topology_get_shells(solid, shells, 8);
    TEST_ASSERT(shell_count == 1);

    lc_entity_handle_t faces[16];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 16);
    TEST_ASSERT(face_count == 6);

    /* Each face should have a mesh with 4 vertices and 2 triangles (6 indices) */
    size_t i;
    for (i = 0; i < face_count; i++)
    {
        const lc_mesh_t *mesh = lc_tessellate_get_mesh(faces[i]);
        TEST_ASSERT(mesh != NULL);
        TEST_ASSERT(mesh->vertex_count == 4);
        TEST_ASSERT(mesh->index_count == 6);
        TEST_ASSERT(mesh->vertices != NULL);
        TEST_ASSERT(mesh->indices != NULL);

        printf("  Face %zu: %u vertices, %u indices (%u triangles)\n",
               i, mesh->vertex_count, mesh->index_count, mesh->index_count / 3);
    }

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_face_normals(void)
{
    printf("TEST: Face Normals\n");

    /* Create a unit box */
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 dims = {1.0f, 1.0f, 1.0f};
    lc_entity_handle_t solid = lc_brep_create_box(origin, dims);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Tessellate the solid */
    bool success = lc_tessellate_solid(solid);
    TEST_ASSERT(success);

    /* Get all faces */
    lc_entity_handle_t shells[8];
    size_t shell_count = lc_topology_get_shells(solid, shells, 8);
    TEST_ASSERT(shell_count == 1);

    lc_entity_handle_t faces[16];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 16);
    TEST_ASSERT(face_count == 6);

    /* Check each face's normals */
    size_t i;
    for (i = 0; i < face_count; i++)
    {
        const lc_mesh_t *mesh = lc_tessellate_get_mesh(faces[i]);
        TEST_ASSERT(mesh != NULL);
        TEST_ASSERT(mesh->vertex_count > 0);

        /* Get first vertex normal */
        float nx = mesh->vertices[0].normal[0];
        float ny = mesh->vertices[0].normal[1];
        float nz = mesh->vertices[0].normal[2];

        /* Verify normal is non-zero */
        float length = sqrtf(nx*nx + ny*ny + nz*nz);
        TEST_ASSERT(length > 0.9f);  /* Should be approximately unit length */
        TEST_ASSERT(length < 1.1f);

        printf("  Face %zu normal: (%.2f, %.2f, %.2f), length: %.3f\n",
               i, nx, ny, nz, length);

        /* Verify all vertices on this face have the same normal (planar face) */
        size_t j;
        for (j = 1; j < mesh->vertex_count; j++)
        {
            TEST_ASSERT(fabsf(mesh->vertices[j].normal[0] - nx) < 0.001f);
            TEST_ASSERT(fabsf(mesh->vertices[j].normal[1] - ny) < 0.001f);
            TEST_ASSERT(fabsf(mesh->vertices[j].normal[2] - nz) < 0.001f);
        }
    }

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_mesh_invalidation(void)
{
    printf("TEST: Mesh Invalidation\n");

    /* Create a unit box */
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 dims = {1.0f, 1.0f, 1.0f};
    lc_entity_handle_t solid = lc_brep_create_box(origin, dims);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Tessellate the solid */
    bool success = lc_tessellate_solid(solid);
    TEST_ASSERT(success);

    /* Get first face */
    lc_entity_handle_t shells[8];
    size_t shell_count = lc_topology_get_shells(solid, shells, 8);
    TEST_ASSERT(shell_count == 1);

    lc_entity_handle_t faces[16];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 16);
    TEST_ASSERT(face_count == 6);

    /* Verify mesh exists */
    const lc_mesh_t *mesh = lc_tessellate_get_mesh(faces[0]);
    TEST_ASSERT(mesh != NULL);

    /* Get face data to check dirty flag */
    lc_face_data_t *face_data = (lc_face_data_t*)lc_entity_get_data(faces[0]);
    TEST_ASSERT(face_data != NULL);
    TEST_ASSERT(face_data->mesh_dirty == false);

    /* Invalidate the mesh */
    success = lc_tessellate_invalidate(faces[0]);
    TEST_ASSERT(success);
    TEST_ASSERT(face_data->mesh_dirty == true);

    printf("  Mesh invalidated, dirty flag set\n");

    /* Mesh should still exist (not freed) */
    mesh = lc_tessellate_get_mesh(faces[0]);
    TEST_ASSERT(mesh != NULL);

    /* Re-tessellate */
    success = lc_tessellate_face(faces[0]);
    TEST_ASSERT(success);
    TEST_ASSERT(face_data->mesh_dirty == false);

    printf("  Mesh re-tessellated, dirty flag cleared\n");

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_total_triangle_count(void)
{
    printf("TEST: Total Triangle Count\n");

    /* Create a unit box */
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 dims = {1.0f, 1.0f, 1.0f};
    lc_entity_handle_t solid = lc_brep_create_box(origin, dims);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Tessellate the solid */
    bool success = lc_tessellate_solid(solid);
    TEST_ASSERT(success);

    /* Count total triangles across all faces */
    lc_entity_handle_t shells[8];
    size_t shell_count = lc_topology_get_shells(solid, shells, 8);
    TEST_ASSERT(shell_count == 1);

    lc_entity_handle_t faces[16];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 16);
    TEST_ASSERT(face_count == 6);

    uint32_t total_triangles = 0;
    size_t i;
    for (i = 0; i < face_count; i++)
    {
        const lc_mesh_t *mesh = lc_tessellate_get_mesh(faces[i]);
        TEST_ASSERT(mesh != NULL);
        total_triangles += mesh->index_count / 3;
    }

    printf("  Total triangles: %u\n", total_triangles);

    /* Box should have 6 faces x 2 triangles = 12 triangles total */
    TEST_ASSERT(total_triangles == 12);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_mesh_cleanup(void)
{
    printf("TEST: Mesh Cleanup\n");

    /* Create a unit box */
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 dims = {1.0f, 1.0f, 1.0f};
    lc_entity_handle_t solid = lc_brep_create_box(origin, dims);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Tessellate the solid */
    bool success = lc_tessellate_solid(solid);
    TEST_ASSERT(success);

    /* Get first face */
    lc_entity_handle_t shells[8];
    size_t shell_count = lc_topology_get_shells(solid, shells, 8);
    TEST_ASSERT(shell_count == 1);

    lc_entity_handle_t faces[16];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 16);
    TEST_ASSERT(face_count == 6);

    /* Verify mesh exists */
    const lc_mesh_t *mesh = lc_tessellate_get_mesh(faces[0]);
    TEST_ASSERT(mesh != NULL);

    /* Get face data */
    lc_face_data_t *face_data = (lc_face_data_t*)lc_entity_get_data(faces[0]);
    TEST_ASSERT(face_data != NULL);
    TEST_ASSERT(face_data->mesh_data != NULL);

    /* Free the mesh */
    success = lc_tessellate_free_mesh(faces[0]);
    TEST_ASSERT(success);

    /* Verify mesh is freed */
    TEST_ASSERT(face_data->mesh_data == NULL);
    TEST_ASSERT(face_data->mesh_vertex_count == 0);
    TEST_ASSERT(face_data->mesh_triangle_count == 0);
    TEST_ASSERT(face_data->mesh_dirty == true);

    mesh = lc_tessellate_get_mesh(faces[0]);
    TEST_ASSERT(mesh == NULL);

    printf("  Mesh freed successfully\n");

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_vertex_positions(void)
{
    printf("TEST: Vertex Positions\n");

    /* Create a box at specific location */
    vec3 origin = {1.0f, 2.0f, 3.0f};
    vec3 dims = {4.0f, 5.0f, 6.0f};
    lc_entity_handle_t solid = lc_brep_create_box(origin, dims);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Tessellate the solid */
    bool success = lc_tessellate_solid(solid);
    TEST_ASSERT(success);

    /* Get all faces */
    lc_entity_handle_t shells[8];
    size_t shell_count = lc_topology_get_shells(solid, shells, 8);
    TEST_ASSERT(shell_count == 1);

    lc_entity_handle_t faces[16];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 16);
    TEST_ASSERT(face_count == 6);

    /* Verify vertex positions are within bounding box */
    vec3 bb_min = {1.0f, 2.0f, 3.0f};
    vec3 bb_max = {5.0f, 7.0f, 9.0f};

    size_t i;
    for (i = 0; i < face_count; i++)
    {
        const lc_mesh_t *mesh = lc_tessellate_get_mesh(faces[i]);
        TEST_ASSERT(mesh != NULL);

        size_t j;
        for (j = 0; j < mesh->vertex_count; j++)
        {
            float x = mesh->vertices[j].position[0];
            float y = mesh->vertices[j].position[1];
            float z = mesh->vertices[j].position[2];

            /* Verify position is within bounding box */
            TEST_ASSERT(x >= bb_min[0] - 0.001f && x <= bb_max[0] + 0.001f);
            TEST_ASSERT(y >= bb_min[1] - 0.001f && y <= bb_max[1] + 0.001f);
            TEST_ASSERT(z >= bb_min[2] - 0.001f && z <= bb_max[2] + 0.001f);
        }
    }

    printf("  All vertex positions within bounding box\n");

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

/***************************************************************
** MARK: MAIN
***************************************************************/

int main(void)
{
    /* Disable stdout buffering for immediate output */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("==============================================\n");
    printf("libcad B-Rep Tessellation Test Suite\n");
    printf("==============================================\n\n");

    lc_entity_init();
    lc_undo_init();
    lc_geometry_init();
    lc_brep_init();

    test_box_tessellation();
    test_face_normals();
    test_mesh_invalidation();
    test_total_triangle_count();
    test_mesh_cleanup();
    test_vertex_positions();

    lc_brep_shutdown();
    lc_geometry_shutdown();
    lc_undo_shutdown();
    lc_entity_shutdown();

    printf("\n==============================================\n");
    printf("ALL TESTS PASSED\n");
    printf("==============================================\n");

    return 0;
}
