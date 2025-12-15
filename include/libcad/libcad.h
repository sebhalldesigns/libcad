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

#define MOUSE_LEFT_BUTTON    1
#define MOUSE_RIGHT_BUTTON   2
#define MOUSE_MIDDLE_BUTTON  3

typedef uintptr_t cad_ctx_t;
typedef uintptr_t cad_model_t;

EXPORT cad_ctx_t    cad_create_context();
EXPORT void         cad_destroy_context(cad_ctx_t ctx);

EXPORT bool         cad_load_model_file(cad_ctx_t ctx, cad_model_t* p_model, const char* filepath);
EXPORT bool         cad_load_model_data(cad_ctx_t ctx, cad_model_t* p_model, const uint8_t* data, size_t size);
EXPORT void         cad_unload_model(cad_ctx_t ctx);

EXPORT bool         cad_write_model_file(cad_ctx_t ctx, cad_model_t model, const char* format, const char* filepath);
EXPORT bool         cad_write_model_data(cad_ctx_t ctx, cad_model_t model, const char* format, uint8_t** p_data, size_t* p_size);

EXPORT bool         cad_create_child_window(void* parent_handle, int x, int y, int width, int height);
EXPORT uintptr_t    cad_get_child_window_handle();
EXPORT void         cad_destroy_child_window();
EXPORT void         cad_update_child_window();

EXPORT void         cad_set_viewport(int x, int y, int width, int height, int window_width, int window_height);
EXPORT void         cad_render_viewport();
EXPORT void         cad_init_viewport();

EXPORT void         cad_set_window_size(int width, int height);
EXPORT void         cad_set_window_pos(int x, int y);
EXPORT void         cad_set_cursor_pos(int x, int y);
EXPORT void         cad_cursor_lost();
EXPORT void         cad_set_cursor_button_state(int button, bool pressed);



#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_H */