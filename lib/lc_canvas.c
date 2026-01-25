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

#include <jansson.h>

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

struct lc_canvas_item_t;
typedef void (*lc_canvas_draw_func_t)(struct lc_canvas_item_t *item);
typedef bool (*lc_canvas_hit_test_funct_t)(struct lc_canvas_item_t *item, vec4 point);

/* struct for items that need to be drawn to the canvas */
typedef struct lc_canvas_item_t
{
    int type;
    mat4 bounds;
    mat4 frame;
    bool is_hovered;
    int hover_handle_index;

    lc_canvas_draw_func_t draw_func;
    lc_canvas_hit_test_funct_t hit_test_func;
    size_t data_size;
    void *data;
} lc_canvas_item_t;

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static vec2 viewport_origin = {0.0f, 0.0f};
static vec2 viewport_size = {800.0f, 600.0f};

static float zoom = 1.0f;

static vec2 origin = {0.0f, 0.0f};

static vec4 cursor_pos = {0.0f, 0.0f, 0.0f, 0.0f};
static bool cursor_valid = false;

static vec2 pan_start_pos = {0.0f, 0.0f};
static bool pan_active = false;

static lc_canvas_item_t *active_item = NULL;

static vec2 drag_start_pos = {0.0f, 0.0f};
static bool drag_active = false;

static mat4 viewport_transform;
static mat4 viewport_transform_inv;
static vec4 world_origin;

static int cursor_request = CURSOR_NORMAL;

static lc_canvas_item_t item;

static int modal_tool_id = 0;

static vec4 tool_start_pos = {0.0f};
static vec4 tool_start_pos_transformed = {0.0f};
static bool tool_start_valid = false;

static lc_canvas_item_t **items = NULL;
static size_t items_count = 0;
static size_t items_capacity = 0;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void draw_line(lc_canvas_item_t *item);
static void draw_ellipse(lc_canvas_item_t *item);
static void draw_rect(lc_canvas_item_t *item);

static bool hit_test_line(lc_canvas_item_t *item, vec4 point);
static bool hit_test_ellipse(lc_canvas_item_t *item, vec4 point);
static bool hit_test_rect(lc_canvas_item_t *item, vec4 point);

static void render_axes();
static void render_grid();
static void render_canvas_item(lc_canvas_item_t *item);

static void commit_modal_tool(vec4 start, vec4 end);

static bool item_contains(lc_canvas_item_t *item, vec4 point);

static const char* labels[] = {
    "None",
    "Line",
    "Circle",
    "Rectangle"
};

static void push_canvas_item(lc_canvas_item_t *item)
{
    if (items_count >= items_capacity)
    {
        size_t new_capacity = items_capacity == 0 ? 4 : items_capacity * 2;
        lc_canvas_item_t **new_items = realloc(items, new_capacity * sizeof(lc_canvas_item_t*));
        if (!new_items)
        {
            fprintf(stderr, "lc_canvas: Failed to allocate memory for canvas items\n");
            return;
        }

        items = new_items;
        items_capacity = new_capacity;
    }

    items[items_count++] = item; 
}

