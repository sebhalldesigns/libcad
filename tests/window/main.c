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

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

int main()
{

    //cad_create_context();

    window_t window = window_create("123", 500, 500);

    gpu_init();
    vector_init();

    while (window_update())
    {
        vec2 size;
        window_get_size(window, &size);

        vec4 color;
        color[0] = 1.0f;
        color[1] = 0.0f;
        color[2] = 0.0f;
        color[3] = 1.0f;
        gpu_clear_color_buffer(color);

        vector_render((int)size[0], (int)size[1]);

        window_swap_buffers(window);
    }

    return 0;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/






