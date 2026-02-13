/***************************************************************
**
** libcad Source File
**
** File         :  type.c
** Module       :  type
** Author       :  SH
** Created      :  2026-02-13 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad Type System API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <util/log/log.h>

#include "type.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/* Initial capacities */
#define INITIAL_TYPE_CAPACITY       (32U)
#define INITIAL_PROPERTY_CAPACITY   (16U)
#define INITIAL_METHOD_CAPACITY     (16U)

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

/* Type registry - dynamically allocated array of types */
static type_t **type_registry = NULL;
static size_t type_count = 0;
static size_t type_capacity = 0;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static type_t* type_registry_find_by_name(const char* name);
static void type_registry_add(type_t* type);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

type_handle_t type_register(
    const char* name,
    type_handle_t parent_handle,
    size_t local_data_size
)
{
    assert(name && "name cannot be NULL");

    /* Check if type already exists */
    type_t* existing = type_registry_find_by_name(name);
    if (existing)
    {
        log_warning("type_register: type '%s' already registered", name);
        return (type_handle_t)existing;
    }

    /* Allocate new type */
    type_t* type = (type_t*)calloc(1, sizeof(type_t));
    if (!type)
    {
        log_error("type_register: failed to allocate type");
        return TYPE_INVALID_HANDLE;
    }

    /* Set parent */
    type->parent = (type_t*)parent_handle;

    /* Calculate data sizes */
    type->local_data_size = local_data_size;
    if (type->parent)
    {
        type->total_data_size = type->parent->total_data_size + local_data_size;
    }
    else
    {
        type->total_data_size = local_data_size;
    }

    /* Copy name (registry owns the string) */
    type->name = strdup(name);
    if (!type->name)
    {
        log_error("type_register: failed to allocate name");
        free(type);
        return TYPE_INVALID_HANDLE;
    }

    /* Allocate property and method arrays */
    type->properties = (property_t*)calloc(INITIAL_PROPERTY_CAPACITY, sizeof(property_t));
    type->properties_count = 0;
    type->properties_capacity = INITIAL_PROPERTY_CAPACITY;

    type->methods = (method_t*)calloc(INITIAL_METHOD_CAPACITY, sizeof(method_t));
    type->methods_count = 0;
    type->methods_capacity = INITIAL_METHOD_CAPACITY;

    if (!type->properties || !type->methods)
    {
        log_error("type_register: failed to allocate property/method arrays");
        free(type->name);
        free(type->properties);
        free(type->methods);
        free(type);
        return TYPE_INVALID_HANDLE;
    }

    /* Add to registry */
    type_registry_add(type);

    log_info("Registered type '%s' (local_size=%zu, total_size=%zu, parent=%s)",
             name, local_data_size, type->total_data_size,
             type->parent ? type->parent->name : "none");

    return (type_handle_t)type;
}

type_handle_t type_get(const char* name)
{
    assert(name && "name cannot be NULL");

    type_t* type = type_registry_find_by_name(name);
    return (type_handle_t)type;
}

property_handle_t type_register_property(
    type_handle_t type_handle,
    const char* name,
    size_t offset,
    size_t size)
{
    assert(name && "name cannot be NULL");
    assert(type_handle && "type_handle cannot be NULL");

    type_t* type = (type_t*)type_handle;

    /* Check if property already exists on this type */
    for (size_t i = 0; i < type->properties_count; i++)
    {
        if (strcmp(type->properties[i].name, name) == 0)
        {
            log_warning("type_register_property: property '%s' already exists on type '%s'",
                       name, type->name);
            return (property_handle_t)&type->properties[i];
        }
    }

    /* Grow array if needed */
    if (type->properties_count >= type->properties_capacity)
    {
        size_t new_capacity = type->properties_capacity * 2;
        property_t* new_props = (property_t*)realloc(type->properties,
                                                      new_capacity * sizeof(property_t));
        if (!new_props)
        {
            log_error("type_register_property: failed to grow property array");
            return TYPE_INVALID_HANDLE;
        }

        /* Zero out new region */
        memset(&new_props[type->properties_capacity], 0,
               (new_capacity - type->properties_capacity) * sizeof(property_t));

        type->properties = new_props;
        type->properties_capacity = new_capacity;
    }

    /* Add property */
    property_t* prop = &type->properties[type->properties_count];
    prop->name = strdup(name);
    prop->offset = offset;
    prop->size = size;

    if (!prop->name)
    {
        log_error("type_register_property: failed to allocate property name");
        return TYPE_INVALID_HANDLE;
    }

    type->properties_count++;

    log_info("Registered property '%s.%s' (offset=%zu, size=%zu)",
             type->name, name, offset, size);

    return (property_handle_t)prop;
}

