/***************************************************************
**
** libcad Header File
**
** File         :  sketch_line.h
** Module       :  sketch
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad 2D line primitive for sketches
**
***************************************************************/

#ifndef LIBCAD_SKETCH_LINE_H
#define LIBCAD_SKETCH_LINE_H

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

typedef struct sketch_line_t sketch_line_t;
typedef struct sketch_line_class_t sketch_line_class_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

/*
** sketch_line_t instance - 2D line segment
**
** Represents a line segment in 2D sketch space.
** Coordinates are in plane-local 2D space.
*/
typedef struct sketch_line_t {
    object_t parent;  /* Inherit from object_t (MUST be first!) */

    /* Line geometry (2D coordinates in sketch space) */
    vec2 start;       /* Start point (x, y) */
    vec2 end;         /* End point (x, y) */

    /* Visual properties */
    vec4 color;       /* RGBA color for rendering */
    float thickness;  /* Line thickness */
    bool construction; /* Is this a construction line? */
} sketch_line_t;

/*
** sketch_line_t class - vtable and metadata
*/
typedef struct sketch_line_class_t {
    object_class_t parent_class;  /* Inherit parent vtable */

    /* Virtual methods specific to sketch_line_t */
    void (*set_points)(sketch_line_t* self, vec2 start, vec2 end);
    float (*length)(const sketch_line_t* self);
} sketch_line_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

type_handle_t sketch_line_get_type(void);
sketch_line_class_t* sketch_line_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

sketch_line_t* sketch_line_new(void);
sketch_line_t* sketch_line_new_with_points(vec2 start, vec2 end);
void sketch_line_free(sketch_line_t* self);

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

/* Set line endpoints */
void sketch_line_set_points(sketch_line_t* self, vec2 start, vec2 end);

/* Get line endpoints */
void sketch_line_get_start(const sketch_line_t* self, vec2 out_start);
void sketch_line_get_end(const sketch_line_t* self, vec2 out_end);

/* Calculate line length */
float sketch_line_length(const sketch_line_t* self);

/* Set visual properties */
void sketch_line_set_style(sketch_line_t* self, vec4 color, float thickness, bool construction);

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void sketch_line_debug_print(sketch_line_t* self);
json_t* sketch_line_to_json(sketch_line_t* self);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

#define SKETCH_LINE_TYPE (sketch_line_get_type())
#define IS_SKETCH_LINE(obj) (type_instance_is_a((void*)(obj), SKETCH_LINE_TYPE))
#define SKETCH_LINE(obj) ((sketch_line_t*)(obj))
#define SKETCH_LINE_AS_OBJECT(line) ((object_t*)(line))

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_SKETCH_LINE_H */