void lc_canvas_save_json(const char *path)
{
    FILE *file = fopen(path, "w");
    if (!file)
    {
        fprintf(stderr, "lc_canvas: Failed to open file for saving JSON: %s\n", path);
        return;
    }

    json_t *root = json_object();
    json_t *items_array = json_array();
    
    for (size_t i = 0; i < items_count; i++)
    {
        lc_canvas_item_t *item = items[i];

        json_t *item_obj = json_object();
        json_object_set_new(item_obj, "type", json_integer(item->type));
        json_object_set_new(item_obj, "bounds", json_array());
        for (int row = 0; row < 4; row++)
        {
            for (int col = 0; col < 4; col++)
            {
                json_array_append_new(json_object_get(item_obj, "bounds"), json_integer(item->bounds[row][col]));
            }
        }

        json_array_append_new(items_array, item_obj);
        
    }

    json_object_set_new(root, "items", items_array);

    json_dumpf(root, file, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
    fclose(file);

    json_decref(root);


}

void lc_canvas_load_json(const char *path)
{
    for (size_t i = 0; i < items_count; i++)
    {
        free(items[i]);
    }

    items_count = 0;

    FILE *file = fopen(path, "r");
    if (!file)
    {
        fprintf(stderr, "lc_canvas: Failed to open file for loading JSON: %s\n", path);
        return;
    }

    json_error_t error;
    json_t *root = json_loadf(file, 0, &error);

    if (!root)
    {
        fprintf(stderr, "lc_canvas: Failed to load JSON from file: %s\n", error.text);
        fclose(file);
        return;
    }

    json_t *items_array = json_object_get(root, "items");
    if (!json_is_array(items_array))
    {
        fprintf(stderr, "lc_canvas: Invalid JSON format: 'items' is not an array\n");
        json_decref(root);
        fclose(file);
        return;
    }

    size_t index;
    json_t *item_obj;
    json_array_foreach(items_array, index, item_obj)
    {
        lc_canvas_item_t *item = malloc(sizeof(lc_canvas_item_t));
        if (!item)
        {
            fprintf(stderr, "lc_canvas: Failed to allocate memory for canvas item\n");
            continue;
        }

        json_t *type_json = json_object_get(item_obj, "type");
        if (!json_is_integer(type_json))
        {
            fprintf(stderr, "lc_canvas: Invalid JSON format: 'type' is not an integer\n");
            free(item);
            continue;
        }
        item->type = (int)json_integer_value(type_json);

        json_t *bounds_json = json_object_get(item_obj, "bounds");
        if (!json_is_array(bounds_json) || json_array_size(bounds_json) != 16)
        {
            fprintf(stderr, "lc_canvas: Invalid JSON format: 'bounds' is not a 16-element array\n");
            free(item);
            continue;
        }

        for (int row = 0; row < 4; row++)
        {
            for (int col = 0; col < 4; col++)
            {
                json_t *value_json = json_array_get(bounds_json, row * 4 + col);
                if (!json_is_integer(value_json))
                {
                    fprintf(stderr, "lc_canvas: Invalid JSON format: 'bounds' element is not an integer\n");
                    free(item);
                    continue;
                }
                item->bounds[row][col] = (float)json_integer_value(value_json);
            }
        }

        // Set draw function based on type
        switch (item->type)
        {
            case 1:
                item->draw_func = draw_line;
                item->hit_test_func = hit_test_line;
                break;
            case 2:
                item->draw_func = draw_ellipse;
                item->hit_test_func = hit_test_ellipse;
                break;
            case 3:
                item->draw_func = draw_rect;
                item->hit_test_func = hit_test_rect;
                break;
            default:
                item->draw_func = NULL;
                item->hit_test_func = NULL;
                break;
        }

        push_canvas_item(item);
    }


}
  
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

    modal_tool_id = 2;

    vec4 start = {0.0f, 0.0f, 0.0f, 1.0f};
    vec4 end = {0.0f, 1.0f, 0.0f, 1.0f};

    commit_modal_tool(start, end);

    modal_tool_id = 0;

}

