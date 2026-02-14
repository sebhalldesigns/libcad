/***************************************************************
**
** libcad Header File
**
** File         :  sketch_circle.h
** Module       :  sketch
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad 2D circle primitive for sketches
**
***************************************************************/

#ifndef LIBCAD_SKETCH_CIRCLE_H
#define LIBCAD_SKETCH_CIRCLE_H

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

typedef struct sketch_circle_t sketch_circle_t;
typedef struct sketch_circle_class_t sketch_circle_class_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

/*
** sketch_circle_t instance - 2D circle
**
** Represents a circle in 2D sketch space.
** Coordinates are in plane-local 2D space.
*/
typedef struct sketch_circle_t {
    object_t parent;  /* Inherit from object_t (MUST be first!) */

    /* Circle geometry (2D coordinates in sketch space) */
    vec2 center;      /* Center point (x, y) */
    float radius;     /* Radius */

    /* Visual properties */
    vec4 color;       /* RGBA color for rendering */
    float thickness;  /* Line thickness */
    bool construction; /* Is this a construction circle? */
} sketch_circle_t;

/*
** sketch_circle_t class - vtable and metadata
*/
typedef struct sketch_circle_class_t {
    object_class_t parent_class;  /* Inherit parent vtable */

    /* Virtual methods specific to sketch_circle_t */
    void (*set_geometry)(sketch_circle_t* self, vec2 center, float radius);
    float (*area)(const sketch_circle_t* self);
    float (*circumference)(const sketch_circle_t* self);
} sketch_circle_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

type_handle_t sketch_circle_get_type(void);
sketch_circle_class_t* sketch_circle_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

sketch_circle_t* sketch_circle_new(void);
sketch_circle_t* sketch_circle_new_with_geometry(vec2 center, float radius);
void sketch_circle_free(sketch_circle_t* self);

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

/* Set circle geometry */
void sketch_circle_set_geometry(sketch_circle_t* self, vec2 center, float radius);

/* Get circle properties */
void sketch_circle_get_center(const sketch_circle_t* self, vec2 out_center);
float sketch_circle_get_radius(const sketch_circle_t* self);

/* Calculate properties */
float sketch_circle_area(const sketch_circle_t* self);
float sketch_circle_circumference(const sketch_circle_t* self);

/* Set visual properties */
void sketch_circle_set_style(sketch_circle_t* self, vec4 color, float thickness, bool construction);

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void sketch_circle_debug_print(sketch_circle_t* self);
json_t* sketch_circle_to_json(sketch_circle_t* self);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

#define SKETCH_CIRCLE_TYPE (sketch_circle_get_type())
#define IS_SKETCH_CIRCLE(obj) (type_instance_is_a((void*)(obj), SKETCH_CIRCLE_TYPE))
#define SKETCH_CIRCLE(obj) ((sketch_circle_t*)(obj))
#define SKETCH_CIRCLE_AS_OBJECT(circle) ((object_t*)(circle))

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_SKETCH_CIRCLE_H */
