/***************************************************************
**
** libcad Test File
**
** File         :  test_entity.c
** Module       :  libcad (test)
** Author       :  SH
** Created      :  2026-01-29 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Simple test for entity system functionality.
**                 Tests entity creation, validation, tree manipulation,
**                 and enumeration.
**
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "lib/lc_entity.h"

/* Test result counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test helper macros */
#define TEST(name) printf("\n[TEST] %s\n", name)
#define ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("  [PASS] %s\n", message); \
            tests_passed++; \
        } else { \
            printf("  [FAIL] %s\n", message); \
            tests_failed++; \
        } \
    } while(0)

void test_entity_initialization(void)
{
    TEST("Entity System Initialization");

    lc_entity_init();

    /* Create a simple entity */
    lc_entity_handle_t entity = lc_entity_create(LC_ENTITY_TYPE_ASSEMBLY);
    ASSERT(entity != LC_ENTITY_INVALID, "Create entity");
    ASSERT(lc_entity_is_valid(entity), "Entity is valid");
    ASSERT(lc_entity_get_type(entity) == LC_ENTITY_TYPE_ASSEMBLY, "Entity type is ASSEMBLY");

    /* Clean up */
    lc_entity_destroy(entity);
    ASSERT(!lc_entity_is_valid(entity), "Entity invalidated after destroy");

    lc_entity_shutdown();
}

void test_entity_lifecycle(void)
{
    TEST("Entity Lifecycle");

    lc_entity_init();

    /* Create multiple entities */
    lc_entity_handle_t e1 = lc_entity_create(LC_ENTITY_TYPE_SKETCH);
    lc_entity_handle_t e2 = lc_entity_create(LC_ENTITY_TYPE_BODY);
    lc_entity_handle_t e3 = lc_entity_create(LC_ENTITY_TYPE_ASSEMBLY);

    ASSERT(lc_entity_is_valid(e1), "Entity 1 valid");
    ASSERT(lc_entity_is_valid(e2), "Entity 2 valid");
    ASSERT(lc_entity_is_valid(e3), "Entity 3 valid");

    /* Destroy middle entity */
    lc_entity_destroy(e2);
    ASSERT(lc_entity_is_valid(e1), "Entity 1 still valid");
    ASSERT(!lc_entity_is_valid(e2), "Entity 2 invalidated");
    ASSERT(lc_entity_is_valid(e3), "Entity 3 still valid");

    /* Create new entity (should reuse e2's slot) */
    lc_entity_handle_t e4 = lc_entity_create(LC_ENTITY_TYPE_SKETCH);
    ASSERT(lc_entity_is_valid(e4), "Entity 4 valid");
    ASSERT(LC_ENTITY_INDEX(e4) == LC_ENTITY_INDEX(e2), "Entity 4 reused slot from entity 2");
    ASSERT(LC_ENTITY_GENERATION(e4) > LC_ENTITY_GENERATION(e2), "Entity 4 has newer generation");

    /* Clean up */
    lc_entity_destroy(e1);
    lc_entity_destroy(e3);
    lc_entity_destroy(e4);

    lc_entity_shutdown();
}

void test_entity_tree(void)
{
    TEST("Entity Tree Manipulation");

    lc_entity_init();

    /* Create parent and children */
    lc_entity_handle_t parent = lc_entity_create(LC_ENTITY_TYPE_ASSEMBLY);
    lc_entity_handle_t child1 = lc_entity_create(LC_ENTITY_TYPE_SKETCH);
    lc_entity_handle_t child2 = lc_entity_create(LC_ENTITY_TYPE_SKETCH);
    lc_entity_handle_t child3 = lc_entity_create(LC_ENTITY_TYPE_BODY);

    /* Add children to parent */
    ASSERT(lc_entity_add_child(parent, child1), "Add child 1");
    ASSERT(lc_entity_add_child(parent, child2), "Add child 2");
    ASSERT(lc_entity_add_child(parent, child3), "Add child 3");

    /* Verify parent-child relationships */
    ASSERT(lc_entity_get_parent(child1) == parent, "Child 1 parent is correct");
    ASSERT(lc_entity_get_parent(child2) == parent, "Child 2 parent is correct");
    ASSERT(lc_entity_get_parent(child3) == parent, "Child 3 parent is correct");

    /* Verify first child */
    lc_entity_handle_t first_child = lc_entity_get_first_child(parent);
    ASSERT(first_child == child3, "First child is child3 (last added)");

    /* Count children by walking sibling list */
    int child_count = 0;
    lc_entity_handle_t current = lc_entity_get_first_child(parent);
    while (current != LC_ENTITY_INVALID)
    {
        child_count++;
        current = lc_entity_get_next_sibling(current);
    }
    ASSERT(child_count == 3, "Parent has 3 children");

    /* Remove a child */
    ASSERT(lc_entity_remove_child(parent, child2), "Remove child 2");
    ASSERT(lc_entity_get_parent(child2) == LC_ENTITY_INVALID, "Child 2 has no parent");

    /* Count children again */
    child_count = 0;
    current = lc_entity_get_first_child(parent);
    while (current != LC_ENTITY_INVALID)
    {
        child_count++;
        current = lc_entity_get_next_sibling(current);
    }
    ASSERT(child_count == 2, "Parent now has 2 children");

    /* Clean up */
    lc_entity_destroy(parent);
    lc_entity_destroy(child1);
    lc_entity_destroy(child2);
    lc_entity_destroy(child3);

    lc_entity_shutdown();
}