void lc_canvas_render(float viewport_width, float viewport_height)
{
    
    //active_item = NULL;

    viewport_size[0] = viewport_width;
    viewport_size[1] = viewport_height;

    
#if 0
    glm_mat4_identity(viewport_transform);
    
    // First, translate to viewport center
    glm_translate(viewport_transform, (vec3){viewport_size[0] / 2.0f, viewport_size[1] / 2.0f, 0.0f});
    
    // Then scale (zooms around viewport center)
    glm_scale_uni(viewport_transform, zoom);
    
    // Finally, apply the pan offset
    glm_translate(viewport_transform, (vec3){viewport_origin[0], viewport_origin[1], 0.0f});
    glm_mat4_inv(viewport_transform, viewport_transform_inv);

    glm_mat4_mulv(viewport_transform, (vec4){0.0f, 0.0f, 0.0f, 1.0f}, world_origin);
#endif
    

    render_grid();
    render_axes();

    for (size_t i = 0; i < items_count; i++)
    {
        glm_mat4_mul(viewport_transform, items[i]->bounds, items[i]->frame);

        for (int j = 0; j < 4; j++)
        {
            float w = 1.0/items[i]->frame[j][3];
            float ndc_x = items[i]->frame[j][0] * w;
            float ndc_y = items[i]->frame[j][1] * w * -1.0f;

            items[i]->frame[j][0] = (ndc_x + 1.0f) * 0.5f * viewport_width;
            items[i]->frame[j][1] = (ndc_y + 1.0f) * 0.5f * viewport_height;
        }

        items[i]->draw_func(items[i]);
    }

    
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

    //render_canvas_item(&item);
    
    glm_mat4_mulv(viewport_transform, tool_start_pos, tool_start_pos_transformed);

    lc_draw_text((vec2){cursor_pos[0] + 15.0f, cursor_pos[1]}, labels[modal_tool_id], 14.0f, IM_COL32(255, 255, 255, 255));

    if (tool_start_valid && modal_tool_id == 1)
    {
        lc_draw_line((vec2){tool_start_pos_transformed[0], tool_start_pos_transformed[1]}, cursor_pos, IM_COL32(255, 255, 255, 255));
    }
    else if (tool_start_valid && modal_tool_id == 2)
    {
        float dist = sqrtf((cursor_pos[0] - tool_start_pos_transformed[0]) * (cursor_pos[0] - tool_start_pos_transformed[0]) +
                               (cursor_pos[1] - tool_start_pos_transformed[1]) * (cursor_pos[1] - tool_start_pos_transformed[1]));
        lc_draw_circle((vec2){tool_start_pos_transformed[0], tool_start_pos_transformed[1]}, dist);
    }
    else if (tool_start_valid && modal_tool_id == 3)
    {
        lc_draw_rect((vec2){tool_start_pos_transformed[0], tool_start_pos_transformed[1]}, cursor_pos, IM_COL32(255, 255, 255, 255));
    }                        
    else if (drag_active)
    {
        lc_draw_rect_filled((vec2){fminf(drag_start_pos[0], cursor_pos[0]), fminf(drag_start_pos[1], cursor_pos[1])},
                            (vec2){fmaxf(drag_start_pos[0], cursor_pos[0]), fmaxf(drag_start_pos[1], cursor_pos[1])},
                            IM_COL32(255, 255, 255, 50));
    }
}

void lc_canvas_set_cursor_pos(float x, float y)
{
    cursor_pos[0] = x;
    cursor_pos[1] = y;
    cursor_valid = true;

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
    else 
    {
        active_item = NULL;

        vec4 screen_mouse_pos = {x, y, 0.0f, 1.0f};
        for (size_t i = 0; i < items_count; i++)
        {
            //items[i]->is_hovered = false;
            //items[i]->hover_handle_index = -1;

            if (items[i]->hit_test_func(items[i], screen_mouse_pos))
            {
                //items[i]->is_hovered = true;
                active_item = items[i];
                break;
            }
        }
    }
}

