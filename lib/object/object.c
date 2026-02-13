/***************************************************************
**
** libcad Source File
**
** File         :  object.c
** Module       :  object
** Author       :  SH
** Created      :  2026-02-13 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad Object API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <util/log/log.h>

#include "object.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

struct object_t;

typedef struct
{
    /* name of the method */
    char* name;

    /* function pointer, self is an object_t */
    void (*function)(struct object_t *self, void *args, void *result);
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

    /* properties should always be defined with global layout
    ** i.e offset by total_data_size - local_data_size 
    ** this is so that each instance's data can be contiguous
    */
    property_t *properties;
    size_t property_count;

    method_t *methods;
    size_t method_count;
} type_t;

typedef struct
{
    type_t *type; /* pointer to the type of this object */
    void *data; /* pointer to the raw data */
} object_t;

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/






