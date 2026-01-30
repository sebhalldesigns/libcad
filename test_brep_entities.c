/***************************************************************
**
** libcad Test File
**
** File         :  test_brep_entities.c
** Module       :  libcad (B-Rep entity system test)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Test suite for B-Rep entity types. Verifies that
**                 vertex, edge, loop, face, shell, and solid entities
**                 can be created, have data attached, and form valid
**                 topology trees.
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lib/lc_entity.h"
#include "lib/lc_geometry.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

/***************************************************************
** MARK: TEST FUNCTIONS
***************************************************************/

static void test_vertex_creation(void)
{
    printf("Test: Vertex Entity Creation\n");

    /* Create vertex entity */
    lc_entity_handle_t vertex = lc_entity_create(LC_ENTITY_TYPE_VERTEX);
    assert(vertex != LC_ENTITY_INVALID);
    assert(lc_entity_is_valid(vertex));
    assert(lc_entity_get_type(vertex) == LC_ENTITY_TYPE_VERTEX);

    /* Allocate and attach vertex data */
    lc_vertex_data_t *data = (lc_vertex_data_t*)malloc(sizeof(lc_vertex_data_t));
    assert(data != NULL);
    memset(data, 0, sizeof(lc_vertex_data_t));
    data->position[0] = 1.0f;
    data->position[1] = 2.0f;
    data->position[2] = 3.0f;

    lc_entity_set_data(vertex, data);

    /* Retrieve and verify data */
    lc_vertex_data_t *retrieved = (lc_vertex_data_t*)lc_entity_get_data(vertex);
    assert(retrieved != NULL);
    assert(retrieved == data);
    assert(retrieved->position[0] == 1.0f);
    assert(retrieved->position[1] == 2.0f);
    assert(retrieved->position[2] == 3.0f);

    /* Destroy vertex (will free data automatically) */
    lc_entity_destroy(vertex);
    assert(!lc_entity_is_valid(vertex));

    printf("  PASS: Vertex entity works correctly\n");
}

static void test_edge_creation(void)
{
    printf("Test: Edge Entity Creation\n");

    /* Create two vertices */
    lc_entity_handle_t v1 = lc_entity_create(LC_ENTITY_TYPE_VERTEX);
    lc_entity_handle_t v2 = lc_entity_create(LC_ENTITY_TYPE_VERTEX);

    /* Create edge entity */
    lc_entity_handle_t edge = lc_entity_create(LC_ENTITY_TYPE_EDGE);
    assert(edge != LC_ENTITY_INVALID);
    assert(lc_entity_get_type(edge) == LC_ENTITY_TYPE_EDGE);

    /* Allocate and attach edge data */
    lc_edge_data_t *data = (lc_edge_data_t*)malloc(sizeof(lc_edge_data_t));
    assert(data != NULL);
    memset(data, 0, sizeof(lc_edge_data_t));
    data->vertex_start = v1;
    data->vertex_end = v2;
    data->curve = 1;  /* Placeholder curve handle */
    data->u_start = 0.0f;
    data->u_end = 1.0f;

    lc_entity_set_data(edge, data);

    /* Retrieve and verify data */
    lc_edge_data_t *retrieved = (lc_edge_data_t*)lc_entity_get_data(edge);
    assert(retrieved != NULL);
    assert(retrieved->vertex_start == v1);
    assert(retrieved->vertex_end == v2);
    assert(retrieved->curve == 1);

    /* Cleanup */
    lc_entity_destroy(edge);
    lc_entity_destroy(v1);
    lc_entity_destroy(v2);

    printf("  PASS: Edge entity works correctly\n");
}

static void test_loop_creation(void)
{
    printf("Test: Loop Entity Creation\n");

    /* Create loop entity */
    lc_entity_handle_t loop = lc_entity_create(LC_ENTITY_TYPE_LOOP);
    assert(loop != LC_ENTITY_INVALID);
    assert(lc_entity_get_type(loop) == LC_ENTITY_TYPE_LOOP);

    /* Allocate and attach loop data */
    lc_loop_data_t *data = (lc_loop_data_t*)malloc(sizeof(lc_loop_data_t));
    assert(data != NULL);
    memset(data, 0, sizeof(lc_loop_data_t));
    data->face = LC_ENTITY_INVALID;  /* No parent face yet */
    data->is_outer = true;

    lc_entity_set_data(loop, data);

    /* Retrieve and verify data */
    lc_loop_data_t *retrieved = (lc_loop_data_t*)lc_entity_get_data(loop);
    assert(retrieved != NULL);
    assert(retrieved->is_outer == true);

    /* Cleanup */
    lc_entity_destroy(loop);

    printf("  PASS: Loop entity works correctly\n");
}

