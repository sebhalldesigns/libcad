/***************************************************************
**
** libcad Test File
**
** File         :  test_type.c
** Module       :  type
** Author       :  SH
** Created      :  2026-02-13 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Test suite for libcad type system
**
***************************************************************/

#include <model/type/type.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
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
** TEST DATA STRUCTURES
***************************************************************/

/* Base type data */
typedef struct {
    uint64_t id;
    bool visible;
} base_data_t;

/* Derived type data (extends base) */
typedef struct {
    int value;
    float x, y, z;
} derived_local_data_t;

/* Method implementations */
static int init_call_count = 0;
static int destroy_call_count = 0;
static int custom_call_count = 0;

void base_init(type_instance_t* self, void* params)
{
    base_data_t* data = (base_data_t*)self->data;
    data->id = 123;
    data->visible = true;
    init_call_count++;
}

void base_destroy(type_instance_t* self)
{
    destroy_call_count++;
}

void derived_init(type_instance_t* self, void* params)
{
    base_data_t* base = (base_data_t*)self->data;
    derived_local_data_t* derived = (derived_local_data_t*)((uint8_t*)self->data + sizeof(base_data_t));

    base->id = 456;
    base->visible = false;
    derived->value = 42;
    derived->x = 1.0f;
    derived->y = 2.0f;
    derived->z = 3.0f;

    init_call_count++;
}

void custom_method(type_instance_t* self, void* args, void* result)
{
    int* arg = (int*)args;
    int* res = (int*)result;

    if (arg && res) {
        *res = *arg * 2;
    }

    custom_call_count++;
}

/* Test methods for method registration tests (these have the standard 3-parameter signature) */
void setup_method(type_instance_t* self, void* args, void* result)
{
    base_data_t* data = (base_data_t*)self->data;
    data->id = 123;
    data->visible = true;
}

void cleanup_method(type_instance_t* self, void* args, void* result)
{
    /* No-op for testing */
}

void derived_setup_method(type_instance_t* self, void* args, void* result)
{
    base_data_t* base = (base_data_t*)self->data;
    derived_local_data_t* derived = (derived_local_data_t*)((uint8_t*)self->data + sizeof(base_data_t));

    base->id = 456;
    base->visible = false;
    derived->value = 42;
}

/***************************************************************
** TESTS
***************************************************************/

void test_type_registration(void)
{
    /* Register base type */
    type_handle_t base_type = type_register(
        "test.Base",
        TYPE_INVALID_HANDLE,
        sizeof(base_data_t)
    );

    ASSERT(base_type != TYPE_INVALID_HANDLE, "Base type registered");

    /* Lookup type by name */
    type_handle_t found = type_get("test.Base");
    ASSERT(found == base_type, "Type lookup works");

    /* Register duplicate - should return existing */
    type_handle_t duplicate = type_register(
        "test.Base",
        TYPE_INVALID_HANDLE,
        sizeof(base_data_t)
    );
    ASSERT(duplicate == base_type, "Duplicate registration returns existing type");

    PASS("Type registration works correctly");
}

void test_type_inheritance(void)
{
    /* Register base */
    type_handle_t base_type = type_register(
        "test.InheritBase",
        TYPE_INVALID_HANDLE,
        sizeof(base_data_t)
    );

    /* Register derived */
    type_handle_t derived_type = type_register(
        "test.InheritDerived",
        base_type,
        sizeof(derived_local_data_t)
    );

    ASSERT(derived_type != TYPE_INVALID_HANDLE, "Derived type registered");
    ASSERT(derived_type != base_type, "Derived type is different from base");

    PASS("Type inheritance works correctly");
}

void test_property_registration(void)
{
    type_handle_t base_type = type_register(
        "test.PropBase",
        TYPE_INVALID_HANDLE,
        sizeof(base_data_t)
    );

    /* Register properties */
    property_handle_t id_prop = type_register_property(
        base_type,
        "id",
        offsetof(base_data_t, id),
        sizeof(uint64_t)
    );

    property_handle_t visible_prop = type_register_property(
        base_type,
        "visible",
        offsetof(base_data_t, visible),
        sizeof(bool)
    );

    ASSERT(id_prop != TYPE_INVALID_HANDLE, "ID property registered");
    ASSERT(visible_prop != TYPE_INVALID_HANDLE, "Visible property registered");
    ASSERT(id_prop != visible_prop, "Properties are distinct");

    /* Lookup properties */
    property_handle_t found_id = type_get_property("id", base_type);
    property_handle_t found_visible = type_get_property("visible", base_type);

    ASSERT(found_id == id_prop, "ID property lookup works");
    ASSERT(found_visible == visible_prop, "Visible property lookup works");

    PASS("Property registration and lookup work correctly");
}

