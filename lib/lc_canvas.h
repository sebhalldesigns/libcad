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
**  Module: lc_canvas
**  Responsibility: 2D sketch plane management, modal drawing tools,
**    pan/zoom state, canvas item storage, selection state.
**  Owns: Pan/zoom transform, active tool, canvas items, hit test state.
**  Uses: lc_draw (for rendering), cglm (for transforms).
**  Does NOT own: GPU resources, 3D scene state, document model.
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

#include <cglm/cglm.h>

#include "libcad_internal.h"

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
void lc_canvas_render_ctx(const lc_render_context_t *ctx);

void lc_canvas_set_cursor_pos(float x, float y);
void lc_canvas_set_cursor_lost();
void lc_canvas_set_cursor_button_state(int button, bool pressed);
void lc_canvas_axis_delta(int axis, float delta);

int lc_canvas_get_cursor_type();

void lc_canvas_set_modal_tool(int tool_id);

void lc_canvas_save_json(const char *path);
void lc_canvas_load_json(const char *path);

void lc_canvas_set_view_matrix(mat4 matrix);

#ifdef __cplusplus
}
#endif

#endif /* LC_CANVAS_H */