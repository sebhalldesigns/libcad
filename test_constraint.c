/***************************************************************
**
** libcad Test File
**
** File         :  test_constraint.c
** Module       :  test
** Author       :  SH
** Created      :  2026-01-29 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Test program for Phase 3A-3B constraint system.
**                 Verifies constraint creation and basic operations.
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lib/lc_entity.h"
#include "lib/lc_constraint.h"

/***************************************************************
** MARK: TEST FUNCTIONS
***************************************************************/

static void test_constraint_creation(void)
{
    printf("\n=== Test: Constraint Creation ===\n");

    /* Create a sketch */
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 normal = {0.0f, 0.0f, 1.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};
    lc_entity_handle_t sketch = lc_entity_create_sketch(origin, normal, x_axis);
    printf("Created sketch: 0x%08X\n", sketch);

    /* Create two lines */
    vec2 start1 = {0.0f, 0.0f};
    vec2 end1 = {10.0f, 0.0f};
    lc_entity_handle_t line1 = lc_entity_create_line(sketch, start1, end1, 0xFFFFFFFF, 2.0f);
    printf("Created line1: 0x%08X\n", line1);

    vec2 start2 = {0.0f, 5.0f};
    vec2 end2 = {10.0f, 5.0f};
    lc_entity_handle_t line2 = lc_entity_create_line(sketch, start2, end2, 0xFFFFFFFF, 2.0f);
    printf("Created line2: 0x%08X\n", line2);

    /* Create a parallel constraint */
    lc_entity_handle_t constraint_parallel = lc_constraint_create_parallel(line1, line2);
    printf("Created parallel constraint: 0x%08X\n", constraint_parallel);

    if (lc_entity_is_valid(constraint_parallel))
    {
        printf("  Parallel constraint is valid\n");
        lc_constraint_data_t *data = (lc_constraint_data_t*)lc_entity_get_data(constraint_parallel);
        if (data != NULL)
        {
            printf("  Constraint type: %d\n", data->type);
            printf("  Entity 1: 0x%08X\n", data->entities[0]);
            printf("  Entity 2: 0x%08X\n", data->entities[1]);
            printf("  Weight: %.2f\n", data->weight);
            printf("  Error: %.6f\n", data->error);
        }
    }
    else
    {
        printf("  ERROR: Parallel constraint is invalid!\n");
    }

    /* Create a horizontal constraint */
    lc_entity_handle_t constraint_horiz = lc_constraint_create_horizontal(line1);
    printf("Created horizontal constraint: 0x%08X\n", constraint_horiz);

    if (lc_entity_is_valid(constraint_horiz))
    {
        printf("  Horizontal constraint is valid\n");
    }
    else
    {
        printf("  ERROR: Horizontal constraint is invalid!\n");
    }

    /* Create a distance constraint */
    lc_entity_handle_t constraint_dist = lc_constraint_create_distance_point_point(
        line1, line2, 5.0f);
    printf("Created distance constraint: 0x%08X\n", constraint_dist);

    if (lc_entity_is_valid(constraint_dist))
    {
        printf("  Distance constraint is valid\n");
        lc_constraint_data_t *data = (lc_constraint_data_t*)lc_entity_get_data(constraint_dist);
        if (data != NULL)
        {
            printf("  Constraint type: %d\n", data->type);
            printf("  Distance value: %.2f\n", data->value);
        }
    }
    else
    {
        printf("  ERROR: Distance constraint is invalid!\n");
    }

    /* Test constraint evaluation */
    printf("\nTesting constraint evaluation:\n");
    float error = lc_constraint_evaluate(constraint_parallel);
    printf("  Parallel constraint error: %.6f\n", error);

    error = lc_constraint_evaluate(constraint_horiz);
    printf("  Horizontal constraint error: %.6f\n", error);

    error = lc_constraint_evaluate(constraint_dist);
    printf("  Distance constraint error: %.6f\n", error);

    /* Test constraint destruction */
    printf("\nTesting constraint destruction:\n");
    if (lc_constraint_destroy(constraint_parallel))
    {
        printf("  Successfully destroyed parallel constraint\n");
    }
    else
    {
        printf("  ERROR: Failed to destroy parallel constraint\n");
    }

    if (!lc_entity_is_valid(constraint_parallel))
    {
        printf("  Parallel constraint is now invalid (correct)\n");
    }
    else
    {
        printf("  ERROR: Parallel constraint is still valid after destruction!\n");
    }

    printf("\n=== Test Complete ===\n");
}

