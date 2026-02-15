/***************************************************************
**
** libcad Header File
**
** File         :  axis.h
** Module       :  core/axis
** Author       :  SH
** Created      :  2026-02-15 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad axis type - represents a 3D axis line
**
***************************************************************/

#ifndef LIBCAD_CORE_AXIS_H
#define LIBCAD_CORE_AXIS_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <types/core/object/object.h>
#include <cglm/cglm.h>

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

typedef struct axis_t axis_t;
typedef struct axis_class_t axis_class_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

/*
** axis_t instance - represents a 3D axis line for viewport orientation
**
** An axis is defined by a start point, direction, and length.
** Typically used to show X, Y, Z axes in the 3D viewport.
**
** IMPORTANT: First field MUST be parent (object_t parent)
**            This enables safe upcasting: axis_t* -> object_t*
*/
typedef struct axis_t {
    object_t parent;  /* Inherit from object_t (MUST be first!) */

    /* Axis geometry */
    vec3 origin;      /* Start point of the axis */
    vec3 direction;   /* Direction vector (should be normalized) */
    float length;     /* Length of the axis line */

    /* Visualization */
    vec4 color;       /* RGBA color for rendering the axis */
    bool visible;     /* Whether the axis should be rendered */
    float thickness;  /* Line thickness for rendering */
    bool show_arrow;  /* Whether to show an arrow at the end */

    /* Vector rendering handle */
    uint32_t vector_line_handle;  /* Handle to the vector line instance */
} axis_t;

/*
** axis_t class - vtable and metadata
**
** Extends parent vtable with axis-specific methods
*/
typedef struct axis_class_t {
    object_class_t parent_class;  /* Inherit parent vtable */

    /* Virtual methods specific to axis_t */
    void (*set_geometry)(axis_t* self, vec3 origin, vec3 direction, float length);
} axis_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

/* Get the type handle (auto-registers on first call) */
type_handle_t axis_get_type(void);

/* Get the class singleton */
axis_class_t* axis_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

/* Create new axis instance */
axis_t* axis_new(void);

/* Destroy axis instance */
void axis_free(axis_t* self);

/***************************************************************
** MARK: PUBLIC API - Geometry
***************************************************************/

/*
** Set the axis geometry
**
** Parameters:
**   self      - Axis instance
**   origin    - Start point in 3D space
**   direction - Direction vector (will be normalized automatically)
**   length    - Length of the axis line
*/
void axis_set_geometry(axis_t* self, vec3 origin, vec3 direction, float length);

/*
** Get the end point of the axis
**
** Parameters:
**   self     - Axis instance
**   out_end  - Output end point in 3D space
*/
void axis_get_end_point(const axis_t* self, vec3 out_end);

/***************************************************************
** MARK: PUBLIC API - Visualization
***************************************************************/

/*
** Set visualization properties
**
** Parameters:
**   self       - Axis instance
**   color      - RGBA color (values 0-1)
**   visible    - Whether axis should be rendered
**   thickness  - Line thickness
**   show_arrow - Whether to show arrow at end
*/
void axis_set_display(axis_t* self, vec4 color, bool visible, float thickness, bool show_arrow);

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

/* Debug print (overrides object_t) */
void axis_debug_print(axis_t* self);

/* JSON serialization (overrides object_t) */
json_t* axis_to_json(axis_t* self);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

/* Type checking */
#define AXIS_TYPE (axis_get_type())
#define IS_AXIS(obj) (type_instance_is_a((void*)(obj), AXIS_TYPE))

/* Casting */
#define AXIS(obj) ((axis_t*)(obj))
#define AXIS_AS_OBJECT(axis) ((object_t*)(axis))

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_CORE_AXIS_H */
