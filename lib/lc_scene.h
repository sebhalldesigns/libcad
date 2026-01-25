/***************************************************************
**
** libcad Header File
**
** File         :  lc_scene.h
** Module       :  lc_scene
** Author       :  SH
** Created      :  2026-01-25 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad internal scene API
**
**  Functions for managing the scene rendering context.
**
***************************************************************/

#ifndef LC_SCENE_H
#define LC_SCENE_H

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


void lc_scene_init();
void lc_scene_render(float viewport_width, float viewport_height);

void lc_scene_set_cursor_pos(float x, float y);
void lc_scene_set_cursor_lost();
void lc_scene_set_cursor_button_state(int button, bool pressed);
void lc_scene_set_modifier_state(int modifier, bool pressed);
void lc_scene_axis_delta(int axis, float delta);

#ifdef __cplusplus
}
#endif

#endif /* LC_SCENE_H */