/***************************************************************
**
** libcad Source File
**
** File         :  gpu.c
** Module       :  render/gpu
** Author       :  SH
** Created      :  2026-01-26 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad GPU API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <libcad/libcad.h>
#include <stdio.h>

#ifdef USE_GLES
    #include <GLES3/gl3.h>
#else
    #include <glad/glad.h>
#endif

#include <cglm/cglm.h>

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

void gpu_init(void)
{
    if (!gladLoadGL())
    {
        #if DEBUG
        log_error("Failed to initialize GLAD\n");
        #endif
        return;
    }
}

void gpu_clear_color_buffer(vec4 color)
{
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
}

void gpu_clear_depth_buffer(void)
{
    glClear(GL_DEPTH_BUFFER_BIT);
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/