property_handle_t type_get_property(const char* name, type_handle_t type_handle)
{
    assert(name && "name cannot be NULL");
    assert(type_handle && "type_handle cannot be NULL");

    type_t* type = (type_t*)type_handle;

    /* Walk up inheritance chain (child -> parent) */
    while (type != NULL)
    {
        /* Search this type's properties */
        for (size_t i = 0; i < type->properties_count; i++)
        {
            if (strcmp(type->properties[i].name, name) == 0)
            {
                return (property_handle_t)&type->properties[i];
            }
        }

        /* Try parent */
        type = type->parent;
    }

    /* Not found */
    return TYPE_INVALID_HANDLE;
}

method_handle_t type_register_method(
    type_handle_t type_handle,
    const char* name,
    void (*function)(type_instance_t* self, void* args, void* result))
{

    assert(name && "name cannot be NULL");
    assert(type_handle && "type_handle cannot be NULL");
    assert(function && "function cannot be NULL");

    type_t* type = (type_t*)type_handle;

    /* Check if method already exists on this type */
    for (size_t i = 0; i < type->methods_count; i++)
    {
        if (strcmp(type->methods[i].name, name) == 0)
        {
            log_warning("type_register_method: method '%s' already exists on type '%s'",
                       name, type->name);
            return (method_handle_t)&type->methods[i];
        }
    }

    /* Grow array if needed */
    if (type->methods_count >= type->methods_capacity)
    {
        size_t new_capacity = type->methods_capacity * 2;
        method_t* new_methods = (method_t*)realloc(type->methods,
                                                    new_capacity * sizeof(method_t));
        if (!new_methods)
        {
            log_error("type_register_method: failed to grow method array");
            return TYPE_INVALID_HANDLE;
        }

        /* Zero out new region */
        memset(&new_methods[type->methods_capacity], 0,
               (new_capacity - type->methods_capacity) * sizeof(method_t));

        type->methods = new_methods;
        type->methods_capacity = new_capacity;
    }

    /* Add method */
    method_t* method = &type->methods[type->methods_count];
    method->name = strdup(name);
    method->function = function;

    if (!method->name)
    {
        log_error("type_register_method: failed to allocate method name");
        return TYPE_INVALID_HANDLE;
    }

    type->methods_count++;

    log_info("Registered method '%s.%s'", type->name, name);

    return (method_handle_t)method;
}

method_handle_t type_get_method(const char* name, type_handle_t type_handle)
{
    assert(name && "name cannot be NULL");
    assert(type_handle && "type_handle cannot be NULL");

    type_t* type = (type_t*)type_handle;

    /* Walk up inheritance chain (child -> parent) */
    while (type != NULL)
    {
        /* Search this type's methods */
        for (size_t i = 0; i < type->methods_count; i++)
        {
            if (strcmp(type->methods[i].name, name) == 0)
            {
                return (method_handle_t)&type->methods[i];
            }
        }

        /* Try parent */
        type = type->parent;
    }

    /* Not found */
    return TYPE_INVALID_HANDLE;
}

void type_set_init(type_handle_t type_handle, void (*init)(struct type_instance_t* self, void* params))
{
    assert(type_handle && "type_handle cannot be NULL");

    type_t* type = (type_t*)type_handle;
    type->init = init;

    log_info("Set init method for type '%s'", type->name);
}

void type_set_destroy(type_handle_t type_handle, void (*destroy)(struct type_instance_t* self))
{
    assert(type_handle && "type_handle cannot be NULL");

    type_t* type = (type_t*)type_handle;
    type->destroy = destroy;

    log_info("Set destroy method for type '%s'", type->name);
}

