/***************************************************************
**
** libcad Source File
**
** File         :  document.c
** Module       :  core/document
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad core document type
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <string.h>

#include <util/log/log.h>
#include <util/vector/vector.h>

#include "document.h"
#include <types/core/plane/plane.h>
#include <types/core/axis/axis.h>

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

static void document_finalize(document_t* self);

/***************************************************************
** MARK: TYPE REGISTRATION
**
** Inherit from object_t - this one line handles everything!
***************************************************************/

LIBCAD_DEFINE_TYPE(document, object_get_type())

/***************************************************************
** MARK: CLASS INITIALIZATION
**
** Setup vtable - inherit parent methods and add new ones
***************************************************************/

static void document_class_init(document_class_t* cls)
{
    /* Parent vtable is already copied by type_register_static() */

    /* Override parent methods if needed */
    object_class_t* parent_class = (object_class_t*)cls;
    parent_class->debug_print = (void(*)(object_t*))document_debug_print;
    parent_class->to_json = (json_t*(*)(object_t*))document_to_json;

    /* Set finalizer */
    cls->parent_class.parent_class.instance_finalize = (void(*)(void*))document_finalize;

    /* Set up our virtual methods */
    cls->save = document_save;

    log_info("document_class_t initialized");
}

/***************************************************************
** MARK: INSTANCE INITIALIZATION
**
** Initialize document_t-specific fields
** (object_t parent is auto-initialized by parent's instance_init)
***************************************************************/

static void document_init(document_t* self)
{
    /* Parent (object_t) is already initialized automatically! */

    /* Initialize our fields */
    self->path = NULL;

    /* Create 3 construction planes: XY, XZ, YZ */

    /* XY Plane (horizontal, normal pointing up in +Z direction) - Blue */
    plane_t* xy_plane = plane_new();
    object_set_name(PLANE_AS_OBJECT(xy_plane), "XY Plane");
    vec3 xy_origin = {0.0f, 0.0f, 0.0f};  /* At origin */
    vec3 xy_normal = {0.0f, 0.0f, 1.0f};  /* Normal in +Z */
    plane_set_transform(xy_plane, xy_origin, xy_normal);
    vec4 xy_color = {0.2f, 0.2f, 0.8f, 0.4f};  /* Blue */
    plane_set_display(xy_plane, xy_color, true, 10.0f);
    object_add_child(DOCUMENT_AS_OBJECT(self), PLANE_AS_OBJECT(xy_plane));

    /* XZ Plane (vertical, normal pointing in +Y direction) - Green */
    plane_t* xz_plane = plane_new();
    object_set_name(PLANE_AS_OBJECT(xz_plane), "XZ Plane");
    vec3 xz_origin = {0.0f, 0.0f, 0.0f};  /* At origin */
    vec3 xz_normal = {0.0f, 1.0f, 0.0f};  /* Normal in +Y */
    plane_set_transform(xz_plane, xz_origin, xz_normal);
    vec4 xz_color = {0.2f, 0.8f, 0.2f, 0.4f};  /* Green */
    plane_set_display(xz_plane, xz_color, true, 10.0f);
    object_add_child(DOCUMENT_AS_OBJECT(self), PLANE_AS_OBJECT(xz_plane));

    /* YZ Plane (vertical, normal pointing in +X direction) - Red */
    plane_t* yz_plane = plane_new();
    object_set_name(PLANE_AS_OBJECT(yz_plane), "YZ Plane");
    vec3 yz_origin = {0.0f, 0.0f, 0.0f};  /* At origin */
    vec3 yz_normal = {1.0f, 0.0f, 0.0f};  /* Normal in +X */
    plane_set_transform(yz_plane, yz_origin, yz_normal);
    vec4 yz_color = {0.8f, 0.2f, 0.2f, 0.4f};  /* Red */
    plane_set_display(yz_plane, yz_color, true, 10.0f);
    object_add_child(DOCUMENT_AS_OBJECT(self), PLANE_AS_OBJECT(yz_plane));

    /* Create 3 axes: X, Y, Z */

    /* X Axis (red) */
    axis_t* x_axis = axis_new();
    object_set_name(AXIS_AS_OBJECT(x_axis), "X Axis");
    vec3 x_origin = {0.0f, 0.0f, 0.0f};
    vec3 x_direction = {1.0f, 0.0f, 0.0f};
    axis_set_geometry(x_axis, x_origin, x_direction, 1000.0f);
    vec4 x_color = {1.0f, 0.0f, 0.0f, 1.0f};  /* Red */
    axis_set_display(x_axis, x_color, true, 2.0f, true);
    object_add_child(DOCUMENT_AS_OBJECT(self), AXIS_AS_OBJECT(x_axis));

    /* Y Axis (green) */
    axis_t* y_axis = axis_new();
    object_set_name(AXIS_AS_OBJECT(y_axis), "Y Axis");
    vec3 y_origin = {0.0f, 0.0f, 0.0f};
    vec3 y_direction = {0.0f, 1.0f, 0.0f};
    axis_set_geometry(y_axis, y_origin, y_direction, 1000.0f);
    vec4 y_color = {0.0f, 1.0f, 0.0f, 1.0f};  /* Green */
    axis_set_display(y_axis, y_color, true, 2.0f, true);
    object_add_child(DOCUMENT_AS_OBJECT(self), AXIS_AS_OBJECT(y_axis));

    /* Z Axis (blue) */
    axis_t* z_axis = axis_new();
    object_set_name(AXIS_AS_OBJECT(z_axis), "Z Axis");
    vec3 z_origin = {0.0f, 0.0f, 0.0f};
    vec3 z_direction = {0.0f, 0.0f, 1.0f};
    axis_set_geometry(z_axis, z_origin, z_direction, 1000.0f);
    vec4 z_color = {0.0f, 0.0f, 1.0f, 1.0f};  /* Blue */
    axis_set_display(z_axis, z_color, true, 2.0f, true);
    object_add_child(DOCUMENT_AS_OBJECT(self), AXIS_AS_OBJECT(z_axis));

    log_info("Document initialized with 3 planes and 3 axes");
}