static void test_face_creation(void)
{
    printf("Test: Face Entity Creation\n");

    /* Create face entity */
    lc_entity_handle_t face = lc_entity_create(LC_ENTITY_TYPE_FACE);
    assert(face != LC_ENTITY_INVALID);
    assert(lc_entity_get_type(face) == LC_ENTITY_TYPE_FACE);

    /* Allocate and attach face data */
    lc_face_data_t *data = (lc_face_data_t*)malloc(sizeof(lc_face_data_t));
    assert(data != NULL);
    memset(data, 0, sizeof(lc_face_data_t));
    data->surface = 1;  /* Placeholder surface handle */
    data->outer_loop = LC_ENTITY_INVALID;
    data->forward = true;
    data->mesh_vertex_count = 0;
    data->mesh_triangle_count = 0;
    data->mesh_data = NULL;
    data->mesh_dirty = true;

    lc_entity_set_data(face, data);

    /* Retrieve and verify data */
    lc_face_data_t *retrieved = (lc_face_data_t*)lc_entity_get_data(face);
    assert(retrieved != NULL);
    assert(retrieved->surface == 1);
    assert(retrieved->forward == true);
    assert(retrieved->mesh_dirty == true);

    /* Cleanup */
    lc_entity_destroy(face);

    printf("  PASS: Face entity works correctly\n");
}

static void test_shell_creation(void)
{
    printf("Test: Shell Entity Creation\n");

    /* Create shell entity */
    lc_entity_handle_t shell = lc_entity_create(LC_ENTITY_TYPE_SHELL);
    assert(shell != LC_ENTITY_INVALID);
    assert(lc_entity_get_type(shell) == LC_ENTITY_TYPE_SHELL);

    /* Allocate and attach shell data */
    lc_shell_data_t *data = (lc_shell_data_t*)malloc(sizeof(lc_shell_data_t));
    assert(data != NULL);
    memset(data, 0, sizeof(lc_shell_data_t));
    data->is_closed = true;

    lc_entity_set_data(shell, data);

    /* Retrieve and verify data */
    lc_shell_data_t *retrieved = (lc_shell_data_t*)lc_entity_get_data(shell);
    assert(retrieved != NULL);
    assert(retrieved->is_closed == true);

    /* Cleanup */
    lc_entity_destroy(shell);

    printf("  PASS: Shell entity works correctly\n");
}

static void test_solid_creation(void)
{
    printf("Test: Solid Entity Creation\n");

    /* Create solid entity */
    lc_entity_handle_t solid = lc_entity_create(LC_ENTITY_TYPE_SOLID);
    assert(solid != LC_ENTITY_INVALID);
    assert(lc_entity_get_type(solid) == LC_ENTITY_TYPE_SOLID);

    /* Allocate and attach solid data */
    lc_solid_data_t *data = (lc_solid_data_t*)malloc(sizeof(lc_solid_data_t));
    assert(data != NULL);
    memset(data, 0, sizeof(lc_solid_data_t));
    data->bbox_min[0] = -1.0f;
    data->bbox_min[1] = -1.0f;
    data->bbox_min[2] = -1.0f;
    data->bbox_max[0] = 1.0f;
    data->bbox_max[1] = 1.0f;
    data->bbox_max[2] = 1.0f;
    data->bbox_dirty = false;

    lc_entity_set_data(solid, data);

    /* Retrieve and verify data */
    lc_solid_data_t *retrieved = (lc_solid_data_t*)lc_entity_get_data(solid);
    assert(retrieved != NULL);
    assert(retrieved->bbox_min[0] == -1.0f);
    assert(retrieved->bbox_max[0] == 1.0f);
    assert(retrieved->bbox_dirty == false);

    /* Cleanup */
    lc_entity_destroy(solid);

    printf("  PASS: Solid entity works correctly\n");
}

