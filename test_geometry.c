/***************************************************************
**
** libcad Test File
**
** File         :  test_geometry.c
** Module       :  libcad (geometry system test)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Comprehensive test suite for geometry system. Tests
**                 curve and surface creation, evaluation, and utility
**                 functions.
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lib/lc_geometry.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define TOLERANCE 1e-5f
#define PI 3.14159265359f

/***************************************************************
** MARK: HELPER FUNCTIONS
***************************************************************/

static bool vec3_equals(vec3 a, vec3 b, float tolerance)
{
    return fabsf(a[0] - b[0]) < tolerance &&
           fabsf(a[1] - b[1]) < tolerance &&
           fabsf(a[2] - b[2]) < tolerance;
}

static bool float_equals(float a, float b, float tolerance)
{
    return fabsf(a - b) < tolerance;
}

/***************************************************************
** MARK: TEST FUNCTIONS
***************************************************************/

static void test_line_curve(void)
{
    printf("Test: Line Curve Creation and Evaluation\n");

    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 direction = {1.0f, 0.0f, 0.0f};
    float length = 10.0f;

    lc_curve_handle_t line = lc_geometry_create_line(origin, direction, length);
    assert(line != LC_CURVE_INVALID);

    /* Evaluate at u = 0.0 (should be origin) */
    vec3 p0;
    lc_geometry_eval_curve(line, 0.0f, p0);
    assert(vec3_equals(p0, origin, TOLERANCE));

    /* Evaluate at u = 1.0 (should be origin + direction * length) */
    vec3 p1;
    lc_geometry_eval_curve(line, 1.0f, p1);
    vec3 expected = {10.0f, 0.0f, 0.0f};
    assert(vec3_equals(p1, expected, TOLERANCE));

    /* Evaluate at u = 0.5 (midpoint) */
    vec3 p_mid;
    lc_geometry_eval_curve(line, 0.5f, p_mid);
    vec3 expected_mid = {5.0f, 0.0f, 0.0f};
    assert(vec3_equals(p_mid, expected_mid, TOLERANCE));

    /* Check tangent (should be direction vector) */
    vec3 tangent;
    lc_geometry_eval_curve_tangent(line, 0.5f, tangent);
    assert(vec3_equals(tangent, direction, TOLERANCE));

    /* Check length */
    float curve_length = lc_geometry_curve_length(line);
    assert(float_equals(curve_length, length, TOLERANCE));

    printf("  PASS: Line curve works correctly\n");
}

static void test_circle_curve(void)
{
    printf("Test: Circle Curve Creation and Evaluation\n");

    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};
    vec3 y_axis = {0.0f, 1.0f, 0.0f};
    float radius = 5.0f;
    float u_start = 0.0f;
    float u_end = 2.0f * PI;

    lc_curve_handle_t circle = lc_geometry_create_circle(center, x_axis, y_axis, radius, u_start, u_end);
    assert(circle != LC_CURVE_INVALID);

    /* Evaluate at u = 0.0 (should be center + radius * x_axis) */
    vec3 p0;
    lc_geometry_eval_curve(circle, 0.0f, p0);
    vec3 expected0 = {radius, 0.0f, 0.0f};
    assert(vec3_equals(p0, expected0, TOLERANCE));

    /* Evaluate at u = 0.5 (PI/2 angle, should be center + radius * y_axis) */
    vec3 p_half;
    lc_geometry_eval_curve(circle, 0.5f, p_half);
    vec3 expected_half = {0.0f, radius, 0.0f};
    assert(vec3_equals(p_half, expected_half, TOLERANCE));

    /* Check tangent at u = 0.0 (should point in +y direction) */
    vec3 tangent;
    lc_geometry_eval_curve_tangent(circle, 0.0f, tangent);
    vec3 expected_tangent = {0.0f, 1.0f, 0.0f};
    assert(vec3_equals(tangent, expected_tangent, TOLERANCE));

    /* Check circumference */
    float curve_length = lc_geometry_curve_length(circle);
    float expected_length = 2.0f * PI * radius;
    assert(float_equals(curve_length, expected_length, 0.01f)); /* Slightly larger tolerance for PI */

    printf("  PASS: Circle curve works correctly\n");
}

