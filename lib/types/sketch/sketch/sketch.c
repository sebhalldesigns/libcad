/***************************************************************
**
** libcad Source File
**
** File         :  sketch.c
** Module       :  sketch/sketch
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad sketch type implementation
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <util/log/log.h>

#include "sketch.h"
#include <types/core/plane/plane.h>
#include <render/vector/vector.h>

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

static void sketch_finalize(sketch_t* self);

/***************************************************************
** MARK: TYPE REGISTRATION
***************************************************************/

LIBCAD_DEFINE_TYPE(sketch, object_get_type())

/***************************************************************
** MARK: CLASS INITIALIZATION
***************************************************************/

static void sketch_class_init(sketch_class_t* cls)
{
    /* Parent vtable is already copied */

    /* Override parent methods */
    object_class_t* parent_class = (object_class_t*)cls;
    parent_class->debug_print = (void(*)(object_t*))sketch_debug_print;
    parent_class->to_json = (json_t*(*)(object_t*))sketch_to_json;

    /* Set finalizer */
    cls->parent_class.parent_class.instance_finalize = (void(*)(void*))sketch_finalize;

    log_info("sketch_class_t initialized");
}

/***************************************************************
** MARK: INSTANCE INITIALIZATION
***************************************************************/

static void sketch_init(sketch_t* self)
{
    /* Parent (object_t) is already initialized */

    /* Initialize sketch-specific fields */
    self->active = false;
    self->reference_plane = NULL;
}

/***************************************************************
** MARK: INSTANCE FINALIZATION
***************************************************************/

static void sketch_finalize(sketch_t* self)
{
    /* No sketch-specific cleanup needed */
    /* Parent finalization (including children) is automatic */
}

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

void sketch_set_active(sketch_t* self, bool active)
{
    if (!self) return;
    self->active = active;
}

bool sketch_is_active(const sketch_t* self)
{
    return self ? self->active : false;
}

void sketch_set_reference_plane(sketch_t* self, plane_t* plane)
{
    if (!self) return;
    self->reference_plane = plane;
}

plane_t* sketch_get_reference_plane(const sketch_t* self)
{
    return self ? self->reference_plane : NULL;
}

uint32_t sketch_get_reference_plane_entity_id(const sketch_t* self)
{
    if (!self || !self->reference_plane) {
        return VECTOR_INVALID_INSTANCE;
    }

    if (self->reference_plane->rectangle_handle == VECTOR_INVALID_INSTANCE) {
        return VECTOR_INVALID_INSTANCE;
    }

    return 0x20000000u | (self->reference_plane->rectangle_handle & 0x0FFFFFFFu);
}

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void sketch_debug_print(sketch_t* self)
{
    if (!self) return;

    const char* name = object_get_name(SKETCH_AS_OBJECT(self));
    const uint32_t plane_entity_id = sketch_get_reference_plane_entity_id(self);

    log_info("sketch_t: name='%s', active=%s, plane=0x%08X, entities=%zu",
             name ? name : "(null)",
             self->active ? "true" : "false",
             plane_entity_id,
             object_get_child_count(SKETCH_AS_OBJECT(self)));
}

json_t* sketch_to_json(sketch_t* self)
{
    if (!self) return json_null();

    /* Start with parent's JSON (includes children) */
    json_t* json = object_to_json(SKETCH_AS_OBJECT(self));

    /* Override type field */
    json_object_set_new(json, "type", json_string("sketch_t"));

    /* Add sketch-specific fields */
    json_object_set_new(json, "active", json_boolean(self->active));
    json_object_set_new(
        json,
        "reference_plane_entity_id",
        json_integer((json_int_t)sketch_get_reference_plane_entity_id(self))
    );

    return json;
}
