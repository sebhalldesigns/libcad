/***************************************************************
**
** libcad Source File
**
** File         :  textfield.c
** Module       :  core/textfield
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad core textfield type
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <string.h>

#include <util/log/log.h>

#include "textfield.h"

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

static void text_field_finalize(text_field_t* self);

/***************************************************************
** MARK: TYPE REGISTRATION
**
** Inherit from object_t - this one line handles everything!
***************************************************************/

LIBCAD_DEFINE_TYPE(text_field, object_get_type())

/***************************************************************
** MARK: CLASS INITIALIZATION
**
** Setup vtable - inherit parent methods and add new ones
***************************************************************/

static void text_field_class_init(text_field_class_t* cls)
{
    /* Parent vtable is already copied by type_register_static() */

    /* Override parent methods if needed */
    object_class_t* parent_class = (object_class_t*)cls;
    parent_class->debug_print = (void(*)(object_t*))text_field_debug_print;
    parent_class->to_json = (json_t*(*)(object_t*))text_field_to_json;

    /* Set finalizer */
    cls->parent_class.parent_class.instance_finalize = (void(*)(void*))text_field_finalize;

    /* Set up our new virtual methods */
    cls->set_position = text_field_set_position;

    log_info("text_field_class_t initialized");
}

/***************************************************************
** MARK: INSTANCE INITIALIZATION
**
** Initialize text_field_t-specific fields
** (object_t parent is auto-initialized by parent's instance_init)
***************************************************************/

static void text_field_init(text_field_t* self)
{
    /* Parent (object_t) is already initialized automatically! */

    /* Initialize our fields */
    self->text = NULL;
    self->x = 0.0f;
    self->y = 0.0f;
    self->font_size = 12.0f;
}

/***************************************************************
** MARK: INSTANCE FINALIZATION
***************************************************************/

static void text_field_finalize(text_field_t* self)
{
    /* Clean up textfield-specific data */
    free(self->text);

    /* Note: Parent finalization is called automatically */
}

/***************************************************************
** MARK: PUBLIC API - Property Accessors
***************************************************************/

void text_field_set_text(text_field_t* self, const char* text)
{
    if (!self) return;

    free(self->text);
    self->text = text ? strdup(text) : NULL;
}

const char* text_field_get_text(const text_field_t* self)
{
    return self ? self->text : NULL;
}

void text_field_set_position(text_field_t* self, float x, float y)
{
    if (!self) return;
    self->x = x;
    self->y = y;
}

void text_field_set_font_size(text_field_t* self, float size)
{
    if (!self) return;
    self->font_size = size;
}

/***************************************************************
** MARK: OVERRIDDEN METHODS
**
** These override the object_t implementations
***************************************************************/

void text_field_debug_print(text_field_t* self)
{
    if (!self) return;

    /* We can call parent implementation if we want */
    const char* name = object_get_name(TEXT_FIELD_AS_OBJECT(self));

    log_info("text_field_t: name='%s', text='%s', pos=(%.1f, %.1f), font_size=%.1f",
             name ? name : "(null)",
             self->text ? self->text : "(null)",
             self->x, self->y,
             self->font_size);
}

json_t* text_field_to_json(text_field_t* self)
{
    if (!self) return json_null();

    /* Start with parent's JSON */
    json_t* json = object_to_json(TEXT_FIELD_AS_OBJECT(self));

    /* Override type field */
    json_object_set_new(json, "type", json_string("text_field_t"));

    /* Add textfield-specific fields */
    json_object_set_new(json, "text",
                       self->text ? json_string(self->text) : json_null());
    json_object_set_new(json, "x", json_real(self->x));
    json_object_set_new(json, "y", json_real(self->y));
    json_object_set_new(json, "font_size", json_real(self->font_size));

    return json;
}
