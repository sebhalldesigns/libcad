/***************************************************************
**
** libcad Source File
**
** File         :  axis.c
** Module       :  core/axis
** Author       :  SH
** Created      :  2026-02-15 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad axis type implementation
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

#include "axis.h"

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

static void axis_finalize(axis_t* self);
void axis_get_end_point(const axis_t* self, vec3 out_end);
static void axis_get_start_point(const axis_t* self, vec3 out_start);

/***************************************************************
** MARK: TYPE REGISTRATION
***************************************************************/

LIBCAD_DEFINE_TYPE(axis, object_get_type())

/***************************************************************
** MARK: CLASS INITIALIZATION
***************************************************************/

static void axis_class_init(axis_class_t* cls)
{
    /* Parent vtable is already copied */

    /* Override parent methods */
    object_class_t* parent_class = (object_class_t*)cls;
    parent_class->debug_print = (void(*)(object_t*))axis_debug_print;
    parent_class->to_json = (json_t*(*)(object_t*))axis_to_json;

    /* Set finalizer */
    cls->parent_class.parent_class.instance_finalize = (void(*)(void*))axis_finalize;

    /* Set up virtual methods */
    cls->set_geometry = axis_set_geometry;

    log_info("axis_class_t initialized");
}

/***************************************************************
** MARK: INSTANCE INITIALIZATION
***************************************************************/

static void axis_init(axis_t* self)
{
    /* Parent (object_t) is already initialized */

    /* Initialize to X axis at origin */
    glm_vec3_zero(self->origin);
    glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, self->direction);
    self->length = 1.0f;

    /* Default visualization */
    glm_vec4_copy((vec4){1.0f, 0.0f, 0.0f, 1.0f}, self->color);
    self->visible = true;
    self->thickness = 2.0f;
    self->show_arrow = true;

    /* Create vector line for rendering */
    vec3 start_point;
    vec3 end_point;
    axis_get_start_point(self, start_point);
    axis_get_end_point(self, end_point);

    vector_line_instance_t line = {
        .start = {start_point[0], start_point[1], start_point[2]},
        .end = {end_point[0], end_point[1], end_point[2]},
        .color = {self->color[0], self->color[1], self->color[2], self->color[3]},
        .stroke_width = self->thickness,
        .dash = 0.0f
    };

    if (!vector_create_axis_line(&line, &self->vector_line_handle)) {
        log_error("Failed to create vector line for axis");
        self->vector_line_handle = VECTOR_INVALID_INSTANCE;
    }
}

/***************************************************************
** MARK: INSTANCE FINALIZATION
***************************************************************/

static void axis_finalize(axis_t* self)
{
    /* Destroy vector line */
    if (self->vector_line_handle != VECTOR_INVALID_INSTANCE) {
        vector_destroy_axis_line(self->vector_line_handle);
        self->vector_line_handle = VECTOR_INVALID_INSTANCE;
    }

    /* Parent finalization (including children) is automatic */
}

/***************************************************************
** MARK: PUBLIC API - Geometry
***************************************************************/

void axis_set_geometry(axis_t* self, vec3 origin, vec3 direction, float length)
{
    if (!self) return;

    /* Copy origin */
    glm_vec3_copy(origin, self->origin);

    /* Normalize and copy direction */
    glm_vec3_normalize_to(direction, self->direction);

    /* Set length */
    self->length = length;

    /* Update vector line */
    if (self->vector_line_handle != VECTOR_INVALID_INSTANCE) {
        vec3 start_point;
        vec3 end_point;
        axis_get_start_point(self, start_point);
        axis_get_end_point(self, end_point);

        vector_line_instance_t line = {
            .start = {start_point[0], start_point[1], start_point[2]},
            .end = {end_point[0], end_point[1], end_point[2]},
            .color = {self->color[0], self->color[1], self->color[2], self->color[3]},
            .stroke_width = self->thickness,
            .dash = 0.0f
        };

        vector_update_axis_line(self->vector_line_handle, &line);
    }
}

void axis_get_end_point(const axis_t* self, vec3 out_end)
{
    if (!self) {
        glm_vec3_zero(out_end);
        return;
    }

    /* end = origin + (direction * length) */
    vec3 direction;
    vec3 scaled_direction;
    glm_vec3_copy((vec3){self->direction[0], self->direction[1], self->direction[2]}, direction);
    glm_vec3_scale(direction, self->length, scaled_direction);
    glm_vec3_add(self->origin, scaled_direction, out_end);
}

