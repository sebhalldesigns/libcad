/***************************************************************
**
** libcad Header File
**
** File         :  textfield.h
** Module       :  core/textfield
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
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

#include <types/core/object/object.h>

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

typedef struct text_field_t text_field_t;
typedef struct text_field_class_t text_field_class_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

/*
** text_field_t instance - inherits from object_t
**
** IMPORTANT: First field MUST be parent (object_t parent)
**            This enables safe upcasting: text_field_t* -> object_t*
*/
typedef struct text_field_t {
    object_t parent;  /* Inherit from object_t (MUST be first!) */

    /* text_field_t-specific instance data */
    char* text;
    float x, y;      /* Position */
    float font_size;
} text_field_t;

/*
** text_field_t class - inherits from object_class_t
**
** Extends parent vtable with new virtual methods
*/
typedef struct text_field_class_t {
    object_class_t parent_class;  /* Inherit parent vtable */

    /* New virtual methods specific to text_field_t */
    void (*set_position)(text_field_t* self, float x, float y);
} text_field_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

type_handle_t text_field_get_type(void);
text_field_class_t* text_field_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

text_field_t* text_field_new(void);
void text_field_free(text_field_t* self);

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

/* Property accessors */
void text_field_set_text(text_field_t* self, const char* text);
const char* text_field_get_text(const text_field_t* self);
void text_field_set_position(text_field_t* self, float x, float y);
void text_field_set_font_size(text_field_t* self, float size);

/* Methods (virtual - can be overridden) */
void text_field_debug_print(text_field_t* self);
json_t* text_field_to_json(text_field_t* self);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

/* Casting */
#define TEXT_FIELD(obj) ((text_field_t*)obj)
#define TEXT_FIELD_CLASS(cls) ((text_field_class_t*)cls)
#define IS_TEXT_FIELD(obj) (obj && type_is_a((type_handle_t)LIBCAD_GET_CLASS(obj), text_field_get_type()))

/* Safe upcast to object_t */
#define TEXT_FIELD_AS_OBJECT(field) ((object_t*)(field))

/* Polymorphic calls */
#define TEXT_FIELD_SET_POSITION(field, x, y) \
    LIBCAD_CALL(field, set_position, x, y)

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_CORE_TEXTFIELD_H */
