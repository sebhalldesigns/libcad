/***************************************************************
**
** libcad Header File
**
** File         :  vector.h
** Module       :  render/vector
** Author       :  SH
** Created      :  2026-02-06 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad vector API
**
***************************************************************/

#ifndef LIBCAD_VECTOR_H
#define LIBCAD_VECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include <cglm/cglm.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define VECTOR_INVALID_INSTANCE ((vector_instance_t)0xFFFFFFFF)

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef uint32_t vector_instance_t;

typedef enum
{
    VECTOR_FILL_NORMAL,
    VECTOR_FILL_OUTSIDE_IN,
    VECTOR_FILL_INSIDE_OUT,
    VECTOR_FILL_LEFT_RIGHT,
    VECTOR_FILL_RIGHT_LEFT
} vector_fill_type_t;

typedef struct
{
    vec3 start;
    vec3 end;
    vec4 color;
    float stroke_width;
    float dash; /* encode into single float somehow */
} vector_line_instance_t; /* 12 floats */

typedef struct
{
    vec3 center;
    vec3 normal;
    vec2 size;
    vec4 color;
    float rotation;
    float sides; /* 1 for ellipse, 2 invalid, 3 triangle etc. */
    float start_angle; /* for arcs, 0 for ellipse */
    float end_angle; /* for arcs, 0 for circle */
    float fill; /* normalise fill, -1 to 1. -ve fill from center, +ve fill from outside*/
    float stroke_width;
    float corner_radius;
    float dash;  /* encode into single float */
} vector_shape_instance_t; /* 20 floats */

typedef struct
{
    vec3 p0, p1, p2, p3;
    vec4 color;
    float fill;
    float fill_param;
    float stroke_width;
    float dash; /* encode into single float */
} vector_bezier_instance_t; /* 20 floats */

typedef struct
{
    vec2 position;
    vec2 size;
    vec2 uv_min;
    vec2 uv_max;
    vec4 color;
} vector_glyph_instance_t; /* 12 floats */

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/*
**
** Notes on API:
** - lots of parameters
** - could make more state machine
** - especially for e.g drawing lots of 2D rectangles needing normals
** - easier to make them without normals, then batch set normal?
** - do we actually need normal? possibly not
*/

bool vector_init(void);

void vector_render(int width, int height);

float vector_build_dash(float period, float ratio);
float vector_build_fill(vector_fill_type_t fill_type, float proportion);

bool vector_create_line(const vector_line_instance_t *data, vector_instance_t *out_handle);
void vector_update_line(vector_instance_t instance, const vector_line_instance_t *data);
void vector_destroy_line(vector_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_VECTOR_H */