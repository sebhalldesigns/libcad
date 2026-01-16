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

#define HANDLE_CATCHMENT (5.0f)

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* struct for items that need to be drawn to the canvas */
typedef struct
{
    mat4 bounds;
    mat4 frame;
    bool is_hovered;
    int hover_handle_index;
} lc_canvas_item_t;

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static vec2 viewport_origin = {0.0f, 0.0f};
static vec2 viewport_size = {800.0f, 600.0f};

static float zoom = 1.0f;

static vec2 origin = {0.0f, 0.0f};

static vec4 cursor_pos = {0.0f, 0.0f, 0.0f, 0.0f};

static vec2 pan_start_pos = {0.0f, 0.0f};
static bool pan_active = false;

static lc_canvas_item_t *active_item = NULL;

static vec2 drag_start_pos = {0.0f, 0.0f};
static bool drag_active = false;

static mat4 viewport_transform;
static vec4 world_origin;

static int cursor_request = CURSOR_NORMAL;

static lc_canvas_item_t item;


/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void render_axes();
static void render_grid();
static void render_canvas_item(lc_canvas_item_t *item);


static bool item_contains(lc_canvas_item_t *item, vec4 point);



  
/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void lc_canvas_init()
{
    glm_mat4_zero(item.bounds);

    item.bounds[0][0] = 0.0f;
    item.bounds[0][1] = 0.0f;
    item.bounds[1][0] = 100.0f;
    item.bounds[1][1] = 0.0f;
    item.bounds[2][0] = 100.0f;
    item.bounds[2][1] = 100.0f;
    item.bounds[3][0] = 0.0f;
    item.bounds[3][1] = 100.0f;

    item.bounds[0][3] = 1.0f;
    item.bounds[1][3] = 1.0f;
    item.bounds[2][3] = 1.0f;
    item.bounds[3][3] = 1.0f;
}

void lc_canvas_render(float viewport_width, float viewport_height)
{
    
    //active_item = NULL;

    viewport_size[0] = viewport_width;
    viewport_size[1] = viewport_height;

    glm_mat4_identity(viewport_transform);
    
    // First, translate to viewport center
    glm_translate(viewport_transform, (vec3){viewport_size[0] / 2.0f, viewport_size[1] / 2.0f, 0.0f});
    
    // Then scale (zooms around viewport center)
    glm_scale_uni(viewport_transform, zoom);
    
    // Finally, apply the pan offset
    glm_translate(viewport_transform, (vec3){viewport_origin[0], viewport_origin[1], 0.0f});

    glm_mat4_mulv(viewport_transform, (vec4){0.0f, 0.0f, 0.0f, 1.0f}, world_origin);

    render_grid();
    render_axes();

    
    glm_mat4_mul(viewport_transform, item.bounds, item.frame);

    if (&item != active_item)
    {

        cursor_request = CURSOR_NORMAL;

        item.hover_handle_index = -1;
        item.is_hovered = item_contains(&item, cursor_pos);

        if (item.hover_handle_index >= 0 && item.hover_handle_index < 4)
        {
            cursor_request = item.hover_handle_index % 2 == 0 ? CURSOR_RESIZE_NWSE : CURSOR_RESIZE_NESW;
        }
        else if (item.hover_handle_index >= 0 && item.hover_handle_index < 8)
        {
            cursor_request = item.hover_handle_index % 2 == 0 ? CURSOR_RESIZE_V : CURSOR_RESIZE_H;
        }
        else if (item.hover_handle_index == 8)
        {
            cursor_request = CURSOR_MOVE;
        }
    }

    render_canvas_item(&item);
}

