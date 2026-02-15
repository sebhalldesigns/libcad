/***************************************************************
**
** libcad Header File
**
** File         :  body.h
** Module       :  solid/body
** Author       :  SH
** Created      :  2026-02-15 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad 3D solid body type
**
***************************************************************/

#ifndef LIBCAD_SOLID_BODY_H
#define LIBCAD_SOLID_BODY_H

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

typedef struct body_t body_t;
typedef struct body_class_t body_class_t;
typedef struct plane_t plane_t;
typedef struct circle_t circle_t;
typedef struct rectangle_t rectangle_t;

/***************************************************************
** MARK: TYPE DEFINITIONS
***************************************************************/

typedef struct body_t {
    object_t parent;           /* Inherit from object_t (MUST be first!) */

    /* Source references (non-owning) */
    object_t* source_profile;
    plane_t* source_plane;

    /* Extrusion parameters */
    float height;
    vec3 direction;

    /* Visual properties */
    vec4 fill_color;
    vec4 edge_color;

    /* CPU mesh data (owned) */
    float* mesh_vertices;      /* interleaved pos+normal, 6 floats/vert */
    uint32_t* mesh_indices;
    float* edge_vertices;      /* pos only, 3 floats/vert */
    uint32_t mesh_vertex_count;
    uint32_t mesh_index_count;
    uint32_t edge_vertex_count;

    /* GPU handle (managed by mesh renderer) */
    uint32_t mesh_handle;
} body_t;

typedef struct body_class_t {
    object_class_t parent_class;

    void (*regenerate_mesh)(body_t* self);
} body_class_t;

/***************************************************************
** MARK: TYPE SYSTEM
***************************************************************/

type_handle_t body_get_type(void);
body_class_t* body_class_get(void);

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

body_t* body_new(void);
void body_free(body_t* self);

body_t* body_new_from_circle(circle_t* circle, plane_t* plane, float height);
body_t* body_new_from_rectangle(rectangle_t* rect, plane_t* plane, float height);

/***************************************************************
** MARK: CONVENIENCE MACROS
***************************************************************/

#define BODY_TYPE (body_get_type())
#define IS_BODY(obj) (type_instance_is_a((void*)(obj), BODY_TYPE))
#define BODY(obj) ((body_t*)(obj))
#define BODY_AS_OBJECT(body) ((object_t*)(body))

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_SOLID_BODY_H */
