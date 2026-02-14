/***************************************************************
**
** libcad Source File
**
** File         :  line.c
** Module       :  sketch/line
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad 2D line primitive implementation
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <math.h>
#include <util/log/log.h>

#include "line.h"

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

static void line_finalize(line_t* self);

/***************************************************************
** MARK: TYPE REGISTRATION
***************************************************************/

LIBCAD_DEFINE_TYPE(line, object_get_type())

/***************************************************************
** MARK: CLASS INITIALIZATION
***************************************************************/

static void line_class_init(line_class_t* cls)
{
    /* Parent vtable is already copied */

    /* Override parent methods */
    object_class_t* parent_class = (object_class_t*)cls;
    parent_class->debug_print = (void(*)(object_t*))line_debug_print;
    parent_class->to_json = (json_t*(*)(object_t*))line_to_json;

    /* Set finalizer */
    cls->parent_class.parent_class.instance_finalize = (void(*)(void*))line_finalize;

    /* Set up virtual methods */
    cls->set_points = line_set_points;
    cls->length = line_length;

    log_info("line_class_t initialized");
}

/***************************************************************
** MARK: INSTANCE INITIALIZATION
***************************************************************/

static void line_init(line_t* self)
{
    /* Parent (object_t) is already initialized */

    /* Initialize line geometry */
    glm_vec2_zero(self->start);
    glm_vec2_zero(self->end);

    /* Default visual properties */
    glm_vec4_copy((vec4){1.0f, 1.0f, 1.0f, 1.0f}, self->color);
    self->thickness = 1.0f;
    self->construction = false;
}

/***************************************************************
** MARK: INSTANCE FINALIZATION
***************************************************************/

static void line_finalize(line_t* self)
{
    /* No line-specific cleanup needed */
    /* Parent finalization is automatic */
}

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

line_t* line_new_with_points(vec2 start, vec2 end)
{
    line_t* line = line_new();
    if (line) {
        line_set_points(line, start, end);
    }
    return line;
}

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

void line_set_points(line_t* self, vec2 start, vec2 end)
{
    if (!self) return;

    glm_vec2_copy(start, self->start);
    glm_vec2_copy(end, self->end);
}

void line_get_start(const line_t* self, vec2 out_start)
{
    if (!self) {
        glm_vec2_zero(out_start);
        return;
    }
    glm_vec2_copy(self->start, out_start);
}

void line_get_end(const line_t* self, vec2 out_end)
{
    if (!self) {
        glm_vec2_zero(out_end);
        return;
    }
    glm_vec2_copy(self->end, out_end);
}

float line_length(const line_t* self)
{
    if (!self) return 0.0f;

    vec2 diff;
    glm_vec2_sub(self->end, self->start, diff);
    return glm_vec2_norm(diff);
}

void line_set_style(line_t* self, vec4 color, float thickness, bool construction)
{
    if (!self) return;

    glm_vec4_copy(color, self->color);
    self->thickness = thickness;
    self->construction = construction;
}

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void line_debug_print(line_t* self)
{
    if (!self) return;

    const char* name = object_get_name(LINE_AS_OBJECT(self));

    log_info("line_t: name='%s', start=(%.2f,%.2f), end=(%.2f,%.2f), length=%.2f",
             name ? name : "(null)",
             self->start[0], self->start[1],
             self->end[0], self->end[1],
             line_length(self));
}

json_t* line_to_json(line_t* self)
{
    if (!self) return json_null();

    /* Start with parent's JSON (includes children) */
    json_t* json = object_to_json(LINE_AS_OBJECT(self));

    /* Override type field */
    json_object_set_new(json, "type", json_string("line_t"));

    /* Add line-specific fields */
    json_t* start_array = json_array();
    json_array_append_new(start_array, json_real(self->start[0]));
    json_array_append_new(start_array, json_real(self->start[1]));
    json_object_set_new(json, "start", start_array);

    json_t* end_array = json_array();
    json_array_append_new(end_array, json_real(self->end[0]));
    json_array_append_new(end_array, json_real(self->end[1]));
    json_object_set_new(json, "end", end_array);

    json_t* color_array = json_array();
    json_array_append_new(color_array, json_real(self->color[0]));
    json_array_append_new(color_array, json_real(self->color[1]));
    json_array_append_new(color_array, json_real(self->color[2]));
    json_array_append_new(color_array, json_real(self->color[3]));
    json_object_set_new(json, "color", color_array);

    json_object_set_new(json, "thickness", json_real(self->thickness));
    json_object_set_new(json, "construction", json_boolean(self->construction));

    return json;
}