void lc_canvas_set_cursor_pos(float x, float y)
{
    cursor_pos[0] = x;
    cursor_pos[1] = y;

    if (pan_active)
    {
        vec2 delta;
        glm_vec2_sub(cursor_pos, pan_start_pos, delta);
        glm_vec2_scale(delta, 1.0f/zoom, delta);

        glm_vec2_add(viewport_origin, delta, viewport_origin);
        
        glm_vec2_copy(cursor_pos, pan_start_pos);
    }
    
    if (drag_active && active_item)
    {
        vec2 delta;
        glm_vec2_sub(cursor_pos, drag_start_pos, delta);
        glm_vec2_scale(delta, 1.0f/zoom, delta);

        vec4 delta4 = {delta[0], delta[1], 0.0f, 1.0f};

        /* HANDLE LOGIC */

        if (active_item->hover_handle_index < 0)
        {
            /* do nothing */
        }
        else if (active_item->hover_handle_index < 4)
        {
            /* for diagonal corners */
            
            /* add the delta directly to the curent handle */
            X(active_item->bounds[active_item->hover_handle_index]) += X(delta);
            Y(active_item->bounds[active_item->hover_handle_index]) += Y(delta);

            /* get handles either side */
            int next_index = (active_item->hover_handle_index + 1) % 4;
            int prev_index = (active_item->hover_handle_index + 3) % 4;

            if (active_item->hover_handle_index % 2 == 0)
            {
                /* if it's top left or bottom right, pin the next index to the same y and the previous to the same x */
                Y(active_item->bounds[next_index]) = Y(active_item->bounds[active_item->hover_handle_index]);
                X(active_item->bounds[prev_index]) = X(active_item->bounds[active_item->hover_handle_index]);
            }
            else
            {
                /* if its top left or bottom right, pin the next to the same X and the previous to the same y */
                X(active_item->bounds[next_index]) = X(active_item->bounds[active_item->hover_handle_index]);
                Y(active_item->bounds[prev_index]) = Y(active_item->bounds[active_item->hover_handle_index]);
            }
        }
        else if (active_item->hover_handle_index < 8 && active_item->hover_handle_index % 2 == 0)
        {
            /* for vertical handles */

            /*
            **  for 4 we want 0 and 1
            **  for 6 we want 2 and 3
            */
            int prev_index = (active_item->hover_handle_index - 4) % 4;
            int next_index = (active_item->hover_handle_index - 3) % 4;
            Y(active_item->bounds[prev_index]) += Y(delta);
            Y(active_item->bounds[next_index]) += Y(delta);
        }
        else if (active_item->hover_handle_index < 8 && active_item->hover_handle_index % 2 == 1)
        {
            /* for horizontal handles */

            /*
            **  for 5 we want 1 and 2
            **  for 7 we want 3 and 0
            */
            int prev_index = (active_item->hover_handle_index - 4) % 4;
            int next_index = (active_item->hover_handle_index - 3) % 4;
            X(active_item->bounds[prev_index]) += X(delta);
            X(active_item->bounds[next_index]) += X(delta);
        }
        else if (active_item->hover_handle_index == 8)
        {
            /* for moving the whole item */

            for (int i = 0; i < 4; i++)
            {
                X(active_item->bounds[i]) += X(delta);
                Y(active_item->bounds[i]) += Y(delta);
            }
        }

        glm_vec2_copy(cursor_pos, drag_start_pos);

        
    }
}

