/***************************************************************
**
** libcad Test File
**
** File         :  test_primitives.c
** Module       :  libcad (B-Rep primitives test)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Test suite for B-Rep cylinder and sphere primitives.
**                 Tests creation, topology counts, Euler characteristic,
**                 bounding boxes, and tessellation.
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

static void test_cylinder_creation(void)
{
    printf("TEST: Cylinder Creation\n");

    /* Create a cylinder with 8 segments */
    vec3 base_center = {0.0f, 0.0f, 0.0f};
    vec3 axis = {0.0f, 0.0f, 1.0f};
    float radius = 1.0f;
    float height = 2.0f;
    int segments = 8;

    lc_entity_handle_t solid = lc_brep_create_cylinder(base_center, axis, radius, height, segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Verify topology element counts */
    size_t vertex_count = 0;
    size_t edge_count = 0;
    size_t face_count = 0;
    lc_topology_count_elements(solid, &vertex_count, &edge_count, &face_count);

    printf("  Topology: V=%zu, E=%zu, F=%zu\n", vertex_count, edge_count, face_count);

    /* Expected: V = 2*segments = 16, E = 3*segments = 24, F = segments+2 = 10 */
    TEST_ASSERT(vertex_count == 16);
    TEST_ASSERT(edge_count == 24);
    TEST_ASSERT(face_count == 10);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_cylinder_euler(void)
{
    printf("TEST: Cylinder Euler Characteristic\n");

    vec3 base_center = {1.0f, 2.0f, 3.0f};
    vec3 axis = {0.0f, 1.0f, 0.0f};
    float radius = 2.5f;
    float height = 5.0f;
    int segments = 16;

    lc_entity_handle_t solid = lc_brep_create_cylinder(base_center, axis, radius, height, segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Verify Euler characteristic (V - E + F = 2 for a solid) */
    size_t vertex_count = 0;
    size_t edge_count = 0;
    size_t face_count = 0;
    lc_topology_count_elements(solid, &vertex_count, &edge_count, &face_count);

    bool euler_valid = lc_topology_check_euler(solid);
    TEST_ASSERT(euler_valid);

    int euler_char = (int)vertex_count - (int)edge_count + (int)face_count;
    printf("  V=%zu, E=%zu, F=%zu\n", vertex_count, edge_count, face_count);
    printf("  Euler: V - E + F = %d (expected 2)\n", euler_char);
    TEST_ASSERT(euler_char == 2);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_cylinder_bbox(void)
{
    printf("TEST: Cylinder Bounding Box\n");

    /* Create cylinder along Z axis */
    vec3 base_center = {0.0f, 0.0f, 0.0f};
    vec3 axis = {0.0f, 0.0f, 1.0f};
    float radius = 2.0f;
    float height = 5.0f;
    int segments = 12;

    lc_entity_handle_t solid = lc_brep_create_cylinder(base_center, axis, radius, height, segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Compute bounding box */
    vec3 bb_min, bb_max;
    lc_topology_compute_bbox(solid, bb_min, bb_max);

    printf("  BBox: min=(%.2f, %.2f, %.2f) max=(%.2f, %.2f, %.2f)\n",
           bb_min[0], bb_min[1], bb_min[2], bb_max[0], bb_max[1], bb_max[2]);

    /* Verify bounding box includes cylinder extents */
    /* X and Y should span [-radius, +radius], Z should span [0, height] */
    TEST_ASSERT(bb_min[0] <= -radius + 0.01f);
    TEST_ASSERT(bb_max[0] >= radius - 0.01f);
    TEST_ASSERT(bb_min[1] <= -radius + 0.01f);
    TEST_ASSERT(bb_max[1] >= radius - 0.01f);
    TEST_ASSERT(bb_min[2] <= 0.01f);
    TEST_ASSERT(bb_max[2] >= height - 0.01f);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_cylinder_tessellation(void)
{
    printf("TEST: Cylinder Tessellation\n");

    vec3 base_center = {0.0f, 0.0f, 0.0f};
    vec3 axis = {0.0f, 0.0f, 1.0f};
    float radius = 1.0f;
    float height = 2.0f;
    int segments = 8;

    lc_entity_handle_t solid = lc_brep_create_cylinder(base_center, axis, radius, height, segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Tessellate the solid */
    bool tessellated = lc_tessellate_solid(solid);
    TEST_ASSERT(tessellated);

    /* Get shells and faces */
    lc_entity_handle_t shells[8];
    size_t shell_count = lc_topology_get_shells(solid, shells, 8);
    TEST_ASSERT(shell_count == 1);

    lc_entity_handle_t faces[64];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 64);
    TEST_ASSERT(face_count == 10);

    /* Verify each face has a mesh */
    size_t total_triangles = 0;
    size_t f;
    for (f = 0; f < face_count; f++)
    {
        const lc_mesh_t *mesh = lc_tessellate_get_mesh(faces[f]);
        TEST_ASSERT(mesh != NULL);
        TEST_ASSERT(mesh->index_count % 3 == 0);
        total_triangles += mesh->index_count / 3;
    }

    printf("  Total triangles: %zu\n", total_triangles);
    TEST_ASSERT(total_triangles > 0);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_sphere_creation(void)
{
    printf("TEST: Sphere Creation\n");

    /* Create a sphere with u=8, v=4 */
    vec3 center = {0.0f, 0.0f, 0.0f};
    float radius = 1.0f;
    int u_segments = 8;
    int v_segments = 4;

    lc_entity_handle_t solid = lc_brep_create_sphere(center, radius, u_segments, v_segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Verify topology element counts */
    size_t vertex_count = 0;
    size_t edge_count = 0;
    size_t face_count = 0;
    lc_topology_count_elements(solid, &vertex_count, &edge_count, &face_count);

    printf("  Topology: V=%zu, E=%zu, F=%zu\n", vertex_count, edge_count, face_count);

    /* Expected: V = (v-1)*u + 2 = 3*8 + 2 = 26
     *           F = u*v = 8*4 = 32
     *           E = V + F - 2 = 26 + 32 - 2 = 56
     */
    TEST_ASSERT(vertex_count == 26);
    TEST_ASSERT(edge_count == 56);
    TEST_ASSERT(face_count == 32);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_sphere_euler(void)
{
    printf("TEST: Sphere Euler Characteristic\n");

    vec3 center = {5.0f, 10.0f, 15.0f};
    float radius = 3.0f;
    int u_segments = 12;
    int v_segments = 6;

    lc_entity_handle_t solid = lc_brep_create_sphere(center, radius, u_segments, v_segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Verify Euler characteristic (V - E + F = 2 for a solid) */
    size_t vertex_count = 0;
    size_t edge_count = 0;
    size_t face_count = 0;
    lc_topology_count_elements(solid, &vertex_count, &edge_count, &face_count);

    bool euler_valid = lc_topology_check_euler(solid);
    TEST_ASSERT(euler_valid);

    int euler_char = (int)vertex_count - (int)edge_count + (int)face_count;
    printf("  V=%zu, E=%zu, F=%zu\n", vertex_count, edge_count, face_count);
    printf("  Euler: V - E + F = %d (expected 2)\n", euler_char);
    TEST_ASSERT(euler_char == 2);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_sphere_bbox(void)
{
    printf("TEST: Sphere Bounding Box\n");

    /* Create sphere centered at origin */
    vec3 center = {0.0f, 0.0f, 0.0f};
    float radius = 5.0f;
    int u_segments = 16;
    int v_segments = 8;

    lc_entity_handle_t solid = lc_brep_create_sphere(center, radius, u_segments, v_segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Compute bounding box */
    vec3 bb_min, bb_max;
    lc_topology_compute_bbox(solid, bb_min, bb_max);

    printf("  BBox: min=(%.2f, %.2f, %.2f) max=(%.2f, %.2f, %.2f)\n",
           bb_min[0], bb_min[1], bb_min[2], bb_max[0], bb_max[1], bb_max[2]);

    /* Verify bounding box is centered and has correct size */
    /* All axes should span [-radius, +radius] */
    TEST_ASSERT(bb_min[0] <= -radius + 0.01f);
    TEST_ASSERT(bb_max[0] >= radius - 0.01f);
    TEST_ASSERT(bb_min[1] <= -radius + 0.01f);
    TEST_ASSERT(bb_max[1] >= radius - 0.01f);
    TEST_ASSERT(bb_min[2] <= -radius + 0.01f);
    TEST_ASSERT(bb_max[2] >= radius - 0.01f);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_sphere_tessellation(void)
{
    printf("TEST: Sphere Tessellation\n");

    vec3 center = {0.0f, 0.0f, 0.0f};
    float radius = 1.0f;
    int u_segments = 8;
    int v_segments = 4;

    lc_entity_handle_t solid = lc_brep_create_sphere(center, radius, u_segments, v_segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Tessellate the solid */
    bool tessellated = lc_tessellate_solid(solid);
    TEST_ASSERT(tessellated);

    /* Get shells and faces */
    lc_entity_handle_t shells[8];
    size_t shell_count = lc_topology_get_shells(solid, shells, 8);
    TEST_ASSERT(shell_count == 1);

    lc_entity_handle_t faces[128];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 128);
    TEST_ASSERT(face_count == 32);

    /* Verify each face has a mesh */
    size_t total_triangles = 0;
    size_t f;
    for (f = 0; f < face_count; f++)
    {
        const lc_mesh_t *mesh = lc_tessellate_get_mesh(faces[f]);
        TEST_ASSERT(mesh != NULL);
        TEST_ASSERT(mesh->index_count % 3 == 0);
        total_triangles += mesh->index_count / 3;
    }

    printf("  Total triangles: %zu\n", total_triangles);
    TEST_ASSERT(total_triangles > 0);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_cylinder_destroy(void)
{
    printf("TEST: Cylinder Destroy\n");

    vec3 base_center = {0.0f, 0.0f, 0.0f};
    vec3 axis = {0.0f, 0.0f, 1.0f};
    float radius = 1.0f;
    float height = 2.0f;
    int segments = 8;

    lc_entity_handle_t solid = lc_brep_create_cylinder(base_center, axis, radius, height, segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Destroy solid */
    bool destroyed = lc_brep_destroy_solid(solid);
    TEST_ASSERT(destroyed);

    /* Verify handle is now invalid */
    TEST_ASSERT(!lc_entity_is_valid(solid));

    printf("  PASS\n\n");
}

static void test_sphere_destroy(void)
{
    printf("TEST: Sphere Destroy\n");

    vec3 center = {0.0f, 0.0f, 0.0f};
    float radius = 1.0f;
    int u_segments = 8;
    int v_segments = 4;

    lc_entity_handle_t solid = lc_brep_create_sphere(center, radius, u_segments, v_segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Destroy solid */
    bool destroyed = lc_brep_destroy_solid(solid);
    TEST_ASSERT(destroyed);

    /* Verify handle is now invalid */
    TEST_ASSERT(!lc_entity_is_valid(solid));

    printf("  PASS\n\n");
}

/***************************************************************
** MARK: MAIN
***************************************************************/

int main(void)
{
    /* Disable buffering to see output immediately */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("=================================================\n");
    printf("  libcad B-Rep Primitives Test Suite\n");
    printf("=================================================\n\n");

    /* Initialize systems */
    lc_entity_init();
    lc_geometry_init();
    lc_brep_init();

    /* Run tests */
    test_cylinder_creation();
    test_cylinder_euler();
    test_cylinder_bbox();
    test_cylinder_tessellation();
    test_sphere_creation();
    test_sphere_euler();
    test_sphere_bbox();
    test_sphere_tessellation();
    test_cylinder_destroy();
    test_sphere_destroy();

    /* Shutdown systems */
    lc_brep_shutdown();
    lc_geometry_shutdown();
    lc_entity_shutdown();

    printf("=================================================\n");
    printf("  ALL TESTS PASSED\n");
    printf("=================================================\n");

    return 0;
}
