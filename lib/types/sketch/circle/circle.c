/***************************************************************
**
** libcad Source File
**
** File         :  circle.c
** Module       :  sketch/circle
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad 2D circle primitive implementation
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <math.h>
#include <util/log/log.h>

#include "circle.h"

/***************************************************************
** MARK: CONSTANTS
***************************************************************/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

static void circle_finalize(circle_t* self);

/***************************************************************
** MARK: TYPE REGISTRATION
***************************************************************/

LIBCAD_DEFINE_TYPE(circle, object_get_type())

/***************************************************************
** MARK: CLASS INITIALIZATION
***************************************************************/

static void circle_class_init(circle_class_t* cls)
{
    /* Parent vtable is already copied */

    /* Override parent methods */
    object_class_t* parent_class = (object_class_t*)cls;
    parent_class->debug_print = (void(*)(object_t*))circle_debug_print;
    parent_class->to_json = (json_t*(*)(object_t*))circle_to_json;

    /* Set finalizer */
    cls->parent_class.parent_class.instance_finalize = (void(*)(void*))circle_finalize;

    /* Set up virtual methods */
    cls->set_geometry = circle_set_geometry;
    cls->area = circle_area;
    cls->circumference = circle_circumference;

    log_info("circle_class_t initialized");
}

/***************************************************************
** MARK: INSTANCE INITIALIZATION
***************************************************************/

static void circle_init(circle_t* self)
{
    /* Parent (object_t) is already initialized */

    /* Initialize circle geometry */
    glm_vec2_zero(self->center);
    self->radius = 1.0f;

    /* Default visual properties */
    glm_vec4_copy((vec4){1.0f, 1.0f, 1.0f, 1.0f}, self->color);
    self->thickness = 1.0f;
    self->construction = false;
}

/***************************************************************
** MARK: INSTANCE FINALIZATION
***************************************************************/

static void circle_finalize(circle_t* self)
{
    /* No circle-specific cleanup needed */
    /* Parent finalization is automatic */
}

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

circle_t* circle_new_with_geometry(vec2 center, float radius)
{
    circle_t* circle = circle_new();
    if (circle) {
        circle_set_geometry(circle, center, radius);
    }
    return circle;
}

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

void circle_set_geometry(circle_t* self, vec2 center, float radius)
{
    if (!self) return;

    glm_vec2_copy(center, self->center);
    self->radius = radius > 0.0f ? radius : 0.0f;
}

void circle_get_center(const circle_t* self, vec2 out_center)
{
    if (!self) {
        glm_vec2_zero(out_center);
        return;
    }
    glm_vec2_copy(self->center, out_center);
}

float circle_get_radius(const circle_t* self)
{
    return self ? self->radius : 0.0f;
}

float circle_area(const circle_t* self)
{
    if (!self) return 0.0f;
    return (float)(M_PI * self->radius * self->radius);
}

float circle_circumference(const circle_t* self)
{
    if (!self) return 0.0f;
    return (float)(2.0 * M_PI * self->radius);
}

void circle_set_style(circle_t* self, vec4 color, float thickness, bool construction)
{
    if (!self) return;

    glm_vec4_copy(color, self->color);
    self->thickness = thickness;
    self->construction = construction;
}

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void circle_debug_print(circle_t* self)
{
    if (!self) return;

    const char* name = object_get_name(CIRCLE_AS_OBJECT(self));

    log_info("circle_t: name='%s', center=(%.2f,%.2f), radius=%.2f, area=%.2f",
             name ? name : "(null)",
             self->center[0], self->center[1],
             self->radius,
             circle_area(self));
}

json_t* circle_to_json(circle_t* self)
{
    if (!self) return json_null();

    /* Start with parent's JSON (includes children) */
    json_t* json = object_to_json(CIRCLE_AS_OBJECT(self));

    /* Override type field */
    json_object_set_new(json, "type", json_string("circle_t"));

    /* Add circle-specific fields */
    json_t* center_array = json_array();
    json_array_append_new(center_array, json_real(self->center[0]));
    json_array_append_new(center_array, json_real(self->center[1]));
    json_object_set_new(json, "center", center_array);

    json_object_set_new(json, "radius", json_real(self->radius));

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