void lc_canvas_set_cursor_button_state(int button, bool pressed)
{

    switch (button)
    {
        case MOUSE_MIDDLE_BUTTON:
        {
            if (pressed)
            {
                glm_vec2_copy(cursor_pos, pan_start_pos);
                pan_active = true;
            }
            else
            {
                pan_active = false;
            }

        } break;

        case MOUSE_LEFT_BUTTON:
        {
            if (pressed)
            {
                glm_vec2_copy(cursor_pos, drag_start_pos);
                drag_active = true;
                
                item.is_hovered = item_contains(&item, cursor_pos);

                if (item.is_hovered)
                {
                    active_item = &item;
                }
            }
            else
            {
                drag_active = false;

                active_item = NULL;
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
    return cursor_request;

}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static void render_axes()
{
    /* draw axes */

    lc_draw_line(
        (vec2){0, world_origin[1]}, 
        (vec2){viewport_size[0], world_origin[1]}, 
        IM_COL32(255, 255, 255, 150)
    );

    lc_draw_line(
        (vec2){world_origin[0], 0}, 
        (vec2){world_origin[0], viewport_size[1]},
        IM_COL32(255, 255, 255, 150)
    );
}

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
    
    vec2 minor_start = {fmodf(world_origin[0], minor_spacing) - minor_spacing, fmodf(world_origin[1], minor_spacing) - minor_spacing};
    vec2 grid_end = {viewport_size[0], viewport_size[1]};

    lc_draw_grid(minor_start, grid_end, minor_spacing, IM_COL32(255, 255, 255, 13));

    /* DRAW MAJOR GRID */
    float major_spacing = minor_spacing * major_minor_ratio;
    vec2 major_start = {fmodf(world_origin[0], major_spacing) - major_spacing, fmodf(world_origin[1], major_spacing) - major_spacing};
    lc_draw_grid(major_start, grid_end, major_spacing, IM_COL32(255, 255, 255, 25));
}

static void render_canvas_item(lc_canvas_item_t *item)
{   

    const static uint32_t fill_color = IM_COL32(255, 255, 255, 25);
    const static uint32_t fill_color_hover = IM_COL32(255, 255, 255, 50);
    const static uint32_t border_color = IM_COL32(255, 255, 255, 150);

    lc_draw_rect_filled(
        (vec2){X(item->frame[0]), Y(item->frame[0])}, 
        (vec2){X(item->frame[2]), Y(item->frame[2])}, 
        item->is_hovered ? fill_color_hover : fill_color
    );

    /* top */
    lc_draw_line(
        (vec2){X(item->frame[0]), Y(item->frame[0])}, 
        (vec2){X(item->frame[1]), Y(item->frame[1])}, 
        border_color
    );

    /* right */
    lc_draw_line(
        (vec2){X(item->frame[1]), Y(item->frame[1])}, 
        (vec2){X(item->frame[2]), Y(item->frame[2])}, 
        border_color
    );

    /* bottom */
    lc_draw_line(
        (vec2){X(item->frame[2]), Y(item->frame[2])}, 
        (vec2){X(item->frame[3]), Y(item->frame[3])}, 
        border_color
    );

    /* left */
    lc_draw_line(
        (vec2){X(item->frame[3]), Y(item->frame[3])}, 
        (vec2){X(item->frame[0]), Y(item->frame[0])}, 
        border_color
    );

    /* draw corner handles */
    lc_draw_handle((vec2){X(item->frame[0]), Y(item->frame[0])}, item->hover_handle_index == 0);
    lc_draw_handle((vec2){X(item->frame[1]), Y(item->frame[1])}, item->hover_handle_index == 1);
    lc_draw_handle((vec2){X(item->frame[2]), Y(item->frame[2])}, item->hover_handle_index == 2);
    lc_draw_handle((vec2){X(item->frame[3]), Y(item->frame[3])}, item->hover_handle_index == 3);

    /* draw side handles */
    lc_draw_handle((vec2){(X(item->frame[0]) + X(item->frame[1])) / 2.0f, Y(item->frame[0])}, item->hover_handle_index == 4);
    lc_draw_handle((vec2){X(item->frame[1]), (Y(item->frame[1]) + Y(item->frame[2])) / 2.0f}, item->hover_handle_index == 5);
    lc_draw_handle((vec2){(X(item->frame[2]) + X(item->frame[3])) / 2.0f, Y(item->frame[2])}, item->hover_handle_index == 6);
    lc_draw_handle((vec2){X(item->frame[3]), (Y(item->frame[3]) + Y(item->frame[0])) / 2.0f}, item->hover_handle_index == 7);
}

static bool item_contains(lc_canvas_item_t *item, vec4 point)
{
    for (int i = 0; i < 4; i++)
    {   

        if (glm_vec4_distance(point, item->frame[i]) <= HANDLE_CATCHMENT)
        {
            item->hover_handle_index = i;
            return true;
        } 
    }

    for (int i = 0; i < 4; i++)
    {   
        vec4 handle = {0.0f, 0.0f, 0.0f, 0.0f};

        int prev_index = i;
        int next_index = (i + 1) % 4;

        X(handle) = (X(item->frame[prev_index]) + X(item->frame[next_index])) / 2.0f;
        Y(handle) = (Y(item->frame[prev_index]) + Y(item->frame[next_index])) / 2.0f;
        
        if (glm_vec4_distance(point, handle) <= HANDLE_CATCHMENT)
        {
            item->hover_handle_index = i + 4;
            return true;
        } 
    }

    if ((X(point) >= X(item->frame[0])) && (X(point) <= X(item->frame[2]))
        && (Y(point) >= Y(item->frame[0])) && (Y(point) <= Y(item->frame[2])))
    {
        item->hover_handle_index = 8;
        return true;    
    }

    return false;
}