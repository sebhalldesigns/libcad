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
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <jansson.h>
#include <util/log/log.h>

#include "types/core/object/object.h"
#include "types/core/document/document.h"
#include "types/core/textfield/textfield.h"
#include "types/core/plane/plane.h"
#include "types/core/axis/axis.h"
#include "types/core/camera/camera.h"
#include "types/sketch/sketch/sketch.h"
#include "types/sketch/line/line.h"
#include "types/sketch/circle/circle.h"
#include "types/sketch/rectangle/rectangle.h"


#include <render/gpu/gpu.h>
#include <render/vector/vector.h>
#include <render/window/window.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef struct {
    object_t* object;
    bool visible;
} sketch_visibility_snapshot_entry_t;

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
static bool sketch_mode_active = false;
static uint32_t sketch_mode_plane_entity_id = VECTOR_INVALID_INSTANCE;
static sketch_t* current_active_sketch = NULL;
static float sketch_mode_ortho_half_height = 10.0f;
static sketch_visibility_snapshot_entry_t* sketch_visibility_snapshot = NULL;
static size_t sketch_visibility_snapshot_count = 0;
static size_t sketch_visibility_snapshot_capacity = 0;
static bool sketch_camera_snapshot_valid = false;
static vec3 sketch_camera_position_snapshot = {0.0f, 0.0f, 0.0f};
static vec3 sketch_camera_target_snapshot = {0.0f, 0.0f, 0.0f};
static vec3 sketch_camera_up_snapshot = {0.0f, 1.0f, 0.0f};
static float sketch_camera_fov_snapshot = 45.0f;
static float sketch_camera_aspect_snapshot = 1.0f;
static float sketch_camera_near_snapshot = 0.1f;
static float sketch_camera_far_snapshot = 1000.0f;
static float sketch_camera_orbit_speed_snapshot = 0.005f;
static float sketch_camera_pan_speed_snapshot = 0.002f;
static float sketch_camera_zoom_speed_snapshot = 0.1f;
static float sketch_camera_min_distance_snapshot = 1.0f;
static float sketch_camera_max_distance_snapshot = 500.0f;

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

static bool cad_is_entity_selectable(uint32_t entity_id)
{
    if (entity_id == 0u || entity_id == VECTOR_INVALID_INSTANCE) {
        return false;
    }

    object_t* root = DOCUMENT_AS_OBJECT(current_document);
    if (!root) {
        return false;
    }

    const uint32_t type_bits = entity_id & 0xF0000000u;
    const uint32_t index = entity_id & 0x0FFFFFFFu;

    object_t* stack[1024];
    size_t sp = 0;
    stack[sp++] = root;

    while (sp > 0) {
        object_t* object = stack[--sp];
        if (!object) {
            continue;
        }

        const type_handle_t object_type = (type_handle_t)LIBCAD_GET_CLASS(object);

        if (type_bits == 0x20000000u && type_is_a(object_type, plane_get_type())) {
            plane_t* plane = PLANE(object);
            if (plane->rectangle_handle == index) return object_is_visible(object);
        }

        if (type_bits == 0x20000000u && type_is_a(object_type, circle_get_type())) {
            circle_t* circle = CIRCLE(object);
            if (circle->vector_shape_handle == index) return object_is_visible(object);
        }

        if (type_bits == 0x20000000u && type_is_a(object_type, rectangle_get_type())) {
            rectangle_t* rect = RECTANGLE(object);
            if (rect->vector_shape_handle == index) return object_is_visible(object);
        }

        if (type_bits == 0x10000000u && type_is_a(object_type, axis_get_type())) {
            axis_t* axis = AXIS(object);
            if (axis->vector_line_handle == index) return object_is_visible(object);
        }

        if (type_bits == 0x00000000u && type_is_a(object_type, line_get_type())) {
            line_t* line = LINE(object);
            if (line->vector_line_handle == index) return object_is_visible(object);
        }

        const size_t child_count = object_get_child_count(object);
        for (size_t i = 0; i < child_count; i++) {
            if (sp < (sizeof(stack) / sizeof(stack[0]))) {
                stack[sp++] = object_get_child(object, i);
            }
        }
    }

    return true;
}