static void axis_get_start_point(const axis_t* self, vec3 out_start)
{
    if (!self) {
        glm_vec3_zero(out_start);
        return;
    }

    /* start = origin - (direction * length) */
    vec3 direction;
    vec3 scaled_direction;
    glm_vec3_copy((vec3){self->direction[0], self->direction[1], self->direction[2]}, direction);
    glm_vec3_scale(direction, self->length, scaled_direction);
    glm_vec3_sub(self->origin, scaled_direction, out_start);
}

/***************************************************************
** MARK: PUBLIC API - Visualization
***************************************************************/

void axis_set_display(axis_t* self, vec4 color, bool visible, float thickness, bool show_arrow)
{
    if (!self) return;

    glm_vec4_copy(color, self->color);
    self->visible = visible;
    self->thickness = thickness;
    self->show_arrow = show_arrow;

    /* Update vector line */
    if (self->vector_line_handle != VECTOR_INVALID_INSTANCE) {
        vec3 start_point;
        vec3 end_point;
        axis_get_start_point(self, start_point);
        axis_get_end_point(self, end_point);

        vector_line_instance_t line = {
            .start = {start_point[0], start_point[1], start_point[2]},
            .end = {end_point[0], end_point[1], end_point[2]},
            .color = {self->color[0], self->color[1], self->color[2], self->color[3]},
            .stroke_width = self->thickness,
            .dash = 0.0f
        };

        vector_update_axis_line(self->vector_line_handle, &line);
    }
}

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void axis_debug_print(axis_t* self)
{
    if (!self) return;

    const char* name = object_get_name(AXIS_AS_OBJECT(self));

    log_info("axis_t: name='%s', origin=(%.2f,%.2f,%.2f), direction=(%.2f,%.2f,%.2f), length=%.2f, children=%zu",
             name ? name : "(null)",
             self->origin[0], self->origin[1], self->origin[2],
             self->direction[0], self->direction[1], self->direction[2],
             self->length,
             object_get_child_count(AXIS_AS_OBJECT(self)));
}

json_t* axis_to_json(axis_t* self)
{
    if (!self) return json_null();

    /* Start with parent's JSON (includes children) */
    json_t* json = object_to_json(AXIS_AS_OBJECT(self));

    /* Override type field */
    json_object_set_new(json, "type", json_string("axis_t"));

    /* Add axis-specific fields */
    json_t* origin_array = json_array();
    json_array_append_new(origin_array, json_real(self->origin[0]));
    json_array_append_new(origin_array, json_real(self->origin[1]));
    json_array_append_new(origin_array, json_real(self->origin[2]));
    json_object_set_new(json, "origin", origin_array);

    json_t* direction_array = json_array();
    json_array_append_new(direction_array, json_real(self->direction[0]));
    json_array_append_new(direction_array, json_real(self->direction[1]));
    json_array_append_new(direction_array, json_real(self->direction[2]));
    json_object_set_new(json, "direction", direction_array);

    json_object_set_new(json, "length", json_real(self->length));

    json_t* color_array = json_array();
    json_array_append_new(color_array, json_real(self->color[0]));
    json_array_append_new(color_array, json_real(self->color[1]));
    json_array_append_new(color_array, json_real(self->color[2]));
    json_array_append_new(color_array, json_real(self->color[3]));
    json_object_set_new(json, "color", color_array);

    json_object_set_new(json, "visible", json_boolean(self->visible));
    json_object_set_new(json, "thickness", json_real(self->thickness));
    json_object_set_new(json, "show_arrow", json_boolean(self->show_arrow));

    /* Match vector picking ID encoding: type(0x1) in top 4 bits + axis handle. */
    uint32_t entity_id = VECTOR_INVALID_INSTANCE;
    if (self->vector_line_handle != VECTOR_INVALID_INSTANCE) {
        entity_id = 0x10000000u | (self->vector_line_handle & 0x0FFFFFFFu);
    }
    json_object_set_new(json, "entity_id", json_integer((json_int_t)entity_id));

    return json;
}
