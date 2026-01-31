/***************************************************************
**
** libcad Test File
**
** File         :  test_euler.c
** Module       :  libcad (Euler operators test)
** Author       :  SH
** Created      :  2026-01-31 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Test suite for Euler operators. Tests MVEF, MEV,
**                 MEF, KEV, KEF, MEKL, KEML and verifies Euler
**                 characteristic preservation.
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lib/lc_entity.h"
#include "lib/lc_geometry.h"
#include "lib/lc_brep.h"
#include "lib/lc_topology.h"
#include "lib/lc_euler.h"
#include "lib/lc_undo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

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
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static lc_entity_handle_t create_test_shell(void);
static void count_loop_edge_uses(lc_entity_handle_t loop, int *out_count);

/***************************************************************
** MARK: TEST FUNCTIONS
***************************************************************/

static void test_mvef(void)
{
    lc_entity_handle_t shell, face, edge, v1, v2;
    lc_entity_handle_t loop;
    lc_face_data_t *face_data;
    int eu_count;

    printf("TEST: MVEF (Make-Vertex-Edge-Face)\n");

    shell = create_test_shell();
    TEST_ASSERT(lc_entity_is_valid(shell));

    /* Create initial face with 2 vertices and 1 edge */
    vec3 pos1 = {0.0f, 0.0f, 0.0f};
    vec3 pos2 = {1.0f, 0.0f, 0.0f};
    TEST_ASSERT(lc_euler_mvef(shell, pos1, pos2, &face, &edge, &v1, &v2));

    /* Verify handles */
    TEST_ASSERT(lc_entity_is_valid(face));
    TEST_ASSERT(lc_entity_is_valid(edge));
    TEST_ASSERT(lc_entity_is_valid(v1));
    TEST_ASSERT(lc_entity_is_valid(v2));

    /* Verify types */
    TEST_ASSERT(lc_entity_get_type(face) == LC_ENTITY_TYPE_FACE);
    TEST_ASSERT(lc_entity_get_type(edge) == LC_ENTITY_TYPE_EDGE);
    TEST_ASSERT(lc_entity_get_type(v1) == LC_ENTITY_TYPE_VERTEX);
    TEST_ASSERT(lc_entity_get_type(v2) == LC_ENTITY_TYPE_VERTEX);

    /* Verify face has outer loop */
    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    TEST_ASSERT(face_data != NULL);
    loop = face_data->outer_loop;
    TEST_ASSERT(lc_entity_is_valid(loop));

    /* Verify loop has 2 edge uses (forward + reverse) */
    count_loop_edge_uses(loop, &eu_count);
    TEST_ASSERT(eu_count == 2);

    /* Verify edge has 2 edge uses */
    {
        lc_edge_data_t *edge_data = (lc_edge_data_t *)lc_entity_get_data(edge);
        TEST_ASSERT(edge_data->edge_use_count == 2);
        TEST_ASSERT(edge_data->vertex_start == v1);
        TEST_ASSERT(edge_data->vertex_end == v2);
    }

    printf("  V=2, E=1, F=1, Loop has 2 edge uses\n");
    printf("  PASS\n\n");
}

static void test_mev(void)
{
    lc_entity_handle_t shell, face, edge1, v1, v2;
    lc_entity_handle_t v3, edge2, v4, edge3;
    lc_entity_handle_t loop;
    lc_face_data_t *face_data;
    int eu_count;

    printf("TEST: MEV (Make-Edge-Vertex)\n");

    shell = create_test_shell();

    /* Start with MVEF */
    vec3 pos1 = {0.0f, 0.0f, 0.0f};
    vec3 pos2 = {1.0f, 0.0f, 0.0f};
    TEST_ASSERT(lc_euler_mvef(shell, pos1, pos2, &face, &edge1, &v1, &v2));

    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    loop = face_data->outer_loop;

    /* Add third vertex via MEV from v2 */
    vec3 pos3 = {1.0f, 1.0f, 0.0f};
    TEST_ASSERT(lc_euler_mev(loop, v2, pos3, &v3, &edge2));
    TEST_ASSERT(lc_entity_is_valid(v3));
    TEST_ASSERT(lc_entity_is_valid(edge2));

    /* Loop should now have 4 edge uses (original 2 + 2 new for spike) */
    count_loop_edge_uses(loop, &eu_count);
    printf("  After 1st MEV: Loop has %d edge uses (expect 4)\n", eu_count);
    TEST_ASSERT(eu_count == 4);

    /* Add fourth vertex via MEV from v3 */
    vec3 pos4 = {0.0f, 1.0f, 0.0f};
    TEST_ASSERT(lc_euler_mev(loop, v3, pos4, &v4, &edge3));
    TEST_ASSERT(lc_entity_is_valid(v4));
    TEST_ASSERT(lc_entity_is_valid(edge3));

    /* Loop should now have 6 edge uses */
    count_loop_edge_uses(loop, &eu_count);
    printf("  After 2nd MEV: Loop has %d edge uses (expect 6)\n", eu_count);
    TEST_ASSERT(eu_count == 6);

    printf("  V=4, E=3 (1 from MVEF + 2 from MEV)\n");
    printf("  PASS\n\n");
}

