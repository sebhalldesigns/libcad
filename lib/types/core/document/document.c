/***************************************************************
**
** libcad Source File
**
** File         :  document.c
** Module       :  core/document
** Author       :  SH
** Created      :  2026-02-13 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad core document type
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

#include "document.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define INITIAL_CHILDREN_CAPACITY 8

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static type_handle_t document_type_handle = TYPE_INVALID_HANDLE;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/* Lifecycle wrappers for type system */
static void document_init_wrapper(type_instance_t* self, void* params);
static void document_destroy_wrapper(type_instance_t* self);

/* Method wrappers for type system */
static void document_debug_print_method(type_instance_t* self, void* args, void* result);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

/* Normal C API - fast path, use these for everyday code */

document_t* document_create(void)
{
    document_t* doc = (document_t*)calloc(1, sizeof(document_t));
    if (!doc)
    {
        log_error("document_create: failed to allocate document");
        return NULL;
    }

    /* Initialize base object part */
    doc->base.type = document_get_type_handle();
    doc->base.name = NULL;

    /* Initialize document-specific fields */
    doc->path = NULL;

    /* Initialize children array */
    doc->children = (object_t**)calloc(INITIAL_CHILDREN_CAPACITY, sizeof(object_t*));
    doc->children_count = 0;
    doc->children_capacity = INITIAL_CHILDREN_CAPACITY;

    if (!doc->children)
    {
        log_error("document_create: failed to allocate children array");
        free(doc);
        return NULL;
    }

    log_info("Created document");
    return doc;
}

void document_destroy(document_t* doc)
{
    if (!doc) return;

    /* Clean up base object part */
    free(doc->base.name);

    /* Clean up document-specific fields */
    free(doc->path);

    /* Note: We don't destroy children here - they are owned by the caller */
    /* Just free the children array itself */
    free(doc->children);

    free(doc);

    log_info("Destroyed document");
}

void document_set_path(document_t* doc, const char* path)
{
    if (!doc) return;

    free(doc->path);
    doc->path = path ? strdup(path) : NULL;
}

const char* document_get_path(const document_t* doc)
{
    return doc ? doc->path : NULL;
}

void document_debug_print(const document_t* doc)
{
    if (!doc) return;

    log_info("Document: name='%s', path='%s', children=%zu",
             doc->base.name ? doc->base.name : "(null)",
             doc->path ? doc->path : "(null)",
             doc->children_count);
}

/* Children management */

void document_add_child(document_t* doc, object_t* child)
{
    if (!doc || !child) return;

    /* Grow array if needed */
    if (doc->children_count >= doc->children_capacity)
    {
        size_t new_capacity = doc->children_capacity * 2;
        object_t** new_children = (object_t**)realloc(doc->children, new_capacity * sizeof(object_t*));
        if (!new_children)
        {
            log_error("document_add_child: failed to grow children array");
            return;
        }

        /* Zero out new region */
        memset(&new_children[doc->children_capacity], 0,
               (new_capacity - doc->children_capacity) * sizeof(object_t*));

        doc->children = new_children;
        doc->children_capacity = new_capacity;
    }

    /* Add child */
    doc->children[doc->children_count++] = child;
    log_info("Added child to document (count now %zu)", doc->children_count);
}

void document_remove_child(document_t* doc, size_t index)
{
    if (!doc || index >= doc->children_count) return;

    /* Shift remaining children down */
    for (size_t i = index; i < doc->children_count - 1; i++)
    {
        doc->children[i] = doc->children[i + 1];
    }

    doc->children_count--;
    doc->children[doc->children_count] = NULL;

    log_info("Removed child from document (count now %zu)", doc->children_count);
}

object_t* document_get_child(const document_t* doc, size_t index)
{
    if (!doc || index >= doc->children_count) return NULL;
    return doc->children[index];
}

size_t document_get_child_count(const document_t* doc)
{
    return doc ? doc->children_count : 0;
}

/* Serialization */

