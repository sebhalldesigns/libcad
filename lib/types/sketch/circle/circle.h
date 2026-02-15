/***************************************************************
**
** libcad Header File
**
** File         :  circle.h
** Module       :  sketch/circle
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

typedef struct circle_t circle_t;
typedef struct circle_class_t circle_class_t;
typedef struct plane_t plane_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

/*
** circle_t instance - 2D circle
**
** Represents a circle in 2D sketch space.
** Coordinates are in plane-local 2D space.
** Can have children (e.g., labels, constraints) via inherited tree structure.
*/
typedef struct circle_t {
    object_t parent;  /* Inherit from object_t (MUST be first!) */

    /* Circle geometry (2D coordinates in sketch space) */
    vec2 center;      /* Center point (x, y) */
    float radius;     /* Radius */

    /* Visual properties */
    vec4 color;       /* RGBA color for rendering */
    float thickness;  /* Line thickness */
    bool construction; /* Is this a construction circle? */

    /* Sketch plane reference + render handle */
    plane_t* reference_plane;    /* Non-owning */
    uint32_t vector_shape_handle;
} circle_t;

/*
** circle_t class - vtable and metadata
*/
typedef struct circle_class_t {
    object_class_t parent_class;  /* Inherit parent vtable */

    /* Virtual methods specific to circle_t */
    void (*set_geometry)(circle_t* self, vec2 center, float radius);
    float (*area)(const circle_t* self);
    float (*circumference)(const circle_t* self);
} circle_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

type_handle_t circle_get_type(void);
circle_class_t* circle_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

circle_t* circle_new(void);
circle_t* circle_new_with_geometry(vec2 center, float radius);
void circle_free(circle_t* self);

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

/* Set circle geometry */
void circle_set_geometry(circle_t* self, vec2 center, float radius);

/* Get circle properties */
void circle_get_center(const circle_t* self, vec2 out_center);
float circle_get_radius(const circle_t* self);

/* Calculate properties */
float circle_area(const circle_t* self);
float circle_circumference(const circle_t* self);

/* Set visual properties */
void circle_set_style(circle_t* self, vec4 color, float thickness, bool construction);
void circle_set_reference_plane(circle_t* self, plane_t* plane);

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void circle_debug_print(circle_t* self);
json_t* circle_to_json(circle_t* self);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

#define CIRCLE_TYPE (circle_get_type())
#define IS_CIRCLE(obj) (type_instance_is_a((void*)(obj), CIRCLE_TYPE))
#define CIRCLE(obj) ((circle_t*)(obj))
#define CIRCLE_AS_OBJECT(circle) ((object_t*)(circle))

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_SKETCH_CIRCLE_H */