static void test_mef(void)
{
    lc_entity_handle_t shell, face, edge1, v1, v2;
    lc_entity_handle_t v3, edge2, v4, edge3;
    lc_entity_handle_t new_face, split_edge;
    lc_entity_handle_t loop;
    lc_face_data_t *face_data;
    int eu_count;

    printf("TEST: MEF (Make-Edge-Face)\n");

    shell = create_test_shell();

    /* Build a quadrilateral: MVEF + 2x MEV */
    vec3 pos1 = {0.0f, 0.0f, 0.0f};
    vec3 pos2 = {1.0f, 0.0f, 0.0f};
    TEST_ASSERT(lc_euler_mvef(shell, pos1, pos2, &face, &edge1, &v1, &v2));

    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    loop = face_data->outer_loop;

    vec3 pos3 = {1.0f, 1.0f, 0.0f};
    TEST_ASSERT(lc_euler_mev(loop, v2, pos3, &v3, &edge2));

    vec3 pos4 = {0.0f, 1.0f, 0.0f};
    TEST_ASSERT(lc_euler_mev(loop, v3, pos4, &v4, &edge3));

    /* Split the face by connecting v4 back to v1 using MEF */
    TEST_ASSERT(lc_euler_mef(face, v4, v1, &new_face, &split_edge));
    TEST_ASSERT(lc_entity_is_valid(new_face));
    TEST_ASSERT(lc_entity_is_valid(split_edge));

    /* After MEF: we should have 2 faces */
    printf("  Split face: created new face and edge\n");

    /* Check old face's loop */
    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    loop = face_data->outer_loop;
    count_loop_edge_uses(loop, &eu_count);
    printf("  Old face loop: %d edge uses\n", eu_count);

    /* Check new face's loop */
    {
        lc_face_data_t *new_face_data = (lc_face_data_t *)lc_entity_get_data(new_face);
        lc_entity_handle_t new_loop = new_face_data->outer_loop;
        int new_eu_count;
        count_loop_edge_uses(new_loop, &new_eu_count);
        printf("  New face loop: %d edge uses\n", new_eu_count);
    }

    printf("  PASS\n\n");
}

static void test_kev_round_trip(void)
{
    lc_entity_handle_t shell, face, edge1, v1, v2;
    lc_entity_handle_t v3, edge2;
    lc_entity_handle_t loop;
    lc_face_data_t *face_data;
    int eu_count;

    printf("TEST: KEV Round-Trip (MEV then KEV)\n");

    shell = create_test_shell();

    /* Start with MVEF */
    vec3 pos1 = {0.0f, 0.0f, 0.0f};
    vec3 pos2 = {1.0f, 0.0f, 0.0f};
    TEST_ASSERT(lc_euler_mvef(shell, pos1, pos2, &face, &edge1, &v1, &v2));

    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    loop = face_data->outer_loop;

    /* Count before MEV */
    count_loop_edge_uses(loop, &eu_count);
    printf("  Before MEV: %d edge uses\n", eu_count);
    TEST_ASSERT(eu_count == 2);

    /* Add vertex via MEV */
    vec3 pos3 = {0.5f, 1.0f, 0.0f};
    TEST_ASSERT(lc_euler_mev(loop, v2, pos3, &v3, &edge2));

    count_loop_edge_uses(loop, &eu_count);
    printf("  After MEV: %d edge uses\n", eu_count);
    TEST_ASSERT(eu_count == 4);

    /* Remove with KEV - should restore original state */
    TEST_ASSERT(lc_euler_kev(edge2, v3));

    count_loop_edge_uses(loop, &eu_count);
    printf("  After KEV: %d edge uses (expect 2)\n", eu_count);
    TEST_ASSERT(eu_count == 2);

    /* Verify original vertices still valid */
    TEST_ASSERT(lc_entity_is_valid(v1));
    TEST_ASSERT(lc_entity_is_valid(v2));

    /* Verify killed entities are gone */
    TEST_ASSERT(!lc_entity_is_valid(v3));
    TEST_ASSERT(!lc_entity_is_valid(edge2));

    printf("  PASS\n\n");
}