static size_t cad_count_children_of_type(const object_t* parent, type_handle_t type)
{
    if (!parent || type == TYPE_INVALID) {
        return 0;
    }

    size_t count = 0;
    const size_t child_count = object_get_child_count(parent);
    for (size_t i = 0; i < child_count; i++) {
        object_t* child = object_get_child(parent, i);
        if (child && type_is_a((type_handle_t)LIBCAD_GET_CLASS(child), type)) {
            count++;
        }
    }
    return count;
}

static sketch_t* cad_find_latest_sketch_on_plane(plane_t* plane)
{
    if (!plane) {
        return NULL;
    }

    object_t* plane_object = PLANE_AS_OBJECT(plane);
    size_t child_count = object_get_child_count(plane_object);
    while (child_count > 0) {
        child_count--;
        object_t* child = object_get_child(plane_object, child_count);
        if (child && type_is_a((type_handle_t)LIBCAD_GET_CLASS(child), sketch_get_type())) {
            return SKETCH(child);
        }
    }

    return NULL;
}

static void cad_set_active_sketch(sketch_t* sketch)
{
    if (current_active_sketch && current_active_sketch != sketch) {
        sketch_set_active(current_active_sketch, false);
    }

    current_active_sketch = sketch;

    if (current_active_sketch) {
        sketch_set_active(current_active_sketch, true);
    }
}

static plane_t* cad_get_active_sketch_plane(void)
{
    if (!current_active_sketch) {
        return NULL;
    }

    return sketch_get_reference_plane(current_active_sketch);
}

static bool cad_screen_to_active_sketch_local(int sx, int sy, vec2 out_local)
{
    plane_t* plane = cad_get_active_sketch_plane();
    if (!current_camera || !plane || !out_local) {
        return false;
    }

    const float safe_width = (float)fmax(1, viewport_width);
    const float safe_height = (float)fmax(1, viewport_height);

    const float u = ((float)sx / safe_width) * 2.0f - 1.0f;
    const float v = 1.0f - ((float)sy / safe_height) * 2.0f;

    vec3 right;
    vec3 up;
    vec3 world;
    vec3 offset_right;
    vec3 offset_up;

    const float aspect = safe_width / safe_height;
    const float half_height = fmaxf(sketch_mode_ortho_half_height, 0.001f);
    const float half_width = half_height * aspect;

    camera_get_right(current_camera, right);
    glm_vec3_copy(current_camera->up, up);
    glm_vec3_normalize(up);

    glm_vec3_copy(current_camera->target, world);
    glm_vec3_scale(right, u * half_width, offset_right);
    glm_vec3_scale(up, v * half_height, offset_up);
    glm_vec3_add(world, offset_right, world);
    glm_vec3_add(world, offset_up, world);

    plane_world_to_local(plane, world, out_local);
    return true;
}

static object_t* cad_find_object_by_id_recursive(object_t* root, uintptr_t object_id)
{
    if (!root) {
        return NULL;
    }

    if ((uintptr_t)root == object_id) {
        return root;
    }

    const size_t child_count = object_get_child_count(root);
    for (size_t i = 0; i < child_count; i++) {
        object_t* child = object_get_child(root, i);
        object_t* found = cad_find_object_by_id_recursive(child, object_id);
        if (found) {
            return found;
        }
    }

    return NULL;
}