static void test_ellipse_curve(void)
{
    printf("Test: Ellipse Curve Creation and Evaluation\n");

    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};
    vec3 y_axis = {0.0f, 1.0f, 0.0f};
    float radius_major = 10.0f;
    float radius_minor = 5.0f;
    float u_start = 0.0f;
    float u_end = 2.0f * PI;

    lc_curve_handle_t ellipse = lc_geometry_create_ellipse(center, x_axis, y_axis, radius_major, radius_minor, u_start, u_end);
    assert(ellipse != LC_CURVE_INVALID);

    /* Evaluate at u = 0.0 (should be center + radius_major * x_axis) */
    vec3 p0;
    lc_geometry_eval_curve(ellipse, 0.0f, p0);
    vec3 expected0 = {radius_major, 0.0f, 0.0f};
    assert(vec3_equals(p0, expected0, TOLERANCE));

    /* Evaluate at u = 0.5 (PI/2 angle, should be center + radius_minor * y_axis) */
    vec3 p_half;
    lc_geometry_eval_curve(ellipse, 0.5f, p_half);
    vec3 expected_half = {0.0f, radius_minor, 0.0f};
    assert(vec3_equals(p_half, expected_half, TOLERANCE));

    /* Check that curve length is reasonable (should be between 2*pi*min and 2*pi*max) */
    float curve_length = lc_geometry_curve_length(ellipse);
    float min_length = 2.0f * PI * radius_minor;
    float max_length = 2.0f * PI * radius_major;
    assert(curve_length > min_length && curve_length < max_length);

    printf("  PASS: Ellipse curve works correctly\n");
}

static void test_plane_surface(void)
{
    printf("Test: Plane Surface Creation and Evaluation\n");

    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};
    vec3 y_axis = {0.0f, 1.0f, 0.0f};

    lc_surface_handle_t plane = lc_geometry_create_plane(origin, x_axis, y_axis);
    assert(plane != LC_SURFACE_INVALID);

    /* Evaluate at (0, 0) should give origin */
    vec3 p00;
    lc_geometry_eval_surface(plane, 0.0f, 0.0f, p00);
    assert(vec3_equals(p00, origin, TOLERANCE));

    /* Evaluate at (1, 0) should give origin + x_axis */
    vec3 p10;
    lc_geometry_eval_surface(plane, 1.0f, 0.0f, p10);
    vec3 expected10 = {1.0f, 0.0f, 0.0f};
    assert(vec3_equals(p10, expected10, TOLERANCE));

    /* Evaluate at (0, 1) should give origin + y_axis */
    vec3 p01;
    lc_geometry_eval_surface(plane, 0.0f, 1.0f, p01);
    vec3 expected01 = {0.0f, 1.0f, 0.0f};
    assert(vec3_equals(p01, expected01, TOLERANCE));

    /* Check normal (should be +z direction) */
    vec3 normal;
    lc_geometry_eval_surface_normal(plane, 0.5f, 0.5f, normal);
    vec3 expected_normal = {0.0f, 0.0f, 1.0f};
    assert(vec3_equals(normal, expected_normal, TOLERANCE));

    printf("  PASS: Plane surface works correctly\n");
}

static void test_cylinder_surface(void)
{
    printf("Test: Cylinder Surface Creation and Evaluation\n");

    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 axis = {0.0f, 0.0f, 1.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};
    float radius = 5.0f;

    lc_surface_handle_t cylinder = lc_geometry_create_cylinder(origin, axis, x_axis, radius);
    assert(cylinder != LC_SURFACE_INVALID);

    /* Evaluate at (0, 0) should give origin + radius * x_axis */
    vec3 p00;
    lc_geometry_eval_surface(cylinder, 0.0f, 0.0f, p00);
    vec3 expected00 = {radius, 0.0f, 0.0f};
    assert(vec3_equals(p00, expected00, TOLERANCE));

    /* Evaluate at (PI/2, 0) should give origin + radius * y_axis */
    vec3 p_pi2_0;
    lc_geometry_eval_surface(cylinder, PI / 2.0f, 0.0f, p_pi2_0);
    vec3 expected_pi2_0 = {0.0f, radius, 0.0f};
    assert(vec3_equals(p_pi2_0, expected_pi2_0, TOLERANCE));

    /* Evaluate at (0, 10) should give origin + 10 * axis + radius * x_axis */
    vec3 p0_10;
    lc_geometry_eval_surface(cylinder, 0.0f, 10.0f, p0_10);
    vec3 expected0_10 = {radius, 0.0f, 10.0f};
    assert(vec3_equals(p0_10, expected0_10, TOLERANCE));

    /* Check normal at (0, 0) should point in +x direction */
    vec3 normal;
    lc_geometry_eval_surface_normal(cylinder, 0.0f, 0.0f, normal);
    vec3 expected_normal = {1.0f, 0.0f, 0.0f};
    assert(vec3_equals(normal, expected_normal, TOLERANCE));

    printf("  PASS: Cylinder surface works correctly\n");
}

