/***************************************************************
**
** Constraint Graph and DOF Analysis Test
** Tests Phase 3D implementation
**
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lib/lc_entity.h"
#include "lib/lc_constraint.h"

int main(void)
{
    printf("=== Constraint Graph & DOF Analysis Tests ===\n\n");

    /* Initialize systems */
    lc_entity_init();
    lc_constraint_init();

    /* Test 1: Empty sketch (should have 0 DOF) */
    printf("Test 1: Empty sketch\n");
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 normal = {0.0f, 0.0f, 1.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};
    lc_entity_handle_t sketch1 = lc_entity_create_sketch(origin, normal, x_axis);

    int dof = lc_constraint_get_dof(sketch1);
    printf("  Empty sketch DOF: %d (expected 0)\n", dof);
    printf("  Fully constrained: %s (expected true)\n\n",
           lc_constraint_is_fully_constrained(sketch1) ? "true" : "false");

    /* Test 2: Single line (4 DOF, unconstrained) */
    printf("Test 2: Single unconstrained line\n");
    lc_entity_handle_t sketch2 = lc_entity_create_sketch(origin, normal, x_axis);
    vec2 start = {0.0f, 0.0f};
    vec2 end = {10.0f, 0.0f};
    lc_entity_handle_t line1 = lc_entity_create_line(sketch2, start, end, 0xFFFFFFFF, 2.0f);

    dof = lc_constraint_get_dof(sketch2);
    printf("  Single line DOF: %d (expected 4)\n", dof);
    printf("  Fully constrained: %s (expected false)\n\n",
           lc_constraint_is_fully_constrained(sketch2) ? "true" : "false");

    /* Test 3: Line with horizontal constraint (3 DOF remaining) */
    printf("Test 3: Horizontal line\n");
    lc_entity_handle_t sketch3 = lc_entity_create_sketch(origin, normal, x_axis);
    vec2 h_start = {0.0f, 5.0f};
    vec2 h_end = {10.0f, 5.0f};
    lc_entity_handle_t line2 = lc_entity_create_line(sketch3, h_start, h_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t c_horiz = lc_constraint_create_horizontal(line2);

    dof = lc_constraint_get_dof(sketch3);
    printf("  Horizontal line DOF: %d (expected 3 = 4 - 1)\n", dof);
    printf("  Fully constrained: %s (expected false)\n\n",
           lc_constraint_is_fully_constrained(sketch3) ? "true" : "false");

    /* Test 4: Two parallel lines (8 DOF - 1 = 7 DOF) */
    printf("Test 4: Two parallel lines\n");
    lc_entity_handle_t sketch4 = lc_entity_create_sketch(origin, normal, x_axis);
    vec2 p1_start = {0.0f, 0.0f};
    vec2 p1_end = {10.0f, 0.0f};
    vec2 p2_start = {0.0f, 5.0f};
    vec2 p2_end = {10.0f, 5.0f};
    lc_entity_handle_t para1 = lc_entity_create_line(sketch4, p1_start, p1_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t para2 = lc_entity_create_line(sketch4, p2_start, p2_end, 0xFFFFFFFF, 2.0f);
    lc_entity_handle_t c_para = lc_constraint_create_parallel(para1, para2);

    dof = lc_constraint_get_dof(sketch4);
    printf("  Two parallel lines DOF: %d (expected 7 = 8 - 1)\n", dof);
    printf("  Fully constrained: %s (expected false)\n\n",
           lc_constraint_is_fully_constrained(sketch4) ? "true" : "false");

    /* Test 5: Single circle (3 DOF) */
    printf("Test 5: Single circle\n");
    lc_entity_handle_t sketch5 = lc_entity_create_sketch(origin, normal, x_axis);
    vec2 center = {5.0f, 5.0f};
    lc_entity_handle_t circle1 = lc_entity_create_circle(sketch5, center, 3.0f, 0xFFFFFFFF);

    dof = lc_constraint_get_dof(sketch5);
    printf("  Single circle DOF: %d (expected 3)\n", dof);
    printf("  Fully constrained: %s (expected false)\n\n",
           lc_constraint_is_fully_constrained(sketch5) ? "true" : "false");

    /* Test 6: Two circles with equal radius (6 DOF - 1 = 5 DOF) */
    printf("Test 6: Two circles with equal radius constraint\n");
    lc_entity_handle_t sketch6 = lc_entity_create_sketch(origin, normal, x_axis);
    vec2 c1_center = {5.0f, 5.0f};
    vec2 c2_center = {15.0f, 5.0f};
    lc_entity_handle_t circ1 = lc_entity_create_circle(sketch6, c1_center, 3.0f, 0xFFFFFFFF);
    lc_entity_handle_t circ2 = lc_entity_create_circle(sketch6, c2_center, 3.0f, 0xFFFFFFFF);
    lc_entity_handle_t c_eq_r = lc_constraint_create_equal_radius(circ1, circ2);

    dof = lc_constraint_get_dof(sketch6);
    printf("  Two circles with equal radius DOF: %d (expected 5 = 6 - 1)\n", dof);
    printf("  Fully constrained: %s (expected false)\n\n",
           lc_constraint_is_fully_constrained(sketch6) ? "true" : "false");

    /* Test 7: Graph invalidation */
    printf("Test 7: Graph invalidation\n");
    int dof_before = lc_constraint_get_dof(sketch6);
    printf("  DOF before invalidation: %d\n", dof_before);

    lc_constraint_invalidate_graph(sketch6);
    int dof_after = lc_constraint_get_dof(sketch6);
    printf("  DOF after invalidation + rebuild: %d\n", dof_after);
    printf("  DOF values match: %s (expected true)\n\n",
           dof_before == dof_after ? "true" : "false");

    /* Test 8: Over-constrained system */
    printf("Test 8: Over-constrained line (hypothetical)\n");
    lc_entity_handle_t sketch8 = lc_entity_create_sketch(origin, normal, x_axis);
    vec2 oc_start = {0.0f, 0.0f};
    vec2 oc_end = {10.0f, 5.0f};
    lc_entity_handle_t oc_line = lc_entity_create_line(sketch8, oc_start, oc_end, 0xFFFFFFFF, 2.0f);

    /* Add 5 constraints to a line (line has 4 DOF) */
    lc_constraint_create_horizontal(oc_line);
    lc_constraint_create_vertical(oc_line);
    /* Note: In reality these constraints conflict, but we're testing DOF counting */

    dof = lc_constraint_get_dof(sketch8);
    printf("  Line with 2 conflicting constraints DOF: %d (expected 2 = 4 - 2)\n", dof);
    printf("  Over-constrained: %s\n\n",
           lc_constraint_is_over_constrained(sketch8) ? "true" : "false");

    /* Cleanup */
    lc_constraint_shutdown();
    lc_entity_shutdown();

    printf("=== All Graph Tests Complete ===\n");
    return 0;
}
