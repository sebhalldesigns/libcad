/***************************************************************
**
** libcad Header File
**
** File         :  document.h
** Module       :  core/document
** Author       :  SH
** Created      :  2026-02-13 (YYYY-MM-DD)
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

#include <stdbool.h>

#include <model/type/type.h>
#include <types/core/object/object.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* Document inherits from object - embeds object_t as first member */
typedef struct document_t {
    object_t base;  /* MUST be first - allows safe upcasting */
    char* path;
} document_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* Normal C API - use these for everyday code */
document_t* document_create(void);
void document_destroy(document_t* doc);
void document_set_path(document_t* doc, const char* path);
const char* document_get_path(const document_t* doc);
void document_debug_print(const document_t* doc);

/* Upcast to base type (always safe because base is first member) */
static inline object_t* document_as_object(document_t* doc) {
    return (object_t*)doc;
}

/* Type system registration - called once at startup */
void document_register_type(void);

/* Get type handle - used for reflection/serialization */
type_handle_t document_get_type_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_CORE_DOCUMENT_H */