/***************************************************************
** MARK: INSTANCE FINALIZATION
***************************************************************/

static void document_finalize(document_t* self)
{
    /* Clean up document-specific data */
    free(self->path);

    /* Note: Parent finalization (including children cleanup) is called automatically */
}

/***************************************************************
** MARK: PUBLIC API - Property Accessors
***************************************************************/

void document_set_path(document_t* self, const char* path)
{
    if (!self) return;

    free(self->path);
    self->path = path ? strdup(path) : NULL;
}

const char* document_get_path(const document_t* self)
{
    return self ? self->path : NULL;
}

/***************************************************************
** MARK: PUBLIC API - Child Management
**
** Note: These are convenience wrappers around inherited object_t methods
***************************************************************/

void document_add_child(document_t* self, object_t* child)
{
    object_add_child((object_t*)self, child);
}

void document_remove_child(document_t* self, size_t index)
{
    object_remove_child((object_t*)self, index);
}

object_t* document_get_child(const document_t* self, size_t index)
{
    return object_get_child((const object_t*)self, index);
}

size_t document_get_child_count(const document_t* self)
{
    return object_get_child_count((const object_t*)self);
}

/***************************************************************
** MARK: PUBLIC API - Serialization
***************************************************************/

bool document_save(const document_t* self, const char* filepath)
{
    if (!self || !filepath) {
        log_error("document_save: invalid arguments");
        return false;
    }

    /* Get JSON representation (polymorphic call) */
    json_t* json = OBJECT_TO_JSON(DOCUMENT_AS_OBJECT((document_t*)self));
    if (!json) return false;

    /* Write to file */
    int result = json_dump_file(json, filepath, JSON_INDENT(2) | JSON_PRESERVE_ORDER);
    json_decref(json);

    if (result != 0) {
        log_error("document_save: failed to write to file '%s'", filepath);
        return false;
    }

    log_info("Saved document to '%s'", filepath);
    return result == 0;
}

document_t* document_load(const char* filepath)
{
    if (!filepath) {
        log_error("document_load: invalid filepath");
        return NULL;
    }

    /* Load JSON from file */
    json_error_t error;
    json_t* json = json_load_file(filepath, 0, &error);
    if (!json) {
        log_error("Failed to load document: %s", error.text);
        return NULL;
    }

    /* Verify type */
    json_t* type_field = json_object_get(json, "type");
    const char* type_str = json_string_value(type_field);
    if (!type_str || strcmp(type_str, "document_t") != 0) {
        log_error("document_load: JSON is not a document_t");
        json_decref(json);
        return NULL;
    }

    /* Create new document */
    document_t* doc = document_new();
    if (!doc) {
        json_decref(json);
        return NULL;
    }

    /* Parse JSON and populate document */
    json_t* name_field = json_object_get(json, "name");
    json_t* path_field = json_object_get(json, "path");

    if (json_is_string(name_field)) {
        object_set_name(DOCUMENT_AS_OBJECT(doc), json_string_value(name_field));
    }

    if (json_is_string(path_field)) {
        document_set_path(doc, json_string_value(path_field));
    }

    /* Load children */
    json_t* children_field = json_object_get(json, "children");
    if (json_is_array(children_field)) {
        size_t array_size = json_array_size(children_field);
        for (size_t i = 0; i < array_size; i++) {
            json_t* child_json = json_array_get(children_field, i);

            /* TODO: Implement polymorphic object loading based on type field */
            /* For now, this is a placeholder - we'll need a registry of type loaders */
            log_warning("document_load: child loading not yet implemented");
        }
    }

    json_decref(json);

    log_info("Loaded document from '%s'", filepath);
    return doc;
}

/***************************************************************
** MARK: OVERRIDDEN METHODS
**
** These override the object_t implementations
***************************************************************/

void document_debug_print(document_t* self)
{
    if (!self) return;

    /* We can call parent implementation if we want */
    const char* name = object_get_name(DOCUMENT_AS_OBJECT(self));

    log_info("document_t: name='%s', path='%s', children=%zu",
             name ? name : "(null)",
             self->path ? self->path : "(null)",
             object_get_child_count(DOCUMENT_AS_OBJECT(self)));
}

json_t* document_to_json(document_t* self)
{
    if (!self) return json_null();

    /* Start with parent's JSON (includes children) */
    json_t* json = object_to_json(DOCUMENT_AS_OBJECT(self));

    /* Override type field */
    json_object_set_new(json, "type", json_string("document_t"));

    /* Add document-specific fields */
    json_object_set_new(json, "path",
                       self->path ? json_string(self->path) : json_null());

    /* Note: children already serialized by parent's to_json */

    return json;
}
