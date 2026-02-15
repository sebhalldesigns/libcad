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
#include "types/core/plane/plane.h"
#include "types/core/axis/axis.h"
#include "types/core/camera/camera.h"


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
static document_t* current_document = NULL;
static camera_t* current_camera = NULL;

/* Input state for camera interaction */
static int cursor_x = 0;
static int cursor_y = 0;
static int last_cursor_x = 0;
static int last_cursor_y = 0;
static bool middle_button_down = false;
static bool shift_modifier = false;

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
    plane_get_type();
    axis_get_type();
    camera_get_type();

    gpu_init();
    vector_init();

    start_time = (double)clock() / CLOCKS_PER_SEC;

    /* Create default document with 3 planes and 3 axes */
    current_document = document_new();
    object_set_name(DOCUMENT_AS_OBJECT(current_document), "Default Document");

    printf("Created document with %zu children (3 planes + 3 axes)\n",
           document_get_child_count(current_document));

    /* Create camera with default CAD-style view */
    current_camera = camera_new();
    object_set_name(CAMERA_AS_OBJECT(current_camera), "Main Camera");

    /* Camera is already initialized with good defaults in camera_init */
    /* Set aspect ratio based on viewport */
    camera_set_projection(current_camera, 45.0f,
                         (float)viewport_width / (float)viewport_height,
                         0.1f, 1000.0f);

    return (cad_ctx_t)1;
}

void cad_destroy_context(cad_ctx_t ctx)
{
    /* Free the camera */
    if (current_camera) {
        camera_free(current_camera);
        current_camera = NULL;
    }

    /* Free the document (this will free all planes and axes) */
    if (current_document) {
        document_free(current_document);
        current_document = NULL;
    }
}

void cad_set_cursor_pos(int x, int y)
{
    cursor_x = x;
    cursor_y = y;

    /* If middle button is down, perform camera interaction */
    if (middle_button_down && current_camera) {
        /* Calculate delta from last position */
        float delta_x = (float)(cursor_x - last_cursor_x);
        float delta_y = (float)(cursor_y - last_cursor_y);

        if (shift_modifier) {
            /* Shift + middle mouse = pan */
            camera_pan(current_camera, delta_x, delta_y);
        } else {
            /* Middle mouse = orbit */
            camera_orbit(current_camera, delta_x, delta_y);
        }
    }

    /* Update last position */
    last_cursor_x = cursor_x;
    last_cursor_y = cursor_y;
}

void cad_cursor_lost()
{
   
}

void cad_set_cursor_button_state(int button, bool pressed)
{
    if (button == MOUSE_MIDDLE_BUTTON) {
        middle_button_down = pressed;

        /* Reset last position when button is pressed to avoid jumps */
        if (pressed) {
            last_cursor_x = cursor_x;
            last_cursor_y = cursor_y;
        }
    }
}

void cad_set_modifier_state(int modifier, bool state)
{
    if (modifier == MODIFIER_SHIFT) {
        shift_modifier = state;
    }
}

void cad_set_viewport(int x, int y, int vpw, int vph, int w, int h)
{
    printf("CAD set viewport: %d x %d\n", vpw, vph);

    viewport_width = vpw;
    viewport_height = vph;

    /* Update camera aspect ratio */
    if (current_camera) {
        float aspect = (float)vpw / (float)vph;
        camera_set_projection(current_camera, current_camera->fov, aspect,
                             current_camera->near_clip, current_camera->far_clip);
    }
}

void cad_set_dpi_scale(float scale)
{
    printf("CAD set dpi scale: %.2f\n", scale);

    vector_set_dpi_scale(scale);
}

void cad_render_viewport()
{

    /*printf("CAD render viewport\n");*/

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

    /* Use camera to generate view-projection matrix */
    mat4 vp;
    if (current_camera) {
        /* Update camera aspect ratio if viewport changed */
        float aspect = size[0] / size[1];
        camera_set_projection(current_camera, current_camera->fov, aspect,
                             current_camera->near_clip, current_camera->far_clip);

        /* Get view-projection matrix from camera */
        camera_get_view_projection_matrix(current_camera, vp);
    } else {
        /* Fallback to identity if no camera */
        glm_mat4_identity(vp);
    }

    /* Render with camera's view-projection matrix */
    vector_render((int)size[0], (int)size[1], vp);
}

void cad_init_viewport()
{
   

}

void cad_axis_delta(int axis, float delta)
{
    /* Assuming axis 0 is the scroll wheel (or vertical axis) */
    if (axis == 0) {
        /* Scroll wheel controls zoom */
        cad_camera_zoom(delta);
    }
}

void cad_camera_orbit(float delta_x, float delta_y)
{
    if (!current_camera) return;
    camera_orbit(current_camera, delta_x, delta_y);
}

void cad_camera_pan(float delta_x, float delta_y)
{
    if (!current_camera) return;
    camera_pan(current_camera, delta_x, delta_y);
}

void cad_camera_zoom(float delta)
{
    if (!current_camera) return;
    camera_zoom(current_camera, delta);
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






