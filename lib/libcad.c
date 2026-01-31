/***************************************************************
**
** libcad Source File
**
** File         :  libcad.c
** Module       :  libcad
** Author       :  SH
** Created      :  2026-01-08 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad core API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <libcad/libcad.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <cglm/cglm.h>

#ifdef EMSCRIPTEN
    #include <GLES3/gl3.h>
#else
    #include <glad/glad.h>
#endif


#include "lc_canvas.h"
#include "lc_draw.h"
#include "lc_scene.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static int width = 800;
static int height = 600;
static int vp_width = 800;
static int vp_height = 600;
static int x_pos = 100;
static int y_pos = 100;
static float zoom_scale = 1.0f; 

static vec2 cursorPos = {0.0f, 0.0f};

static vec2 mouse3StartPos = {0.0f, 0.0f};
static bool mouse3Active = false;

static vec2 offset = {0.0f, 0.0f};

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/


cad_ctx_t  cad_create_context()
{
    printf("cad_create_context called\n");
    
    lc_canvas_init();

    return (cad_ctx_t)1;
}

void cad_destroy_context(cad_ctx_t ctx)
{

}

void cad_set_cursor_pos(int x, int y)
{
    lc_canvas_set_cursor_pos((float)x, (float)y);
    lc_scene_set_cursor_pos((float)x, (float)y);
}

void cad_cursor_lost()
{
    lc_canvas_set_cursor_lost();
    lc_scene_set_cursor_lost();
}

void cad_set_cursor_button_state(int button, bool pressed)
{
    lc_canvas_set_cursor_button_state(button, pressed);
    lc_scene_set_cursor_button_state(button, pressed);
}

void cad_set_modifier_state(int modifier, bool state)
{
    lc_scene_set_modifier_state(modifier, state);
}

void cad_set_viewport(int x, int y, int vpw, int vph, int w, int h)
{
    vp_width = vpw;
    vp_height = vph;
    width = w;
    height = h;
    x_pos = x;
    y_pos = y;
}

void cad_render_viewport()
{
    //lc_scene_render((float)vp_width, (float)vp_height);

    //lc_draw_begin(vp_width, vp_height);

    //lc_canvas_render((float)vp_width, (float)vp_height);

    //lc_draw_end();

    lc_draw_render((float)vp_width, (float)vp_height);

}

void cad_init_viewport()
{
    printf("cad_init_viewport called\n");

    #ifndef EMSCRIPTEN
    if (!gladLoadGL() /*&& !gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)*/)
    {
        printf("Failed to initialize GLAD\n");
        return;
    }
    #endif

    printf("GLAD initialized successfully\n");
    printf("OpenGL %s\n", glGetString(GL_VERSION));
    
    if (!lc_draw_init())
    {
        printf("Failed to initialize lc_draw\n");
        return;
    }

    lc_scene_init();

}

void cad_axis_delta(int axis, float delta)
{
    lc_canvas_axis_delta(axis, delta);
    lc_scene_axis_delta(axis, delta);
}

int cad_get_cursor_type()
{
    return lc_canvas_get_cursor_type();
}


void cad_start_modal_tool(int tool_id)
{
    lc_canvas_set_modal_tool(tool_id);
}

void cad_clear_modal_tool()
{
    lc_canvas_set_modal_tool(0);
}

void cad_save_json(const char *path)
{
    return lc_canvas_save_json(path);
}

void cad_load_json(const char *path)
{
    return lc_canvas_load_json(path);
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/