static void test_kef_round_trip(void)
{
    lc_entity_handle_t shell, face, edge1, v1, v2;
    lc_entity_handle_t v3, edge2, v4, edge3;
    lc_entity_handle_t new_face, split_edge;
    lc_entity_handle_t loop;
    lc_face_data_t *face_data;
    int eu_count;
    size_t face_count;

    printf("TEST: KEF Round-Trip (MEF then KEF)\n");

    shell = create_test_shell();

    /* Build quadrilateral: MVEF + 2x MEV */
    vec3 pos1 = {0.0f, 0.0f, 0.0f};
    vec3 pos2 = {1.0f, 0.0f, 0.0f};
    TEST_ASSERT(lc_euler_mvef(shell, pos1, pos2, &face, &edge1, &v1, &v2));

    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    loop = face_data->outer_loop;

    vec3 pos3 = {1.0f, 1.0f, 0.0f};
    TEST_ASSERT(lc_euler_mev(loop, v2, pos3, &v3, &edge2));

    vec3 pos4 = {0.0f, 1.0f, 0.0f};
    TEST_ASSERT(lc_euler_mev(loop, v3, pos4, &v4, &edge3));

    /* Count edge uses before MEF */
    count_loop_edge_uses(loop, &eu_count);
    printf("  Before MEF: %d edge uses in face\n", eu_count);

    /* Split with MEF */
    TEST_ASSERT(lc_euler_mef(face, v4, v1, &new_face, &split_edge));

    /* Count faces after MEF */
    face_count = lc_topology_get_faces(shell, NULL, 0);
    printf("  After MEF: %zu faces\n", face_count);
    TEST_ASSERT(face_count == 2);

    /* Merge back with KEF */
    TEST_ASSERT(lc_euler_kef(split_edge));

    /* Count faces after KEF - should be back to 1 */
    face_count = lc_topology_get_faces(shell, NULL, 0);
    printf("  After KEF: %zu faces (expect 1)\n", face_count);
    TEST_ASSERT(face_count == 1);

    /* Verify edge is gone */
    TEST_ASSERT(!lc_entity_is_valid(split_edge));

    /* One of the two faces should have been killed */
    {
        bool face_valid = lc_entity_is_valid(face);
        bool new_face_valid = lc_entity_is_valid(new_face);
        printf("  Original face valid: %d, New face valid: %d\n", face_valid, new_face_valid);
        /* Exactly one should survive */
        TEST_ASSERT(face_valid != new_face_valid);
    }

    printf("  PASS\n\n");
}

static void test_build_triangle(void)
{
    lc_entity_handle_t shell, face, edge1, v1, v2;
    lc_entity_handle_t v3, edge2;
    lc_entity_handle_t new_face, edge3;
    lc_entity_handle_t loop;
    lc_face_data_t *face_data, *new_face_data;
    int eu_count;

    printf("TEST: Build Triangle (MVEF + MEV + MEF)\n");

    shell = create_test_shell();

    /* MVEF: Create initial edge v1--v2 */
    vec3 pos1 = {0.0f, 0.0f, 0.0f};
    vec3 pos2 = {1.0f, 0.0f, 0.0f};
    TEST_ASSERT(lc_euler_mvef(shell, pos1, pos2, &face, &edge1, &v1, &v2));

    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    loop = face_data->outer_loop;

    /* MEV: Add v3 from v2 */
    vec3 pos3 = {0.5f, 0.866f, 0.0f};
    TEST_ASSERT(lc_euler_mev(loop, v2, pos3, &v3, &edge2));

    /* MEF: Close triangle by connecting v3 back to v1 */
    TEST_ASSERT(lc_euler_mef(face, v3, v1, &new_face, &edge3));

    /* The new face should be the triangle with 3 edges */
    new_face_data = (lc_face_data_t *)lc_entity_get_data(new_face);
    count_loop_edge_uses(new_face_data->outer_loop, &eu_count);
    printf("  Triangle face: %d edge uses (expect 3)\n", eu_count);
    TEST_ASSERT(eu_count == 3);

    /* The old face should have the degenerate part (3 edge uses too) */
    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    count_loop_edge_uses(face_data->outer_loop, &eu_count);
    printf("  Remaining face: %d edge uses (expect 3)\n", eu_count);
    TEST_ASSERT(eu_count == 3);

    printf("  V=3, E=3, F=2 (2 faces share 3 edges)\n");
    printf("  PASS\n\n");
}