void test_entity_sketch_creation(void)
{
    TEST("Sketch and Geometry Creation");

    lc_entity_init();

    /* Create a sketch */
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 normal = {0.0f, 0.0f, 1.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};

    lc_entity_handle_t sketch = lc_entity_create_sketch(origin, normal, x_axis);
    ASSERT(sketch != LC_ENTITY_INVALID, "Create sketch");
    ASSERT(lc_entity_get_type(sketch) == LC_ENTITY_TYPE_SKETCH, "Sketch type correct");

    /* Get sketch data */
    lc_sketch_data_t *sketch_data = (lc_sketch_data_t*)lc_entity_get_data(sketch);
    ASSERT(sketch_data != NULL, "Sketch data exists");
    ASSERT(sketch_data->origin[0] == 0.0f && sketch_data->origin[1] == 0.0f && sketch_data->origin[2] == 0.0f, "Sketch origin correct");

    /* Create line geometry in sketch */
    vec2 start = {0.0f, 0.0f};
    vec2 end = {10.0f, 10.0f};
    lc_entity_handle_t line = lc_entity_create_line(sketch, start, end, 0xFFFFFFFF, 2.0f);
    ASSERT(line != LC_ENTITY_INVALID, "Create line");
    ASSERT(lc_entity_get_type(line) == LC_ENTITY_TYPE_GEOMETRY_LINE, "Line type correct");
    ASSERT(lc_entity_get_parent(line) == sketch, "Line is child of sketch");

    /* Create circle geometry */
    vec2 center = {5.0f, 5.0f};
    lc_entity_handle_t circle = lc_entity_create_circle(sketch, center, 3.0f, 0xFF0000FF);
    ASSERT(circle != LC_ENTITY_INVALID, "Create circle");
    ASSERT(lc_entity_get_type(circle) == LC_ENTITY_TYPE_GEOMETRY_CIRCLE, "Circle type correct");

    /* Create rectangle geometry */
    vec2 min = {-5.0f, -5.0f};
    vec2 max = {5.0f, 5.0f};
    lc_entity_handle_t rect = lc_entity_create_rect(sketch, min, max, 0x00FF00FF);
    ASSERT(rect != LC_ENTITY_INVALID, "Create rectangle");
    ASSERT(lc_entity_get_type(rect) == LC_ENTITY_TYPE_GEOMETRY_RECT, "Rectangle type correct");

    /* Verify sketch has children */
    int child_count = 0;
    lc_entity_handle_t current = lc_entity_get_first_child(sketch);
    while (current != LC_ENTITY_INVALID)
    {
        child_count++;
        current = lc_entity_get_next_sibling(current);
    }
    ASSERT(child_count == 3, "Sketch has 3 geometry children");

    /* Clean up */
    lc_entity_destroy(sketch);
    lc_entity_destroy(line);
    lc_entity_destroy(circle);
    lc_entity_destroy(rect);

    lc_entity_shutdown();
}

void test_entity_enumeration(void)
{
    TEST("Entity Enumeration by Type");

    lc_entity_init();

    /* Create various entity types */
    lc_entity_create(LC_ENTITY_TYPE_ASSEMBLY);
    lc_entity_create(LC_ENTITY_TYPE_SKETCH);
    lc_entity_create(LC_ENTITY_TYPE_SKETCH);
    lc_entity_create(LC_ENTITY_TYPE_BODY);
    lc_entity_create(LC_ENTITY_TYPE_SKETCH);

    /* Enumerate sketches */
    lc_entity_handle_t sketches[10];
    size_t sketch_count = lc_entity_enumerate_type(LC_ENTITY_TYPE_SKETCH, sketches, 10);
    ASSERT(sketch_count == 3, "Found 3 sketches");

    /* Enumerate bodies */
    lc_entity_handle_t bodies[10];
    size_t body_count = lc_entity_enumerate_type(LC_ENTITY_TYPE_BODY, bodies, 10);
    ASSERT(body_count == 1, "Found 1 body");

    /* Enumerate assemblies */
    lc_entity_handle_t assemblies[10];
    size_t assembly_count = lc_entity_enumerate_type(LC_ENTITY_TYPE_ASSEMBLY, assemblies, 10);
    ASSERT(assembly_count == 1, "Found 1 assembly");

    lc_entity_shutdown();
}

void test_entity_flags(void)
{
    TEST("Entity Flags");

    lc_entity_init();

    lc_entity_handle_t entity = lc_entity_create(LC_ENTITY_TYPE_SKETCH);

    /* Check default flags */
    uint32_t flags = lc_entity_get_flags(entity);
    ASSERT(flags & LC_ENTITY_FLAG_VISIBLE, "Entity visible by default");
    ASSERT(!(flags & LC_ENTITY_FLAG_LOCKED), "Entity not locked by default");
    ASSERT(!(flags & LC_ENTITY_FLAG_SELECTED), "Entity not selected by default");

    /* Set flags */
    lc_entity_set_flags(entity, LC_ENTITY_FLAG_VISIBLE | LC_ENTITY_FLAG_LOCKED);
    flags = lc_entity_get_flags(entity);
    ASSERT(flags & LC_ENTITY_FLAG_VISIBLE, "Entity visible");
    ASSERT(flags & LC_ENTITY_FLAG_LOCKED, "Entity locked");
    ASSERT(!(flags & LC_ENTITY_FLAG_SELECTED), "Entity not selected");

    lc_entity_destroy(entity);
    lc_entity_shutdown();
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("libcad Entity System Test Suite\n");
    printf("========================================\n");

    test_entity_initialization();
    test_entity_lifecycle();
    test_entity_tree();
    test_entity_sketch_creation();
    test_entity_enumeration();
    test_entity_flags();

    printf("\n========================================\n");
    printf("Test Results\n");
    printf("========================================\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    if (tests_failed == 0)
    {
        printf("\nAll tests passed!\n");
        return 0;
    }
    else
    {
        printf("\nSome tests failed.\n");
        return 1;
    }
}
