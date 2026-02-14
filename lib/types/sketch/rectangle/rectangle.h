/***************************************************************
**
** libcad Header File
**
** File         :  rectangle.h
** Module       :  sketch/rectangle
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad 2D rectangle primitive for sketches
**
***************************************************************/

#ifndef LIBCAD_SKETCH_RECTANGLE_H
#define LIBCAD_SKETCH_RECTANGLE_H

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

typedef struct rectangle_t rectangle_t;
typedef struct rectangle_class_t rectangle_class_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

/*
** rectangle_t instance - 2D axis-aligned rectangle
**
** Represents a rectangle in 2D sketch space.
** Coordinates are in plane-local 2D space.
** Rectangle is defined by two opposite corners.
** Can have children (e.g., labels, constraints) via inherited tree structure.
*/
typedef struct rectangle_t {
    object_t parent;  /* Inherit from object_t (MUST be first!) */

    /* Rectangle geometry (2D coordinates in sketch space) */
    vec2 corner1;     /* First corner (x, y) */
    vec2 corner2;     /* Opposite corner (x, y) */

    /* Visual properties */
    vec4 color;       /* RGBA color for rendering */
    float thickness;  /* Line thickness */
    bool filled;      /* Whether rectangle is filled */
    bool construction; /* Is this a construction rectangle? */
} rectangle_t;

/*
** rectangle_t class - vtable and metadata
*/
typedef struct rectangle_class_t {
    object_class_t parent_class;  /* Inherit parent vtable */

    /* Virtual methods specific to rectangle_t */
    void (*set_corners)(rectangle_t* self, vec2 corner1, vec2 corner2);
    float (*width)(const rectangle_t* self);
    float (*height)(const rectangle_t* self);
    float (*area)(const rectangle_t* self);
} rectangle_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

type_handle_t rectangle_get_type(void);
rectangle_class_t* rectangle_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

rectangle_t* rectangle_new(void);
rectangle_t* rectangle_new_with_corners(vec2 corner1, vec2 corner2);
void rectangle_free(rectangle_t* self);

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

/* Set rectangle corners */
void rectangle_set_corners(rectangle_t* self, vec2 corner1, vec2 corner2);

/* Get rectangle properties */
void rectangle_get_corner1(const rectangle_t* self, vec2 out_corner);
void rectangle_get_corner2(const rectangle_t* self, vec2 out_corner);
void rectangle_get_center(const rectangle_t* self, vec2 out_center);

/* Calculate properties */
float rectangle_width(const rectangle_t* self);
float rectangle_height(const rectangle_t* self);
float rectangle_area(const rectangle_t* self);

/* Set visual properties */
void rectangle_set_style(rectangle_t* self, vec4 color, float thickness, bool filled, bool construction);

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void rectangle_debug_print(rectangle_t* self);
json_t* rectangle_to_json(rectangle_t* self);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

#define RECTANGLE_TYPE (rectangle_get_type())
#define IS_RECTANGLE(obj) (type_instance_is_a((void*)(obj), RECTANGLE_TYPE))
#define RECTANGLE(obj) ((rectangle_t*)(obj))
#define RECTANGLE_AS_OBJECT(rect) ((object_t*)(rect))

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_SKETCH_RECTANGLE_H */
