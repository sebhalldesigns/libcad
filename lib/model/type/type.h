/***************************************************************
**
** libcad Header File
**
** File         :  type.h
** Module       :  type
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad Type System - Class-based with macros
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
#include <stddef.h>
#include <stdbool.h>

#include <libcad/libcad.h>

/***************************************************************
** MARK: CORE TYPE SYSTEM
***************************************************************/

/* Forward declarations */
typedef struct type_class_t type_class_t;
typedef uintptr_t type_handle_t;

#define TYPE_INVALID ((type_handle_t)0)

/* Base class structure - all type classes inherit from this */
struct type_class_t {
    /* Type metadata */
    const char* type_name;
    type_handle_t parent_type;
    size_t instance_size;

    /* Lifecycle - called automatically */
    void (*instance_init)(void* instance);
    void (*instance_finalize)(void* instance);

    /* Reserved for future use */
    void* reserved[4];
};

/* Type registry functions */
EXPORT type_handle_t type_register_static(
    const char* type_name,
    type_handle_t parent_type,
    size_t class_size,
    void (*class_init)(void* cls),
    size_t instance_size,
    void (*instance_init)(void* instance)
);

EXPORT type_handle_t type_from_name(const char* name);
EXPORT type_class_t* type_class_peek(type_handle_t type);
EXPORT const char* type_name(type_handle_t type);
EXPORT bool type_is_a(type_handle_t type, type_handle_t ancestor);

/* Instance allocation (uses class metadata) */
EXPORT void* type_instance_new(type_handle_t type);
EXPORT void type_instance_free(void* instance);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

/* Get the type_class_t* from an instance */
#define LIBCAD_GET_CLASS(instance) \
    ((type_class_t*)(*(void**)(instance)))

/* Cast checking macros */
#define LIBCAD_CHECK_CAST(instance, type_t, type_func) \
    ((type_t*)instance) /* TODO: Add runtime check in debug builds */

#define LIBCAD_CHECK_CLASS_CAST(cls, type_class_t) \
    ((type_class_t*)cls)

/***************************************************************
** MARK: TYPE DEFINITION MACROS
**
** These drastically reduce boilerplate for defining new types
***************************************************************/

/*
** Define a new type with no parent
**
** Usage:
**   LIBCAD_DEFINE_TYPE(object, TYPE_INVALID)
**
** Generates:
**   - type_handle_t object_get_type(void)
**   - object_t* object_new(void)
**   - void object_free(object_t* self)
**   - object_class_t* object_class_get(void)
*/
#define LIBCAD_DEFINE_TYPE(type_name, parent_type_func) \
    LIBCAD_DEFINE_TYPE_EXTENDED(type_name, parent_type_func, 0)

/*
** Extended version with flags (for future expansion)
*/
#define LIBCAD_DEFINE_TYPE_EXTENDED(type_name, parent_type_func, flags) \
    \
    /* Forward declare class init */ \
    static void type_name##_class_init(type_name##_class_t* cls); \
    static void type_name##_init(type_name##_t* self); \
    \
    /* Get or register the type */ \
    type_handle_t type_name##_get_type(void) { \
        static type_handle_t type_handle = TYPE_INVALID; \
        if (type_handle == TYPE_INVALID) { \
            type_handle = type_register_static( \
                #type_name "_t", \
                parent_type_func, \
                sizeof(type_name##_class_t), \
                (void(*)(void*))type_name##_class_init, \
                sizeof(type_name##_t), \
                (void(*)(void*))type_name##_init \
            ); \
        } \
        return type_handle; \
    } \
    \
    /* Get the class (singleton) */ \
    type_name##_class_t* type_name##_class_get(void) { \
        return (type_name##_class_t*)type_class_peek(type_name##_get_type()); \
    } \
    \
    /* Convenience constructor */ \
    type_name##_t* type_name##_new(void) { \
        return (type_name##_t*)type_instance_new(type_name##_get_type()); \
    } \
    \
    /* Convenience destructor */ \
    void type_name##_free(type_name##_t* self) { \
        type_instance_free(self); \
    }

/*
** Helper macros for accessing parent class
*/
#define LIBCAD_DEFINE_TYPE_WITH_CODE(type_name, parent_type_func, code) \
    LIBCAD_DEFINE_TYPE(type_name, parent_type_func) \
    code

/* Get parent class from a class structure */
#define LIBCAD_PARENT_CLASS(cls) \
    ((type_class_t*)type_class_peek(((type_class_t*)cls)->parent_type))

/***************************************************************
** MARK: PROPERTY SYSTEM (Future)
**
** Placeholder for GObject-style properties
***************************************************************/

/* TODO: Add property registration system similar to GParamSpec */

/***************************************************************
** MARK: METHOD INVOCATION
***************************************************************/

/* Direct vtable call - fastest */
#define LIBCAD_CALL(instance, method, ...) \
    ((LIBCAD_GET_CLASS(instance))->method((instance), ##__VA_ARGS__))

/* Check if method exists before calling */
#define LIBCAD_CALL_IF_EXISTS(instance, method, ...) \
    do { \
        void* _method = (void*)(LIBCAD_GET_CLASS(instance))->method; \
        if (_method) { \
            ((LIBCAD_GET_CLASS(instance))->method((instance), ##__VA_ARGS__)); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_TYPE_H */
