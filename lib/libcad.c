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
#include <time.h>

#include "types/core/object/object.h"
#include "types/core/document/document.h"
#include "types/core/textfield/textfield.h"


#include <render/gpu/gpu.h>
#include <render/vector/vector.h>
#include <render/window/window.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static double start_time;
static int viewport_width = 800;
static int viewport_height = 600;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/


cad_ctx_t  cad_create_context()
{
    printf("cad_create_context called 2\n");
    
    /* Register core types */
    object_get_type();
    document_get_type();
    text_field_get_type();

    gpu_init();
    vector_init();

    start_time = (double)clock() / CLOCKS_PER_SEC;

    /* Arrange shapes on a single plane - all at z=0 */
    float radius = 200.0f;
    float center_x = 400.0f;
    float center_y = 300.0f;
    float plane_z = 0.0f;

    /* Create border rectangle to show the plane */
    float border_left = 50.0f;
    float border_right = 750.0f;
    float border_top = 50.0f;
    float border_bottom = 550.0f;

    /* Top border line */
    vector_line_instance_t border_top_line = {
        .start = {border_left, border_top, plane_z},
        .end = {border_right, border_top, plane_z},
        .color = {1.0f, 1.0f, 1.0f, 0.5f},
        .stroke_width = 2.0f,
        .dash = 0.0f
    };
    vector_instance_t border_top_handle;
    vector_create_line(&border_top_line, &border_top_handle);

    /* Bottom border line */
    vector_line_instance_t border_bottom_line = {
        .start = {border_left, border_bottom, plane_z},
        .end = {border_right, border_bottom, plane_z},
        .color = {1.0f, 1.0f, 1.0f, 0.5f},
        .stroke_width = 2.0f,
        .dash = 0.0f
    };
    vector_instance_t border_bottom_handle;
    vector_create_line(&border_bottom_line, &border_bottom_handle);

    /* Left border line */
    vector_line_instance_t border_left_line = {
        .start = {border_left, border_top, plane_z},
        .end = {border_left, border_bottom, plane_z},
        .color = {1.0f, 1.0f, 1.0f, 0.5f},
        .stroke_width = 2.0f,
        .dash = 0.0f
    };
    vector_instance_t border_left_handle;
    vector_create_line(&border_left_line, &border_left_handle);

    /* Right border line */
    vector_line_instance_t border_right_line = {
        .start = {border_right, border_top, plane_z},
        .end = {border_right, border_bottom, plane_z},
        .color = {1.0f, 1.0f, 1.0f, 0.5f},
        .stroke_width = 2.0f,
        .dash = 0.0f
    };
    vector_instance_t border_right_handle;
    vector_create_line(&border_right_line, &border_right_handle);

    /* create a red circle (filled) at plane center */
    vector_shape_instance_t circle = {
        .center = {center_x, center_y, plane_z},
        .normal = {0.0f, 0.0f, 1.0f},
        .size = {100.0f, 100.0f},
        .color = {1.0f, 0.0f, 0.0f, 1.0f},
        .rotation = 0.0f,
        .sides = 1.0f,  /* circle */
        .start_angle = 0.0f,
        .end_angle = 0.0f,
        .fill = 0.0f,
        .stroke_width = 0.0f,  /* filled */
        .corner_radius = 0.0f,
        .dash = 0.0f
    };
    vector_instance_t circle_handle;
    if (vector_create_shape(&circle, &circle_handle)) {
        printf("Created circle with handle %u\n", circle_handle);
    } else {
        printf("Failed to create circle!\n");
    }

    /* create a blue rectangle (stroked) */
    vector_shape_instance_t rect = {
        .center = {center_x + radius * cosf(0.785f), center_y + radius * sinf(0.785f), plane_z},
        .normal = {0.0f, 0.0f, 1.0f},
        .size = {100.0f, 100.0f},
        .color = {0.0f, 0.0f, 1.0f, 1.0f},
        .rotation = 0.0f,
        .sides = 4.0f,  /* rectangle */
        .start_angle = 0.0f,
        .end_angle = 0.0f,
        .fill = 0.0f,
        .stroke_width = 5.0f,  /* stroked */
        .corner_radius = 0.0f,
        .dash = 0.0f
    };
    vector_instance_t rect_handle;
    vector_create_shape(&rect, &rect_handle);

    /* create a green triangle (filled) */
    vector_shape_instance_t triangle = {
        .center = {center_x + radius * cosf(1.57f), center_y + radius * sinf(1.57f), plane_z},
        .normal = {0.0f, 0.0f, 1.0f},
        .size = {100.0f, 100.0f},
        .color = {0.0f, 1.0f, 0.0f, 1.0f},
        .rotation = 0.0f,
        .sides = 3.0f,  /* triangle */
        .start_angle = 0.0f,
        .end_angle = 0.0f,
        .fill = 0.0f,
        .stroke_width = 0.0f,  /* filled */
        .corner_radius = 0.0f,
        .dash = 0.0f
    };
    vector_instance_t triangle_handle;
    vector_create_shape(&triangle, &triangle_handle);

    /* create a yellow rounded rectangle (filled) */
    vector_shape_instance_t rounded_rect = {
        .center = {center_x + radius * cosf(2.36f), center_y + radius * sinf(2.36f), plane_z},
        .normal = {0.0f, 0.0f, 1.0f},
        .size = {100.0f, 100.0f},
        .color = {1.0f, 1.0f, 0.0f, 1.0f},
        .rotation = 0.0f,
        .sides = 4.0f,  /* rectangle */
        .start_angle = 0.0f,
        .end_angle = 0.0f,
        .fill = 0.0f,
        .stroke_width = 0.0f,  /* filled */
        .corner_radius = 20.0f,  /* rounded corners */
        .dash = 0.0f
    };
    vector_instance_t rounded_rect_handle;
    vector_create_shape(&rounded_rect, &rounded_rect_handle);

    /* create a cyan pentagon (filled) */
    vector_shape_instance_t pentagon = {
        .center = {center_x + radius * cosf(3.14f), center_y + radius * sinf(3.14f), plane_z},
        .normal = {0.0f, 0.0f, 1.0f},
        .size = {100.0f, 100.0f},
        .color = {0.0f, 1.0f, 1.0f, 1.0f},
        .rotation = 0.0f,
        .sides = 5.0f,  /* pentagon */
        .start_angle = 0.0f,
        .end_angle = 0.0f,
        .fill = 0.0f,
        .stroke_width = 0.0f,  /* filled */
        .corner_radius = 0.0f,
        .dash = 0.0f
    };
    vector_instance_t pentagon_handle;
    vector_create_shape(&pentagon, &pentagon_handle);

    /* create a magenta hexagon (stroked) */
    vector_shape_instance_t hexagon = {
        .center = {center_x + radius * cosf(3.93f), center_y + radius * sinf(3.93f), plane_z},
        .normal = {0.0f, 0.0f, 1.0f},
        .size = {100.0f, 100.0f},
        .color = {1.0f, 0.0f, 1.0f, 1.0f},
        .rotation = 0.0f,
        .sides = 6.0f,  /* hexagon */
        .start_angle = 0.0f,
        .end_angle = 0.0f,
        .fill = 0.0f,
        .stroke_width = 3.0f,  /* stroked */
        .corner_radius = 0.0f,
        .dash = 0.0f
    };
    vector_instance_t hexagon_handle;
    vector_create_shape(&hexagon, &hexagon_handle);

    /* create a rotated orange square (filled) */
    vector_shape_instance_t rotated_square = {
        .center = {center_x + radius * cosf(4.71f), center_y + radius * sinf(4.71f), plane_z},
        .normal = {0.0f, 0.0f, 1.0f},
        .size = {100.0f, 100.0f},
        .color = {1.0f, 0.5f, 0.0f, 1.0f},
        .rotation = 0.785398f,  /* 45 degrees (PI/4) */
        .sides = 4.0f,  /* rectangle */
        .start_angle = 0.0f,
        .end_angle = 0.0f,
        .fill = 0.0f,
        .stroke_width = 0.0f,  /* filled */
        .corner_radius = 0.0f,
        .dash = 0.0f
    };
    vector_instance_t rotated_square_handle;
    vector_create_shape(&rotated_square, &rotated_square_handle);

    /* create a dashed circle (stroked) */
    float dash = vector_build_dash(20.0f, 0.5f);
    vector_shape_instance_t dashed_circle = {
        .center = {center_x + radius * cosf(5.50f), center_y + radius * sinf(5.50f), plane_z},
        .normal = {0.0f, 0.0f, 1.0f},
        .size = {100.0f, 100.0f},
        .color = {0.5f, 0.0f, 0.5f, 1.0f},
        .rotation = 0.0f,
        .sides = 1.0f,  /* circle */
        .start_angle = 0.0f,
        .end_angle = 0.0f,
        .fill = 0.0f,
        .stroke_width = 2.0f,  /* stroked */
        .corner_radius = 0.0f,
        .dash = dash  /* dashed */
    };
    vector_instance_t dashed_circle_handle;
    vector_create_shape(&dashed_circle, &dashed_circle_handle);


    return (cad_ctx_t)1;
}

