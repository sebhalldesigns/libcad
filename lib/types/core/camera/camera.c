/***************************************************************
**
** libcad Source File
**
** File         :  camera.c
** Module       :  core/camera
** Author       :  SH
** Created      :  2026-02-15 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad camera type implementation
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <util/log/log.h>

#include "camera.h"

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

static void camera_finalize(camera_t* self);

/***************************************************************
** MARK: TYPE REGISTRATION
***************************************************************/

LIBCAD_DEFINE_TYPE(camera, object_get_type())

/***************************************************************
** MARK: CLASS INITIALIZATION
***************************************************************/

static void camera_class_init(camera_class_t* cls)
{
    /* Parent vtable is already copied */

    /* Override parent methods */
    object_class_t* parent_class = (object_class_t*)cls;
    parent_class->debug_print = (void(*)(object_t*))camera_debug_print;
    parent_class->to_json = (json_t*(*)(object_t*))camera_to_json;

    /* Set finalizer */
    cls->parent_class.parent_class.instance_finalize = (void(*)(void*))camera_finalize;

    /* Set up virtual methods */
    cls->orbit = camera_orbit;
    cls->pan = camera_pan;
    cls->zoom = camera_zoom;
    cls->get_view_matrix = camera_get_view_matrix;
    cls->get_projection_matrix = camera_get_projection_matrix;
    cls->get_distance = camera_get_distance;

    log_info("camera_class_t initialized");
}

/***************************************************************
** MARK: INSTANCE INITIALIZATION
***************************************************************/

static void camera_init(camera_t* self)
{
    /* Parent (object_t) is already initialized */

    /* Initialize to a default isometric-style view */
    glm_vec3_copy((vec3){20.0f, 15.0f, 20.0f}, self->position);
    glm_vec3_zero(self->target);
    glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, self->up);

    /* Default projection parameters */
    self->fov = 45.0f;
    self->aspect = 16.0f / 9.0f;
    self->near_clip = 0.1f;
    self->far_clip = 5000.0f;  /* Increased to accommodate long axes */

    /* Default interaction parameters */
    self->orbit_speed = 0.005f;
    self->pan_speed = 0.002f;  /* Reduced for better control */
    self->zoom_speed = 0.1f;

    /* Default distance limits */
    self->min_distance = 1.0f;
    self->max_distance = 500.0f;
}

/***************************************************************
** MARK: INSTANCE FINALIZATION
***************************************************************/

static void camera_finalize(camera_t* self)
{
    /* No camera-specific cleanup needed */
    /* Parent finalization (including children) is automatic */
}

/***************************************************************
** MARK: PUBLIC API - Camera Setup
***************************************************************/

void camera_set_view(camera_t* self, vec3 position, vec3 target, vec3 up)
{
    if (!self) return;

    glm_vec3_copy(position, self->position);
    glm_vec3_copy(target, self->target);
    glm_vec3_normalize_to(up, self->up);
}

void camera_set_projection(camera_t* self, float fov, float aspect, float near_clip, float far_clip)
{
    if (!self) return;

    self->fov = fov;
    self->aspect = aspect;
    self->near_clip = near_clip;
    self->far_clip = far_clip;
}

void camera_set_interaction(camera_t* self, float orbit_speed, float pan_speed, float zoom_speed)
{
    if (!self) return;

    self->orbit_speed = orbit_speed;
    self->pan_speed = pan_speed;
    self->zoom_speed = zoom_speed;
}

void camera_set_distance_limits(camera_t* self, float min_distance, float max_distance)
{
    if (!self) return;

    self->min_distance = min_distance;
    self->max_distance = max_distance;
}

/***************************************************************
** MARK: PUBLIC API - Camera Interactions
***************************************************************/

void camera_orbit(camera_t* self, float delta_yaw, float delta_pitch)
{
    if (!self) return;

    /* Apply sensitivity (negate to match CAD-style interaction) */
    float yaw = -delta_yaw * self->orbit_speed;
    float pitch = -delta_pitch * self->orbit_speed;

    /* Get current offset from target */
    vec3 offset;
    glm_vec3_sub(self->position, self->target, offset);
    float distance = glm_vec3_norm(offset);

    /* Convert to spherical coordinates */
    vec3 direction;
    glm_vec3_divs(offset, distance, direction);

    /* Current angles */
    float theta = atan2f(direction[0], direction[2]);  /* Yaw (horizontal) */
    float phi = acosf(direction[1]);                    /* Pitch (vertical) */

    /* Apply rotation */
    theta += yaw;
    phi = glm_clamp(phi + pitch, 0.01f, GLM_PI - 0.01f);  /* Prevent gimbal lock */

    /* Convert back to Cartesian */
    direction[0] = sinf(phi) * sinf(theta);
    direction[1] = cosf(phi);
    direction[2] = sinf(phi) * cosf(theta);

    /* Update position */
    vec3 new_offset;
    glm_vec3_scale(direction, distance, new_offset);
    glm_vec3_add(self->target, new_offset, self->position);
}

void camera_pan(camera_t* self, float delta_x, float delta_y)
{
    if (!self) return;

    /* Get camera basis vectors */
    vec3 right, up;
    camera_get_right(self, right);
    glm_vec3_copy(self->up, up);

    /* Scale pan by distance for consistent speed */
    float distance = camera_get_distance(self);
    float pan_scale = self->pan_speed * distance;

    /* Calculate pan offset */
    vec3 offset_x, offset_y, offset;
    glm_vec3_scale(right, -delta_x * pan_scale, offset_x);
    glm_vec3_scale(up, delta_y * pan_scale, offset_y);
    glm_vec3_add(offset_x, offset_y, offset);

    /* Move both camera and target */
    glm_vec3_add(self->position, offset, self->position);
    glm_vec3_add(self->target, offset, self->target);
}

