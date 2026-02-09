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

    for (int x = 0; x < 3440; x += 2)
    {
        for (int y = 0; y < 1400; y += 2)
        {
            /* create a line */
            vector_line_instance_t line_data = {
                .start = {(float)x, (float)y, 0.0f},
                .end = {(float)x, (float)y + 1.0f, 0.0f},
                .color = {0.0f, 0.0f, 0.0f, 1.0f},  // red
                .stroke_width = 1.0f,
                .dash = 0.0f  // solid line
            };

            vector_instance_t line_handle;
            vector_create_line(&line_data, &line_handle);

        }
    }
   

    
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

    vector_render((int)size[0], (int)size[1]);

    window_swap_buffers(window);
}