void cad_destroy_context(cad_ctx_t ctx)
{

}

void cad_set_cursor_pos(int x, int y)
{
    
    
}

void cad_cursor_lost()
{
   
}

void cad_set_cursor_button_state(int button, bool pressed)
{
  
}

void cad_set_modifier_state(int modifier, bool state)
{
   
}

void cad_set_viewport(int x, int y, int vpw, int vph, int w, int h)
{
    printf("CAD set viewport: %d x %d\n", vpw, vph);

    viewport_width = vpw;
    viewport_height = vph;
}

void cad_render_viewport()
{

    printf("CAD render viewport\n");

    vec2 size;
    size[0] = (float)viewport_width;
    size[1] = (float)viewport_height;

    vec4 clear_color;
    clear_color[0] = 0.1f;
    clear_color[1] = 0.1f;
    clear_color[2] = 0.1f;
    clear_color[3] = 1.0f;
    gpu_clear_color_buffer(clear_color);
    gpu_clear_depth_buffer();

    /* Calculate time for animation */
    double current_time = (double)clock() / CLOCKS_PER_SEC;
    float time = (float)(current_time - start_time);

    /* Create perspective projection matrix */
    mat4 projection, view, vp;
    float aspect = size[0] / size[1];
    glm_perspective(glm_rad(60.0f), aspect, 0.1f, 1000.0f, projection);

    /* Create rotating camera */
    vec3 eye, center, up;
    float cam_distance = 600.0f;
    float cam_angle = time * 0.5f;  /* Rotate slowly */

    eye[0] = 400.0f + cam_distance * cosf(cam_angle);
    eye[1] = 300.0f + cam_distance * sinf(cam_angle) * 0.3f;  /* Slight vertical movement */
    eye[2] = cam_distance * sinf(cam_angle);

    center[0] = 400.0f;
    center[1] = 300.0f;
    center[2] = 0.0f;

    up[0] = 0.0f;
    up[1] = 1.0f;
    up[2] = 0.0f;

    glm_lookat(eye, center, up, view);
    glm_mat4_mul(projection, view, vp);

    /* Render with perspective projection */
    vector_render((int)size[0], (int)size[1], vp);
}

void cad_init_viewport()
{
   

}

void cad_axis_delta(int axis, float delta)
{
   
}

int cad_get_cursor_type()
{
    return 0;
}

void cad_start_modal_tool(int tool_id)
{
    
}

void cad_clear_modal_tool()
{

}

void cad_save_json(const char *path)
{

}

void cad_load_json(const char *path)
{

}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/