bool document_save(const document_t* doc, const char* filepath)
{
    if (!doc || !filepath)
    {
        log_error("document_save: invalid arguments");
        return false;
    }

    /* Encode document to JSON */
    json_t* json = json_object();

    /* Add type */
    json_object_set_new(json, "type", json_string("document"));

    /* Add document properties */
    json_object_set_new(json, "name", doc->base.name ? json_string(doc->base.name) : json_null());
    json_object_set_new(json, "path", doc->path ? json_string(doc->path) : json_null());

    /* Add children array */
    json_t* children_array = json_array();
    for (size_t i = 0; i < doc->children_count; i++)
    {
        object_t* child = doc->children[i];
        if (child && child->type)
        {
            /* Get the encode_to_json method for the child's actual type (polymorphic) */
            method_handle_t encode_method = type_get_method("encode_to_json", child->type);
            if (encode_method != TYPE_INVALID_HANDLE)
            {
                /* Create a temporary type_instance wrapper */
                type_instance_t temp_instance;
                temp_instance.type = (type_t*)child->type;
                temp_instance.data = child;

                /* Call the polymorphic method */
                json_t* child_json = NULL;
                type_instance_call_method((type_instance_handle_t)&temp_instance, encode_method, NULL, &child_json);

                if (child_json) {
                    json_array_append_new(children_array, child_json);
                }
            }
        }
    }
    json_object_set_new(json, "children", children_array);

    /* Write to file */
    int result = json_dump_file(json, filepath, JSON_INDENT(2));
    json_decref(json);

    if (result != 0)
    {
        log_error("document_save: failed to write JSON to file '%s'", filepath);
        return false;
    }

    log_info("Saved document to '%s'", filepath);
    return true;
}

document_t* document_load(const char* filepath)
{
    if (!filepath)
    {
        log_error("document_load: invalid filepath");
        return NULL;
    }

    /* Load JSON from file */
    json_error_t error;
    json_t* json = json_load_file(filepath, 0, &error);
    if (!json)
    {
        log_error("document_load: failed to load JSON from '%s': %s", filepath, error.text);
        return NULL;
    }

    /* Verify it's a document */
    json_t* type_json = json_object_get(json, "type");
    const char* type_str = json_string_value(type_json);
    if (!type_str || strcmp(type_str, "document") != 0)
    {
        log_error("document_load: JSON is not a document");
        json_decref(json);
        return NULL;
    }

    /* Create document */
    document_t* doc = document_create();
    if (!doc)
    {
        json_decref(json);
        return NULL;
    }

    /* Load properties */
    json_t* name_json = json_object_get(json, "name");
    if (json_is_string(name_json))
    {
        doc->base.name = strdup(json_string_value(name_json));
    }

    json_t* path_json = json_object_get(json, "path");
    if (json_is_string(path_json))
    {
        doc->path = strdup(json_string_value(path_json));
    }

    /* Load children */
    json_t* children_json = json_object_get(json, "children");
    if (json_is_array(children_json))
    {
        size_t array_size = json_array_size(children_json);
        for (size_t i = 0; i < array_size; i++)
        {
            json_t* child_json = json_array_get(children_json, i);

            /* TODO: Implement polymorphic object loading based on type field */
            /* For now, this is a placeholder - we'll need a registry of type loaders */
            log_warning("document_load: child loading not yet implemented");
        }
    }

    json_decref(json);

    log_info("Loaded document from '%s'", filepath);
    return doc;
}

/* Type system registration - metadata layer */

void document_register_type(void)
{
    if (document_type_handle != TYPE_INVALID_HANDLE)
    {
        log_warning("document_register_type: type already registered");
        return;
    }

    /* Register type with metadata system, inheriting from object */
    document_type_handle = type_register(
        "document",
        object_get_type_handle(),  /* parent type */
        sizeof(document_t) - sizeof(object_t)  /* local size = only document-specific fields */
    );

    /* Register lifecycle methods */
    type_set_init(document_type_handle, document_init_wrapper);
    type_set_destroy(document_type_handle, document_destroy_wrapper);

    /* Register document-specific properties (base properties already registered) */
    type_register_property(
        document_type_handle,
        "path",
        offsetof(document_t, path),  /* global offset from start of struct */
        sizeof(char*)
    );

    /* Override base methods for polymorphism */
    type_register_method(
        document_type_handle,
        "debug_print",
        document_debug_print_method
    );

    log_info("Registered document type (inherits from object)");
}

type_handle_t document_get_type_handle(void)
{
    return document_type_handle;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

/* These wrappers bridge the type system to the normal C API */

static void document_init_wrapper(type_instance_t* self, void* params)
{
    /* Type system allocated the memory, just initialize it */
    document_t* doc = (document_t*)self->data;
    doc->base.name = NULL;
    doc->path = NULL;
}

static void document_destroy_wrapper(type_instance_t* self)
{
    /* Clean up document data (type system will free the memory) */
    document_t* doc = (document_t*)self->data;
    free(doc->base.name);
    free(doc->path);
}

static void document_debug_print_method(type_instance_t* self, void* args, void* result)
{
    /* Bridge to the normal C function */
    document_t* doc = (document_t*)self->data;
    document_debug_print(doc);
}