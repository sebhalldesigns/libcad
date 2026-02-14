/***************************************************************
**
** libcad Source File
**
** File         :  object.c
** Module       :  core/object
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad core object type
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <string.h>

#include <util/log/log.h>

#include "object.h"

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

static void object_finalize(object_t* self);

/***************************************************************
** MARK: TYPE REGISTRATION
**
** This single macro replaces ~50 lines of boilerplate!
***************************************************************/

LIBCAD_DEFINE_TYPE(object, TYPE_INVALID)

/***************************************************************
** MARK: CLASS INITIALIZATION
**
** Called ONCE when the type is first registered.
** Setup vtable and class-level data here.
***************************************************************/

static void object_class_init(object_class_t* cls)
{
    /* Set up vtable - point to default implementations */
    cls->debug_print = object_debug_print;
    cls->to_json = object_to_json;

    /* Set finalizer */
    cls->parent_class.instance_finalize = (void(*)(void*))object_finalize;

    log_info("object_class_t initialized");
}

/***************************************************************
** MARK: INSTANCE INITIALIZATION
**
** Called for EACH new instance created.
** Initialize instance fields to defaults.
***************************************************************/

static void object_init(object_t* self)
{
    /* Initialize instance data */
    self->name = NULL;

    /* Note: No need to set self->cls - that's done automatically
       by type_instance_new() before this is called */
}

/***************************************************************
** MARK: INSTANCE FINALIZATION
**
** Called when instance is destroyed.
** Clean up any allocated resources.
***************************************************************/

static void object_finalize(object_t* self)
{
    /* Clean up instance data */
    free(self->name);
    self->name = NULL;
}

/***************************************************************
** MARK: PUBLIC API - Property Accessors
***************************************************************/

void object_set_name(object_t* self, const char* name)
{
    if (!self) return;

    free(self->name);
    self->name = name ? strdup(name) : NULL;
}

const char* object_get_name(const object_t* self)
{
    return self ? self->name : NULL;
}

/***************************************************************
** MARK: PUBLIC API - Methods (Virtual)
**
** These are the DEFAULT implementations.
** Subclasses can override by setting different function pointers
** in their class_init function.
***************************************************************/

void object_debug_print(object_t* self)
{
    if (!self) return;

    log_info("object_t: name='%s'", self->name ? self->name : "(null)");
}

json_t* object_to_json(object_t* self)
{
    if (!self) return json_null();

    json_t* json = json_object();

    /* Add type information */
    json_object_set_new(json, "type", json_string("object_t"));

    /* Add properties */
    json_object_set_new(json, "name",
                       self->name ? json_string(self->name) : json_null());

    return json;
}