static void cad_sync_runtime_visibility(object_t* object)
{
    if (!object) {
        return;
    }

    const bool visible = object_is_visible(object);
    const type_handle_t object_type = (type_handle_t)LIBCAD_GET_CLASS(object);

    if (type_is_a(object_type, plane_get_type())) {
        plane_t* plane = PLANE(object);
        plane_set_display(plane, plane->color, visible, plane->grid_size);
        return;
    }

    if (type_is_a(object_type, axis_get_type())) {
        axis_t* axis = AXIS(object);
        axis_set_display(axis, axis->color, visible, axis->thickness, axis->show_arrow);
        return;
    }

    if (type_is_a(object_type, line_get_type())) {
        line_t* line = LINE(object);
        line_set_style(line, line->color, line->thickness, line->construction);
        return;
    }

    if (type_is_a(object_type, circle_get_type())) {
        circle_t* circle = CIRCLE(object);
        circle_set_style(circle, circle->color, circle->thickness, circle->construction);
        return;
    }

    if (type_is_a(object_type, rectangle_get_type())) {
        rectangle_t* rect = RECTANGLE(object);
        rectangle_set_style(rect, rect->color, rect->thickness, rect->filled, rect->construction);
    }
}

static void cad_set_visibility_recursive(object_t* object, bool visible)
{
    if (!object) {
        return;
    }

    object_set_visible(object, visible);
    cad_sync_runtime_visibility(object);

    const size_t child_count = object_get_child_count(object);
    for (size_t i = 0; i < child_count; i++) {
        cad_set_visibility_recursive(object_get_child(object, i), visible);
    }
}

static void cad_clear_sketch_visibility_snapshot(void)
{
    free(sketch_visibility_snapshot);
    sketch_visibility_snapshot = NULL;
    sketch_visibility_snapshot_count = 0;
    sketch_visibility_snapshot_capacity = 0;
}

static bool cad_append_visibility_snapshot(object_t* object)
{
    if (!object) {
        return true;
    }

    if (sketch_visibility_snapshot_count >= sketch_visibility_snapshot_capacity) {
        size_t new_capacity = sketch_visibility_snapshot_capacity == 0 ? 64 : sketch_visibility_snapshot_capacity * 2;
        sketch_visibility_snapshot_entry_t* new_snapshot = realloc(
            sketch_visibility_snapshot,
            new_capacity * sizeof(sketch_visibility_snapshot_entry_t)
        );

        if (!new_snapshot) {
            log_error("Failed to allocate sketch visibility snapshot");
            return false;
        }

        sketch_visibility_snapshot = new_snapshot;
        sketch_visibility_snapshot_capacity = new_capacity;
    }

    sketch_visibility_snapshot[sketch_visibility_snapshot_count].object = object;
    sketch_visibility_snapshot[sketch_visibility_snapshot_count].visible = object_is_visible(object);
    sketch_visibility_snapshot_count++;
    return true;
}

static bool cad_capture_visibility_snapshot_recursive(object_t* object)
{
    if (!object) {
        return true;
    }

    if (!cad_append_visibility_snapshot(object)) {
        return false;
    }

    const size_t child_count = object_get_child_count(object);
    for (size_t i = 0; i < child_count; i++) {
        if (!cad_capture_visibility_snapshot_recursive(object_get_child(object, i))) {
            return false;
        }
    }

    return true;
}

static void cad_restore_visibility_snapshot(void)
{
    for (size_t i = 0; i < sketch_visibility_snapshot_count; i++) {
        object_t* object = sketch_visibility_snapshot[i].object;
        if (!object) {
            continue;
        }

        object_set_visible(object, sketch_visibility_snapshot[i].visible);
        cad_sync_runtime_visibility(object);
    }
}