void test_property_inheritance(void)
{
    type_handle_t base_type = type_register(
        "test.PropInheritBase",
        TYPE_INVALID_HANDLE,
        sizeof(base_data_t)
    );

    type_handle_t derived_type = type_register(
        "test.PropInheritDerived",
        base_type,
        sizeof(derived_local_data_t)
    );

    /* Register base property */
    type_register_property(base_type, "id",
                          offsetof(base_data_t, id),
                          sizeof(uint64_t));

    /* Register derived property */
    type_register_property(derived_type, "value",
                          sizeof(base_data_t) + offsetof(derived_local_data_t, value),
                          sizeof(int));

    /* Derived type should find both properties */
    property_handle_t derived_id = type_get_property("id", derived_type);
    property_handle_t derived_value = type_get_property("value", derived_type);

    ASSERT(derived_id != TYPE_INVALID_HANDLE, "Derived finds inherited property");
    ASSERT(derived_value != TYPE_INVALID_HANDLE, "Derived finds own property");

    /* Base type should not find derived property */
    property_handle_t base_value = type_get_property("value", base_type);
    ASSERT(base_value == TYPE_INVALID_HANDLE, "Base doesn't find derived property");

    PASS("Property inheritance works correctly");
}

void test_method_registration(void)
{
    type_handle_t base_type = type_register(
        "test.MethodBase",
        TYPE_INVALID_HANDLE,
        sizeof(base_data_t)
    );

    /* Register methods */
    method_handle_t setup = type_register_method(
        base_type,
        "setup",
        setup_method
    );

    method_handle_t cleanup = type_register_method(
        base_type,
        "cleanup",
        cleanup_method
    );

    ASSERT(setup != TYPE_INVALID_HANDLE, "Setup method registered");
    ASSERT(cleanup != TYPE_INVALID_HANDLE, "Cleanup method registered");

    /* Lookup methods */
    method_handle_t found_setup = type_get_method("setup", base_type);
    method_handle_t found_cleanup = type_get_method("cleanup", base_type);

    ASSERT(found_setup == setup, "Setup method lookup works");
    ASSERT(found_cleanup == cleanup, "Cleanup method lookup works");

    PASS("Method registration and lookup work correctly");
}

void test_method_inheritance_and_override(void)
{
    type_handle_t base_type = type_register(
        "test.MethodInheritBase",
        TYPE_INVALID_HANDLE,
        sizeof(base_data_t)
    );

    type_handle_t derived_type = type_register(
        "test.MethodInheritDerived",
        base_type,
        sizeof(derived_local_data_t)
    );

    /* Register base methods */
    method_handle_t base_setup = type_register_method(base_type, "setup", setup_method);
    type_register_method(base_type, "cleanup", cleanup_method);

    /* Override setup in derived */
    method_handle_t derived_setup = type_register_method(derived_type, "setup", derived_setup_method);

    /* Derived setup should override base setup */
    method_handle_t found_setup = type_get_method("setup", derived_type);
    ASSERT(found_setup == derived_setup, "Derived setup overrides base setup");
    ASSERT(found_setup != base_setup, "Derived setup is different from base setup");

    /* Derived should inherit cleanup */
    method_handle_t found_cleanup = type_get_method("cleanup", derived_type);
    ASSERT(found_cleanup != TYPE_INVALID_HANDLE, "Derived inherits cleanup method");

    PASS("Method inheritance and override work correctly");
}

void test_instance_creation_and_destruction(void)
{
    type_handle_t base_type = type_register(
        "test.InstanceBase",
        TYPE_INVALID_HANDLE,
        sizeof(base_data_t)
    );

    type_set_init(base_type, base_init);
    type_set_destroy(base_type, base_destroy);

    init_call_count = 0;
    destroy_call_count = 0;

    /* Create instance */
    type_instance_handle_t instance = type_instance_create(base_type);

    ASSERT(instance != TYPE_INVALID_HANDLE, "Instance created successfully");
    ASSERT(init_call_count == 1, "Init method called on creation");

    /* Destroy instance */
    type_instance_destroy(instance);

    ASSERT(destroy_call_count == 1, "Destroy method called on destruction");

    PASS("Instance creation and destruction work correctly");
}

