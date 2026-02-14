/***************************************************************
**
** libcad Header File
**
** File         :  sketch_rectangle.h
** Module       :  sketch
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

typedef struct sketch_rectangle_t sketch_rectangle_t;
typedef struct sketch_rectangle_class_t sketch_rectangle_class_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

/*
** sketch_rectangle_t instance - 2D axis-aligned rectangle
**
** Represents a rectangle in 2D sketch space.
** Coordinates are in plane-local 2D space.
** Rectangle is defined by two opposite corners.
*/
typedef struct sketch_rectangle_t {
    object_t parent;  /* Inherit from object_t (MUST be first!) */

    /* Rectangle geometry (2D coordinates in sketch space) */
    vec2 corner1;     /* First corner (x, y) */
    vec2 corner2;     /* Opposite corner (x, y) */

    /* Visual properties */
    vec4 color;       /* RGBA color for rendering */
    float thickness;  /* Line thickness */
    bool filled;      /* Whether rectangle is filled */
    bool construction; /* Is this a construction rectangle? */
} sketch_rectangle_t;

/*
** sketch_rectangle_t class - vtable and metadata
*/
typedef struct sketch_rectangle_class_t {
    object_class_t parent_class;  /* Inherit parent vtable */

    /* Virtual methods specific to sketch_rectangle_t */
    void (*set_corners)(sketch_rectangle_t* self, vec2 corner1, vec2 corner2);
    float (*width)(const sketch_rectangle_t* self);
    float (*height)(const sketch_rectangle_t* self);
    float (*area)(const sketch_rectangle_t* self);
} sketch_rectangle_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

type_handle_t sketch_rectangle_get_type(void);
sketch_rectangle_class_t* sketch_rectangle_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

sketch_rectangle_t* sketch_rectangle_new(void);
sketch_rectangle_t* sketch_rectangle_new_with_corners(vec2 corner1, vec2 corner2);
void sketch_rectangle_free(sketch_rectangle_t* self);

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

/* Set rectangle corners */
void sketch_rectangle_set_corners(sketch_rectangle_t* self, vec2 corner1, vec2 corner2);

/* Get rectangle properties */
void sketch_rectangle_get_corner1(const sketch_rectangle_t* self, vec2 out_corner);
void sketch_rectangle_get_corner2(const sketch_rectangle_t* self, vec2 out_corner);
void sketch_rectangle_get_center(const sketch_rectangle_t* self, vec2 out_center);

/* Calculate properties */
float sketch_rectangle_width(const sketch_rectangle_t* self);
float sketch_rectangle_height(const sketch_rectangle_t* self);
float sketch_rectangle_area(const sketch_rectangle_t* self);

/* Set visual properties */
void sketch_rectangle_set_style(sketch_rectangle_t* self, vec4 color, float thickness, bool filled, bool construction);

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void sketch_rectangle_debug_print(sketch_rectangle_t* self);
json_t* sketch_rectangle_to_json(sketch_rectangle_t* self);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

#define SKETCH_RECTANGLE_TYPE (sketch_rectangle_get_type())
#define IS_SKETCH_RECTANGLE(obj) (type_instance_is_a((void*)(obj), SKETCH_RECTANGLE_TYPE))
#define SKETCH_RECTANGLE(obj) ((sketch_rectangle_t*)(obj))
#define SKETCH_RECTANGLE_AS_OBJECT(rect) ((object_t*)(rect))

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_SKETCH_RECTANGLE_H */
