/***************************************************************
**
** libcad Source File
**
** File         :  type.c
** Module       :  type
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad Type System - Class-based with macros
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

#define INITIAL_TYPE_CAPACITY 32

/***************************************************************
** MARK: TYPE REGISTRY
***************************************************************/

typedef struct {
    type_handle_t handle;
    const char* name;
    type_class_t* cls;
} type_registry_entry_t;

static type_registry_entry_t* type_registry = NULL;
static size_t type_count = 0;
static size_t type_capacity = 0;

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static type_handle_t registry_add(const char* name, type_class_t* cls)
{
    /* Initialize registry if needed */
    if (!type_registry) {
        type_registry = calloc(INITIAL_TYPE_CAPACITY, sizeof(type_registry_entry_t));
        if (!type_registry) {
            log_error("Failed to initialize type registry");
            return TYPE_INVALID;
        }
        type_capacity = INITIAL_TYPE_CAPACITY;
        type_count = 0;
    }

    /* Grow if needed */
    if (type_count >= type_capacity) {
        size_t new_capacity = type_capacity * 2;
        type_registry_entry_t* new_registry = realloc(
            type_registry,
            new_capacity * sizeof(type_registry_entry_t)
        );
        if (!new_registry) {
            log_error("Failed to grow type registry");
            return TYPE_INVALID;
        }
        memset(
            &new_registry[type_capacity],
            0,
            (new_capacity - type_capacity) * sizeof(type_registry_entry_t)
        );
        type_registry = new_registry;
        type_capacity = new_capacity;
    }

    /* Add entry */
    type_handle_t handle = (type_handle_t)cls;
    type_registry[type_count].handle = handle;
    type_registry[type_count].name = name;
    type_registry[type_count].cls = cls;
    type_count++;

    return handle;
}

static type_class_t* registry_find(const char* name)
{
    for (size_t i = 0; i < type_count; i++) {
        if (strcmp(type_registry[i].name, name) == 0) {
            return type_registry[i].cls;
        }
    }
    return NULL;
}

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

type_handle_t type_register_static(
    const char* type_name,
    type_handle_t parent_type,
    size_t class_size,
    void (*class_init)(void* cls),
    size_t instance_size,
    void (*instance_init)(void* instance)
)
{
    assert(type_name && "type_name cannot be NULL");
    assert(class_size >= sizeof(type_class_t) && "class_size too small");
    assert(instance_size > 0 && "instance_size must be > 0");

    /* Check if already registered */
    type_class_t* existing = registry_find(type_name);
    if (existing) {
        log_warning("Type '%s' already registered", type_name);
        return (type_handle_t)existing;
    }

    /* Allocate class structure */
    type_class_t* cls = calloc(1, class_size);
    if (!cls) {
        log_error("Failed to allocate class for type '%s'", type_name);
        return TYPE_INVALID;
    }

    /* Initialize base class fields */
    cls->type_name = type_name;
    cls->parent_type = parent_type;
    cls->instance_size = instance_size;
    cls->instance_init = instance_init;
    cls->instance_finalize = NULL; /* Set by subclass if needed */

    /* Copy parent class vtable if we have a parent */
    if (parent_type != TYPE_INVALID) {
        type_class_t* parent_class = (type_class_t*)parent_type;
        if (parent_class) {
            /* Copy parent vtable entries (everything after base type_class_t) */
            size_t parent_size = parent_class->instance_size;
            if (class_size > sizeof(type_class_t) && parent_size >= sizeof(type_class_t)) {
                /* Copy parent's vtable section */
                memcpy(
                    (char*)cls + sizeof(type_class_t),
                    (char*)parent_class + sizeof(type_class_t),
                    (class_size > parent_size ? parent_size : class_size) - sizeof(type_class_t)
                );
            }
        }
    }

    /* Call class initializer to setup vtable and class data */
    if (class_init) {
        class_init(cls);
    }

    /* Register in global registry */
    type_handle_t handle = registry_add(type_name, cls);

    log_info("Registered type '%s' (class_size=%zu, instance_size=%zu, parent=%s)",
             type_name, class_size, instance_size,
             parent_type != TYPE_INVALID ? ((type_class_t*)parent_type)->type_name : "none");

    return handle;
}

type_handle_t type_from_name(const char* name)
{
    assert(name && "name cannot be NULL");

    type_class_t* cls = registry_find(name);
    return cls ? (type_handle_t)cls : TYPE_INVALID;
}

type_class_t* type_class_peek(type_handle_t type)
{
    return (type_class_t*)type;
}

const char* type_name(type_handle_t type)
{
    if (type == TYPE_INVALID) {
        return NULL;
    }
    type_class_t* cls = (type_class_t*)type;
    return cls->type_name;
}

bool type_is_a(type_handle_t type, type_handle_t ancestor)
{
    if (type == TYPE_INVALID || ancestor == TYPE_INVALID) {
        return false;
    }

    type_class_t* cls = (type_class_t*)type;

    /* Walk up the inheritance chain */
    while (cls) {
        if ((type_handle_t)cls == ancestor) {
            return true;
        }
        if (cls->parent_type == TYPE_INVALID) {
            break;
        }
        cls = (type_class_t*)cls->parent_type;
    }

    return false;
}

void* type_instance_new(type_handle_t type)
{
    if (type == TYPE_INVALID) {
        log_error("Cannot create instance of TYPE_INVALID");
        return NULL;
    }

    type_class_t* cls = (type_class_t*)type;

    /* Allocate instance */
    void* instance = calloc(1, cls->instance_size);
    if (!instance) {
        log_error("Failed to allocate instance of type '%s'", cls->type_name);
        return NULL;
    }

    /* Set class pointer as first field */
    *(type_class_t**)instance = cls;

    /* Call instance initializer chain (from base to derived) */
    /* Build initialization chain */
    type_class_t* chain[32]; /* Max inheritance depth */
    int depth = 0;
    type_class_t* current = cls;
    while (current && depth < 32) {
        chain[depth++] = current;
        current = (current->parent_type != TYPE_INVALID)
                  ? (type_class_t*)current->parent_type
                  : NULL;
    }

    /* Call initializers from base to derived */
    for (int i = depth - 1; i >= 0; i--) {
        if (chain[i]->instance_init) {
            chain[i]->instance_init(instance);
        }
    }

    log_info("Created instance of type '%s' (size=%zu)",
             cls->type_name, cls->instance_size);

    return instance;
}

void type_instance_free(void* instance)
{
    if (!instance) {
        return;
    }

    type_class_t* cls = *(type_class_t**)instance;
    if (!cls) {
        log_error("Instance has NULL class pointer");
        free(instance);
        return;
    }

    /* Call finalizer chain (from derived to base) */
    type_class_t* current = cls;
    while (current) {
        if (current->instance_finalize) {
            current->instance_finalize(instance);
        }
        current = (current->parent_type != TYPE_INVALID)
                  ? (type_class_t*)current->parent_type
                  : NULL;
    }

    log_info("Destroying instance of type '%s'", cls->type_name);

    free(instance);
}
