/***************************************************************
**
** libcad Header File
**
** File         :  document.h
** Module       :  core/document
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad core document type
**
***************************************************************/

#ifndef LIBCAD_CORE_DOCUMENT_H
#define LIBCAD_CORE_DOCUMENT_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <types/core/object/object.h>

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

typedef struct document_t document_t;
typedef struct document_class_t document_class_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

/*
** document_t instance - inherits from object_t
**
** IMPORTANT: First field MUST be parent (object_t parent)
**            This enables safe upcasting: document_t* -> object_t*
*/
typedef struct document_t {
    object_t parent;  /* Inherit from object_t (MUST be first!) */

    /* document_t-specific instance data */
    char* path;

    /* Children */
    object_t** children;
    size_t children_count;
    size_t children_capacity;
} document_t;

/*
** document_t class - inherits from object_class_t
**
** Extends parent vtable with new virtual methods
*/
typedef struct document_class_t {
    object_class_t parent_class;  /* Inherit parent vtable */

    /* New virtual methods specific to document_t */
    void (*add_child)(document_t* self, object_t* child);
    void (*remove_child)(document_t* self, size_t index);
    bool (*save)(document_t* self, const char* filepath);
} document_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

type_handle_t document_get_type(void);
document_class_t* document_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

document_t* document_new(void);
void document_free(document_t* self);

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

/* Property accessors */
void document_set_path(document_t* self, const char* path);
const char* document_get_path(const document_t* self);

/* Methods */
void document_add_child(document_t* self, object_t* child);
void document_remove_child(document_t* self, size_t index);
object_t* document_get_child(const document_t* self, size_t index);
size_t document_get_child_count(const document_t* self);

bool document_save(const document_t* self, const char* filepath);
document_t* document_load(const char* filepath);

/* Virtual method implementations */
void document_debug_print(document_t* self);
json_t* document_to_json(document_t* self);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

/* Casting */
#define DOCUMENT(obj) ((document_t*)obj)
#define DOCUMENT_CLASS(cls) ((document_class_t*)cls)
#define IS_DOCUMENT(obj) (obj && type_is_a((type_handle_t)LIBCAD_GET_CLASS(obj), document_get_type()))

/* Safe upcast to object_t */
#define DOCUMENT_AS_OBJECT(doc) ((object_t*)(doc))

/* Polymorphic calls */
#define DOCUMENT_ADD_CHILD(doc, child) \
    LIBCAD_CALL(doc, add_child, child)

#define DOCUMENT_SAVE(doc, path) \
    LIBCAD_CALL(doc, save, path)

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_CORE_DOCUMENT_H */
