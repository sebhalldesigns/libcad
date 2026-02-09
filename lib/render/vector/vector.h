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

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/


typedef uint32_t vector_instance_t;

typedef float vector_dash_t;
typedef float vector_fill_t;

typedef enum
{
    VECTOR_FILL_NORMAL,
    VECTOR_FILL_OUTSIDE_IN,
    VECTOR_FILL_INSIDE_OUT,
    VECTOR_FILL_LEFT_RIGHT,
    VECTOR_FILL_RIGHT_LEFT
} vector_fill_type_t;

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

vector_dash_t vector_build_dash(float period, float ratio);
vector_fill_t vector_build_fill(vector_fill_type_t fill_type, float proportion);

vector_instance_t vector_create_line();
void vector_build_line(vector_instance_t instance, vec3 start, vec3 end, vec4 color, float stroke_width, vector_dash_t dash);

vector_instance_t vector_create_shape();
void vector_build_circle(vector_instance_t instance, vec3 center, vec3 normal, vec2 size, vector_dash_t dash, vector_fill_t fill);


#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_VECTOR_H */