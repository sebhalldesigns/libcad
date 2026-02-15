/***************************************************************
**
** libcad Header File
**
** File         :  camera.h
** Module       :  core/camera
** Author       :  SH
** Created      :  2026-02-15 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad camera type - CAD-style orbit camera
**
***************************************************************/

#ifndef LIBCAD_CORE_CAMERA_H
#define LIBCAD_CORE_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <types/core/object/object.h>
#include <cglm/cglm.h>

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

typedef struct camera_t camera_t;
typedef struct camera_class_t camera_class_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

/*
** camera_t instance - CAD-style orbit camera
**
** A camera that orbits around a target point with pan and zoom controls.
** Typical for CAD applications where you want to inspect a model.
**
** IMPORTANT: First field MUST be parent (object_t parent)
**            This enables safe upcasting: camera_t* -> object_t*
*/
typedef struct camera_t {
    object_t parent;  /* Inherit from object_t (MUST be first!) */

    /* Camera state */
    vec3 position;    /* Camera position in 3D space */
    vec3 target;      /* Point the camera is looking at */
    vec3 up;          /* Up vector (typically [0, 1, 0]) */

    /* Projection parameters */
    float fov;        /* Field of view in degrees */
    float aspect;     /* Aspect ratio (width/height) */
    float near_clip;  /* Near clipping plane */
    float far_clip;   /* Far clipping plane */

    /* Interaction constraints */
    float min_distance;  /* Minimum distance from target */
    float max_distance;  /* Maximum distance from target */
    float orbit_speed;   /* Sensitivity for orbit operations */
    float pan_speed;     /* Sensitivity for pan operations */
    float zoom_speed;    /* Sensitivity for zoom operations */
} camera_t;

/*
** camera_t class - vtable and metadata
**
** Extends parent vtable with camera-specific methods
*/
typedef struct camera_class_t {
    object_class_t parent_class;  /* Inherit parent vtable */

    /* Virtual methods specific to camera_t */
    void (*orbit)(camera_t* self, float delta_yaw, float delta_pitch);
    void (*pan)(camera_t* self, float delta_x, float delta_y);
    void (*zoom)(camera_t* self, float delta);
    void (*get_view_matrix)(const camera_t* self, mat4 out_view);
    void (*get_projection_matrix)(const camera_t* self, mat4 out_projection);
    float (*get_distance)(const camera_t* self);
} camera_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

/* Get the type handle (auto-registers on first call) */
type_handle_t camera_get_type(void);

/* Get the class singleton */
camera_class_t* camera_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

/* Create new camera instance */
camera_t* camera_new(void);

/* Destroy camera instance */
void camera_free(camera_t* self);

/***************************************************************
** MARK: PUBLIC API - Camera Setup
***************************************************************/

/*
** Set the camera's position and target
**
** Parameters:
**   self     - Camera instance
**   position - Camera position in 3D space
**   target   - Point to look at
**   up       - Up vector (will be normalized)
*/
void camera_set_view(camera_t* self, vec3 position, vec3 target, vec3 up);

/*
** Set projection parameters
**
** Parameters:
**   self       - Camera instance
**   fov        - Field of view in degrees
**   aspect     - Aspect ratio (width/height)
**   near_clip  - Near clipping plane distance
**   far_clip   - Far clipping plane distance
*/
void camera_set_projection(camera_t* self, float fov, float aspect, float near_clip, float far_clip);

/*
** Set interaction parameters
**
** Parameters:
**   self         - Camera instance
**   orbit_speed  - Orbit sensitivity (radians per unit)
**   pan_speed    - Pan sensitivity (world units per pixel)
**   zoom_speed   - Zoom sensitivity (scale factor per unit)
*/
void camera_set_interaction(camera_t* self, float orbit_speed, float pan_speed, float zoom_speed);

/*
** Set distance constraints
**
** Parameters:
**   self         - Camera instance
**   min_distance - Minimum distance from target
**   max_distance - Maximum distance from target
*/
void camera_set_distance_limits(camera_t* self, float min_distance, float max_distance);

/***************************************************************
** MARK: PUBLIC API - Camera Interactions
***************************************************************/

/*
** Orbit the camera around the target point
**
** Parameters:
**   self        - Camera instance
**   delta_yaw   - Horizontal rotation amount (pixels or normalized)
**   delta_pitch - Vertical rotation amount (pixels or normalized)
*/
void camera_orbit(camera_t* self, float delta_yaw, float delta_pitch);

/*
** Pan the camera (move both camera and target)
**
** Parameters:
**   self    - Camera instance
**   delta_x - Horizontal pan amount (pixels or normalized)
**   delta_y - Vertical pan amount (pixels or normalized)
*/
void camera_pan(camera_t* self, float delta_x, float delta_y);

/*
** Zoom the camera (move closer/farther from target)
**
** Parameters:
**   self  - Camera instance
**   delta - Zoom amount (positive = zoom in, negative = zoom out)
*/
void camera_zoom(camera_t* self, float delta);

/***************************************************************
** MARK: PUBLIC API - Matrix Generation
***************************************************************/

/*
** Get the view matrix for rendering
**
** Parameters:
**   self     - Camera instance
**   out_view - Output 4x4 view matrix
*/
void camera_get_view_matrix(const camera_t* self, mat4 out_view);

/*
** Get the projection matrix for rendering
**
** Parameters:
**   self           - Camera instance
**   out_projection - Output 4x4 projection matrix
*/
void camera_get_projection_matrix(const camera_t* self, mat4 out_projection);

/*
** Get the combined view-projection matrix
**
** Parameters:
**   self   - Camera instance
**   out_vp - Output 4x4 view-projection matrix
*/
void camera_get_view_projection_matrix(const camera_t* self, mat4 out_vp);

/***************************************************************
** MARK: PUBLIC API - Queries
***************************************************************/

/*
** Get the current distance from camera to target
**
** Parameters:
**   self - Camera instance
**
** Returns:
**   Distance in world units
*/
float camera_get_distance(const camera_t* self);

/*
** Get the camera's forward direction vector
**
** Parameters:
**   self        - Camera instance
**   out_forward - Output normalized forward vector
*/
void camera_get_forward(const camera_t* self, vec3 out_forward);

/*
** Get the camera's right direction vector
**
** Parameters:
**   self      - Camera instance
**   out_right - Output normalized right vector
*/
void camera_get_right(const camera_t* self, vec3 out_right);

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

/* Debug print (overrides object_t) */
void camera_debug_print(camera_t* self);

/* JSON serialization (overrides object_t) */
json_t* camera_to_json(camera_t* self);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

/* Type checking */
#define CAMERA_TYPE (camera_get_type())
#define IS_CAMERA(obj) (type_instance_is_a((void*)(obj), CAMERA_TYPE))

/* Casting */
#define CAMERA(obj) ((camera_t*)(obj))
#define CAMERA_AS_OBJECT(camera) ((object_t*)(camera))

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_CORE_CAMERA_H */