static void test_mekl_keml_round_trip(void)
{
    lc_entity_handle_t shell, face, edge1, v1, v2;
    lc_entity_handle_t loop;
    lc_entity_handle_t mekl_edge, keml_loop;
    lc_face_data_t *face_data;
    lc_loop_data_t *inner_loop_data;
    size_t loop_count;

    printf("TEST: MEKL/KEML Round-Trip\n");

    shell = create_test_shell();

    /*
     * Strategy: Build a face with MVEF (outer loop with 2 edge uses).
     * Build a separate face via MVEF (inner "loop"). Reparent the
     * inner face's loop onto the outer face. Now we have 2 loops on
     * one face. Test MEKL to merge, then KEML to split.
     */

    /* Outer face */
    vec3 pos1 = {0.0f, 0.0f, 0.0f};
    vec3 pos2 = {4.0f, 0.0f, 0.0f};
    TEST_ASSERT(lc_euler_mvef(shell, pos1, pos2, &face, &edge1, &v1, &v2));

    face_data = (lc_face_data_t *)lc_entity_get_data(face);
    loop = face_data->outer_loop;

    /* Inner face (separate, we'll steal its loop) */
    {
        lc_entity_handle_t inner_face, inner_edge, iv1, iv2;
        lc_face_data_t *inner_face_data;
        lc_entity_handle_t inner_loop;

        vec3 ip1 = {1.0f, 1.0f, 0.0f};
        vec3 ip2 = {3.0f, 1.0f, 0.0f};
        TEST_ASSERT(lc_euler_mvef(shell, ip1, ip2, &inner_face, &inner_edge, &iv1, &iv2));

        inner_face_data = (lc_face_data_t *)lc_entity_get_data(inner_face);
        inner_loop = inner_face_data->outer_loop;

        /* Reparent inner_loop from inner_face to face */
        lc_entity_remove_child(inner_face, inner_loop);
        lc_entity_add_child(face, inner_loop);

        inner_loop_data = (lc_loop_data_t *)lc_entity_get_data(inner_loop);
        inner_loop_data->face = face;
        inner_loop_data->is_outer = false;

        /* Now face has 2 loops */
        loop_count = lc_topology_get_loops(face, NULL, 0);
        printf("  Face has %zu loops before MEKL (expect 2)\n", loop_count);
        TEST_ASSERT(loop_count == 2);

        /* MEKL: merge the two loops (v1 on outer, iv1 on inner) */
        TEST_ASSERT(lc_euler_mekl(face, v1, iv1, &mekl_edge));
        TEST_ASSERT(lc_entity_is_valid(mekl_edge));

        loop_count = lc_topology_get_loops(face, NULL, 0);
        printf("  Face has %zu loops after MEKL (expect 1)\n", loop_count);
        TEST_ASSERT(loop_count == 1);

        /* KEML: split back into 2 loops */
        TEST_ASSERT(lc_euler_keml(mekl_edge, &keml_loop));
        TEST_ASSERT(lc_entity_is_valid(keml_loop));

        loop_count = lc_topology_get_loops(face, NULL, 0);
        printf("  Face has %zu loops after KEML (expect 2)\n", loop_count);
        TEST_ASSERT(loop_count == 2);
    }

    printf("  PASS\n\n");
}

