/***************************************************************
**
** libcad Test File
**
** File         :  test_brep.c
** Module       :  libcad (B-Rep test)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Test suite for B-Rep primitive construction and
**                 topology queries. Tests box creation, topology
**                 traversal, bounding box computation, validation,
**                 and edge sharing.
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

static void test_box_creation(void)
{
    printf("TEST: Box Creation\n");

    /* Create a box at origin with dimensions (2,3,4) */
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 dims = {2.0f, 3.0f, 4.0f};
    lc_entity_handle_t solid = lc_brep_create_box(origin, dims);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Verify topology element counts */
    size_t vertex_count = 0;
    size_t edge_count = 0;
    size_t face_count = 0;
    lc_topology_count_elements(solid, &vertex_count, &edge_count, &face_count);

    printf("  Topology: V=%zu, E=%zu, F=%zu\n", vertex_count, edge_count, face_count);
    TEST_ASSERT(vertex_count == 8);
    TEST_ASSERT(edge_count == 12);
    TEST_ASSERT(face_count == 6);

    /* Verify Euler characteristic (V - E + F = 2 for a solid) */
    bool euler_valid = lc_topology_check_euler(solid);
    TEST_ASSERT(euler_valid);
    printf("  Euler: V - E + F = %d (valid)\n",
           (int)vertex_count - (int)edge_count + (int)face_count);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_box_topology_traversal(void)
{
    printf("TEST: Box Topology Traversal\n");

    /* Create a unit box */
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 dims = {1.0f, 1.0f, 1.0f};
    lc_entity_handle_t solid = lc_brep_create_box(origin, dims);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Get shells (expect 1) */
    lc_entity_handle_t shells[8];
    size_t shell_count = lc_topology_get_shells(solid, shells, 8);
    printf("  Shell count: %zu\n", shell_count);
    TEST_ASSERT(shell_count == 1);

    /* Get faces from shell (expect 6) */
    lc_entity_handle_t faces[16];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 16);
    printf("  Face count: %zu\n", face_count);
    TEST_ASSERT(face_count == 6);

    /* Get loops from first face (expect 1) */
    lc_entity_handle_t loops[8];
    size_t loop_count = lc_topology_get_loops(faces[0], loops, 8);
    printf("  Loop count (face 0): %zu\n", loop_count);
    TEST_ASSERT(loop_count == 1);

    /* Get edges from loop (expect 4) */
    lc_entity_handle_t edges[16];
    size_t edge_count = lc_topology_get_edges(loops[0], edges, 16);
    printf("  Edge count (loop 0): %zu\n", edge_count);
    TEST_ASSERT(edge_count == 4);

    /* Get vertices from first edge */
    lc_entity_handle_t start_vertex;
    lc_entity_handle_t end_vertex;
    bool got_vertices = lc_topology_get_edge_vertices(edges[0], &start_vertex, &end_vertex);
    TEST_ASSERT(got_vertices);
    TEST_ASSERT(lc_entity_is_valid(start_vertex));
    TEST_ASSERT(lc_entity_is_valid(end_vertex));

    /* Get vertex positions and verify they're within the unit cube */
    vec3 start_pos, end_pos;
    lc_topology_get_vertex_position(start_vertex, start_pos);
    lc_topology_get_vertex_position(end_vertex, end_pos);

    printf("  Edge 0 start: (%.2f, %.2f, %.2f)\n", start_pos[0], start_pos[1], start_pos[2]);
    printf("  Edge 0 end:   (%.2f, %.2f, %.2f)\n", end_pos[0], end_pos[1], end_pos[2]);

    /* Verify positions are within [0,1] range */
    int i;
    for (i = 0; i < 3; i++)
    {
        TEST_ASSERT(start_pos[i] >= -0.001f && start_pos[i] <= 1.001f);
        TEST_ASSERT(end_pos[i] >= -0.001f && end_pos[i] <= 1.001f);
    }

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_box_bounding_box(void)
{
    printf("TEST: Box Bounding Box\n");

    /* Create box at (1,2,3) with dimensions (4,5,6) */
    vec3 origin = {1.0f, 2.0f, 3.0f};
    vec3 dims = {4.0f, 5.0f, 6.0f};
    lc_entity_handle_t solid = lc_brep_create_box(origin, dims);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Compute bounding box */
    vec3 bb_min, bb_max;
    lc_topology_compute_bbox(solid, bb_min, bb_max);

    printf("  Min: (%.2f, %.2f, %.2f)\n", bb_min[0], bb_min[1], bb_min[2]);
    printf("  Max: (%.2f, %.2f, %.2f)\n", bb_max[0], bb_max[1], bb_max[2]);

    /* Verify min is approximately (1,2,3) */
    TEST_ASSERT(fabsf(bb_min[0] - 1.0f) < 0.001f);
    TEST_ASSERT(fabsf(bb_min[1] - 2.0f) < 0.001f);
    TEST_ASSERT(fabsf(bb_min[2] - 3.0f) < 0.001f);

    /* Verify max is approximately (5,7,9) */
    TEST_ASSERT(fabsf(bb_max[0] - 5.0f) < 0.001f);
    TEST_ASSERT(fabsf(bb_max[1] - 7.0f) < 0.001f);
    TEST_ASSERT(fabsf(bb_max[2] - 9.0f) < 0.001f);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_box_validation(void)
{
    printf("TEST: Box Validation\n");

    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 dims = {1.0f, 1.0f, 1.0f};
    lc_entity_handle_t solid = lc_brep_create_box(origin, dims);
    TEST_ASSERT(lc_entity_is_valid(solid));

    bool is_valid = lc_brep_validate_solid(solid);
    TEST_ASSERT(is_valid);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_box_destroy(void)
{
    printf("TEST: Box Destroy\n");

    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 dims = {1.0f, 1.0f, 1.0f};
    lc_entity_handle_t solid = lc_brep_create_box(origin, dims);
    TEST_ASSERT(lc_entity_is_valid(solid));

    lc_brep_destroy_solid(solid);
    TEST_ASSERT(!lc_entity_is_valid(solid));

    printf("  PASS\n\n");
}

static void test_edge_sharing(void)
{
    printf("TEST: Edge Sharing\n");

    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 dims = {1.0f, 1.0f, 1.0f};
    lc_entity_handle_t solid = lc_brep_create_box(origin, dims);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Verify exactly 12 edges (shared between faces) */
    size_t vertex_count = 0;
    size_t edge_count = 0;
    size_t face_count = 0;
    lc_topology_count_elements(solid, &vertex_count, &edge_count, &face_count);
    printf("  Edge count: %zu (shared)\n", edge_count);
    TEST_ASSERT(edge_count == 12);

    /* Get shells -> faces -> loops -> edges */
    lc_entity_handle_t shells[8];
    size_t shell_count = lc_topology_get_shells(solid, shells, 8);
    TEST_ASSERT(shell_count == 1);

    lc_entity_handle_t faces[16];
    size_t fc = lc_topology_get_faces(shells[0], faces, 16);
    TEST_ASSERT(fc == 6);

    lc_entity_handle_t loops[8];
    size_t lc_count = lc_topology_get_loops(faces[0], loops, 8);
    TEST_ASSERT(lc_count == 1);

    lc_entity_handle_t edges[16];
    size_t ec = lc_topology_get_edges(loops[0], edges, 16);
    TEST_ASSERT(ec == 4);

    /* First edge should be shared by 2 faces */
    lc_entity_handle_t adjacent_faces[8];
    size_t adj_count = lc_topology_get_edge_faces(edges[0], adjacent_faces, 8);
    printf("  Edge 0 adjacent faces: %zu\n", adj_count);
    TEST_ASSERT(adj_count == 2);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_multiple_boxes(void)
{
    printf("TEST: Multiple Boxes\n");

    lc_entity_handle_t boxes[10];
    int i;

    for (i = 0; i < 10; i++)
    {
        vec3 origin = {(float)i * 2.0f, (float)i * 3.0f, (float)i * 4.0f};
        vec3 dims = {1.0f, 1.0f, 1.0f};
        boxes[i] = lc_brep_create_box(origin, dims);
        TEST_ASSERT(lc_entity_is_valid(boxes[i]));
    }

    printf("  Created 10 boxes\n");

    for (i = 0; i < 10; i++)
    {
        size_t v = 0, e = 0, f = 0;
        lc_topology_count_elements(boxes[i], &v, &e, &f);
        TEST_ASSERT(v == 8);
        TEST_ASSERT(e == 12);
        TEST_ASSERT(f == 6);
        TEST_ASSERT(lc_topology_check_euler(boxes[i]));
    }

    printf("  All validated (V=8, E=12, F=6, Euler OK)\n");

    for (i = 0; i < 10; i++)
    {
        lc_brep_destroy_solid(boxes[i]);
        TEST_ASSERT(!lc_entity_is_valid(boxes[i]));
    }

    printf("  All destroyed\n");
    printf("  PASS\n\n");
}

static void test_cylinder_creation(void)
{
    printf("TEST: Cylinder Creation\n");

    vec3 base = {0.0f, 0.0f, 0.0f};
    vec3 axis = {0.0f, 0.0f, 1.0f};
    int segments = 8;
    lc_entity_handle_t solid = lc_brep_create_cylinder(base, axis, 1.0f, 2.0f, segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Verify topology counts */
    size_t vertex_count = 0;
    size_t edge_count = 0;
    size_t face_count = 0;
    lc_topology_count_elements(solid, &vertex_count, &edge_count, &face_count);

    printf("  Topology: V=%zu, E=%zu, F=%zu\n", vertex_count, edge_count, face_count);
    TEST_ASSERT(vertex_count == (size_t)(segments * 2));
    TEST_ASSERT(edge_count == (size_t)(segments * 3));
    TEST_ASSERT(face_count == (size_t)(segments + 2));

    /* Verify Euler characteristic */
    bool euler_valid = lc_topology_check_euler(solid);
    TEST_ASSERT(euler_valid);
    printf("  Euler: V - E + F = %d (valid)\n",
           (int)vertex_count - (int)edge_count + (int)face_count);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_cylinder_surface_types(void)
{
    printf("TEST: Cylinder Surface Types\n");

    vec3 base = {0.0f, 0.0f, 0.0f};
    vec3 axis = {0.0f, 0.0f, 1.0f};
    int segments = 6;
    lc_entity_handle_t solid = lc_brep_create_cylinder(base, axis, 1.0f, 2.0f, segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Get all faces */
    lc_entity_handle_t shells[8];
    size_t shell_count = lc_topology_get_shells(solid, shells, 8);
    TEST_ASSERT(shell_count == 1);

    lc_entity_handle_t faces[64];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 64);
    TEST_ASSERT(face_count == (size_t)(segments + 2));

    /* Count surface types: side faces should be cylinder, caps should be plane */
    int cylinder_count = 0;
    int plane_count = 0;
    size_t fi;
    for (fi = 0; fi < face_count; fi++)
    {
        lc_face_data_t *fd = (lc_face_data_t *)lc_entity_get_data(faces[fi]);
        TEST_ASSERT(fd != NULL);
        lc_surface_type_t st = lc_geometry_get_surface_type(fd->surface);
        if (st == LC_SURFACE_CYLINDER)
        {
            cylinder_count++;
        }
        else if (st == LC_SURFACE_PLANE)
        {
            plane_count++;
        }
    }

    printf("  Cylinder faces: %d, Plane faces: %d\n", cylinder_count, plane_count);
    TEST_ASSERT(cylinder_count == segments);
    TEST_ASSERT(plane_count == 2);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_sphere_creation(void)
{
    printf("TEST: Sphere Creation\n");

    vec3 center = {0.0f, 0.0f, 0.0f};
    int u_seg = 6;
    int v_seg = 4;
    lc_entity_handle_t solid = lc_brep_create_sphere(center, 1.0f, u_seg, v_seg);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Verify topology counts */
    size_t vertex_count = 0;
    size_t edge_count = 0;
    size_t face_count = 0;
    lc_topology_count_elements(solid, &vertex_count, &edge_count, &face_count);

    printf("  Topology: V=%zu, E=%zu, F=%zu\n", vertex_count, edge_count, face_count);

    /* Expected: 2 poles + (v_seg-1)*u_seg ring vertices */
    size_t expected_verts = 2 + (size_t)(v_seg - 1) * u_seg;
    /* Expected faces: 2*u_seg pole triangles + (v_seg-2)*u_seg quads */
    size_t expected_faces = (size_t)(2 * u_seg + (v_seg - 2) * u_seg);
    printf("  Expected V=%zu, F=%zu\n", expected_verts, expected_faces);
    TEST_ASSERT(vertex_count == expected_verts);
    TEST_ASSERT(face_count == expected_faces);

    /* Verify Euler characteristic */
    bool euler_valid = lc_topology_check_euler(solid);
    TEST_ASSERT(euler_valid);
    printf("  Euler: V - E + F = %d (valid)\n",
           (int)vertex_count - (int)edge_count + (int)face_count);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_sphere_surface_types(void)
{
    printf("TEST: Sphere Surface Types\n");

    vec3 center = {0.0f, 0.0f, 0.0f};
    int u_seg = 4;
    int v_seg = 3;
    lc_entity_handle_t solid = lc_brep_create_sphere(center, 1.0f, u_seg, v_seg);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Get all faces */
    lc_entity_handle_t shells[8];
    size_t shell_count = lc_topology_get_shells(solid, shells, 8);
    TEST_ASSERT(shell_count == 1);

    lc_entity_handle_t faces[64];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 64);

    /* All sphere faces should have sphere surface type */
    int sphere_count = 0;
    size_t fi;
    for (fi = 0; fi < face_count; fi++)
    {
        lc_face_data_t *fd = (lc_face_data_t *)lc_entity_get_data(faces[fi]);
        TEST_ASSERT(fd != NULL);
        lc_surface_type_t st = lc_geometry_get_surface_type(fd->surface);
        if (st == LC_SURFACE_SPHERE)
        {
            sphere_count++;
        }
    }

    printf("  Sphere faces: %d / %zu\n", sphere_count, face_count);
    TEST_ASSERT(sphere_count == (int)face_count);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_cylinder_tessellation(void)
{
    printf("TEST: Cylinder Tessellation\n");

    vec3 base = {0.0f, 0.0f, 0.0f};
    vec3 axis = {0.0f, 0.0f, 1.0f};
    int segments = 8;
    lc_entity_handle_t solid = lc_brep_create_cylinder(base, axis, 1.0f, 2.0f, segments);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Tessellate all faces */
    bool tess_ok = lc_tessellate_solid(solid);
    TEST_ASSERT(tess_ok);

    /* Check that side faces have smooth normals (not all the same) */
    lc_entity_handle_t shells[8];
    lc_topology_get_shells(solid, shells, 8);
    lc_entity_handle_t faces[64];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 64);

    int side_faces_with_varying_normals = 0;
    size_t fi;
    for (fi = 0; fi < face_count; fi++)
    {
        lc_face_data_t *fd = (lc_face_data_t *)lc_entity_get_data(faces[fi]);
        if (lc_geometry_get_surface_type(fd->surface) != LC_SURFACE_CYLINDER)
        {
            continue;
        }

        const lc_mesh_t *mesh = lc_tessellate_get_mesh(faces[fi]);
        TEST_ASSERT(mesh != NULL);
        TEST_ASSERT(mesh->vertex_count > 0);
        TEST_ASSERT(mesh->index_count > 0);

        /* Check that normals vary across the face (smooth shading) */
        if (mesh->vertex_count > 1)
        {
            float n0x = mesh->vertices[0].normal[0];
            float n0y = mesh->vertices[0].normal[1];
            float n0z = mesh->vertices[0].normal[2];
            uint32_t vi;
            for (vi = 1; vi < mesh->vertex_count; vi++)
            {
                float dx = mesh->vertices[vi].normal[0] - n0x;
                float dy = mesh->vertices[vi].normal[1] - n0y;
                float dz = mesh->vertices[vi].normal[2] - n0z;
                if (dx * dx + dy * dy + dz * dz > 0.001f)
                {
                    side_faces_with_varying_normals++;
                    break;
                }
            }
        }
    }

    printf("  Side faces with smooth normals: %d / %d\n", side_faces_with_varying_normals, segments);
    TEST_ASSERT(side_faces_with_varying_normals == segments);

    lc_brep_destroy_solid(solid);
    printf("  PASS\n\n");
}

static void test_sphere_tessellation(void)
{
    printf("TEST: Sphere Tessellation\n");

    vec3 center = {0.0f, 0.0f, 0.0f};
    int u_seg = 4;
    int v_seg = 3;
    lc_entity_handle_t solid = lc_brep_create_sphere(center, 1.0f, u_seg, v_seg);
    TEST_ASSERT(lc_entity_is_valid(solid));

    /* Tessellate all faces */
    bool tess_ok = lc_tessellate_solid(solid);
    TEST_ASSERT(tess_ok);

    /* Verify all faces have meshes and normals point outward */
    lc_entity_handle_t shells[8];
    lc_topology_get_shells(solid, shells, 8);
    lc_entity_handle_t faces[64];
    size_t face_count = lc_topology_get_faces(shells[0], faces, 64);

    size_t fi;
    for (fi = 0; fi < face_count; fi++)
    {
        const lc_mesh_t *mesh = lc_tessellate_get_mesh(faces[fi]);
        TEST_ASSERT(mesh != NULL);
        TEST_ASSERT(mesh->vertex_count > 0);
        TEST_ASSERT(mesh->index_count > 0);

        /* All normals on a sphere should point outward from center.
         * dot(normal, position - center) should be > 0 */
        uint32_t vi;
        for (vi = 0; vi < mesh->vertex_count; vi++)
        {
            float px = mesh->vertices[vi].position[0] - center[0];
            float py = mesh->vertices[vi].position[1] - center[1];
            float pz = mesh->vertices[vi].position[2] - center[2];
            float nx = mesh->vertices[vi].normal[0];
            float ny = mesh->vertices[vi].normal[1];
            float nz = mesh->vertices[vi].normal[2];
            float dot_val = px * nx + py * ny + pz * nz;
            TEST_ASSERT(dot_val > -0.01f);
        }
    }

    printf("  All sphere face normals point outward\n");

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
    printf("libcad B-Rep Construction Test Suite\n");
    printf("==============================================\n\n");

    lc_entity_init();
    lc_undo_init();
    lc_geometry_init();
    lc_brep_init();

    test_box_creation();
    test_box_topology_traversal();
    test_box_bounding_box();
    test_box_validation();
    test_box_destroy();
    test_edge_sharing();
    test_multiple_boxes();
    test_cylinder_creation();
    test_cylinder_surface_types();
    test_sphere_creation();
    test_sphere_surface_types();
    test_cylinder_tessellation();
    test_sphere_tessellation();

    lc_brep_shutdown();
    lc_geometry_shutdown();
    lc_undo_shutdown();
    lc_entity_shutdown();

    printf("\n==============================================\n");
    printf("ALL TESTS PASSED\n");
    printf("==============================================\n");

    return 0;
}
