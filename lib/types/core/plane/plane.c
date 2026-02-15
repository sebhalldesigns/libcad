/***************************************************************
**
** libcad Source File
**
** File         :  plane.c
** Module       :  core/plane
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad plane type implementation
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <util/log/log.h>
#include <render/vector/vector.h>

#include "plane.h"

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

static void plane_finalize(plane_t* self);
static void plane_create_rectangle(plane_t* self);
static void plane_destroy_rectangle(plane_t* self);

/***************************************************************
** MARK: TYPE REGISTRATION
***************************************************************/

LIBCAD_DEFINE_TYPE(plane, object_get_type())

/***************************************************************
** MARK: CLASS INITIALIZATION
***************************************************************/

static void plane_class_init(plane_class_t* cls)
{
    /* Parent vtable is already copied */

    /* Override parent methods */
    object_class_t* parent_class = (object_class_t*)cls;
    parent_class->debug_print = (void(*)(object_t*))plane_debug_print;
    parent_class->to_json = (json_t*(*)(object_t*))plane_to_json;

    /* Set finalizer */
    cls->parent_class.parent_class.instance_finalize = (void(*)(void*))plane_finalize;

    /* Set up virtual methods */
    cls->set_transform = plane_set_transform;
    cls->get_transform_matrix = plane_get_transform_matrix;

    log_info("plane_class_t initialized");
}

/***************************************************************
** MARK: INSTANCE INITIALIZATION
***************************************************************/

static void plane_init(plane_t* self)
{
    /* Parent (object_t) is already initialized */

    /* Initialize to XY plane at origin */
    glm_vec3_zero(self->origin);
    glm_vec3_copy((vec3){0.0f, 0.0f, 1.0f}, self->normal);
    glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, self->u_axis);
    glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, self->v_axis);

    /* Default visualization */
    glm_vec4_copy((vec4){0.8f, 0.8f, 0.8f, 0.3f}, self->color);
    self->visible = true;
    self->grid_size = 1.0f;
    self->plane_size = 20.0f;  /* 20x20 unit plane */

    /* Initialize rectangle handles */
    self->rectangle_handle = VECTOR_INVALID_INSTANCE;

    /* Create rectangle */
    plane_create_rectangle(self);
}

/***************************************************************
** MARK: INSTANCE FINALIZATION
***************************************************************/

static void plane_finalize(plane_t* self)
{
    /* Destroy rectangle */
    plane_destroy_rectangle(self);

    /* Parent finalization (including children) is automatic */
}

/***************************************************************
** MARK: PUBLIC API - Geometry
***************************************************************/

void plane_set_transform(plane_t* self, vec3 origin, vec3 normal)
{
    if (!self) return;

    const char* name = object_get_name(PLANE_AS_OBJECT(self));
    log_info("plane_set_transform: %s origin=(%.1f,%.1f,%.1f) normal=(%.1f,%.1f,%.1f)",
             name ? name : "unnamed",
             origin[0], origin[1], origin[2],
             normal[0], normal[1], normal[2]);

    /* Copy origin */
    glm_vec3_copy(origin, self->origin);

    /* Normalize and copy normal */
    glm_vec3_normalize_to(normal, self->normal);

    /* Compute orthonormal basis (u_axis, v_axis) from normal */
    /* Find a vector not parallel to normal */
    vec3 temp;
    if (fabsf(self->normal[0]) < 0.9f) {
        glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, temp);
    } else {
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, temp);
    }

    /* u_axis = normalize(cross(temp, normal)) */
    glm_vec3_cross(temp, self->normal, self->u_axis);
    glm_vec3_normalize(self->u_axis);

    /* v_axis = cross(normal, u_axis) */
    glm_vec3_cross(self->normal, self->u_axis, self->v_axis);

    /* Update rectangle (fill + stroke) */
    if (self->rectangle_handle != VECTOR_INVALID_INSTANCE) {
        vector_shape_instance_t rect_fill = {
            .center = {self->origin[0], self->origin[1], self->origin[2]},
            .normal = {self->normal[0], self->normal[1], self->normal[2]},
            .size = {self->plane_size, self->plane_size},
            .color = {self->color[0], self->color[1], self->color[2], self->color[3]},
            .rotation = 0.0f,
            .sides = 4.0f,
            .start_angle = 0.0f,
            .end_angle = 0.0f,
            .fill = -1.0f, /* request white edge in shape shader */
            .stroke_width = 2.0f,
            .corner_radius = 0.0f,
            .dash = 0.0f
        };
        vector_update_shape(self->rectangle_handle, &rect_fill);
    }
}