void camera_zoom(camera_t* self, float delta)
{
    if (!self) return;

    /* Continuous zoom scale: positive delta zooms in, negative zooms out */
    float factor = expf(-delta * self->zoom_speed);

    /* Get current distance */
    float current_distance = camera_get_distance(self);
    float new_distance = current_distance * factor;

    /* Clamp to limits */
    new_distance = glm_clamp(new_distance, self->min_distance, self->max_distance);

    /* Get direction from target to camera */
    vec3 direction;
    glm_vec3_sub(self->position, self->target, direction);
    glm_vec3_normalize(direction);

    /* Update position */
    vec3 offset;
    glm_vec3_scale(direction, new_distance, offset);
    glm_vec3_add(self->target, offset, self->position);
}

/***************************************************************
** MARK: PUBLIC API - Matrix Generation
***************************************************************/

void camera_get_view_matrix(const camera_t* self, mat4 out_view)
{
    if (!self) {
        glm_mat4_identity(out_view);
        return;
    }

    glm_lookat(self->position, self->target, self->up, out_view);
}

void camera_get_projection_matrix(const camera_t* self, mat4 out_projection)
{
    if (!self) {
        glm_mat4_identity(out_projection);
        return;
    }

    glm_perspective(glm_rad(self->fov), self->aspect, self->near_clip, self->far_clip, out_projection);
}

void camera_get_view_projection_matrix(const camera_t* self, mat4 out_vp)
{
    if (!self) {
        glm_mat4_identity(out_vp);
        return;
    }

    mat4 view, projection;
    camera_get_view_matrix(self, view);
    camera_get_projection_matrix(self, projection);
    glm_mat4_mul(projection, view, out_vp);
}

/***************************************************************
** MARK: PUBLIC API - Queries
***************************************************************/

float camera_get_distance(const camera_t* self)
{
    if (!self) return 0.0f;

    vec3 offset;
    glm_vec3_sub(self->position, self->target, offset);
    return glm_vec3_norm(offset);
}

void camera_get_forward(const camera_t* self, vec3 out_forward)
{
    if (!self) {
        glm_vec3_zero(out_forward);
        return;
    }

    glm_vec3_sub(self->target, self->position, out_forward);
    glm_vec3_normalize(out_forward);
}

void camera_get_right(const camera_t* self, vec3 out_right)
{
    if (!self) {
        glm_vec3_zero(out_right);
        return;
    }

    vec3 forward;
    camera_get_forward(self, forward);
    glm_vec3_cross(forward, self->up, out_right);
    glm_vec3_normalize(out_right);
}

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void camera_debug_print(camera_t* self)
{
    if (!self) return;

    const char* name = object_get_name(CAMERA_AS_OBJECT(self));
    float distance = camera_get_distance(self);

    log_info("camera_t: name='%s', position=(%.2f,%.2f,%.2f), target=(%.2f,%.2f,%.2f), distance=%.2f, fov=%.1f",
             name ? name : "(null)",
             self->position[0], self->position[1], self->position[2],
             self->target[0], self->target[1], self->target[2],
             distance,
             self->fov);
}

json_t* camera_to_json(camera_t* self)
{
    if (!self) return json_null();

    /* Start with parent's JSON (includes children) */
    json_t* json = object_to_json(CAMERA_AS_OBJECT(self));

    /* Override type field */
    json_object_set_new(json, "type", json_string("camera_t"));

    /* Add position */
    json_t* position_array = json_array();
    json_array_append_new(position_array, json_real(self->position[0]));
    json_array_append_new(position_array, json_real(self->position[1]));
    json_array_append_new(position_array, json_real(self->position[2]));
    json_object_set_new(json, "position", position_array);

    /* Add target */
    json_t* target_array = json_array();
    json_array_append_new(target_array, json_real(self->target[0]));
    json_array_append_new(target_array, json_real(self->target[1]));
    json_array_append_new(target_array, json_real(self->target[2]));
    json_object_set_new(json, "target", target_array);

    /* Add up */
    json_t* up_array = json_array();
    json_array_append_new(up_array, json_real(self->up[0]));
    json_array_append_new(up_array, json_real(self->up[1]));
    json_array_append_new(up_array, json_real(self->up[2]));
    json_object_set_new(json, "up", up_array);

    /* Add projection parameters */
    json_object_set_new(json, "fov", json_real(self->fov));
    json_object_set_new(json, "aspect", json_real(self->aspect));
    json_object_set_new(json, "near_clip", json_real(self->near_clip));
    json_object_set_new(json, "far_clip", json_real(self->far_clip));

    /* Add interaction parameters */
    json_object_set_new(json, "orbit_speed", json_real(self->orbit_speed));
    json_object_set_new(json, "pan_speed", json_real(self->pan_speed));
    json_object_set_new(json, "zoom_speed", json_real(self->zoom_speed));

    /* Add distance limits */
    json_object_set_new(json, "min_distance", json_real(self->min_distance));
    json_object_set_new(json, "max_distance", json_real(self->max_distance));

    return json;
}