void test_instance_property_access(void)
{
    type_handle_t base_type = type_register(
        "test.PropAccessBase",
        TYPE_INVALID_HANDLE,
        sizeof(base_data_t)
    );

    property_handle_t id_prop = type_register_property(
        base_type,
        "id",
        offsetof(base_data_t, id),
        sizeof(uint64_t)
    );

    property_handle_t visible_prop = type_register_property(
        base_type,
        "visible",
        offsetof(base_data_t, visible),
        sizeof(bool)
    );

    /* Create instance */
    type_instance_handle_t instance = type_instance_create(base_type);

    /* Set properties */
    uint64_t id = 999;
    bool visible = false;

    type_instance_set_property(instance, id_prop, &id);
    type_instance_set_property(instance, visible_prop, &visible);

    /* Get properties */
    uint64_t* id_ptr = (uint64_t*)type_instance_get_property_ptr(instance, id_prop);
    bool* visible_ptr = (bool*)type_instance_get_property_ptr(instance, visible_prop);

    ASSERT(*id_ptr == 999, "ID property set correctly");
    ASSERT(*visible_ptr == false, "Visible property set correctly");

    /* Modify via pointer */
    *id_ptr = 1234;

    uint64_t* id_ptr2 = (uint64_t*)type_instance_get_property_ptr(instance, id_prop);
    ASSERT(*id_ptr2 == 1234, "Property modification via pointer works");

    type_instance_destroy(instance);

    PASS("Instance property access works correctly");
}

void test_instance_method_invocation(void)
{
    type_handle_t base_type = type_register(
        "test.MethodCallBase",
        TYPE_INVALID_HANDLE,
        sizeof(base_data_t)
    );

    method_handle_t custom = type_register_method(base_type, "custom", custom_method);

    custom_call_count = 0;

    type_instance_handle_t instance = type_instance_create(base_type);

    /* Call method with args and result */
    int input = 21;
    int output = 0;

    type_instance_call_method(instance, custom, &input, &output);

    ASSERT(custom_call_count == 1, "Method was called");
    ASSERT(output == 42, "Method computed correct result");

    type_instance_destroy(instance);

    PASS("Instance method invocation works correctly");
}

void test_full_inheritance_example(void)
{
    /* Setup types */
    type_handle_t base_type = type_register(
        "test.FullBase",
        TYPE_INVALID_HANDLE,
        sizeof(base_data_t)
    );

    type_handle_t derived_type = type_register(
        "test.FullDerived",
        base_type,
        sizeof(derived_local_data_t)
    );

    /* Setup base properties */
    property_handle_t id_prop = type_register_property(
        base_type,
        "id",
        offsetof(base_data_t, id),
        sizeof(uint64_t)
    );

    /* Setup derived properties */
    property_handle_t value_prop = type_register_property(
        derived_type,
        "value",
        sizeof(base_data_t) + offsetof(derived_local_data_t, value),
        sizeof(int)
    );

    /* Setup methods */
    type_set_init(base_type, base_init);
    type_set_init(derived_type, derived_init);  /* Override */
    type_register_method(derived_type, "custom", custom_method);

    /* Create derived instance */
    init_call_count = 0;
    type_instance_handle_t instance = type_instance_create(derived_type);

    ASSERT(init_call_count == 1, "Derived init called (not base init)");

    /* Access inherited property */
    uint64_t* id_ptr = (uint64_t*)type_instance_get_property_ptr(instance, id_prop);
    ASSERT(*id_ptr == 456, "Derived init set ID to 456");

    /* Access derived property */
    int* value_ptr = (int*)type_instance_get_property_ptr(instance, value_prop);
    ASSERT(*value_ptr == 42, "Derived property initialized correctly");

    /* Modify inherited property */
    uint64_t new_id = 9999;
    type_instance_set_property(instance, id_prop, &new_id);

    uint64_t* id_ptr2 = (uint64_t*)type_instance_get_property_ptr(instance, id_prop);
    ASSERT(*id_ptr2 == 9999, "Can modify inherited property");

    /* Call derived method */
    custom_call_count = 0;
    method_handle_t custom = type_get_method("custom", derived_type);

    int input = 100;
    int output = 0;
    type_instance_call_method(instance, custom, &input, &output);

    ASSERT(custom_call_count == 1, "Derived method called");
    ASSERT(output == 200, "Derived method works correctly");

    type_instance_destroy(instance);

    PASS("Full inheritance example works correctly");
}

/***************************************************************
** MAIN
***************************************************************/

int main(void)
{
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║   libcad Type System Test Suite       ║\n");
    printf("╚════════════════════════════════════════╝\n");

    /* Run all tests */
    RUN_TEST(type_registration);
    RUN_TEST(type_inheritance);
    RUN_TEST(property_registration);
    RUN_TEST(property_inheritance);
    RUN_TEST(method_registration);
    RUN_TEST(method_inheritance_and_override);
    RUN_TEST(instance_creation_and_destruction);
    RUN_TEST(instance_property_access);
    RUN_TEST(instance_method_invocation);
    RUN_TEST(full_inheritance_example);

    /* Print results */
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║   Test Results                         ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║   Total:  %3d                          ║\n", test_count);
    printf("║   Passed: %3d ✓                        ║\n", test_passed);
    printf("║   Failed: %3d ✗                        ║\n", test_failed);
    printf("╚════════════════════════════════════════╝\n\n");

    return (test_failed == 0) ? 0 : 1;
}
