/***************************************************************
**
** libcad Header File
**
** File         :  type.h
** Module       :  type
** Author       :  SH
** Created      :  2026-02-13 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad Type System API
**
***************************************************************/

#ifndef LIBCAD_TYPE_H
#define LIBCAD_TYPE_H

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

#define TYPE_INVALID_HANDLE (0U)

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef uintptr_t type_handle_t;
typedef uintptr_t property_handle_t;
typedef uintptr_t method_handle_t;

typedef uintptr_t type_instance_handle_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* Type lookup */
type_handle_t type_get(const char* name);

/* Property lookup */
property_handle_t type_get_property(const char* name, type_handle_t type_handle);

/* Method lookup */
method_handle_t type_get_method(const char* name, type_handle_t type_handle);

/* Type instance creation and manipulation */
type_instance_handle_t type_instance_create(type_handle_t type_handle);
void type_instance_set_property(type_instance_handle_t instance_handle, property_handle_t property_handle, const void* value);
void type_instance_call_method(type_instance_handle_t instance_handle, method_handle_t method_handle, const void* args, void* result);

/* Type registration */
type_handle_t type_register(const char* name, type_handle_t parent_type_handle, size_t local_data_size);

/* Property registration */
property_handle_t type_register_property(type_handle_t type_handle, const char* name, size_t offset, size_t size);

/* Method registration - forward declare type_instance_t for signature */
struct type_instance_t;
method_handle_t type_register_method(type_handle_t type_handle, const char* name,
                                     void (*function)(struct type_instance_t* self, void* args, void* result));

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_TYPE_H */
