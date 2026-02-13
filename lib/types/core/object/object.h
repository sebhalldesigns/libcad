/***************************************************************
**
** libcad Header File
**
** File         :  object.h
** Module       :  core/object
** Author       :  SH
** Created      :  2026-02-13 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad core object type
**
***************************************************************/

#ifndef LIBCAD_CORE_OBJECT_H
#define LIBCAD_CORE_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdbool.h>

#include <model/type/type.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* Base object struct - all document model types inherit from this */
typedef struct object_t {
    char* name;
} object_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* Normal C API - use these for everyday code */
object_t* object_create(void);
void object_destroy(object_t* obj);
void object_set_name(object_t* obj, const char* name);
const char* object_get_name(const object_t* obj);
void object_debug_print(const object_t* obj);

/* Type system registration - called once at startup */
void object_register_type(void);

/* Get type handle - used for reflection/serialization */
type_handle_t object_get_type_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_CORE_OBJECT_H */