static void test_edge_use_creation(void)
{
    printf("Test: Edge Use Entity Creation\n");

    /* Create edge use entity */
    lc_entity_handle_t edge_use = lc_entity_create(LC_ENTITY_TYPE_EDGE_USE);
    assert(edge_use != LC_ENTITY_INVALID);
    assert(lc_entity_get_type(edge_use) == LC_ENTITY_TYPE_EDGE_USE);

    /* Allocate and attach edge use data */
    lc_edge_use_data_t *data = (lc_edge_use_data_t*)malloc(sizeof(lc_edge_use_data_t));
    assert(data != NULL);
    memset(data, 0, sizeof(lc_edge_use_data_t));
    data->edge = LC_ENTITY_INVALID;
    data->loop = LC_ENTITY_INVALID;
    data->next_in_loop = LC_ENTITY_INVALID;
    data->prev_in_loop = LC_ENTITY_INVALID;
    data->forward = true;

    lc_entity_set_data(edge_use, data);

    /* Retrieve and verify data */
    lc_edge_use_data_t *retrieved = (lc_edge_use_data_t*)lc_entity_get_data(edge_use);
    assert(retrieved != NULL);
    assert(retrieved->forward == true);

    /* Cleanup */
    lc_entity_destroy(edge_use);

    printf("  PASS: Edge use entity works correctly\n");
}

static void test_topology_tree(void)
{
    printf("Test: B-Rep Topology Tree\n");

    /* Create a simple topology: solid -> shell -> face -> loop */
    lc_entity_handle_t solid = lc_entity_create(LC_ENTITY_TYPE_SOLID);
    lc_entity_handle_t shell = lc_entity_create(LC_ENTITY_TYPE_SHELL);
    lc_entity_handle_t face = lc_entity_create(LC_ENTITY_TYPE_FACE);
    lc_entity_handle_t loop = lc_entity_create(LC_ENTITY_TYPE_LOOP);

    /* Build tree: solid -> shell -> face -> loop */
    lc_entity_add_child(solid, shell);
    lc_entity_add_child(shell, face);
    lc_entity_add_child(face, loop);

    /* Verify parent relationships */
    assert(lc_entity_get_parent(shell) == solid);
    assert(lc_entity_get_parent(face) == shell);
    assert(lc_entity_get_parent(loop) == face);

    /* Verify child relationships */
    assert(lc_entity_get_first_child(solid) == shell);
    assert(lc_entity_get_first_child(shell) == face);
    assert(lc_entity_get_first_child(face) == loop);

    /* Cleanup (destroying parent will destroy children) */
    lc_entity_destroy(solid);
    lc_entity_destroy(shell);
    lc_entity_destroy(face);
    lc_entity_destroy(loop);

    printf("  PASS: Topology tree structure works correctly\n");
}

static void test_large_scale(void)
{
    printf("Test: Large Scale B-Rep Entity Creation (1000 of each type)\n");

    int i;

    /* Create 1000 vertices */
    for (i = 0; i < 1000; i++)
    {
        lc_entity_handle_t vertex = lc_entity_create(LC_ENTITY_TYPE_VERTEX);
        assert(vertex != LC_ENTITY_INVALID);

        lc_vertex_data_t *data = (lc_vertex_data_t*)malloc(sizeof(lc_vertex_data_t));
        memset(data, 0, sizeof(lc_vertex_data_t));
        data->position[0] = (float)i;
        lc_entity_set_data(vertex, data);
    }

    /* Create 1000 edges */
    for (i = 0; i < 1000; i++)
    {
        lc_entity_handle_t edge = lc_entity_create(LC_ENTITY_TYPE_EDGE);
        assert(edge != LC_ENTITY_INVALID);

        lc_edge_data_t *data = (lc_edge_data_t*)malloc(sizeof(lc_edge_data_t));
        memset(data, 0, sizeof(lc_edge_data_t));
        lc_entity_set_data(edge, data);
    }

    /* Create 1000 faces */
    for (i = 0; i < 1000; i++)
    {
        lc_entity_handle_t face = lc_entity_create(LC_ENTITY_TYPE_FACE);
        assert(face != LC_ENTITY_INVALID);

        lc_face_data_t *data = (lc_face_data_t*)malloc(sizeof(lc_face_data_t));
        memset(data, 0, sizeof(lc_face_data_t));
        lc_entity_set_data(face, data);
    }

    printf("  PASS: Created 3000 B-Rep entities successfully\n");
}

/***************************************************************
** MARK: MAIN
***************************************************************/

int main(void)
{
    printf("==============================================\n");
    printf("libcad B-Rep Entity System Test Suite\n");
    printf("==============================================\n\n");

    lc_entity_init();
    lc_geometry_init();

    test_vertex_creation();
    test_edge_creation();
    test_edge_use_creation();
    test_loop_creation();
    test_face_creation();
    test_shell_creation();
    test_solid_creation();
    test_topology_tree();
    test_large_scale();

    lc_geometry_shutdown();
    lc_entity_shutdown();

    printf("\n==============================================\n");
    printf("ALL TESTS PASSED\n");
    printf("==============================================\n");

    return 0;
}
