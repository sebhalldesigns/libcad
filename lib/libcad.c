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
#include <jansson.h>
#include <util/log/log.h>

#include "types/core/object/object.h"
#include "types/core/document/document.h"
#include "types/core/textfield/textfield.h"
#include "types/core/plane/plane.h"
#include "types/core/axis/axis.h"
#include "types/core/camera/camera.h"
#include "types/sketch/sketch/sketch.h"


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

static uint32_t cad_plane_entity_id_from_plane(const plane_t* plane)
{
    if (!plane || plane->rectangle_handle == VECTOR_INVALID_INSTANCE) {
        return VECTOR_INVALID_INSTANCE;
    }

    return 0x20000000u | (plane->rectangle_handle & 0x0FFFFFFFu);
}

static plane_t* cad_find_plane_by_entity_id(uint32_t plane_entity_id)
{
    if (!current_document) {
        return NULL;
    }

    if (plane_entity_id == 0u || plane_entity_id == VECTOR_INVALID_INSTANCE) {
        return NULL;
    }

    if ((plane_entity_id & 0xF0000000u) != 0x20000000u) {
        return NULL;
    }

    object_t* root = DOCUMENT_AS_OBJECT(current_document);
    const size_t child_count = object_get_child_count(root);

    for (size_t i = 0; i < child_count; i++) {
        object_t* child = object_get_child(root, i);
        if (!child || !type_is_a((type_handle_t)LIBCAD_GET_CLASS(child), plane_get_type())) {
            continue;
        }

        plane_t* plane = PLANE(child);
        if (cad_plane_entity_id_from_plane(plane) == plane_entity_id) {
            return plane;
        }
    }

    return NULL;
}

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
    //printf("CAD set viewport: %d x %d\n", vpw, vph);

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
    //printf("CAD set dpi scale: %.2f\n", scale);

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

uint32_t cad_pick_entity(int screen_x, int screen_y)
{
    if (!current_camera) {
        printf("cad_pick_entity: No camera\n");
        return 0xFFFFFFFF;
    }

    /* Picking uses the same view-projection as rendering */
    mat4 view_projection;
    camera_get_view_projection_matrix(current_camera, view_projection);

    /* Call vector renderer picking */
    uint32_t entity_id = vector_pick_entity(screen_x, screen_y,
                                           viewport_width, viewport_height,
                                           view_projection);

    printf("cad_pick_entity: (%d,%d) -> 0x%08X\n", screen_x, screen_y, entity_id);
    return entity_id;
}

void cad_set_hovered_entity(uint32_t entity_id)
{
    vector_set_hovered_entity(entity_id);
}

uint32_t cad_get_hovered_entity()
{
    return vector_get_hovered_entity();
}

void cad_set_selected_entity(uint32_t entity_id)
{
    vector_set_selected_entity(entity_id);
}

uint32_t cad_get_selected_entity()
{
    return vector_get_selected_entity();
}

const char* cad_get_document_json()
{
    if (!current_document) {
        return "{}";
    }

    /* Convert document to JSON */
    json_t* json = document_to_json(current_document);
    if (!json) {
        return "{}";
    }

    /* Convert JSON to string (JSON_COMPACT for smaller output) */
    static char* json_string = NULL;
    if (json_string) {
        free(json_string);
    }
    json_string = json_dumps(json, JSON_COMPACT);
    json_decref(json);

    return json_string ? json_string : "{}";
}

bool cad_create_sketch_on_plane(uint32_t plane_entity_id)
{
    if (!current_document) {
        log_error("cad_create_sketch_on_plane: no active document");
        return false;
    }

    plane_t* plane = cad_find_plane_by_entity_id(plane_entity_id);
    if (!plane) {
        log_warning("cad_create_sketch_on_plane: plane entity 0x%08X not found", plane_entity_id);
        return false;
    }

    sketch_t* sketch = sketch_new();
    if (!sketch) {
        log_error("cad_create_sketch_on_plane: failed to allocate sketch");
        return false;
    }

    const size_t sketch_index = object_get_child_count(PLANE_AS_OBJECT(plane)) + 1;
    char sketch_name[64];
    snprintf(sketch_name, sizeof(sketch_name), "Sketch %03zu", sketch_index);

    object_set_name(SKETCH_AS_OBJECT(sketch), sketch_name);
    sketch_set_reference_plane(sketch, plane);
    object_add_child(PLANE_AS_OBJECT(plane), SKETCH_AS_OBJECT(sketch));

    const char* plane_name = object_get_name(PLANE_AS_OBJECT(plane));
    log_info(
        "Created sketch '%s' on plane '%s' (entity=0x%08X)",
        sketch_name,
        plane_name ? plane_name : "unnamed",
        plane_entity_id
    );

    return true;
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