void plane_get_transform_matrix(const plane_t* self, mat4 out_matrix)
{
    if (!self) {
        glm_mat4_identity(out_matrix);
        return;
    }

    /* Build transform matrix from plane basis
    ** Matrix transforms from 2D plane coords (x, y, 0) to 3D world coords
    ** [ u_axis.x  v_axis.x  normal.x  origin.x ]
    ** [ u_axis.y  v_axis.y  normal.y  origin.y ]
    ** [ u_axis.z  v_axis.z  normal.z  origin.z ]
    ** [ 0         0         0         1        ]
    */
    glm_mat4_identity(out_matrix);

    /* Set basis vectors as columns */
    out_matrix[0][0] = self->u_axis[0];
    out_matrix[1][0] = self->u_axis[1];
    out_matrix[2][0] = self->u_axis[2];

    out_matrix[0][1] = self->v_axis[0];
    out_matrix[1][1] = self->v_axis[1];
    out_matrix[2][1] = self->v_axis[2];

    out_matrix[0][2] = self->normal[0];
    out_matrix[1][2] = self->normal[1];
    out_matrix[2][2] = self->normal[2];

    /* Set origin as translation */
    out_matrix[3][0] = self->origin[0];
    out_matrix[3][1] = self->origin[1];
    out_matrix[3][2] = self->origin[2];
}

void plane_local_to_world(const plane_t* self, vec2 local_2d, vec3 out_world)
{
    if (!self) {
        glm_vec3_zero(out_world);
        return;
    }

    /* world = origin + (local.x * u_axis) + (local.y * v_axis) */
    vec3 u_component, v_component;
    glm_vec3_scale(self->u_axis, local_2d[0], u_component);
    glm_vec3_scale(self->v_axis, local_2d[1], v_component);

    glm_vec3_copy(self->origin, out_world);
    glm_vec3_add(out_world, u_component, out_world);
    glm_vec3_add(out_world, v_component, out_world);
}

void plane_world_to_local(const plane_t* self, vec3 world_3d, vec2 out_local)
{
    if (!self) {
        glm_vec2_zero(out_local);
        return;
    }

    /* Project world point onto plane, then get 2D coordinates */
    vec3 relative;
    glm_vec3_sub(world_3d, self->origin, relative);

    /* Project onto u and v axes */
    out_local[0] = glm_vec3_dot(relative, self->u_axis);
    out_local[1] = glm_vec3_dot(relative, self->v_axis);
}

/***************************************************************
** MARK: PUBLIC API - Visualization
***************************************************************/

void plane_set_display(plane_t* self, vec4 color, bool visible, float grid_size)
{
    if (!self) return;

    glm_vec4_copy(color, self->color);
    self->visible = visible;
    self->grid_size = grid_size;

    /* Update rectangle (fill + stroke) */
    if (self->rectangle_handle != VECTOR_INVALID_INSTANCE) {
        vector_shape_instance_t rect_fill = {
            .center = {self->origin[0], self->origin[1], self->origin[2]},
            .normal = {self->normal[0], self->normal[1], self->normal[2]},
            .size = {self->plane_size, self->plane_size},
            .color = {self->color[0], self->color[1], self->color[2], self->color[3]},
            .rotation = 0.0f,
            .sides = 4.0f,
            .start_angle = 0.0f,
            .end_angle = 0.0f,
            .fill = -1.0f, /* request white edge in shape shader */
            .stroke_width = 2.0f,
            .corner_radius = 0.0f,
            .dash = 0.0f
        };
        vector_update_shape(self->rectangle_handle, &rect_fill);
    }
}

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void plane_debug_print(plane_t* self)
{
    if (!self) return;

    const char* name = object_get_name(PLANE_AS_OBJECT(self));

    log_info("plane_t: name='%s', origin=(%.2f,%.2f,%.2f), normal=(%.2f,%.2f,%.2f), children=%zu",
             name ? name : "(null)",
             self->origin[0], self->origin[1], self->origin[2],
             self->normal[0], self->normal[1], self->normal[2],
             object_get_child_count(PLANE_AS_OBJECT(self)));
}