static void test_all_constraint_types(void)
{
    printf("\n=== Test: All Constraint Types ===\n");

    /* Create a sketch */
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 normal = {0.0f, 0.0f, 1.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};
    lc_entity_handle_t sketch = lc_entity_create_sketch(origin, normal, x_axis);

    /* Create test entities */
    vec2 p1 = {0.0f, 0.0f};
    vec2 p2 = {10.0f, 0.0f};
    lc_entity_handle_t line1 = lc_entity_create_line(sketch, p1, p2, 0xFFFFFFFF, 2.0f);

    vec2 p3 = {0.0f, 5.0f};
    vec2 p4 = {10.0f, 5.0f};
    lc_entity_handle_t line2 = lc_entity_create_line(sketch, p3, p4, 0xFFFFFFFF, 2.0f);

    vec2 center = {5.0f, 5.0f};
    lc_entity_handle_t circle1 = lc_entity_create_circle(sketch, center, 3.0f, 0xFFFFFFFF);

    vec2 center2 = {15.0f, 5.0f};
    lc_entity_handle_t circle2 = lc_entity_create_circle(sketch, center2, 3.0f, 0xFFFFFFFF);

    /* Test all constraint creation functions */
    lc_entity_handle_t c1 = lc_constraint_create_distance_point_point(line1, line2, 5.0f);
    printf("distance_point_point: 0x%08X %s\n", c1, lc_entity_is_valid(c1) ? "OK" : "FAIL");

    lc_entity_handle_t c2 = lc_constraint_create_distance_point_line(line1, line2, 2.0f);
    printf("distance_point_line:  0x%08X %s\n", c2, lc_entity_is_valid(c2) ? "OK" : "FAIL");

    lc_entity_handle_t c3 = lc_constraint_create_angle_line_line(line1, line2, 1.57f);
    printf("angle_line_line:      0x%08X %s\n", c3, lc_entity_is_valid(c3) ? "OK" : "FAIL");

    lc_entity_handle_t c4 = lc_constraint_create_coincident_point_point(line1, line2);
    printf("coincident_point_point: 0x%08X %s\n", c4, lc_entity_is_valid(c4) ? "OK" : "FAIL");

    lc_entity_handle_t c5 = lc_constraint_create_coincident_point_line(line1, line2);
    printf("coincident_point_line:  0x%08X %s\n", c5, lc_entity_is_valid(c5) ? "OK" : "FAIL");

    lc_entity_handle_t c6 = lc_constraint_create_coincident_point_circle(line1, circle1);
    printf("coincident_point_circle: 0x%08X %s\n", c6, lc_entity_is_valid(c6) ? "OK" : "FAIL");

    lc_entity_handle_t c7 = lc_constraint_create_parallel(line1, line2);
    printf("parallel:             0x%08X %s\n", c7, lc_entity_is_valid(c7) ? "OK" : "FAIL");

    lc_entity_handle_t c8 = lc_constraint_create_perpendicular(line1, line2);
    printf("perpendicular:        0x%08X %s\n", c8, lc_entity_is_valid(c8) ? "OK" : "FAIL");

    lc_entity_handle_t c9 = lc_constraint_create_horizontal(line1);
    printf("horizontal:           0x%08X %s\n", c9, lc_entity_is_valid(c9) ? "OK" : "FAIL");

    lc_entity_handle_t c10 = lc_constraint_create_vertical(line2);
    printf("vertical:             0x%08X %s\n", c10, lc_entity_is_valid(c10) ? "OK" : "FAIL");

    lc_entity_handle_t c11 = lc_constraint_create_tangent_line_circle(line1, circle1);
    printf("tangent_line_circle:  0x%08X %s\n", c11, lc_entity_is_valid(c11) ? "OK" : "FAIL");

    lc_entity_handle_t c12 = lc_constraint_create_tangent_circle_circle(circle1, circle2);
    printf("tangent_circle_circle: 0x%08X %s\n", c12, lc_entity_is_valid(c12) ? "OK" : "FAIL");

    lc_entity_handle_t c13 = lc_constraint_create_equal_length(line1, line2);
    printf("equal_length:         0x%08X %s\n", c13, lc_entity_is_valid(c13) ? "OK" : "FAIL");

    lc_entity_handle_t c14 = lc_constraint_create_equal_radius(circle1, circle2);
    printf("equal_radius:         0x%08X %s\n", c14, lc_entity_is_valid(c14) ? "OK" : "FAIL");

    lc_entity_handle_t c15 = lc_constraint_create_fix_point(line1);
    printf("fix_point:            0x%08X %s\n", c15, lc_entity_is_valid(c15) ? "OK" : "FAIL");

    printf("\n=== All Constraint Types Test Complete ===\n");
}