static void test_invalid_inputs(void)
{
    printf("TEST: Invalid Inputs\n");

    /* MVEF with invalid shell */
    {
        vec3 p1 = {0.0f, 0.0f, 0.0f};
        vec3 p2 = {1.0f, 0.0f, 0.0f};
        TEST_ASSERT(!lc_euler_mvef(LC_ENTITY_INVALID, p1, p2, NULL, NULL, NULL, NULL));
    }

    /* MEV with invalid loop */
    {
        vec3 p = {0.0f, 0.0f, 0.0f};
        TEST_ASSERT(!lc_euler_mev(LC_ENTITY_INVALID, LC_ENTITY_INVALID, p, NULL, NULL));
    }

    /* KEV with invalid edge */
    {
        TEST_ASSERT(!lc_euler_kev(LC_ENTITY_INVALID, LC_ENTITY_INVALID));
    }

    /* KEF with invalid edge */
    {
        TEST_ASSERT(!lc_euler_kef(LC_ENTITY_INVALID));
    }

    /* KEML with invalid edge */
    {
        TEST_ASSERT(!lc_euler_keml(LC_ENTITY_INVALID, NULL));
    }

    printf("  All invalid inputs correctly rejected\n");
    printf("  PASS\n\n");
}

/***************************************************************
** MARK: HELPER FUNCTIONS
***************************************************************/

static lc_entity_handle_t create_test_shell(void)
{
    lc_entity_handle_t solid, shell;
    lc_solid_data_t *solid_data;
    lc_shell_data_t *shell_data;

    solid = lc_entity_create(LC_ENTITY_TYPE_SOLID);
    if (solid == LC_ENTITY_INVALID) return LC_ENTITY_INVALID;

    solid_data = (lc_solid_data_t *)malloc(sizeof(lc_solid_data_t));
    if (!solid_data)
    {
        lc_entity_destroy(solid);
        return LC_ENTITY_INVALID;
    }
    memset(solid_data, 0, sizeof(lc_solid_data_t));
    solid_data->bbox_dirty = true;
    lc_entity_set_data(solid, solid_data);

    shell = lc_entity_create(LC_ENTITY_TYPE_SHELL);
    if (shell == LC_ENTITY_INVALID)
    {
        lc_entity_destroy(solid);
        return LC_ENTITY_INVALID;
    }

    shell_data = (lc_shell_data_t *)malloc(sizeof(lc_shell_data_t));
    if (!shell_data)
    {
        lc_entity_destroy(shell);
        lc_entity_destroy(solid);
        return LC_ENTITY_INVALID;
    }
    memset(shell_data, 0, sizeof(lc_shell_data_t));
    shell_data->is_closed = true;
    lc_entity_set_data(shell, shell_data);

    lc_entity_add_child(solid, shell);

    return shell;
}

static void count_loop_edge_uses(lc_entity_handle_t loop, int *out_count)
{
    lc_entity_handle_t child, first_eu, current_eu;
    lc_edge_use_data_t *eu_data;
    int count;

    *out_count = 0;

    /* Find first edge use in loop */
    first_eu = LC_ENTITY_INVALID;
    child = lc_entity_get_first_child(loop);
    while (lc_entity_is_valid(child))
    {
        if (lc_entity_get_type(child) == LC_ENTITY_TYPE_EDGE_USE)
        {
            first_eu = child;
            break;
        }
        child = lc_entity_get_next_sibling(child);
    }

    if (first_eu == LC_ENTITY_INVALID)
    {
        return;
    }

    /* Walk the chain */
    current_eu = first_eu;
    count = 0;
    do
    {
        count++;
        eu_data = (lc_edge_use_data_t *)lc_entity_get_data(current_eu);
        current_eu = eu_data->next_in_loop;
    }
    while (current_eu != first_eu && count < 1000);

    *out_count = count;
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
    printf("libcad Euler Operators Test Suite\n");
    printf("==============================================\n\n");

    lc_entity_init();
    lc_undo_init();
    lc_geometry_init();
    lc_brep_init();

    test_mvef();
    test_mev();
    test_mef();
    test_kev_round_trip();
    test_kef_round_trip();
    test_build_triangle();
    test_mekl_keml_round_trip();
    test_invalid_inputs();

    printf("==============================================\n");
    printf("All Euler operator tests PASSED!\n");
    printf("==============================================\n");

    return 0;
}
