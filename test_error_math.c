/***************************************************************
**
** Simple Error Function Math Test
** Tests constraint error computations without entity system
**
***************************************************************/

#include <stdio.h>
#include <math.h>

typedef float vec2[2];

/* Test horizontal error */
static void test_horizontal(void)
{
    printf("Test: Horizontal constraint\n");

    /* Horizontal line: dy should be 0 */
    vec2 h_start = {0.0f, 5.0f};
    vec2 h_end = {10.0f, 5.0f};
    float dy = h_end[1] - h_start[1];
    float error = dy * dy;
    printf("  Horizontal line (y=5 to y=5): error = %.9f (expected 0)\n", error);

    /* Non-horizontal line: dy = 5 */
    vec2 nh_start = {0.0f, 0.0f};
    vec2 nh_end = {10.0f, 5.0f};
    dy = nh_end[1] - nh_start[1];
    error = dy * dy;
    printf("  Non-horizontal line (y=0 to y=5): error = %.9f (expected 25)\n", error);
}

/* Test parallel error */
static void test_parallel(void)
{
    printf("\nTest: Parallel constraint\n");

    /* Parallel lines (same slope) */
    vec2 L1_start = {0.0f, 0.0f};
    vec2 L1_end = {10.0f, 5.0f};
    vec2 L2_start = {0.0f, 10.0f};
    vec2 L2_end = {10.0f, 15.0f};

    vec2 dir1 = {L1_end[0] - L1_start[0], L1_end[1] - L1_start[1]};
    vec2 dir2 = {L2_end[0] - L2_start[0], L2_end[1] - L2_start[1]};

    float len1 = sqrtf(dir1[0]*dir1[0] + dir1[1]*dir1[1]);
    float len2 = sqrtf(dir2[0]*dir2[0] + dir2[1]*dir2[1]);

    dir1[0] /= len1; dir1[1] /= len1;
    dir2[0] /= len2; dir2[1] /= len2;

    float cross = dir1[0]*dir2[1] - dir1[1]*dir2[0];
    float error = cross * cross;

    printf("  Parallel lines: error = %.9f (expected ~0)\n", error);

    /* Perpendicular lines */
    vec2 L3_start = {0.0f, 0.0f};
    vec2 L3_end = {10.0f, 0.0f};
    vec2 L4_start = {0.0f, 0.0f};
    vec2 L4_end = {0.0f, 10.0f};

    vec2 dir3 = {L3_end[0] - L3_start[0], L3_end[1] - L3_start[1]};
    vec2 dir4 = {L4_end[0] - L4_start[0], L4_end[1] - L4_start[1]};

    len1 = sqrtf(dir3[0]*dir3[0] + dir3[1]*dir3[1]);
    len2 = sqrtf(dir4[0]*dir4[0] + dir4[1]*dir4[1]);

    dir3[0] /= len1; dir3[1] /= len1;
    dir4[0] /= len2; dir4[1] /= len2;

    cross = dir3[0]*dir4[1] - dir3[1]*dir4[0];
    error = cross * cross;

    printf("  Perpendicular lines: error = %.9f (expected 1.0)\n", error);
}

/* Test distance error */
static void test_distance(void)
{
    printf("\nTest: Distance constraint\n");

    vec2 p1 = {0.0f, 0.0f};
    vec2 p2 = {10.0f, 0.0f};

    float dx = p2[0] - p1[0];
    float dy = p2[1] - p1[1];
    float dist = sqrtf(dx*dx + dy*dy);

    float target = 10.0f;
    float err = dist - target;
    float error = err * err;

    printf("  Distance 10 between (0,0) and (10,0): error = %.9f (expected 0)\n", error);

    target = 5.0f;
    err = dist - target;
    error = err * err;

    printf("  Distance 5 between (0,0) and (10,0): error = %.9f (expected 25)\n", error);
}

int main(void)
{
    printf("=== Constraint Error Function Math Tests ===\n\n");

    test_horizontal();
    test_parallel();
    test_distance();

    printf("\n=== Tests Complete ===\n");
    return 0;
}