static void cad_pick_axes_for_plane(const plane_t* plane, axis_t** out_axis_a, axis_t** out_axis_b)
{
    if (out_axis_a) *out_axis_a = NULL;
    if (out_axis_b) *out_axis_b = NULL;

    if (!current_document || !plane) {
        return;
    }

    float best_dot_a = FLT_MAX;
    float best_dot_b = FLT_MAX;
    axis_t* best_axis_a = NULL;
    axis_t* best_axis_b = NULL;

    object_t* root = DOCUMENT_AS_OBJECT(current_document);
    const size_t child_count = object_get_child_count(root);

    for (size_t i = 0; i < child_count; i++) {
        object_t* child = object_get_child(root, i);
        if (!child || !type_is_a((type_handle_t)LIBCAD_GET_CLASS(child), axis_get_type())) {
            continue;
        }

        axis_t* axis = AXIS(child);
        const float axis_dot = fabsf(glm_vec3_dot(axis->direction, plane->normal));

        if (axis_dot < best_dot_a) {
            best_dot_b = best_dot_a;
            best_axis_b = best_axis_a;
            best_dot_a = axis_dot;
            best_axis_a = axis;
        } else if (axis_dot < best_dot_b) {
            best_dot_b = axis_dot;
            best_axis_b = axis;
        }
    }

    if (out_axis_a) *out_axis_a = best_axis_a;
    if (out_axis_b) *out_axis_b = best_axis_b;
}

static void cad_orient_camera_to_plane(const plane_t* plane)
{
    if (!current_camera || !plane) {
        return;
    }

    vec3 target;
    vec3 position;
    vec3 up;
    vec3 offset;

    const float distance = fmaxf(plane->plane_size * 1.5f, 15.0f);

    glm_vec3_copy(plane->origin, target);
    glm_vec3_scale(plane->normal, distance, offset);
    glm_vec3_add(target, offset, position);

    glm_vec3_copy(plane->v_axis, up);
    if (fabsf(glm_vec3_dot(up, plane->normal)) > 0.99f) {
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, up);
    }

    camera_set_view(current_camera, position, target, up);
}

static void cad_get_active_view_projection(mat4 out_vp)
{
    if (!current_camera) {
        glm_mat4_identity(out_vp);
        return;
    }

    if (!sketch_mode_active) {
        camera_get_view_projection_matrix(current_camera, out_vp);
        return;
    }

    mat4 view;
    mat4 projection;
    camera_get_view_matrix(current_camera, view);

    const float safe_height = (float)fmax(1, viewport_height);
    const float safe_width = (float)fmax(1, viewport_width);
    const float aspect = safe_width / safe_height;
    const float half_height = fmaxf(sketch_mode_ortho_half_height, 0.001f);
    const float half_width = half_height * aspect;

    glm_ortho(
        -half_width,
        half_width,
        -half_height,
        half_height,
        current_camera->near_clip,
        current_camera->far_clip,
        projection
    );

    glm_mat4_mul(projection, view, out_vp);
}

static void cad_pan_camera_orthographic(float delta_x, float delta_y)
{
    if (!current_camera) {
        return;
    }

    vec3 right;
    vec3 up;
    vec3 offset_x;
    vec3 offset_y;
    vec3 offset;

    camera_get_right(current_camera, right);
    glm_vec3_copy(current_camera->up, up);
    glm_vec3_normalize(up);

    const float safe_width = (float)fmax(1, viewport_width);
    const float safe_height = (float)fmax(1, viewport_height);
    const float aspect = safe_width / safe_height;
    const float half_height = fmaxf(sketch_mode_ortho_half_height, 0.001f);
    const float half_width = half_height * aspect;

    const float units_per_pixel_x = (2.0f * half_width) / safe_width;
    const float units_per_pixel_y = (2.0f * half_height) / safe_height;

    glm_vec3_scale(right, -delta_x * units_per_pixel_x, offset_x);
    glm_vec3_scale(up, delta_y * units_per_pixel_y, offset_y);
    glm_vec3_add(offset_x, offset_y, offset);

    glm_vec3_add(current_camera->position, offset, current_camera->position);
    glm_vec3_add(current_camera->target, offset, current_camera->target);
}