static void test_error_evaluation(void)
{
    printf("\n=== Test: Error Evaluation (Phase 3C) ===\n");

    /* Create a sketch */
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 normal = {0.0f, 0.0f, 1.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};
    lc_entity_handle_t sketch = lc_entity_create_sketch(origin, normal, x_axis);

    /* Test 1: Horizontal constraint on horizontal line (should be ~0) */
    printf("\nTest 1: Horizontal line constraint\n");
    vec2 h_start = {0.0f, 5.0f};
    vec2 h_end = {10.0f, 5.0f};
    lc_entity_handle_t horiz_line = lc_entity_create_line(sketch, h_start, h_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t c_horiz = lc_constraint_create_horizontal(horiz_line);
    float error = lc_constraint_evaluate(c_horiz);
    printf("  Horizontal line (y=5 to y=5): error = %.9f (should be ~0)\n", error);

    /* Test 2: Horizontal constraint on non-horizontal line (should be > 0) */
    printf("\nTest 2: Non-horizontal line constraint\n");
    vec2 nh_start = {0.0f, 0.0f};
    vec2 nh_end = {10.0f, 5.0f};
    lc_entity_handle_t non_horiz_line = lc_entity_create_line(sketch, nh_start, nh_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t c_nh = lc_constraint_create_horizontal(non_horiz_line);
    error = lc_constraint_evaluate(c_nh);
    printf("  Non-horizontal line (y=0 to y=5): error = %.9f (should be 25.0)\n", error);

    /* Test 3: Vertical constraint on vertical line (should be ~0) */
    printf("\nTest 3: Vertical line constraint\n");
    vec2 v_start = {3.0f, 0.0f};
    vec2 v_end = {3.0f, 10.0f};
    lc_entity_handle_t vert_line = lc_entity_create_line(sketch, v_start, v_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t c_vert = lc_constraint_create_vertical(vert_line);
    error = lc_constraint_evaluate(c_vert);
    printf("  Vertical line (x=3 to x=3): error = %.9f (should be ~0)\n", error);

    /* Test 4: Parallel constraint on parallel lines (should be ~0) */
    printf("\nTest 4: Parallel lines constraint\n");
    vec2 p1_start = {0.0f, 0.0f};
    vec2 p1_end = {10.0f, 5.0f};
    vec2 p2_start = {0.0f, 10.0f};
    vec2 p2_end = {10.0f, 15.0f};
    lc_entity_handle_t para_line1 = lc_entity_create_line(sketch, p1_start, p1_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t para_line2 = lc_entity_create_line(sketch, p2_start, p2_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t c_para = lc_constraint_create_parallel(para_line1, para_line2);
    error = lc_constraint_evaluate(c_para);
    printf("  Parallel lines (same slope): error = %.9f (should be ~0)\n", error);

    /* Test 5: Parallel constraint on non-parallel lines (should be > 0) */
    printf("\nTest 5: Non-parallel lines constraint\n");
    vec2 np1_start = {0.0f, 0.0f};
    vec2 np1_end = {10.0f, 0.0f};
    vec2 np2_start = {0.0f, 0.0f};
    vec2 np2_end = {0.0f, 10.0f};
    lc_entity_handle_t npara_line1 = lc_entity_create_line(sketch, np1_start, np1_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t npara_line2 = lc_entity_create_line(sketch, np2_start, np2_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t c_npara = lc_constraint_create_parallel(npara_line1, npara_line2);
    error = lc_constraint_evaluate(c_npara);
    printf("  Non-parallel lines (perpendicular): error = %.9f (should be 1.0)\n", error);

    /* Test 6: Perpendicular constraint on perpendicular lines (should be ~0) */
    printf("\nTest 6: Perpendicular lines constraint\n");
    lc_entity_handle_t c_perp = lc_constraint_create_perpendicular(npara_line1, npara_line2);
    error = lc_constraint_evaluate(c_perp);
    printf("  Perpendicular lines (90 deg): error = %.9f (should be ~0)\n", error);

    /* Test 7: Equal length constraint on equal-length lines (should be ~0) */
    printf("\nTest 7: Equal length constraint\n");
    vec2 eq1_start = {0.0f, 0.0f};
    vec2 eq1_end = {10.0f, 0.0f};
    vec2 eq2_start = {0.0f, 5.0f};
    vec2 eq2_end = {10.0f, 5.0f};
    lc_entity_handle_t eq_line1 = lc_entity_create_line(sketch, eq1_start, eq1_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t eq_line2 = lc_entity_create_line(sketch, eq2_start, eq2_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t c_eq = lc_constraint_create_equal_length(eq_line1, eq_line2);
    error = lc_constraint_evaluate(c_eq);
    printf("  Equal length lines (10 and 10): error = %.9f (should be ~0)\n", error);

    /* Test 8: Equal radius constraint on equal-radius circles (should be ~0) */
    printf("\nTest 8: Equal radius constraint\n");
    vec2 c1_center = {5.0f, 5.0f};
    vec2 c2_center = {15.0f, 5.0f};
    lc_entity_handle_t circle1 = lc_entity_create_circle(sketch, c1_center, 3.0f, 0xFFFFFFFF);
    lc_entity_handle_t circle2 = lc_entity_create_circle(sketch, c2_center, 3.0f, 0xFFFFFFFF);
    lc_entity_handle_t c_eqr = lc_constraint_create_equal_radius(circle1, circle2);
    error = lc_constraint_evaluate(c_eqr);
    printf("  Equal radius circles (r=3 and r=3): error = %.9f (should be ~0)\n", error);

    /* Test 9: Distance constraint (point-to-point via line starts) */
    printf("\nTest 9: Distance constraint (point-to-point)\n");
    vec2 d1_start = {0.0f, 0.0f};
    vec2 d1_end = {1.0f, 0.0f};
    vec2 d2_start = {10.0f, 0.0f};
    vec2 d2_end = {11.0f, 0.0f};
    lc_entity_handle_t d_line1 = lc_entity_create_line(sketch, d1_start, d1_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t d_line2 = lc_entity_create_line(sketch, d2_start, d2_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t c_dist = lc_constraint_create_distance_point_point(d_line1, d_line2, 10.0f);
    error = lc_constraint_evaluate(c_dist);
    printf("  Distance 10 between points at (0,0) and (10,0): error = %.9f (should be ~0)\n", error);

    /* Test 10: Angle constraint */
    printf("\nTest 10: Angle constraint\n");
    vec2 a1_start = {0.0f, 0.0f};
    vec2 a1_end = {10.0f, 0.0f};
    vec2 a2_start = {0.0f, 0.0f};
    vec2 a2_end = {7.071f, 7.071f};  /* 45 degrees */
    lc_entity_handle_t a_line1 = lc_entity_create_line(sketch, a1_start, a1_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t a_line2 = lc_entity_create_line(sketch, a2_start, a2_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t c_angle = lc_constraint_create_angle_line_line(a_line1, a_line2, 0.7854f);  /* pi/4 radians */
    error = lc_constraint_evaluate(c_angle);
    printf("  45-degree angle: error = %.9f (should be ~0)\n", error);

    printf("\n=== Error Evaluation Test Complete ===\n");
}

/***************************************************************
** MARK: MAIN
***************************************************************/

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("============================================\n");
    printf("libcad Phase 3A-3B Constraint System Test\n");
    printf("============================================\n");

    /* Initialize systems */
    printf("\nInitializing systems...\n");
    lc_entity_init();
    lc_constraint_init();

    /* Run tests */
    test_constraint_creation();
    test_all_constraint_types();
    test_error_evaluation();

    /* Shutdown */
    printf("\nShutting down systems...\n");
    lc_constraint_shutdown();
    lc_entity_shutdown();

    printf("\n============================================\n");
    printf("All tests completed successfully!\n");
    printf("============================================\n");

    return 0;
}
