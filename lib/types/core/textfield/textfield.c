/***************************************************************
**
** libcad Source File
**
** File         :  textfield.c
** Module       :  core/textfield
** Author       :  SH
** Created      :  2026-02-13 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad core textfield type
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <jansson.h>
#include <util/log/log.h>

#include "textfield.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static type_handle_t textfield_type_handle = TYPE_INVALID_HANDLE;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/* Lifecycle wrappers for type system */
static void textfield_init_wrapper(type_instance_t* self, void* params);
static void textfield_destroy_wrapper(type_instance_t* self);

/* Method wrappers for type system */
static void textfield_debug_print_method(type_instance_t* self, void* args, void* result);
static void textfield_encode_to_json_method(type_instance_t* self, void* args, void* result);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

/* Normal C API - fast path, use these for everyday code */

textfield_t* textfield_create(void)
{
    textfield_t* field = (textfield_t*)calloc(1, sizeof(textfield_t));
    if (!field)
    {
        log_error("textfield_create: failed to allocate textfield");
        return NULL;
    }

    /* Initialize base object part */
    field->base.type = textfield_get_type_handle();
    field->base.name = NULL;

    /* Initialize textfield-specific fields */
    field->text = NULL;
    field->x = 0.0f;
    field->y = 0.0f;
    field->font_size = 12.0f;

    log_info("Created textfield");
    return field;
}

void textfield_destroy(textfield_t* field)
{
    if (!field) return;

    /* Clean up base object part */
    free(field->base.name);

    /* Clean up textfield-specific fields */
    free(field->text);

    free(field);

    log_info("Destroyed textfield");
}

void textfield_set_text(textfield_t* field, const char* text)
{
    if (!field) return;

    free(field->text);
    field->text = text ? strdup(text) : NULL;
}

const char* textfield_get_text(const textfield_t* field)
{
    return field ? field->text : NULL;
}

void textfield_set_position(textfield_t* field, float x, float y)
{
    if (!field) return;
    field->x = x;
    field->y = y;
}

void textfield_set_font_size(textfield_t* field, float size)
{
    if (!field) return;
    field->font_size = size;
}

void textfield_debug_print(const textfield_t* field)
{
    if (!field) return;

    log_info("TextField: name='%s', text='%s', pos=(%.1f, %.1f), font_size=%.1f",
             field->base.name ? field->base.name : "(null)",
             field->text ? field->text : "(null)",
             field->x, field->y,
             field->font_size);
}

json_t* textfield_encode_to_json(const textfield_t* field)
{
    if (!field) return json_null();

    json_t* json = json_object();

    /* Add type information */
    json_object_set_new(json, "type", json_string("textfield"));

    /* Add base object properties */
    json_object_set_new(json, "name", field->base.name ? json_string(field->base.name) : json_null());

    /* Add textfield-specific properties */
    json_object_set_new(json, "text", field->text ? json_string(field->text) : json_null());
    json_object_set_new(json, "x", json_real(field->x));
    json_object_set_new(json, "y", json_real(field->y));
    json_object_set_new(json, "font_size", json_real(field->font_size));

    return json;
}

/* Type system registration - metadata layer */

void textfield_register_type(void)
{
    if (textfield_type_handle != TYPE_INVALID_HANDLE)
    {
        log_warning("textfield_register_type: type already registered");
        return;
    }

    /* Register type with metadata system, inheriting from object */
    textfield_type_handle = type_register(
        "textfield",
        object_get_type_handle(),  /* parent type */
        sizeof(textfield_t) - sizeof(object_t)  /* local size = only textfield-specific fields */
    );

    /* Register lifecycle methods */
    type_set_init(textfield_type_handle, textfield_init_wrapper);
    type_set_destroy(textfield_type_handle, textfield_destroy_wrapper);

    /* Register textfield-specific properties (base properties already registered) */
    type_register_property(
        textfield_type_handle,
        "text",
        offsetof(textfield_t, text),
        sizeof(char*)
    );

    type_register_property(
        textfield_type_handle,
        "x",
        offsetof(textfield_t, x),
        sizeof(float)
    );

    type_register_property(
        textfield_type_handle,
        "y",
        offsetof(textfield_t, y),
        sizeof(float)
    );

    type_register_property(
        textfield_type_handle,
        "font_size",
        offsetof(textfield_t, font_size),
        sizeof(float)
    );

    /* Override base methods for polymorphism */
    type_register_method(
        textfield_type_handle,
        "debug_print",
        textfield_debug_print_method
    );

    type_register_method(
        textfield_type_handle,
        "encode_to_json",
        textfield_encode_to_json_method
    );

    log_info("Registered textfield type (inherits from object)");
}

type_handle_t textfield_get_type_handle(void)
{
    return textfield_type_handle;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

/* These wrappers bridge the type system to the normal C API */

static void textfield_init_wrapper(type_instance_t* self, void* params)
{
    /* Type system allocated the memory, just initialize it */
    textfield_t* field = (textfield_t*)self->data;
    field->base.name = NULL;
    field->text = NULL;
    field->x = 0.0f;
    field->y = 0.0f;
    field->font_size = 12.0f;
}

static void textfield_destroy_wrapper(type_instance_t* self)
{
    /* Clean up textfield data (type system will free the memory) */
    textfield_t* field = (textfield_t*)self->data;
    free(field->base.name);
    free(field->text);
}

static void textfield_debug_print_method(type_instance_t* self, void* args, void* result)
{
    /* Bridge to the normal C function */
    textfield_t* field = (textfield_t*)self->data;
    textfield_debug_print(field);
}

static void textfield_encode_to_json_method(type_instance_t* self, void* args, void* result)
{
    /* Bridge to the normal C function */
    textfield_t* field = (textfield_t*)self->data;
    json_t** json_result = (json_t**)result;
    if (json_result) {
        *json_result = textfield_encode_to_json(field);
    }
}