static void cad_capture_camera_snapshot(void)
{
    if (!current_camera) {
        sketch_camera_snapshot_valid = false;
        return;
    }

    glm_vec3_copy(current_camera->position, sketch_camera_position_snapshot);
    glm_vec3_copy(current_camera->target, sketch_camera_target_snapshot);
    glm_vec3_copy(current_camera->up, sketch_camera_up_snapshot);
    sketch_camera_fov_snapshot = current_camera->fov;
    sketch_camera_aspect_snapshot = current_camera->aspect;
    sketch_camera_near_snapshot = current_camera->near_clip;
    sketch_camera_far_snapshot = current_camera->far_clip;
    sketch_camera_orbit_speed_snapshot = current_camera->orbit_speed;
    sketch_camera_pan_speed_snapshot = current_camera->pan_speed;
    sketch_camera_zoom_speed_snapshot = current_camera->zoom_speed;
    sketch_camera_min_distance_snapshot = current_camera->min_distance;
    sketch_camera_max_distance_snapshot = current_camera->max_distance;
    sketch_camera_snapshot_valid = true;
}

static void cad_restore_camera_snapshot(void)
{
    if (!current_camera || !sketch_camera_snapshot_valid) {
        return;
    }

    camera_set_view(
        current_camera,
        sketch_camera_position_snapshot,
        sketch_camera_target_snapshot,
        sketch_camera_up_snapshot
    );

    camera_set_projection(
        current_camera,
        sketch_camera_fov_snapshot,
        sketch_camera_aspect_snapshot,
        sketch_camera_near_snapshot,
        sketch_camera_far_snapshot
    );

    camera_set_interaction(
        current_camera,
        sketch_camera_orbit_speed_snapshot,
        sketch_camera_pan_speed_snapshot,
        sketch_camera_zoom_speed_snapshot
    );

    camera_set_distance_limits(
        current_camera,
        sketch_camera_min_distance_snapshot,
        sketch_camera_max_distance_snapshot
    );
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
    cad_clear_sketch_visibility_snapshot();
    sketch_mode_active = false;
    sketch_mode_plane_entity_id = VECTOR_INVALID_INSTANCE;
    current_active_sketch = NULL;
    sketch_mode_ortho_half_height = 10.0f;
    sketch_camera_snapshot_valid = false;

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

        if (sketch_mode_active) {
            /* In sketch mode, lock orbit and pan in orthographic screen space. */
            cad_pan_camera_orthographic(delta_x, delta_y);
        } else if (shift_modifier) {
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

    /* Use camera to generate active view-projection matrix */
    mat4 vp;
    if (current_camera) {
        /* Update camera aspect ratio if viewport changed */
        float aspect = size[0] / size[1];
        camera_set_projection(current_camera, current_camera->fov, aspect,
                             current_camera->near_clip, current_camera->far_clip);
        cad_get_active_view_projection(vp);
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

    /* Picking uses the same active view-projection as rendering */
    mat4 view_projection;
    cad_get_active_view_projection(view_projection);

    /* Call vector renderer picking */
    uint32_t entity_id = vector_pick_entity(screen_x, screen_y,
                                           viewport_width, viewport_height,
                                           view_projection);
    if (!cad_is_entity_selectable(entity_id)) {
        entity_id = VECTOR_INVALID_INSTANCE;
    }

    printf("cad_pick_entity: (%d,%d) -> 0x%08X\n", screen_x, screen_y, entity_id);
    return entity_id;
}

void cad_set_hovered_entity(uint32_t entity_id)
{
    if (!cad_is_entity_selectable(entity_id)) {
        entity_id = VECTOR_INVALID_INSTANCE;
    }
    vector_set_hovered_entity(entity_id);
}

uint32_t cad_get_hovered_entity()
{
    return vector_get_hovered_entity();
}

void cad_set_selected_entity(uint32_t entity_id)
{
    if (!cad_is_entity_selectable(entity_id)) {
        entity_id = VECTOR_INVALID_INSTANCE;
    }
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
    cad_set_active_sketch(sketch);

    const char* plane_name = object_get_name(PLANE_AS_OBJECT(plane));
    log_info(
        "Created sketch '%s' on plane '%s' (entity=0x%08X)",
        sketch_name,
        plane_name ? plane_name : "unnamed",
        plane_entity_id
    );

    return true;
}

bool cad_set_object_visibility(uintptr_t object_id, bool visible)
{
    if (!current_document || object_id == 0) {
        return false;
    }

    object_t* root = DOCUMENT_AS_OBJECT(current_document);
    object_t* target = cad_find_object_by_id_recursive(root, object_id);
    if (!target) {
        log_warning("cad_set_object_visibility: object_id=%zu not found", (size_t)object_id);
        return false;
    }

    cad_set_visibility_recursive(target, visible);

    if (!cad_is_entity_selectable(vector_get_selected_entity())) {
        vector_set_selected_entity(VECTOR_INVALID_INSTANCE);
    }
    if (!cad_is_entity_selectable(vector_get_hovered_entity())) {
        vector_set_hovered_entity(VECTOR_INVALID_INSTANCE);
    }

    return true;
}

bool cad_enter_sketch_mode(uint32_t plane_entity_id)
{
    if (!current_document || !current_camera) {
        return false;
    }

    plane_t* plane = cad_find_plane_by_entity_id(plane_entity_id);
    if (!plane) {
        log_warning("cad_enter_sketch_mode: plane entity 0x%08X not found", plane_entity_id);
        return false;
    }

    if (sketch_mode_active) {
        cad_exit_sketch_mode();
    }

    sketch_t* target_sketch = cad_find_latest_sketch_on_plane(plane);
    cad_set_active_sketch(target_sketch);

    cad_clear_sketch_visibility_snapshot();
    if (!cad_capture_visibility_snapshot_recursive(DOCUMENT_AS_OBJECT(current_document))) {
        cad_clear_sketch_visibility_snapshot();
        return false;
    }

    cad_capture_camera_snapshot();
    cad_orient_camera_to_plane(plane);
    sketch_mode_ortho_half_height = fmaxf(plane->plane_size * 0.75f, 5.0f);

    object_t* root = DOCUMENT_AS_OBJECT(current_document);
    const size_t child_count = object_get_child_count(root);
    for (size_t i = 0; i < child_count; i++) {
        cad_set_visibility_recursive(object_get_child(root, i), false);
    }

    axis_t* axis_a = NULL;
    axis_t* axis_b = NULL;
    cad_pick_axes_for_plane(plane, &axis_a, &axis_b);

    if (axis_a) {
        cad_set_visibility_recursive(AXIS_AS_OBJECT(axis_a), true);
    }
    if (axis_b && axis_b != axis_a) {
        cad_set_visibility_recursive(AXIS_AS_OBJECT(axis_b), true);
    }

    if (current_active_sketch) {
        cad_set_visibility_recursive(SKETCH_AS_OBJECT(current_active_sketch), true);
    }

    vector_set_hovered_entity(VECTOR_INVALID_INSTANCE);
    vector_set_selected_entity(VECTOR_INVALID_INSTANCE);

    sketch_mode_active = true;
    sketch_mode_plane_entity_id = plane_entity_id;
    log_info("Entered sketch mode on plane entity 0x%08X", plane_entity_id);
    return true;
}

bool cad_exit_sketch_mode(void)
{
    if (!sketch_mode_active) {
        return true;
    }

    cad_restore_camera_snapshot();
    cad_restore_visibility_snapshot();
    cad_clear_sketch_visibility_snapshot();

    vector_set_hovered_entity(VECTOR_INVALID_INSTANCE);
    vector_set_selected_entity(VECTOR_INVALID_INSTANCE);

    sketch_mode_active = false;
    sketch_mode_plane_entity_id = VECTOR_INVALID_INSTANCE;
    cad_set_active_sketch(NULL);
    sketch_mode_ortho_half_height = 10.0f;
    sketch_camera_snapshot_valid = false;
    log_info("Exited sketch mode");
    return true;
}

bool cad_create_line_in_active_sketch(void)
{
    if (!current_active_sketch) {
        log_warning("cad_create_line_in_active_sketch: no active sketch");
        return false;
    }

    line_t* line = line_new();
    if (!line) {
        return false;
    }

    const size_t line_index = cad_count_children_of_type(SKETCH_AS_OBJECT(current_active_sketch), line_get_type()) + 1;
    char name[64];
    snprintf(name, sizeof(name), "Line %03zu", line_index);
    object_set_name(LINE_AS_OBJECT(line), name);

    vec2 start = {-3.0f, 1.0f};
    vec2 end = {3.0f, 2.0f};
    line_set_reference_plane(line, sketch_get_reference_plane(current_active_sketch));
    line_set_points(line, start, end);
    line_set_style(line, (vec4){1.0f, 0.9f, 0.1f, 1.0f}, 2.0f, false);

    object_add_child(SKETCH_AS_OBJECT(current_active_sketch), LINE_AS_OBJECT(line));
    return true;
}

bool cad_create_line_in_active_sketch_screen(int sx0, int sy0, int sx1, int sy1)
{
    if (!current_active_sketch) {
        return false;
    }

    vec2 start_local;
    vec2 end_local;
    if (!cad_screen_to_active_sketch_local(sx0, sy0, start_local)) {
        return false;
    }
    if (!cad_screen_to_active_sketch_local(sx1, sy1, end_local)) {
        return false;
    }

    if (glm_vec2_distance(start_local, end_local) < 1e-5f) {
        return false;
    }

    line_t* line = line_new();
    if (!line) {
        return false;
    }

    const size_t line_index = cad_count_children_of_type(SKETCH_AS_OBJECT(current_active_sketch), line_get_type()) + 1;
    char name[64];
    snprintf(name, sizeof(name), "Line %03zu", line_index);
    object_set_name(LINE_AS_OBJECT(line), name);

    line_set_reference_plane(line, sketch_get_reference_plane(current_active_sketch));
    line_set_points(line, start_local, end_local);
    line_set_style(line, (vec4){1.0f, 0.9f, 0.1f, 1.0f}, 2.0f, false);

    object_add_child(SKETCH_AS_OBJECT(current_active_sketch), LINE_AS_OBJECT(line));
    return true;
}

bool cad_create_circle_in_active_sketch(void)
{
    if (!current_active_sketch) {
        log_warning("cad_create_circle_in_active_sketch: no active sketch");
        return false;
    }

    circle_t* circle = circle_new();
    if (!circle) {
        return false;
    }

    const size_t circle_index = cad_count_children_of_type(SKETCH_AS_OBJECT(current_active_sketch), circle_get_type()) + 1;
    char name[64];
    snprintf(name, sizeof(name), "Circle %03zu", circle_index);
    object_set_name(CIRCLE_AS_OBJECT(circle), name);

    vec2 center = {0.0f, 0.0f};
    circle_set_reference_plane(circle, sketch_get_reference_plane(current_active_sketch));
    circle_set_geometry(circle, center, 1.5f);
    circle_set_style(circle, (vec4){0.2f, 0.9f, 1.0f, 1.0f}, 2.0f, false);

    object_add_child(SKETCH_AS_OBJECT(current_active_sketch), CIRCLE_AS_OBJECT(circle));
    return true;
}

bool cad_create_circle_in_active_sketch_screen(int sx0, int sy0, int sx1, int sy1)
{
    if (!current_active_sketch) {
        return false;
    }

    vec2 center_local;
    vec2 edge_local;
    if (!cad_screen_to_active_sketch_local(sx0, sy0, center_local)) {
        return false;
    }
    if (!cad_screen_to_active_sketch_local(sx1, sy1, edge_local)) {
        return false;
    }

    const float radius = glm_vec2_distance(center_local, edge_local);
    if (radius < 1e-5f) {
        return false;
    }

    circle_t* circle = circle_new();
    if (!circle) {
        return false;
    }

    const size_t circle_index = cad_count_children_of_type(SKETCH_AS_OBJECT(current_active_sketch), circle_get_type()) + 1;
    char name[64];
    snprintf(name, sizeof(name), "Circle %03zu", circle_index);
    object_set_name(CIRCLE_AS_OBJECT(circle), name);

    circle_set_reference_plane(circle, sketch_get_reference_plane(current_active_sketch));
    circle_set_geometry(circle, center_local, radius);
    circle_set_style(circle, (vec4){0.2f, 0.9f, 1.0f, 1.0f}, 2.0f, false);

    object_add_child(SKETCH_AS_OBJECT(current_active_sketch), CIRCLE_AS_OBJECT(circle));
    return true;
}

bool cad_create_corner_rectangle_in_active_sketch(void)
{
    if (!current_active_sketch) {
        log_warning("cad_create_corner_rectangle_in_active_sketch: no active sketch");
        return false;
    }

    rectangle_t* rect = rectangle_new();
    if (!rect) {
        return false;
    }

    const size_t rect_index = cad_count_children_of_type(SKETCH_AS_OBJECT(current_active_sketch), rectangle_get_type()) + 1;
    char name[64];
    snprintf(name, sizeof(name), "Corner Rectangle %03zu", rect_index);
    object_set_name(RECTANGLE_AS_OBJECT(rect), name);

    vec2 corner1 = {-1.5f, -1.0f};
    vec2 corner2 = {1.5f, 1.0f};
    rectangle_set_reference_plane(rect, sketch_get_reference_plane(current_active_sketch));
    rectangle_set_corners(rect, corner1, corner2);
    rectangle_set_style(rect, (vec4){1.0f, 0.5f, 0.2f, 1.0f}, 2.0f, false, false);

    object_add_child(SKETCH_AS_OBJECT(current_active_sketch), RECTANGLE_AS_OBJECT(rect));
    return true;
}

bool cad_create_corner_rectangle_in_active_sketch_screen(int sx0, int sy0, int sx1, int sy1)
{
    if (!current_active_sketch) {
        return false;
    }

    vec2 corner0;
    vec2 corner1;
    if (!cad_screen_to_active_sketch_local(sx0, sy0, corner0)) {
        return false;
    }
    if (!cad_screen_to_active_sketch_local(sx1, sy1, corner1)) {
        return false;
    }

    if (glm_vec2_distance(corner0, corner1) < 1e-5f) {
        return false;
    }

    rectangle_t* rect = rectangle_new();
    if (!rect) {
        return false;
    }

    const size_t rect_index = cad_count_children_of_type(SKETCH_AS_OBJECT(current_active_sketch), rectangle_get_type()) + 1;
    char name[64];
    snprintf(name, sizeof(name), "Corner Rectangle %03zu", rect_index);
    object_set_name(RECTANGLE_AS_OBJECT(rect), name);

    rectangle_set_reference_plane(rect, sketch_get_reference_plane(current_active_sketch));
    rectangle_set_corners(rect, corner0, corner1);
    rectangle_set_style(rect, (vec4){1.0f, 0.5f, 0.2f, 1.0f}, 2.0f, false, false);

    object_add_child(SKETCH_AS_OBJECT(current_active_sketch), RECTANGLE_AS_OBJECT(rect));
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
    if (sketch_mode_active) {
        cad_pan_camera_orthographic(delta_x, delta_y);
        return;
    }
    camera_orbit(current_camera, delta_x, delta_y);
}

void cad_camera_pan(float delta_x, float delta_y)
{
    if (!current_camera) return;
    if (sketch_mode_active) {
        cad_pan_camera_orthographic(delta_x, delta_y);
        return;
    }
    camera_pan(current_camera, delta_x, delta_y);
}

void cad_camera_zoom(float delta)
{
    if (!current_camera) return;
    if (sketch_mode_active) {
        const float factor = expf(-delta * current_camera->zoom_speed);
        sketch_mode_ortho_half_height = fmaxf(0.05f, fminf(sketch_mode_ortho_half_height * factor, 100000.0f));
        return;
    }
    camera_zoom(current_camera, delta);
}

int cad_get_cursor_type()
{
    return 0;
}

void cad_start_modal_tool(int tool_id)
{
    (void)tool_id;
}

void cad_clear_modal_tool()
{
    cad_exit_sketch_mode();
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






