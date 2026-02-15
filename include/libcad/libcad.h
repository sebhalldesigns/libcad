#ifndef LIBCAD_H
#define LIBCAD_H


#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXPORT
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#elif __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT __attribute__((visibility("default")))
#endif
#endif


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/***************************************************************
** Type System & Document Model
***************************************************************/

/* Include type system and core types - these headers have EXPORT macros */
/*#include "../../lib/model/type/type.h"*/
/*#include "../../lib/types/core/object/object.h"*/
/*#include "../../lib/types/core/document/document.h"*/
/*#include "../../lib/types/core/textfield/textfield.h"*/

/***************************************************************
** Legacy API (to be organized)
***************************************************************/

#define MOUSE_LEFT_BUTTON    1
#define MOUSE_MIDDLE_BUTTON  2
#define MOUSE_RIGHT_BUTTON   3

#define MODIFIER_CONTROL    1
#define MODIFIER_SHIFT      2
#define MODIFIER_ALT        3

#define CURSOR_NORMAL       0
#define CURSOR_MOVE         1
#define CURSOR_RESIZE_H     2
#define CURSOR_RESIZE_V     3
#define CURSOR_RESIZE_NWSE  4
#define CURSOR_RESIZE_NESW  5
#define CURSOR_POINTER      6

typedef uintptr_t cad_ctx_t;
typedef uintptr_t cad_model_t;

EXPORT cad_ctx_t    cad_create_context();
EXPORT void         cad_destroy_context(cad_ctx_t ctx);

EXPORT bool         cad_load_model_file(cad_ctx_t ctx, cad_model_t* p_model, const char* filepath);
EXPORT bool         cad_load_model_data(cad_ctx_t ctx, cad_model_t* p_model, const uint8_t* data, size_t size);
EXPORT void         cad_unload_model(cad_ctx_t ctx);

EXPORT bool         cad_write_model_file(cad_ctx_t ctx, cad_model_t model, const char* format, const char* filepath);
EXPORT bool         cad_write_model_data(cad_ctx_t ctx, cad_model_t model, const char* format, uint8_t** p_data, size_t* p_size);

EXPORT void         cad_set_viewport(int x, int y, int width, int height, int window_width, int window_height);
EXPORT void         cad_set_dpi_scale(float scale);
EXPORT void         cad_render_viewport();
EXPORT void         cad_init_viewport();

EXPORT uint32_t     cad_pick_entity(int screen_x, int screen_y);
EXPORT void         cad_set_hovered_entity(uint32_t entity_id);
EXPORT uint32_t     cad_get_hovered_entity();
EXPORT void         cad_set_selected_entity(uint32_t entity_id);
EXPORT uint32_t     cad_get_selected_entity();
EXPORT const char*  cad_get_document_json();
EXPORT bool         cad_create_sketch_on_plane(uint32_t plane_entity_id);
EXPORT bool         cad_set_object_visibility(uintptr_t object_id, bool visible);
EXPORT bool         cad_enter_sketch_mode(uint32_t plane_entity_id);
EXPORT bool         cad_exit_sketch_mode(void);
EXPORT bool         cad_create_line_in_active_sketch(void);
EXPORT bool         cad_create_circle_in_active_sketch(void);
EXPORT bool         cad_create_corner_rectangle_in_active_sketch(void);
EXPORT bool         cad_create_line_in_active_sketch_screen(int sx0, int sy0, int sx1, int sy1);
EXPORT bool         cad_create_circle_in_active_sketch_screen(int sx0, int sy0, int sx1, int sy1);
EXPORT bool         cad_create_corner_rectangle_in_active_sketch_screen(int sx0, int sy0, int sx1, int sy1);

EXPORT void         cad_set_cursor_pos(int x, int y);
EXPORT void         cad_cursor_lost();
EXPORT void         cad_set_cursor_button_state(int button, bool pressed);
EXPORT void         cad_set_modifier_state(int modifier, bool state);
EXPORT void         cad_axis_delta(int axis, float delta);
EXPORT void         cad_camera_orbit(float delta_x, float delta_y);
EXPORT void         cad_camera_pan(float delta_x, float delta_y);
EXPORT void         cad_camera_zoom(float delta);

EXPORT void         cad_start_modal_tool(int tool_id);
EXPORT void         cad_clear_modal_tool();

EXPORT int          cad_get_cursor_type();

EXPORT void         cad_save_json(const char *path);
EXPORT void         cad_load_json(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_H */

