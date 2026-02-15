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
#include <util/vector/vector.h>

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
    cls->add_child = object_add_child;
    cls->remove_child = object_remove_child;

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
    self->visible = true;

    /* Initialize tree structure */
    self->children = NULL;
    self->children_count = 0;
    self->children_capacity = 0;

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

    /* Free children array (but not the children themselves - caller owns them) */
    VECTOR_FREE(self->children);
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

void object_set_visible(object_t* self, bool visible)
{
    if (!self) return;
    self->visible = visible;
}

bool object_is_visible(const object_t* self)
{
    return self ? self->visible : false;
}

/***************************************************************
** MARK: PUBLIC API - Tree Management
***************************************************************/

void object_add_child(object_t* self, object_t* child)
{
    if (!self || !child) return;

    VECTOR_PUSH(self->children, self->children_count, self->children_capacity, object_t*, child);
}

void object_remove_child(object_t* self, size_t index)
{
    if (!self || index >= self->children_count) return;

    VECTOR_REMOVE(self->children, self->children_count, index);
}

object_t* object_get_child(const object_t* self, size_t index)
{
    if (!self) return NULL;
    return VECTOR_GET(self->children, self->children_count, index);
}

size_t object_get_child_count(const object_t* self)
{
    return self ? self->children_count : 0;
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

    log_info("object_t: name='%s', visible=%s",
             self->name ? self->name : "(null)",
             self->visible ? "true" : "false");
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
    json_object_set_new(json, "visible", json_boolean(self->visible));
    json_object_set_new(json, "object_id", json_integer((json_int_t)(uintptr_t)self));

    /* Add children */
    json_t* children_array = json_array();
    for (size_t i = 0; i < self->children_count; i++) {
        /* Polymorphic call - each child serializes itself */
        json_t* child_json = OBJECT_TO_JSON(self->children[i]);
        json_array_append_new(children_array, child_json);
    }
    json_object_set_new(json, "children", children_array);

    return json;
}