void lc_canvas_set_cursor_lost()
{
    cursor_valid = false;
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
                if (modal_tool_id)
                {
                    if (!tool_start_valid)
                    {
                        tool_start_pos[0] = cursor_pos[0];
                        tool_start_pos[1] = cursor_pos[1];
                        tool_start_pos[2] = 0.0f;
                        tool_start_pos[3] = 1.0f;

                        glm_mat4_mulv(viewport_transform_inv, tool_start_pos, tool_start_pos);

                        tool_start_valid = true;
                    }
                    else
                    {
                        tool_start_valid = false;

                        vec4 end_pos;
                        end_pos[0] = cursor_pos[0];
                        end_pos[1] = cursor_pos[1];
                        end_pos[2] = 0.0f;
                        end_pos[3] = 1.0f;

                        glm_mat4_mulv(viewport_transform_inv, end_pos, end_pos);
                        commit_modal_tool(tool_start_pos, end_pos);
                    }
                }
                else
                {
                    glm_vec2_copy(cursor_pos, drag_start_pos);
                    drag_active = true;
                    
                    item.is_hovered = item_contains(&item, cursor_pos);

                    if (item.is_hovered)
                    {
                        active_item = &item;
                    }
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

void lc_canvas_set_modal_tool(int tool_id)
{
    modal_tool_id = tool_id;
    tool_start_valid = false;
}

void lc_canvas_set_view_matrix(mat4 matrix)
{
    glm_mat4_copy(matrix, viewport_transform);
    
    //glm_translate_x(viewport_transform, viewport_size[0] / 2.0f);
    //glm_translate_y(viewport_transform, viewport_size[1] / 2.0f);

    glm_mat4_inv(viewport_transform, viewport_transform_inv);
    
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

static void commit_modal_tool(vec4 start, vec4 end)
{
    switch (modal_tool_id)
    {
        case 1: // Line
        {
            lc_canvas_item_t *new_item = calloc(1, sizeof(lc_canvas_item_t));
            if (!new_item)
            {
                fprintf(stderr, "lc_canvas: Failed to allocate memory for new canvas item\n");
                return;
            }

            new_item->type = 1; // Line
            new_item->bounds[0][0] = start[0];
            new_item->bounds[0][1] = start[1];
            new_item->bounds[1][0] = end[0];
            new_item->bounds[1][1] = end[1];
            new_item->bounds[0][3] = 1.0f;
            new_item->bounds[1][3] = 1.0f;

            new_item->draw_func = draw_line;
            new_item->hit_test_func = hit_test_line;

            push_canvas_item(new_item);

        } break;

        case 2: // Ellipse
        {
            lc_canvas_item_t *new_item = calloc(1, sizeof(lc_canvas_item_t));
            if (!new_item)
            {
                fprintf(stderr, "lc_canvas: Failed to allocate memory for new canvas item\n");
                return;
            }

            new_item->type = 2; // Ellipse

            float radius = sqrtf((end[0] - start[0]) * (end[0] - start[0]) +
                               (end[1] - start[1]) * (end[1] - start[1]));

            new_item->bounds[0][0] = start[0] - radius;
            new_item->bounds[0][1] = start[1] - radius;
            new_item->bounds[1][0] = start[0] + radius;
            new_item->bounds[1][1] = start[1] - radius;
            new_item->bounds[2][0] = start[0] + radius;
            new_item->bounds[2][1] = start[1] + radius;
            new_item->bounds[3][0] = start[0] - radius;
            new_item->bounds[3][1] = start[1] + radius;
            new_item->bounds[0][3] = 1.0f;
            new_item->bounds[1][3] = 1.0f;
            new_item->bounds[2][3] = 1.0f;
            new_item->bounds[3][3] = 1.0f;

            new_item->draw_func = draw_ellipse;
            new_item->hit_test_func = hit_test_ellipse;

            push_canvas_item(new_item);
        } break;

        case 3: // Rectangle
        {
            lc_canvas_item_t *new_item = calloc(1, sizeof(lc_canvas_item_t));
            if (!new_item)
            {
                fprintf(stderr, "lc_canvas: Failed to allocate memory for new canvas item\n");
                return;
            }

            new_item->type = 3; // Rectangle
            new_item->bounds[0][0] = start[0];
            new_item->bounds[0][1] = start[1];
            new_item->bounds[1][0] = end[0];
            new_item->bounds[1][1] = start[1];
            new_item->bounds[2][0] = end[0];
            new_item->bounds[2][1] = end[1];
            new_item->bounds[3][0] = start[0];
            new_item->bounds[3][1] = end[1];
            new_item->bounds[0][3] = 1.0f;
            new_item->bounds[1][3] = 1.0f;
            new_item->bounds[2][3] = 1.0f;
            new_item->bounds[3][3] = 1.0f;

            new_item->draw_func = draw_rect;
            new_item->hit_test_func = hit_test_rect;

            push_canvas_item(new_item);
        } break;
    }
}

static void draw_line(lc_canvas_item_t *item)
{
    lc_draw_line(
        (vec2){item->frame[0][0], item->frame[0][1]},
        (vec2){item->frame[1][0], item->frame[1][1]},
        item == active_item ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 255, 255, 150)
    );
}

static void draw_ellipse(lc_canvas_item_t *item)
{
    vec2 center;
    center[0] = (item->frame[0][0] + item->frame[2][0]) / 2.0f;
    center[1] = (item->frame[0][1] + item->frame[2][1]) / 2.0f;

    vec2 size;
    size[0] = fabsf(item->frame[1][0] - item->frame[0][0]);
    size[1] = fabsf(item->frame[2][1] - item->frame[1][1]);

    lc_draw_ellipse(center, size,
        item == active_item ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 255, 255, 150)
    );
}

static void draw_rect(lc_canvas_item_t *item)
{
    lc_draw_rect(
        (vec2){item->frame[0][0], item->frame[0][1]},
        (vec2){item->frame[2][0], item->frame[2][1]},
        item == active_item ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 255, 255, 150)
    );
}