json_t* plane_to_json(plane_t* self)
{
    if (!self) return json_null();

    /* Start with parent's JSON (includes children) */
    json_t* json = object_to_json(PLANE_AS_OBJECT(self));

    /* Override type field */
    json_object_set_new(json, "type", json_string("plane_t"));

    /* Add plane-specific fields */
    json_t* origin_array = json_array();
    json_array_append_new(origin_array, json_real(self->origin[0]));
    json_array_append_new(origin_array, json_real(self->origin[1]));
    json_array_append_new(origin_array, json_real(self->origin[2]));
    json_object_set_new(json, "origin", origin_array);

    json_t* normal_array = json_array();
    json_array_append_new(normal_array, json_real(self->normal[0]));
    json_array_append_new(normal_array, json_real(self->normal[1]));
    json_array_append_new(normal_array, json_real(self->normal[2]));
    json_object_set_new(json, "normal", normal_array);

    json_t* color_array = json_array();
    json_array_append_new(color_array, json_real(self->color[0]));
    json_array_append_new(color_array, json_real(self->color[1]));
    json_array_append_new(color_array, json_real(self->color[2]));
    json_array_append_new(color_array, json_real(self->color[3]));
    json_object_set_new(json, "color", color_array);

    json_object_set_new(json, "visible", json_boolean(self->visible));
    json_object_set_new(json, "grid_size", json_real(self->grid_size));

    /* Match vector picking ID encoding: type(0x2) in top 4 bits + shape handle. */
    uint32_t entity_id = VECTOR_INVALID_INSTANCE;
    if (self->rectangle_handle != VECTOR_INVALID_INSTANCE) {
        entity_id = 0x20000000u | (self->rectangle_handle & 0x0FFFFFFFu);
    }
    json_object_set_new(json, "entity_id", json_integer((json_int_t)entity_id));

    return json;
}

/***************************************************************
** MARK: STATIC FUNCTIONS - Rectangle Management
***************************************************************/

static void plane_create_rectangle(plane_t* self)
{
    if (!self) return;

    /* Create single rectangle shape (fill + stroke) */
    vector_shape_instance_t rect_fill = {
        .center = {self->origin[0], self->origin[1], self->origin[2]},
        .normal = {self->normal[0], self->normal[1], self->normal[2]},
        .size = {self->plane_size, self->plane_size},
        .color = {self->color[0], self->color[1], self->color[2], self->color[3]},
        .rotation = 0.0f,
        .sides = 4.0f,  /* Rectangle */
        .start_angle = 0.0f,
        .end_angle = 0.0f,
        .fill = -1.0f, /* request white edge in shape shader */
        .stroke_width = 2.0f,
        .corner_radius = 0.0f,
        .dash = 0.0f
    };

    if (!vector_create_shape(&rect_fill, &self->rectangle_handle)) {
        log_error("Failed to create plane rectangle");
        self->rectangle_handle = VECTOR_INVALID_INSTANCE;
    }

    const char* name = object_get_name(PLANE_AS_OBJECT(self));
    log_info("Created plane: %s at (%.1f,%.1f,%.1f) normal=(%.1f,%.1f,%.1f)",
             name ? name : "unnamed",
             self->origin[0], self->origin[1], self->origin[2],
             self->normal[0], self->normal[1], self->normal[2]);
}

static void plane_destroy_rectangle(plane_t* self)
{
    if (!self) return;

    /* Destroy filled rectangle */
    if (self->rectangle_handle != VECTOR_INVALID_INSTANCE) {
        vector_destroy_shape(self->rectangle_handle);
        self->rectangle_handle = VECTOR_INVALID_INSTANCE;
    }

}
