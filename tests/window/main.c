/***************************************************************
**
** libcad Test File
**
** File         :  main.c
** Test         :  window
** Author       :  SH
** Created      :  2026-02-05 (YYYY-MM-DD)
** License      :  MIT
** Description  :  testing window
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdio.h>

#include <libcad/libcad.h>


#include <render/gpu/gpu.h>
#include <render/vector/vector.h>
#include <render/window/window.h>

#include <util/log/log.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static window_t window;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void draw_callback();

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

int main()
{

    //cad_create_context();

    window = window_create("123", 500, 500);

    gpu_init();
    vector_init();

    /* create a red circle (filled) */
    vector_shape_instance_t circle = {
        .center = {100.0f, 100.0f, 0.0f},
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
        .center = {250.0f, 100.0f, 0.0f},
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
        .center = {100.0f, 250.0f, 0.0f},
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
        .center = {250.0f, 250.0f, 0.0f},
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
        .center = {400.0f, 100.0f, 0.0f},
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
        .center = {400.0f, 250.0f, 0.0f},
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
        .center = {100.0f, 400.0f, 0.0f},
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
        .center = {250.0f, 400.0f, 0.0f},
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


    window_set_draw_callback(window, draw_callback);

    while (window_update())
    {   
        
    }

    return 0;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static void draw_callback()
{
    vec2 size;
    window_get_size(window, &size);

    vec4 color;
    color[0] = 1.0f;
    color[1] = 1.0f;
    color[2] = 1.0f;
    color[3] = 1.0f;
    gpu_clear_color_buffer(color);
    gpu_clear_depth_buffer();

    vector_render((int)size[0], (int)size[1]);

    window_swap_buffers(window);
}