type_instance_handle_t type_instance_create(type_handle_t type_handle)
{
    assert(type_handle && "type_handle cannot be NULL");

    type_t* type = (type_t*)type_handle;

    /* Allocate instance structure */
    type_instance_t* instance = (type_instance_t*)calloc(1, sizeof(type_instance_t));
    if (!instance)
    {
        log_error("type_instance_create: failed to allocate instance");
        return TYPE_INVALID_HANDLE;
    }

    /* Allocate instance data buffer */
    instance->data = calloc(1, type->total_data_size);
    if (!instance->data)
    {
        log_error("type_instance_create: failed to allocate instance data");
        free(instance);
        return TYPE_INVALID_HANDLE;
    }

    instance->type = type;

    /* Call init function if set (direct call - no string lookup) */
    if (type->init)
    {
        type->init(instance, NULL);
    }

    log_info("Created instance of type '%s' (data_size=%zu)",
             type->name, type->total_data_size);

    return (type_instance_handle_t)instance;
}

void type_instance_destroy(type_instance_handle_t instance_handle)
{
    if (instance_handle == TYPE_INVALID_HANDLE)
    {
        return;
    }

    type_instance_t* instance = (type_instance_t*)instance_handle;

    /* Call destroy function if set (direct call - no string lookup) */
    if (instance->type->destroy)
    {
        instance->type->destroy(instance);
    }

    log_info("Destroying instance of type '%s'", instance->type->name);

    /* Free data buffer */
    free(instance->data);

    /* Free instance structure */
    free(instance);
}

void type_instance_set_property(
    type_instance_handle_t instance_handle,
    property_handle_t property_handle,
    const void* value)
{
    assert(instance_handle && "instance_handle cannot be NULL");
    assert(property_handle && "property_handle cannot be NULL");
    assert(value && "value cannot be NULL");

    type_instance_t* instance = (type_instance_t*)instance_handle;
    property_t* prop = (property_t*)property_handle;

    /* Get destination pointer in instance data */
    uint8_t* dest = (uint8_t*)instance->data + prop->offset;

    /* Copy value */
    memcpy(dest, value, prop->size);
}

void* type_instance_get_property_ptr(
    type_instance_handle_t instance_handle,
    property_handle_t property_handle)
{
    assert(instance_handle && "instance_handle cannot be NULL");
    assert(property_handle && "property_handle cannot be NULL");

    type_instance_t* instance = (type_instance_t*)instance_handle;
    property_t* prop = (property_t*)property_handle;

    /* Return pointer to property in instance data */
    return (uint8_t*)instance->data + prop->offset;
}

void type_instance_call_method(
    type_instance_handle_t instance_handle,
    method_handle_t method_handle,
    const void* args,
    void* result)
{
    assert(instance_handle && "instance_handle cannot be NULL");
    assert(method_handle && "method_handle cannot be NULL");

    type_instance_t* instance = (type_instance_t*)instance_handle;
    method_t* method = (method_t*)method_handle;

    /* Call the method function */
    if (method->function)
    {
        method->function(instance, (void*)args, result);
    }
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static type_t* type_registry_find_by_name(const char* name)
{
    assert(name && "name cannot be NULL");

    for (size_t i = 0; i < type_count; i++)
    {
        if (strcmp(type_registry[i]->name, name) == 0)
        {
            return type_registry[i];
        }
    }
    return NULL;
}

static void type_registry_add(type_t* type)
{
    assert(type && "type cannot be NULL");

    /* Initialize registry if needed */
    if (type_registry == NULL)
    {
        type_registry = (type_t**)calloc(INITIAL_TYPE_CAPACITY, sizeof(type_t*));
        if (!type_registry)
        {
            log_error("type_registry_add: failed to initialize registry");
            return;
        }
        type_capacity = INITIAL_TYPE_CAPACITY;
        type_count = 0;
    }

    /* Grow if needed */
    if (type_count >= type_capacity)
    {
        size_t new_capacity = type_capacity * 2;
        type_t** new_registry = (type_t**)realloc(type_registry,
                                                   new_capacity * sizeof(type_t*));
        if (!new_registry)
        {
            log_error("type_registry_add: failed to grow registry");
            return;
        }

        /* Zero out new region */
        memset(&new_registry[type_capacity], 0,
               (new_capacity - type_capacity) * sizeof(type_t*));

        type_registry = new_registry;
        type_capacity = new_capacity;
    }

    /* Add type */
    type_registry[type_count++] = type;
}
