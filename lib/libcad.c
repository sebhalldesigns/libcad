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
#include <string.h>

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
#include "lc_entity.h"
#include "lc_constraint.h"
#include "lc_undo.h"
#include "libcad_internal.h"

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
    /* convert to viewport-relative coordinates for picking */
    float vp_x = (float)(x - x_pos);
    float vp_y = (float)(y - y_pos);
    lc_draw_set_cursor_pos(vp_x, vp_y);
}

void cad_cursor_lost()
{
    lc_canvas_set_cursor_lost();
    lc_scene_set_cursor_lost();
    lc_draw_set_cursor_lost();
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
    /* set viewport to panel dimensions (convert from top-left to bottom-left origin) */
    int gl_y = height - y_pos - vp_height;
    glViewport(x_pos, gl_y, vp_width, vp_height);

    /* build render context for this frame */
    lc_render_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    lc_scene_compute_context(&ctx, (float)vp_width, (float)vp_height);

    /* render 3D scene */
    lc_scene_render((float)vp_width, (float)vp_height);

    /* render 2D vector shapes using context */
    lc_draw_render_ctx(&ctx);
}

void cad_init_viewport()
{
    printf("cad_init_viewport called\n");

    #ifndef EMSCRIPTEN
    if (!gladLoadGL() && !gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return;
    }
    #endif

    printf("GLAD initialized successfully\n");
    printf("OpenGL %s\n", glGetString(GL_VERSION));

    /* Initialize entity system */
    lc_entity_init();

    /* Initialize constraint system */
    lc_constraint_init();

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

const char* cad_get_hovered_name(void)
{
    return lc_draw_get_hovered_name();
}

int cad_get_hovered_id(void)
{
    return lc_draw_get_hovered_id();
}

/* ---- Document Model (Phase 2) ---- */

cad_sketch_t cad_create_sketch(cad_ctx_t ctx)
{
    (void)ctx;

    /* Create sketch at origin, XY plane */
    vec3 origin = {0.0f, 0.0f, 0.0f};
    vec3 normal = {0.0f, 0.0f, 1.0f};
    vec3 x_axis = {1.0f, 0.0f, 0.0f};

    lc_entity_handle_t handle = lc_entity_create_sketch(origin, normal, x_axis);
    if (handle == LC_ENTITY_INVALID)
    {
        printf("[cad_create_sketch] Failed to create sketch\n");
        return CAD_INVALID_ENTITY;
    }

    printf("[cad_create_sketch] Created sketch (handle: 0x%08X)\n", handle);
    return (cad_sketch_t)handle;
}

cad_entity_t cad_sketch_add_line(cad_ctx_t ctx, cad_sketch_t sketch,
                                  float x1, float y1, float x2, float y2)
{
    (void)ctx;

    vec2 start = {x1, y1};
    vec2 end = {x2, y2};
    uint32_t color = 0xFFFFFFFF; /* White */
    float thickness = 2.0f;

    lc_entity_handle_t handle = lc_entity_create_line((lc_entity_handle_t)sketch,
                                                        start, end, color, thickness);
    if (handle == LC_ENTITY_INVALID)
    {
        printf("[cad_sketch_add_line] Failed to create line\n");
        return CAD_INVALID_ENTITY;
    }

    printf("[cad_sketch_add_line] Created line (handle: 0x%08X)\n", handle);
    return (cad_entity_t)handle;
}

cad_entity_t cad_sketch_add_circle(cad_ctx_t ctx, cad_sketch_t sketch,
                                    float cx, float cy, float radius)
{
    (void)ctx;

    vec2 center = {cx, cy};
    uint32_t color = 0xFFFFFFFF; /* White */

    lc_entity_handle_t handle = lc_entity_create_circle((lc_entity_handle_t)sketch,
                                                          center, radius, color);
    if (handle == LC_ENTITY_INVALID)
    {
        printf("[cad_sketch_add_circle] Failed to create circle\n");
        return CAD_INVALID_ENTITY;
    }

    printf("[cad_sketch_add_circle] Created circle (handle: 0x%08X)\n", handle);
    return (cad_entity_t)handle;
}

cad_entity_t cad_sketch_add_rect(cad_ctx_t ctx, cad_sketch_t sketch,
                                  float x1, float y1, float x2, float y2)
{
    (void)ctx;

    vec2 min = {x1, y1};
    vec2 max = {x2, y2};
    uint32_t color = 0xFFFFFFFF; /* White */

    lc_entity_handle_t handle = lc_entity_create_rect((lc_entity_handle_t)sketch,
                                                        min, max, color);
    if (handle == LC_ENTITY_INVALID)
    {
        printf("[cad_sketch_add_rect] Failed to create rectangle\n");
        return CAD_INVALID_ENTITY;
    }

    printf("[cad_sketch_add_rect] Created rectangle (handle: 0x%08X)\n", handle);
    return (cad_entity_t)handle;
}

void cad_delete_entity(cad_ctx_t ctx, cad_entity_t entity)
{
    (void)ctx;

    if (lc_entity_destroy((lc_entity_handle_t)entity))
    {
        printf("[cad_delete_entity] Deleted entity (handle: 0x%08X)\n", entity);
    }
    else
    {
        printf("[cad_delete_entity] Failed to delete entity (handle: 0x%08X)\n", entity);
    }
}

void cad_set_entity_name(cad_ctx_t ctx, cad_entity_t entity, const char *name)
{
    (void)ctx;
    (void)entity;
    (void)name;
    printf("STUB: cad_set_entity_name\n");
}

const char* cad_get_entity_name(cad_ctx_t ctx, cad_entity_t entity)
{
    (void)ctx;
    (void)entity;
    printf("STUB: cad_get_entity_name\n");
    return NULL;
}

void cad_select_entity(cad_ctx_t ctx, cad_entity_t entity)
{
    (void)ctx;
    (void)entity;
    printf("STUB: cad_select_entity\n");
}

void cad_deselect_all(cad_ctx_t ctx)
{
    (void)ctx;
    printf("STUB: cad_deselect_all\n");
}

int cad_get_selection_count(cad_ctx_t ctx)
{
    (void)ctx;
    printf("STUB: cad_get_selection_count\n");
    return 0;
}

void cad_undo(cad_ctx_t ctx)
{
    (void)ctx;
    printf("STUB: cad_undo\n");
}

void cad_redo(cad_ctx_t ctx)
{
    (void)ctx;
    printf("STUB: cad_redo\n");
}

bool cad_can_undo(cad_ctx_t ctx)
{
    (void)ctx;
    printf("STUB: cad_can_undo\n");
    return false;
}

bool cad_can_redo(cad_ctx_t ctx)
{
    (void)ctx;
    printf("STUB: cad_can_redo\n");
    return false;
}

/* ---- Constraint System (Phase 3) ---- */

cad_entity_t cad_constraint_distance_point_point(cad_ctx_t ctx,
                                                  cad_entity_t point1,
                                                  cad_entity_t point2,
                                                  float distance)
{
    (void)ctx;
    lc_entity_handle_t h1 = (lc_entity_handle_t)point1;
    lc_entity_handle_t h2 = (lc_entity_handle_t)point2;
    lc_entity_handle_t constraint = lc_constraint_create_distance_point_point(h1, h2, distance);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_distance_point_line(cad_ctx_t ctx,
                                                 cad_entity_t point,
                                                 cad_entity_t line,
                                                 float distance)
{
    (void)ctx;
    lc_entity_handle_t h_point = (lc_entity_handle_t)point;
    lc_entity_handle_t h_line = (lc_entity_handle_t)line;
    lc_entity_handle_t constraint = lc_constraint_create_distance_point_line(h_point, h_line, distance);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_angle_line_line(cad_ctx_t ctx,
                                             cad_entity_t line1,
                                             cad_entity_t line2,
                                             float angle_radians)
{
    (void)ctx;
    lc_entity_handle_t h1 = (lc_entity_handle_t)line1;
    lc_entity_handle_t h2 = (lc_entity_handle_t)line2;
    lc_entity_handle_t constraint = lc_constraint_create_angle_line_line(h1, h2, angle_radians);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_parallel(cad_ctx_t ctx,
                                      cad_entity_t line1,
                                      cad_entity_t line2)
{
    (void)ctx;
    lc_entity_handle_t h1 = (lc_entity_handle_t)line1;
    lc_entity_handle_t h2 = (lc_entity_handle_t)line2;
    lc_entity_handle_t constraint = lc_constraint_create_parallel(h1, h2);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_perpendicular(cad_ctx_t ctx,
                                           cad_entity_t line1,
                                           cad_entity_t line2)
{
    (void)ctx;
    lc_entity_handle_t h1 = (lc_entity_handle_t)line1;
    lc_entity_handle_t h2 = (lc_entity_handle_t)line2;
    lc_entity_handle_t constraint = lc_constraint_create_perpendicular(h1, h2);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_horizontal(cad_ctx_t ctx, cad_entity_t line)
{
    (void)ctx;
    lc_entity_handle_t h_line = (lc_entity_handle_t)line;
    lc_entity_handle_t constraint = lc_constraint_create_horizontal(h_line);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_vertical(cad_ctx_t ctx, cad_entity_t line)
{
    (void)ctx;
    lc_entity_handle_t h_line = (lc_entity_handle_t)line;
    lc_entity_handle_t constraint = lc_constraint_create_vertical(h_line);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_coincident_point_point(cad_ctx_t ctx,
                                                    cad_entity_t point1,
                                                    cad_entity_t point2)
{
    (void)ctx;
    lc_entity_handle_t h1 = (lc_entity_handle_t)point1;
    lc_entity_handle_t h2 = (lc_entity_handle_t)point2;
    lc_entity_handle_t constraint = lc_constraint_create_coincident_point_point(h1, h2);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_coincident_point_line(cad_ctx_t ctx,
                                                   cad_entity_t point,
                                                   cad_entity_t line)
{
    (void)ctx;
    lc_entity_handle_t h_point = (lc_entity_handle_t)point;
    lc_entity_handle_t h_line = (lc_entity_handle_t)line;
    lc_entity_handle_t constraint = lc_constraint_create_coincident_point_line(h_point, h_line);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_coincident_point_circle(cad_ctx_t ctx,
                                                     cad_entity_t point,
                                                     cad_entity_t circle)
{
    (void)ctx;
    lc_entity_handle_t h_point = (lc_entity_handle_t)point;
    lc_entity_handle_t h_circle = (lc_entity_handle_t)circle;
    lc_entity_handle_t constraint = lc_constraint_create_coincident_point_circle(h_point, h_circle);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_tangent_line_circle(cad_ctx_t ctx,
                                                 cad_entity_t line,
                                                 cad_entity_t circle)
{
    (void)ctx;
    lc_entity_handle_t h_line = (lc_entity_handle_t)line;
    lc_entity_handle_t h_circle = (lc_entity_handle_t)circle;
    lc_entity_handle_t constraint = lc_constraint_create_tangent_line_circle(h_line, h_circle);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_tangent_circle_circle(cad_ctx_t ctx,
                                                   cad_entity_t circle1,
                                                   cad_entity_t circle2)
{
    (void)ctx;
    lc_entity_handle_t h1 = (lc_entity_handle_t)circle1;
    lc_entity_handle_t h2 = (lc_entity_handle_t)circle2;
    lc_entity_handle_t constraint = lc_constraint_create_tangent_circle_circle(h1, h2);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_equal_length(cad_ctx_t ctx,
                                          cad_entity_t line1,
                                          cad_entity_t line2)
{
    (void)ctx;
    lc_entity_handle_t h1 = (lc_entity_handle_t)line1;
    lc_entity_handle_t h2 = (lc_entity_handle_t)line2;
    lc_entity_handle_t constraint = lc_constraint_create_equal_length(h1, h2);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_equal_radius(cad_ctx_t ctx,
                                          cad_entity_t circle1,
                                          cad_entity_t circle2)
{
    (void)ctx;
    lc_entity_handle_t h1 = (lc_entity_handle_t)circle1;
    lc_entity_handle_t h2 = (lc_entity_handle_t)circle2;
    lc_entity_handle_t constraint = lc_constraint_create_equal_radius(h1, h2);
    return (cad_entity_t)constraint;
}

cad_entity_t cad_constraint_fix_point(cad_ctx_t ctx, cad_entity_t point)
{
    (void)ctx;
    lc_entity_handle_t h_point = (lc_entity_handle_t)point;
    lc_entity_handle_t constraint = lc_constraint_create_fix_point(h_point);
    return (cad_entity_t)constraint;
}

bool cad_constraint_delete(cad_ctx_t ctx, cad_entity_t constraint)
{
    (void)ctx;
    lc_entity_handle_t h_constraint = (lc_entity_handle_t)constraint;
    return lc_constraint_destroy(h_constraint);
}

bool cad_constraint_set_value(cad_ctx_t ctx, cad_entity_t constraint, float value)
{
    (void)ctx;
    lc_entity_handle_t h_constraint = (lc_entity_handle_t)constraint;

    /* Get constraint data */
    lc_constraint_data_t *data = (lc_constraint_data_t*)lc_entity_get_data(h_constraint);
    if (data == NULL)
    {
        return false;
    }

    /* Record undo command */
    lc_undo_command_t cmd;
    cmd.type = LC_COMMAND_MODIFY_PROPERTY;
    cmd.entity = h_constraint;
    cmd.data.modify_property.property_id = LC_PROPERTY_CONSTRAINT_VALUE;

    /* Serialize old and new values (just store float as bytes) */
    memcpy(cmd.data.modify_property.old_value, &data->value, sizeof(float));
    memcpy(cmd.data.modify_property.new_value, &value, sizeof(float));
    memset(cmd.data.modify_property.old_value + sizeof(float), 0,
           LC_PROPERTY_VALUE_MAX_SIZE - sizeof(float));
    memset(cmd.data.modify_property.new_value + sizeof(float), 0,
           LC_PROPERTY_VALUE_MAX_SIZE - sizeof(float));

    lc_undo_record_command(&cmd);

    /* Update value */
    data->value = value;

    /* Invalidate graph (constraint parameters changed) */
    lc_entity_handle_t parent = lc_entity_get_parent(h_constraint);
    if (parent != LC_ENTITY_INVALID && lc_entity_get_type(parent) == LC_ENTITY_TYPE_SKETCH)
    {
        lc_constraint_invalidate_graph(parent);
    }

    return true;
}

float cad_constraint_get_value(cad_ctx_t ctx, cad_entity_t constraint)
{
    (void)ctx;
    lc_entity_handle_t h_constraint = (lc_entity_handle_t)constraint;

    /* Get constraint data */
    lc_constraint_data_t *data = (lc_constraint_data_t*)lc_entity_get_data(h_constraint);
    if (data == NULL)
    {
        return 0.0f;
    }

    return data->value;
}

float cad_constraint_get_error(cad_ctx_t ctx, cad_entity_t constraint)
{
    (void)ctx;
    lc_entity_handle_t h_constraint = (lc_entity_handle_t)constraint;
    return lc_constraint_get_error(h_constraint);
}

bool cad_constraint_is_satisfied(cad_ctx_t ctx, cad_entity_t constraint)
{
    (void)ctx;
    lc_entity_handle_t h_constraint = (lc_entity_handle_t)constraint;

    /* Evaluate constraint to update satisfaction flag */
    lc_constraint_evaluate(h_constraint);

    /* Get constraint data */
    lc_constraint_data_t *data = (lc_constraint_data_t*)lc_entity_get_data(h_constraint);
    if (data == NULL)
    {
        return false;
    }

    return (data->flags & LC_CONSTRAINT_FLAG_SATISFIED) != 0;
}

int cad_sketch_get_dof(cad_ctx_t ctx, cad_sketch_t sketch)
{
    (void)ctx;
    lc_entity_handle_t h_sketch = (lc_entity_handle_t)sketch;
    return lc_constraint_get_dof(h_sketch);
}

bool cad_sketch_is_fully_constrained(cad_ctx_t ctx, cad_sketch_t sketch)
{
    (void)ctx;
    lc_entity_handle_t h_sketch = (lc_entity_handle_t)sketch;
    return lc_constraint_is_fully_constrained(h_sketch);
}

bool cad_sketch_is_over_constrained(cad_ctx_t ctx, cad_sketch_t sketch)
{
    (void)ctx;
    lc_entity_handle_t h_sketch = (lc_entity_handle_t)sketch;
    return lc_constraint_is_over_constrained(h_sketch);
}

int cad_sketch_get_constraint_count(cad_ctx_t ctx, cad_sketch_t sketch)
{
    (void)ctx;
    lc_entity_handle_t h_sketch = (lc_entity_handle_t)sketch;

    /* Count constraint entities in sketch */
    int count = 0;
    lc_entity_handle_t child = lc_entity_get_first_child(h_sketch);
    while (child != LC_ENTITY_INVALID)
    {
        if (lc_entity_get_type(child) == LC_ENTITY_TYPE_CONSTRAINT)
        {
            count++;
        }
        child = lc_entity_get_next_sibling(child);
    }

    return count;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/






