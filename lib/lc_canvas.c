/***************************************************************
**
** libcad Source File
**
** File         :  lc_canvas.c
** Module       :  lc_canvas
** Author       :  SH
** Created      :  2026-01-08 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad internal canvas API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_canvas.h"
#include "lc_draw.h"

#include <libcad/libcad.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define ZOOM_SENSITIVITY (0.15f)
#define MIN_ZOOM (0.01f)
#define MAX_ZOOM (100.0f)

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static vec2 viewport_origin = {0.0f, 0.0f};
static vec2 viewport_size = {800.0f, 600.0f};

static float zoom = 1.0f;

static vec2 origin = {0.0f, 0.0f};

static vec2 cursor_pos = {0.0f, 0.0f};

static vec2 drag_start_pos = {0.0f, 0.0f};
static bool drag_active = false;


/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/


static void render_grid();

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void lc_canvas_init()
{

}

void lc_canvas_render(float viewport_width, float viewport_height)
{
    viewport_size[0] = viewport_width;
    viewport_size[1] = viewport_height;

    origin[0] = (viewport_size[0] / 2.0f) + viewport_origin[0];
    origin[1] = (viewport_size[1] / 2.0f) + viewport_origin[1];

    render_grid();

}

void lc_canvas_set_cursor_pos(float x, float y)
{
    cursor_pos[0] = x;
    cursor_pos[1] = y;

    if (drag_active)
    {
        vec2 delta;
        glm_vec2_sub(cursor_pos, drag_start_pos, delta);

        glm_vec2_add(viewport_origin, delta, viewport_origin);
        
        glm_vec2_copy(cursor_pos, drag_start_pos);
    }
}

void lc_canvas_set_cursor_button_state(int button, bool pressed)
{
    switch (button)
    {
        case 2:
        {
            if (pressed)
            {
                glm_vec2_copy(cursor_pos, drag_start_pos);
                drag_active = true;
            }
            else
            {
                drag_active = false;

            }

        } break;

        default:
        {
            /* do nothing */
        } break;
    }
}

void lc_canvas_axis_delta(int axis, float delta)
{
    if (axis == 0) // Y axis for zoom
    {
        zoom *= expf(delta * ZOOM_SENSITIVITY);
        if (zoom < MIN_ZOOM) zoom = MIN_ZOOM;
        if (zoom > MAX_ZOOM) zoom = MAX_ZOOM;
    }
}

int lc_canvas_get_cursor_type()
{
    if (drag_active)
    {
        return CURSOR_MOVE;
    }

    return CURSOR_NORMAL;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static void render_grid()
{
    /* DRAW MINOR GRID */

    /* minor grid spacing from 5 to 50 px*/
    const float major_minor_ratio = 5.0f;
    
    const float minor_min = 10.0f;
    const float minor_max = 50.0f;
    const float base_spacing = 25.0f;

    float minor_spacing = zoom * base_spacing;

    while (minor_spacing < minor_min)
    {
        minor_spacing *= major_minor_ratio;
    }

    while (minor_spacing > minor_max)
    {
        minor_spacing /= major_minor_ratio;
    }
    
    vec2 minor_start = {fmodf(origin[0], minor_spacing) - minor_spacing, fmodf(origin[1], minor_spacing) - minor_spacing};
    vec2 grid_end = {viewport_size[0], viewport_size[1]};

    lc_draw_grid(minor_start, grid_end, minor_spacing, IM_COL32(255, 255, 255, 13));

    /* DRAW MAJOR GRID */
    float major_spacing = minor_spacing * major_minor_ratio;
    vec2 major_start = {fmodf(origin[0], major_spacing) - major_spacing, fmodf(origin[1], major_spacing) - major_spacing};
    lc_draw_grid(major_start, grid_end, major_spacing, IM_COL32(255, 255, 255, 25));
}