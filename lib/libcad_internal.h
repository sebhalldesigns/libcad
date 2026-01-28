/***************************************************************
**
** libcad Header File
**
** File         :  libcad_internal.h
** Module       :  libcad (internal)
** Author       :  SH
** Created      :  2026-01-28 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Shared internal types for libcad modules.
**                 Not part of the public API.
**
***************************************************************/

#ifndef LIBCAD_INTERNAL_H
#define LIBCAD_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <cglm/cglm.h>
#include <stdint.h>
#include <stdbool.h>

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* Rendering context passed between modules during a frame.
 * Contains the view-projection transform, viewport dimensions,
 * and canvas plane parameters needed by all render functions. */
typedef struct {
    mat4 view_projection;       /* world -> clip space */
    mat4 view_projection_inv;   /* clip space -> world (for unprojection) */
    int viewport_width;
    int viewport_height;

    /* canvas (2D sketch plane) parameters */
    vec2 canvas_origin;         /* world position of 2D plane origin */
    vec3 canvas_normal;         /* plane normal (typically z-axis) */
    float canvas_zoom;          /* current zoom level */
} lc_render_context_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/


#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_INTERNAL_H */
