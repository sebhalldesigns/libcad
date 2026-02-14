/***************************************************************
**
** libcad Header File
**
** File         :  line.h
** Module       :  sketch/line
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

typedef struct line_t line_t;
typedef struct line_class_t line_class_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

/*
** line_t instance - 2D line segment
**
** Represents a line segment in 2D sketch space.
** Coordinates are in plane-local 2D space.
** Can have children (e.g., labels, constraints) via inherited tree structure.
*/
typedef struct line_t {
    object_t parent;  /* Inherit from object_t (MUST be first!) */

    /* Line geometry (2D coordinates in sketch space) */
    vec2 start;       /* Start point (x, y) */
    vec2 end;         /* End point (x, y) */

    /* Visual properties */
    vec4 color;       /* RGBA color for rendering */
    float thickness;  /* Line thickness */
    bool construction; /* Is this a construction line? */
} line_t;

/*
** line_t class - vtable and metadata
*/
typedef struct line_class_t {
    object_class_t parent_class;  /* Inherit parent vtable */

    /* Virtual methods specific to line_t */
    void (*set_points)(line_t* self, vec2 start, vec2 end);
    float (*length)(const line_t* self);
} line_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

type_handle_t line_get_type(void);
line_class_t* line_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

line_t* line_new(void);
line_t* line_new_with_points(vec2 start, vec2 end);
void line_free(line_t* self);

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

/* Set line endpoints */
void line_set_points(line_t* self, vec2 start, vec2 end);

/* Get line endpoints */
void line_get_start(const line_t* self, vec2 out_start);
void line_get_end(const line_t* self, vec2 out_end);

/* Calculate line length */
float line_length(const line_t* self);

/* Set visual properties */
void line_set_style(line_t* self, vec4 color, float thickness, bool construction);

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void line_debug_print(line_t* self);
json_t* line_to_json(line_t* self);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

#define LINE_TYPE (line_get_type())
#define IS_LINE(obj) (type_instance_is_a((void*)(obj), LINE_TYPE))
#define LINE(obj) ((line_t*)(obj))
#define LINE_AS_OBJECT(line) ((object_t*)(line))

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_SKETCH_LINE_H */