static void test_sphere_surface(void)
{
    printf("Test: Sphere Surface Creation and Evaluation\n");

    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};
    vec3 y_axis = {0.0f, 1.0f, 0.0f};
    vec3 z_axis = {0.0f, 0.0f, 1.0f};
    float radius = 5.0f;

    lc_surface_handle_t sphere = lc_geometry_create_sphere(center, x_axis, y_axis, z_axis, radius);
    assert(sphere != LC_SURFACE_INVALID);

    /* Evaluate at (0, 0) should give center + radius * x_axis */
    vec3 p00;
    lc_geometry_eval_surface(sphere, 0.0f, 0.0f, p00);
    vec3 expected00 = {radius, 0.0f, 0.0f};
    assert(vec3_equals(p00, expected00, TOLERANCE));

    /* Evaluate at (0, PI/2) should give center + radius * z_axis */
    vec3 p0_pi2;
    lc_geometry_eval_surface(sphere, 0.0f, PI / 2.0f, p0_pi2);
    vec3 expected0_pi2 = {0.0f, 0.0f, radius};
    assert(vec3_equals(p0_pi2, expected0_pi2, TOLERANCE));

    /* All points should be at distance 'radius' from center */
    vec3 test_point;
    lc_geometry_eval_surface(sphere, PI / 4.0f, PI / 6.0f, test_point);
    float distance = sqrtf(test_point[0] * test_point[0] +
                           test_point[1] * test_point[1] +
                           test_point[2] * test_point[2]);
    assert(float_equals(distance, radius, TOLERANCE));

    printf("  PASS: Sphere surface works correctly\n");
}

static void test_cone_surface(void)
{
    printf("Test: Cone Surface Creation and Evaluation\n");

    vec3 apex = {0.0f, 0.0f, 0.0f};
    vec3 axis = {0.0f, 0.0f, 1.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};
    float angle = PI / 4.0f; /* 45 degree cone */

    lc_surface_handle_t cone = lc_geometry_create_cone(apex, axis, x_axis, angle);
    assert(cone != LC_SURFACE_INVALID);

    /* Evaluate at (0, 0) should give apex */
    vec3 p00;
    lc_geometry_eval_surface(cone, 0.0f, 0.0f, p00);
    assert(vec3_equals(p00, apex, TOLERANCE));

    /* Evaluate at (0, 1) should give apex + axis + tan(angle) * x_axis */
    vec3 p0_1;
    lc_geometry_eval_surface(cone, 0.0f, 1.0f, p0_1);
    float expected_x = tanf(angle);
    vec3 expected0_1 = {expected_x, 0.0f, 1.0f};
    assert(vec3_equals(p0_1, expected0_1, TOLERANCE));

    printf("  PASS: Cone surface works correctly\n");
}

static void test_torus_surface(void)
{
    printf("Test: Torus Surface Creation and Evaluation\n");

    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};
    vec3 y_axis = {0.0f, 1.0f, 0.0f};
    vec3 z_axis = {0.0f, 0.0f, 1.0f};
    float major_radius = 10.0f;
    float minor_radius = 2.0f;

    lc_surface_handle_t torus = lc_geometry_create_torus(center, x_axis, y_axis, z_axis, major_radius, minor_radius);
    assert(torus != LC_SURFACE_INVALID);

    /* Evaluate at (0, 0) should give center + (major + minor) * x_axis */
    vec3 p00;
    lc_geometry_eval_surface(torus, 0.0f, 0.0f, p00);
    vec3 expected00 = {major_radius + minor_radius, 0.0f, 0.0f};
    assert(vec3_equals(p00, expected00, TOLERANCE));

    /* Evaluate at (0, PI) should give center + (major - minor) * x_axis */
    vec3 p0_pi;
    lc_geometry_eval_surface(torus, 0.0f, PI, p0_pi);
    vec3 expected0_pi = {major_radius - minor_radius, 0.0f, 0.0f};
    assert(vec3_equals(p0_pi, expected0_pi, TOLERANCE));

    printf("  PASS: Torus surface works correctly\n");
}

static void test_large_scale(void)
{
    printf("Test: Large Scale Creation (1000 curves + 1000 surfaces)\n");

    /* Create 1000 line curves */
    int i;
    for (i = 0; i < 1000; i++)
    {
        vec3 origin = {(float)i, 0.0f, 0.0f};
        vec3 direction = {1.0f, 0.0f, 0.0f};
        lc_curve_handle_t curve = lc_geometry_create_line(origin, direction, 1.0f);
        assert(curve != LC_CURVE_INVALID);
    }

    /* Create 1000 plane surfaces */
    for (i = 0; i < 1000; i++)
    {
        vec3 origin = {0.0f, (float)i, 0.0f};
        vec3 x_axis = {1.0f, 0.0f, 0.0f};
        vec3 y_axis = {0.0f, 1.0f, 0.0f};
        lc_surface_handle_t surface = lc_geometry_create_plane(origin, x_axis, y_axis);
        assert(surface != LC_SURFACE_INVALID);
    }

    printf("  PASS: Created 1000 curves and 1000 surfaces successfully\n");
}

/***************************************************************
** MARK: MAIN
***************************************************************/

int main(void)
{
    printf("==============================================\n");
    printf("libcad Geometry System Test Suite\n");
    printf("==============================================\n\n");

    lc_geometry_init();

    test_line_curve();
    test_circle_curve();
    test_ellipse_curve();
    test_plane_surface();
    test_cylinder_surface();
    test_sphere_surface();
    test_cone_surface();
    test_torus_surface();
    test_large_scale();

    lc_geometry_shutdown();

    printf("\n==============================================\n");
    printf("ALL TESTS PASSED\n");
    printf("==============================================\n");

    return 0;
}