static bool hit_test_line(lc_canvas_item_t *item, vec4 point)
{

    vec2 A = { X(item->frame[0]), Y(item->frame[0]) };
    vec2 B = { X(item->frame[1]), Y(item->frame[1]) };
    vec2 P = { X(point), Y(point) };

    vec2 AB = { X(B) - X(A), Y(B) - Y(A) };
    vec2 AP = { X(P) - X(A), Y(P) - Y(A) };

    float ab_len2 = X(AB)*X(AB) + Y(AB)*Y(AB);
    if (ab_len2 == 0.0f)
        return false; // degenerate segment

    float t = (X(AP)*X(AB) + Y(AP)*Y(AB)) / ab_len2;
    t = fmaxf(0.0f, fminf(1.0f, t));

    vec2 C = { X(A) + t*X(AB), Y(A) + t*Y(AB) };

    float dx = X(P) - X(C);
    float dy = Y(P) - Y(C);
    float dist = sqrtf(dx*dx + dy*dy);

    return dist <= HANDLE_CATCHMENT;
    
}

static bool hit_test_ellipse(lc_canvas_item_t *item, vec4 point)
{
    vec2 center = {
        (item->frame[0][0] + item->frame[2][0]) / 2.0f,
        (item->frame[0][1] + item->frame[2][1]) / 2.0f
    };

    float radius_x = fabsf(item->frame[1][0] - item->frame[0][0]) / 2.0f;
    float radius_y = fabsf(item->frame[2][1] - item->frame[1][1]) / 2.0f;
    float dx = X(point) - center[0];
    float dy = Y(point) - center[1];

    return ((dx * dx) / (radius_x * radius_x) + (dy * dy) / (radius_y * radius_y)) <= 1.0f;
}

static bool hit_test_rect(lc_canvas_item_t *item, vec4 point)
{

    float min_x = fminf(X(item->frame[0]), X(item->frame[2]));
    float max_x = fmaxf(X(item->frame[0]), X(item->frame[2]));
    float min_y = fminf(Y(item->frame[0]), Y(item->frame[2]));
    float max_y = fmaxf(Y(item->frame[0]), Y(item->frame[2]));

    return (X(point) >= min_x) && (X(point) <= max_x)
        && (Y(point) >= min_y) && (Y(point) <= max_y);
}
