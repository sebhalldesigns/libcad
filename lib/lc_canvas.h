/***************************************************************
**
** libcad Header File
**
** File         :  lc_canvas.h
** Module       :  lc_canvas
** Author       :  SH
** Created      :  2026-01-08 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad internal canvas API
**
**  Functions for managing the canvas rendering context.
**
***************************************************************/

#ifndef LC_CANVAS_H
#define LC_CANVAS_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdbool.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/


void lc_canvas_init();
void lc_canvas_render(float viewport_width, float viewport_height);

void lc_canvas_set_cursor_pos(float x, float y);
void lc_canvas_set_cursor_button_state(int button, bool pressed);
void lc_canvas_axis_delta(int axis, float delta);

int lc_canvas_get_cursor_type();

#ifdef __cplusplus
}
#endif

#endif /* LC_CANVAS_H */