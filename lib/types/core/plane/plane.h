/***************************************************************
**
** libcad Header File
**
** File         :  plane.h
** Module       :  core/plane
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad plane type - represents a 2D plane in 3D space
**
***************************************************************/

#ifndef LIBCAD_CORE_PLANE_H
#define LIBCAD_CORE_PLANE_H

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

typedef struct plane_t plane_t;
typedef struct plane_class_t plane_class_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

/*
** plane_t instance - represents a 2D construction plane in 3D space
**
** A plane is defined by an origin point and a normal vector.
** It can contain child objects (typically sketch instances) via inherited
** children management from object_t.
**
** IMPORTANT: First field MUST be parent (object_t parent)
**            This enables safe upcasting: plane_t* -> object_t*
*/
typedef struct plane_t {
    object_t parent;  /* Inherit from object_t (MUST be first!) */

    /* Plane geometry */
    vec3 origin;      /* Origin point of the plane in 3D space */
    vec3 normal;      /* Normal vector (should be normalized) */
    vec3 u_axis;      /* U axis (right direction on plane, should be normalized) */
    vec3 v_axis;      /* V axis (up direction on plane, should be normalized) */

    /* Visualization */
    vec4 color;       /* RGBA color for rendering the plane */
    bool visible;     /* Whether the plane should be rendered */
    float grid_size;  /* Size of grid squares for visualization */

    /* Vector rendering handle */
    uint32_t rectangle_handle;    /* Handle to rectangle shape (fill + stroke) */
    float plane_size;             /* Size of the plane rectangle */
} plane_t;

/*
** plane_t class - vtable and metadata
**
** Extends parent vtable with plane-specific methods
*/
typedef struct plane_class_t {
    object_class_t parent_class;  /* Inherit parent vtable */

    /* Virtual methods specific to plane_t */
    void (*set_transform)(plane_t* self, vec3 origin, vec3 normal);
    void (*get_transform_matrix)(const plane_t* self, mat4 out_matrix);
} plane_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

/* Get the type handle (auto-registers on first call) */
type_handle_t plane_get_type(void);

/* Get the class singleton */
plane_class_t* plane_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

/* Create new plane instance */
plane_t* plane_new(void);

/* Destroy plane instance */
void plane_free(plane_t* self);

/***************************************************************
** MARK: PUBLIC API - Geometry
***************************************************************/

/*
** Set the plane's position and orientation
**
** Parameters:
**   self   - Plane instance
**   origin - Origin point in 3D space
**   normal - Normal vector (will be normalized automatically)
*/
void plane_set_transform(plane_t* self, vec3 origin, vec3 normal);

/*
** Get the plane's transform as a 4x4 matrix
**
** The matrix transforms from plane-local 2D coordinates (x,y,0)
** to world 3D coordinates.
**
** Parameters:
**   self       - Plane instance
**   out_matrix - Output 4x4 matrix
*/
void plane_get_transform_matrix(const plane_t* self, mat4 out_matrix);

/*
** Transform a 2D point in plane-local coordinates to 3D world coordinates
**
** Parameters:
**   self      - Plane instance
**   local_2d  - 2D point in plane coordinates (x, y)
**   out_world - Output 3D point in world coordinates
*/
void plane_local_to_world(const plane_t* self, vec2 local_2d, vec3 out_world);

/*
** Project a 3D world point onto the plane, returning 2D plane coordinates
**
** Parameters:
**   self       - Plane instance
**   world_3d   - 3D point in world coordinates
**   out_local  - Output 2D point in plane coordinates
*/
void plane_world_to_local(const plane_t* self, vec3 world_3d, vec2 out_local);

/***************************************************************
** MARK: PUBLIC API - Visualization
***************************************************************/

/*
** Set visualization properties
**
** Parameters:
**   self      - Plane instance
**   color     - RGBA color (values 0-1)
**   visible   - Whether plane should be rendered
**   grid_size - Size of grid squares for visualization
*/
void plane_set_display(plane_t* self, vec4 color, bool visible, float grid_size);

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

/* Debug print (overrides object_t) */
void plane_debug_print(plane_t* self);

/* JSON serialization (overrides object_t) */
json_t* plane_to_json(plane_t* self);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

/* Type checking */
#define PLANE_TYPE (plane_get_type())
#define IS_PLANE(obj) (type_instance_is_a((void*)(obj), PLANE_TYPE))

/* Casting */
#define PLANE(obj) ((plane_t*)(obj))
#define PLANE_AS_OBJECT(plane) ((object_t*)(plane))

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_CORE_PLANE_H */
