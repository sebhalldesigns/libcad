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

struct type_instance_t;

typedef struct
{
    /* name of the method */
    char* name;

    /* function pointer, self is a type_instance_t */
    void (*function)(struct type_instance_t *self, void *args, void *result);
} method_t;

typedef struct
{
    /* name of the property */
    char* name;

    /* data layout - always GLOBAL offset in instance data */
    size_t offset;
    size_t size;
} property_t;

typedef struct type_t
{
    /* name of the type */
    char* name;

    /* parent for inheritance */
    struct type_t *parent; /* can be NULL */

    size_t local_data_size; /* data size just for this type */
    size_t total_data_size; /* combined data size of this and all parent types */

    /* lifecycle methods (fast path - called directly without lookup) */
    void (*init)(struct type_instance_t* self, void* params);
    void (*destroy)(struct type_instance_t* self);

    /* properties should always be defined with global layout
    ** i.e offset by total_data_size - local_data_size
    ** this is so that each instance's data can be contiguous
    */
    property_t *properties;
    size_t properties_count;
    size_t properties_capacity;

    method_t *methods;
    size_t methods_count;
    size_t methods_capacity;
} type_t;

typedef struct type_instance_t
{
    type_t *type; /* pointer to the type of this instance */
    void *data;   /* pointer to the raw instance data */
} type_instance_t;

typedef uintptr_t type_handle_t;
typedef uintptr_t property_handle_t;
typedef uintptr_t method_handle_t;

typedef uintptr_t type_instance_handle_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* lookup functions */
type_handle_t type_get(const char* name);
property_handle_t type_get_property(const char* name, type_handle_t type_handle);
method_handle_t type_get_method(const char* name, type_handle_t type_handle);

/* Type instance creation and destruction */
type_instance_handle_t type_instance_create(type_handle_t type_handle);
void type_instance_destroy(type_instance_handle_t instance_handle);

/* Type instance property access */
void type_instance_set_property(type_instance_handle_t instance_handle, property_handle_t property_handle, const void* value);
void* type_instance_get_property_ptr(type_instance_handle_t instance_handle, property_handle_t property_handle);

/* Type instance method invocation */
void type_instance_call_method(type_instance_handle_t instance_handle, method_handle_t method_handle, const void* args, void* result);

/* Type registration */
type_handle_t type_register(const char* name, type_handle_t parent_type_handle, size_t local_data_size);

/* Lifecycle method registration (fast path - no string lookup) */
void type_set_init(type_handle_t type_handle, void (*init)(struct type_instance_t* self, void* params));
void type_set_destroy(type_handle_t type_handle, void (*destroy)(struct type_instance_t* self));

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
