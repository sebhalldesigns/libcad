/***************************************************************
**
** libcad Test File
**
** File         :  test_type.c
** Module       :  type
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Test suite for libcad type system (Class-based)
**
***************************************************************/

#include <model/type/type.h>
#include <types/core/object/object.h>
#include <types/core/document/document.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/***************************************************************
** TEST UTILITIES
***************************************************************/

static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("  ✗ FAILED: %s\n", message); \
            printf("      at %s:%d\n", __FILE__, __LINE__); \
            test_failed++; \
            return; \
        } \
    } while (0)

#define PASS(message) \
    do { \
        printf("  ✓ PASSED: %s\n", message); \
        test_passed++; \
    } while (0)

#define RUN_TEST(name) \
    do { \
        printf("\n[TEST] %s\n", #name); \
        test_count++; \
        test_##name(); \
    } while (0)

/***************************************************************
** TEST TYPE DEFINITIONS
**
** Define a custom type for testing inheritance and vtable
***************************************************************/

/* test_shape_t - inherits from object_t */
typedef struct test_shape_t {
    object_t parent;
    int x, y;
    int area_call_count;
} test_shape_t;

typedef struct test_shape_class_t {
    object_class_t parent_class;
    int (*compute_area)(test_shape_t* self);
} test_shape_class_t;

/* Forward declarations */
type_handle_t test_shape_get_type(void);
test_shape_t* test_shape_new(void);
void test_shape_free(test_shape_t* self);
void test_shape_set_position(test_shape_t* self, int x, int y);
int test_shape_compute_area(test_shape_t* self);

/* test_rectangle_t - inherits from test_shape_t */
typedef struct test_rectangle_t {
    test_shape_t parent;
    int width, height;
} test_rectangle_t;

typedef struct test_rectangle_class_t {
    test_shape_class_t parent_class;
    /* Can add more methods here */
} test_rectangle_class_t;

/* Forward declarations */
type_handle_t test_rectangle_get_type(void);
test_rectangle_t* test_rectangle_new(void);
void test_rectangle_free(test_rectangle_t* self);
void test_rectangle_set_size(test_rectangle_t* self, int width, int height);
int test_rectangle_compute_area(test_rectangle_t* self);

/***************************************************************
** TEST TYPE IMPLEMENTATIONS
***************************************************************/

/* test_shape_t implementation */

LIBCAD_DEFINE_TYPE(test_shape, object_get_type())

static void test_shape_class_init(test_shape_class_t* cls)
{
    cls->compute_area = test_shape_compute_area;
}

static void test_shape_init(test_shape_t* self)
{
    self->x = 0;
    self->y = 0;
    self->area_call_count = 0;
}

void test_shape_set_position(test_shape_t* self, int x, int y)
{
    if (!self) return;
    self->x = x;
    self->y = y;
}

int test_shape_compute_area(test_shape_t* self)
{
    if (!self) return 0;
    self->area_call_count++;
    return 0;  /* Default - no area */
}

/* test_rectangle_t implementation */

LIBCAD_DEFINE_TYPE(test_rectangle, test_shape_get_type())

static void test_rectangle_class_init(test_rectangle_class_t* cls)
{
    /* Override parent's compute_area method */
    test_shape_class_t* parent = (test_shape_class_t*)cls;
    parent->compute_area = (int(*)(test_shape_t*))test_rectangle_compute_area;
}

static void test_rectangle_init(test_rectangle_t* self)
{
    self->width = 0;
    self->height = 0;
}

void test_rectangle_set_size(test_rectangle_t* self, int width, int height)
{
    if (!self) return;
    self->width = width;
    self->height = height;
}

int test_rectangle_compute_area(test_rectangle_t* self)
{
    if (!self) return 0;
    self->parent.area_call_count++;
    return self->width * self->height;
}

/***************************************************************
** TESTS
***************************************************************/

void test_basic_type_registration(void)
{
    type_handle_t type = test_shape_get_type();

    ASSERT(type != TYPE_INVALID, "Type registered successfully");

    const char* name = type_name(type);
    ASSERT(name != NULL, "Type has a name");
    ASSERT(strcmp(name, "test_shape_t") == 0, "Type name is correct");

    PASS("Basic type registration works");
}

void test_type_lookup(void)
{
    type_handle_t type1 = test_shape_get_type();
    type_handle_t type2 = type_from_name("test_shape_t");

    ASSERT(type2 != TYPE_INVALID, "Type lookup by name works");
    ASSERT(type1 == type2, "Lookup returns same type handle");

    PASS("Type lookup works correctly");
}

void test_instance_creation(void)
{
    test_shape_t* shape = test_shape_new();

    ASSERT(shape != NULL, "Instance created successfully");
    ASSERT(shape->parent.cls != NULL, "Instance has class pointer");
    ASSERT(shape->x == 0, "Instance fields initialized (x)");
    ASSERT(shape->y == 0, "Instance fields initialized (y)");
    ASSERT(shape->area_call_count == 0, "Instance fields initialized (counter)");

    test_shape_free(shape);

    PASS("Instance creation works correctly");
}

void test_instance_methods(void)
{
    test_shape_t* shape = test_shape_new();

    test_shape_set_position(shape, 10, 20);

    ASSERT(shape->x == 10, "Method modified field (x)");
    ASSERT(shape->y == 20, "Method modified field (y)");

    test_shape_free(shape);

    PASS("Instance methods work correctly");
}

void test_vtable_dispatch(void)
{
    test_shape_t* shape = test_shape_new();
    test_shape_class_t* cls = (test_shape_class_t*)LIBCAD_GET_CLASS(shape);

    ASSERT(cls->compute_area != NULL, "Vtable has compute_area method");

    int area = cls->compute_area(shape);

    ASSERT(area == 0, "Vtable method returns correct value");
    ASSERT(shape->area_call_count == 1, "Vtable method was called");

    test_shape_free(shape);

    PASS("Vtable dispatch works correctly");
}

void test_inheritance_basic(void)
{
    type_handle_t shape_type = test_shape_get_type();
    type_handle_t rect_type = test_rectangle_get_type();

    ASSERT(rect_type != TYPE_INVALID, "Child type registered");
    ASSERT(rect_type != shape_type, "Child type is distinct from parent");

    ASSERT(type_is_a(rect_type, shape_type), "Child is-a parent");
    ASSERT(!type_is_a(shape_type, rect_type), "Parent is-not-a child");

    PASS("Basic inheritance works correctly");
}

void test_inheritance_instance_creation(void)
{
    test_rectangle_t* rect = test_rectangle_new();

    ASSERT(rect != NULL, "Child instance created");

    /* Check parent fields initialized */
    ASSERT(rect->parent.x == 0, "Parent fields initialized (x)");
    ASSERT(rect->parent.y == 0, "Parent fields initialized (y)");

    /* Check child fields initialized */
    ASSERT(rect->width == 0, "Child fields initialized (width)");
    ASSERT(rect->height == 0, "Child fields initialized (height)");

    test_rectangle_free(rect);

    PASS("Inheritance instance creation works");
}

void test_inheritance_method_override(void)
{
    test_shape_t* shape = test_shape_new();
    test_rectangle_t* rect = test_rectangle_new();

    test_rectangle_set_size(rect, 5, 10);

    /* Call via test_shape_t pointer (polymorphism!) */
    test_shape_t* rect_as_shape = (test_shape_t*)rect;
    test_shape_class_t* cls = (test_shape_class_t*)LIBCAD_GET_CLASS(rect_as_shape);

    int area = cls->compute_area(rect_as_shape);

    ASSERT(area == 50, "Overridden method returns correct value");
    ASSERT(rect->parent.area_call_count == 1, "Overridden method was called");

    test_shape_free(shape);
    test_rectangle_free(rect);

    PASS("Method override works correctly");
}

void test_polymorphism(void)
{
    test_shape_t* shape = test_shape_new();
    test_rectangle_t* rect = test_rectangle_new();
    test_rectangle_set_size(rect, 4, 6);

    /* Array of test_shape_t pointers (polymorphic) */
    test_shape_t* shapes[2];
    shapes[0] = shape;
    shapes[1] = (test_shape_t*)rect;

    /* Call compute_area on both - should dispatch to correct implementation */
    int total_area = 0;
    for (int i = 0; i < 2; i++) {
        test_shape_class_t* cls = (test_shape_class_t*)LIBCAD_GET_CLASS(shapes[i]);
        total_area += cls->compute_area(shapes[i]);
    }

    ASSERT(total_area == 24, "Polymorphic dispatch works (0 + 24 = 24)");
    ASSERT(shape->area_call_count == 1, "Shape method called once");
    ASSERT(rect->parent.area_call_count == 1, "Rectangle method called once");

    test_shape_free(shape);
    test_rectangle_free(rect);

    PASS("Polymorphism works correctly");
}

void test_object_type(void)
{
    object_t* obj = object_new();

    ASSERT(obj != NULL, "object_t created successfully");
    ASSERT(obj->cls != NULL, "object_t has class pointer");
    ASSERT(obj->name == NULL, "object_t name initialized to NULL");

    object_set_name(obj, "Test object_t");
    const char* name = object_get_name(obj);

    ASSERT(name != NULL, "object_t name set successfully");
    ASSERT(strcmp(name, "Test object_t") == 0, "object_t name is correct");

    object_free(obj);

    PASS("object_t type works correctly");
}

void test_document_type(void)
{
    document_t* doc = document_new();

    ASSERT(doc != NULL, "document_t created successfully");
    ASSERT(doc->parent.cls != NULL, "document_t has class pointer");
    ASSERT(doc->path == NULL, "document_t path initialized to NULL");
    ASSERT(doc->children_count == 0, "document_t has no children");

    document_set_path(doc, "/test/path.cad");
    const char* path = document_get_path(doc);

    ASSERT(path != NULL, "document_t path set successfully");
    ASSERT(strcmp(path, "/test/path.cad") == 0, "document_t path is correct");

    document_free(doc);

    PASS("document_t type works correctly");
}

void test_document_inheritance(void)
{
    type_handle_t obj_type = object_get_type();
    type_handle_t doc_type = document_get_type();

    ASSERT(type_is_a(doc_type, obj_type), "document_t is-a object_t");

    document_t* doc = document_new();
    object_t* obj_ptr = (object_t*)doc;

    /* Set name via object_t interface */
    object_set_name(obj_ptr, "My document_t");

    /* Set path via document_t interface */
    document_set_path(doc, "/path/to/doc.cad");

    const char* name = object_get_name(obj_ptr);
    const char* path = document_get_path(doc);

    ASSERT(strcmp(name, "My document_t") == 0, "object_t methods work on document_t");
    ASSERT(strcmp(path, "/path/to/doc.cad") == 0, "document_t methods work");

    document_free(doc);

    PASS("document_t inheritance works correctly");
}

void test_document_children(void)
{
    document_t* doc = document_new();
    object_t* child1 = object_new();
    object_t* child2 = object_new();

    object_set_name(child1, "Child 1");
    object_set_name(child2, "Child 2");

    document_add_child(doc, child1);
    document_add_child(doc, child2);

    ASSERT(document_get_child_count(doc) == 2, "document_t has 2 children");

    object_t* retrieved = document_get_child(doc, 0);
    ASSERT(retrieved == child1, "First child retrieved correctly");

    document_remove_child(doc, 0);
    ASSERT(document_get_child_count(doc) == 1, "Child removed");

    retrieved = document_get_child(doc, 0);
    ASSERT(retrieved == child2, "Remaining child is child2");

    document_free(doc);
    object_free(child1);
    object_free(child2);

    PASS("document_t children management works correctly");
}

void test_polymorphic_json_serialization(void)
{
    document_t* doc = document_new();
    object_t* obj = object_new();

    object_set_name((object_t*)doc, "Test document_t");
    document_set_path(doc, "/test.cad");

    object_set_name(obj, "Test object_t");

    /* Get JSON via object_t interface (polymorphic!) */
    json_t* doc_json = OBJECT_TO_JSON((object_t*)doc);
    json_t* obj_json = OBJECT_TO_JSON(obj);

    ASSERT(doc_json != NULL, "document_t JSON created");
    ASSERT(obj_json != NULL, "object_t JSON created");

    /* Check type field */
    json_t* doc_type = json_object_get(doc_json, "type");
    json_t* obj_type = json_object_get(obj_json, "type");

    ASSERT(strcmp(json_string_value(doc_type), "document_t") == 0,
           "document_t JSON has correct type");
    ASSERT(strcmp(json_string_value(obj_type), "object_t") == 0,
           "object_t JSON has correct type");

    /* Check document-specific field */
    json_t* path_field = json_object_get(doc_json, "path");
    ASSERT(strcmp(json_string_value(path_field), "/test.cad") == 0,
           "document_t JSON has path field");

    json_decref(doc_json);
    json_decref(obj_json);

    document_free(doc);
    object_free(obj);

    PASS("Polymorphic JSON serialization works correctly");
}

void test_class_singleton(void)
{
    /* Ensure type is registered first */
    object_get_type();

    /* Get class multiple times */
    object_class_t* klass1 = object_class_get();
    object_class_t* klass2 = object_class_get();

    ASSERT(klass1 == klass2, "Class is a singleton");
    ASSERT(klass1->debug_print != NULL, "Class has vtable methods");

    PASS("Class singleton works correctly");
}

void test_libcad_call_macro(void)
{
    object_t* obj = object_new();
    object_set_name(obj, "Macro Test");

    /* Test the OBJECT_TO_JSON macro (which uses LIBCAD_CALL internally) */
    json_t* json = OBJECT_TO_JSON(obj);

    ASSERT(json != NULL, "OBJECT_TO_JSON macro works");

    json_decref(json);
    object_free(obj);

    PASS("object_t polymorphic macros work correctly");
}

/***************************************************************
** MAIN
***************************************************************/

int main(void)
{
    printf("\n╔═══════════════════════════════════════════╗\n");
    printf("║  libcad Type System Test Suite (V2)      ║\n");
    printf("║  Class-Based with Macros                  ║\n");
    printf("╚═══════════════════════════════════════════╝\n");

    /* Core type system tests */
    RUN_TEST(basic_type_registration);
    RUN_TEST(type_lookup);
    RUN_TEST(instance_creation);
    RUN_TEST(instance_methods);
    RUN_TEST(vtable_dispatch);

    /* Inheritance tests */
    RUN_TEST(inheritance_basic);
    RUN_TEST(inheritance_instance_creation);
    RUN_TEST(inheritance_method_override);
    RUN_TEST(polymorphism);

    /* object_t/document_t tests */
    RUN_TEST(object_type);
    RUN_TEST(document_type);
    RUN_TEST(document_inheritance);
    RUN_TEST(document_children);
    RUN_TEST(polymorphic_json_serialization);

    /* Utility tests */
    RUN_TEST(class_singleton);
    RUN_TEST(libcad_call_macro);

    /* Print results */
    printf("\n╔═══════════════════════════════════════════╗\n");
    printf("║   Test Results                            ║\n");
    printf("╠═══════════════════════════════════════════╣\n");
    printf("║   Total:  %3d                             ║\n", test_count);
    printf("║   Passed: %3d ✓                           ║\n", test_passed);
    printf("║   Failed: %3d ✗                           ║\n", test_failed);
    printf("╚═══════════════════════════════════════════╝\n\n");

    if (test_failed == 0) {
        printf("🎉 All tests passed! The new type system is working perfectly.\n\n");
    }

    return (test_failed == 0) ? 0 : 1;
}
