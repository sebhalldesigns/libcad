/***************************************************************
**
** libcad Source File
**
** File         :  object.c
** Module       :  core/object
** Author       :  SH
** Created      :  2026-02-13 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad core object type
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <util/log/log.h>

#include "object.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static type_handle_t object_type_handle = TYPE_INVALID_HANDLE;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/* Lifecycle wrappers for type system */
static void object_init_wrapper(type_instance_t* self, void* params);
static void object_destroy_wrapper(type_instance_t* self);

/* Method wrappers for type system */
static void object_debug_print_method(type_instance_t* self, void* args, void* result);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

/* Normal C API - fast path, use these for everyday code */

object_t* object_create(void)
{
    object_t* obj = (object_t*)calloc(1, sizeof(object_t));
    if (!obj)
    {
        log_error("object_create: failed to allocate object");
        return NULL;
    }

    obj->name = NULL;

    log_info("Created object");
    return obj;
}

void object_destroy(object_t* obj)
{
    if (!obj) return;

    free(obj->name);
    free(obj);

    log_info("Destroyed object");
}

void object_set_name(object_t* obj, const char* name)
{
    if (!obj) return;

    free(obj->name);
    obj->name = name ? strdup(name) : NULL;
}

const char* object_get_name(const object_t* obj)
{
    return obj ? obj->name : NULL;
}

void object_debug_print(const object_t* obj)
{
    if (!obj) return;

    log_info("Object: name='%s'", obj->name ? obj->name : "(null)");
}

/* Type system registration - metadata layer */

void object_register_type(void)
{
    if (object_type_handle != TYPE_INVALID_HANDLE)
    {
        log_warning("object_register_type: type already registered");
        return;
    }

    /* Register type with metadata system */
    object_type_handle = type_register(
        "object",
        TYPE_INVALID_HANDLE,  /* no parent */
        sizeof(object_t)
    );

    /* Register lifecycle methods */
    type_set_init(object_type_handle, object_init_wrapper);
    type_set_destroy(object_type_handle, object_destroy_wrapper);

    /* Register properties for serialization/introspection */
    type_register_property(
        object_type_handle,
        "name",
        offsetof(object_t, name),
        sizeof(char*)
    );

    /* Register methods for dynamic dispatch/polymorphism */
    type_register_method(
        object_type_handle,
        "debug_print",
        object_debug_print_method
    );

    log_info("Registered object type");
}

type_handle_t object_get_type_handle(void)
{
    return object_type_handle;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

/* These wrappers bridge the type system to the normal C API */

static void object_init_wrapper(type_instance_t* self, void* params)
{
    /* Type system allocated the memory, just initialize it */
    object_t* obj = (object_t*)self->data;
    obj->name = NULL;
}

static void object_destroy_wrapper(type_instance_t* self)
{
    /* Clean up object data (type system will free the memory) */
    object_t* obj = (object_t*)self->data;
    free(obj->name);
}

static void object_debug_print_method(type_instance_t* self, void* args, void* result)
{
    /* Bridge to the normal C function */
    object_t* obj = (object_t*)self->data;
    object_debug_print(obj);
}