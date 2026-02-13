/***************************************************************
**
** libcad Header File
**
** File         :  textfield.h
** Module       :  core/textfield
** Author       :  SH
** Created      :  2026-02-13 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad core textfield type
**
***************************************************************/

#ifndef LIBCAD_CORE_TEXTFIELD_H
#define LIBCAD_CORE_TEXTFIELD_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdbool.h>

#include <model/type/type.h>
#include <types/core/object/object.h>

#include <libcad/libcad.h>

#include <jansson.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* TextField inherits from object - embeds object_t as first member */
typedef struct textfield_t {
    object_t base;  /* MUST be first - allows safe upcasting */
    char* text;
    float x, y;     /* Position */
    float font_size;
} textfield_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* Normal C API - use these for everyday code */
EXPORT textfield_t* textfield_create(void);
EXPORT void textfield_destroy(textfield_t* field);
EXPORT void textfield_set_text(textfield_t* field, const char* text);
EXPORT const char* textfield_get_text(const textfield_t* field);
EXPORT void textfield_set_position(textfield_t* field, float x, float y);
EXPORT void textfield_set_font_size(textfield_t* field, float size);
EXPORT void textfield_debug_print(const textfield_t* field);

/* Serialization */
EXPORT json_t* textfield_encode_to_json(const textfield_t* field);

/* Upcast to base type (always safe because base is first member) */
static inline object_t* textfield_as_object(textfield_t* field) {
    return (object_t*)field;
}

/* Type system registration - called once at startup */
EXPORT void textfield_register_type(void);

/* Get type handle - used for reflection/serialization */
EXPORT type_handle_t textfield_get_type_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_CORE_TEXTFIELD_H */
