/***************************************************************
**
** libcad Header File
**
** File         :  object.h
** Module       :  object
** Author       :  SH
** Created      :  2026-02-13 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad Object API
**
***************************************************************/

#ifndef LIBCAD_OBJECT_H
#define LIBCAD_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdint.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define OBJECT_INVALID_HANDLE (0U)

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/


typedef uintptr_t type_handle_t;
typedef uintptr_t property_handle_t;
typedef uintptr_t method_handle_t;

typedef uintptr_t object_handle_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

type_handle_t object_get_type(const char* name);
property_handle_t object_get_property(const char* name, type_handle_t type_handle);
method_handle_t object_get_method(const char* name, type_handle_t type_handle);

object_handle_t object_create(type_handle_t type_handle);
void object_set_property(object_handle_t object_handle, property_handle_t property_handle, const void* value);

void object_call_method(object_handle_t object_handle, method_handle_t method_handle, const void* args, void* result);

/* Type registration */
type_handle_t object_register_type(const char* name, type_handle_t parent_type_handle, size_t local_data_size);

/* Property registration */
property_handle_t object_register_property(type_handle_t type_handle, const char* name, size_t offset, size_t size);

/* Method registration - forward declare object_t for signature */
struct object_t;
method_handle_t object_register_method(type_handle_t type_handle, const char* name,
                                       void (*function)(struct object_t* self, void* args, void* result));

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_OBJECT_H */