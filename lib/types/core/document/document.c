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

    /* Set up our new virtual methods */
    cls->add_child = document_add_child;
    cls->remove_child = document_remove_child;
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
    self->children = NULL;
    self->children_count = 0;
    self->children_capacity = 0;
}

/***************************************************************
** MARK: INSTANCE FINALIZATION
***************************************************************/

static void document_finalize(document_t* self)
{
    /* Clean up document-specific data */
    free(self->path);

    /* Free children array (but not the children themselves - they're owned by caller) */
    VECTOR_FREE(self->children);

    /* Note: Parent finalization is called automatically */
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
***************************************************************/

void document_add_child(document_t* self, object_t* child)
{
    if (!self || !child) return;

    VECTOR_PUSH(self->children, self->children_count, self->children_capacity, object_t*, child);
    log_info("Added child to document (count now %zu)", self->children_count);
}

void document_remove_child(document_t* self, size_t index)
{
    if (!self || index >= self->children_count) return;

    VECTOR_REMOVE(self->children, self->children_count, index);
    log_info("Removed child from document (count now %zu)", self->children_count);
}

object_t* document_get_child(const document_t* self, size_t index)
{
    if (!self) return NULL;
    return VECTOR_GET(self->children, self->children_count, index);
}

size_t document_get_child_count(const document_t* self)
{
    return self ? self->children_count : 0;
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
             self->children_count);
}

json_t* document_to_json(document_t* self)
{
    if (!self) return json_null();

    /* Start with parent's JSON */
    json_t* json = object_to_json(DOCUMENT_AS_OBJECT(self));

    /* Override type field */
    json_object_set_new(json, "type", json_string("document_t"));

    /* Add document-specific fields */
    json_object_set_new(json, "path",
                       self->path ? json_string(self->path) : json_null());

    /* Add children */
    json_t* children_array = json_array();
    for (size_t i = 0; i < self->children_count; i++) {
        /* Polymorphic call - each child serializes itself! */
        json_t* child_json = OBJECT_TO_JSON(self->children[i]);
        json_array_append_new(children_array, child_json);
    }
    json_object_set_new(json, "children", children_array);

    return json;
}
