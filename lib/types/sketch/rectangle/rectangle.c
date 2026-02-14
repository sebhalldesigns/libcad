/***************************************************************
**
** libcad Source File
**
** File         :  rectangle.c
** Module       :  sketch/rectangle
** Author       :  SH
** Created      :  2026-02-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad 2D rectangle primitive implementation
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <math.h>
#include <util/log/log.h>

#include "rectangle.h"

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

static void rectangle_finalize(rectangle_t* self);

/***************************************************************
** MARK: TYPE REGISTRATION
***************************************************************/

LIBCAD_DEFINE_TYPE(rectangle, object_get_type())

/***************************************************************
** MARK: CLASS INITIALIZATION
***************************************************************/

static void rectangle_class_init(rectangle_class_t* cls)
{
    /* Parent vtable is already copied */

    /* Override parent methods */
    object_class_t* parent_class = (object_class_t*)cls;
    parent_class->debug_print = (void(*)(object_t*))rectangle_debug_print;
    parent_class->to_json = (json_t*(*)(object_t*))rectangle_to_json;

    /* Set finalizer */
    cls->parent_class.parent_class.instance_finalize = (void(*)(void*))rectangle_finalize;

    /* Set up virtual methods */
    cls->set_corners = rectangle_set_corners;
    cls->width = rectangle_width;
    cls->height = rectangle_height;
    cls->area = rectangle_area;

    log_info("rectangle_class_t initialized");
}

/***************************************************************
** MARK: INSTANCE INITIALIZATION
***************************************************************/

static void rectangle_init(rectangle_t* self)
{
    /* Parent (object_t) is already initialized */

    /* Initialize rectangle geometry */
    glm_vec2_zero(self->corner1);
    glm_vec2_copy((vec2){1.0f, 1.0f}, self->corner2);

    /* Default visual properties */
    glm_vec4_copy((vec4){1.0f, 1.0f, 1.0f, 1.0f}, self->color);
    self->thickness = 1.0f;
    self->filled = false;
    self->construction = false;
}

/***************************************************************
** MARK: INSTANCE FINALIZATION
***************************************************************/

static void rectangle_finalize(rectangle_t* self)
{
    /* No rectangle-specific cleanup needed */
    /* Parent finalization is automatic */
}

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

rectangle_t* rectangle_new_with_corners(vec2 corner1, vec2 corner2)
{
    rectangle_t* rect = rectangle_new();
    if (rect) {
        rectangle_set_corners(rect, corner1, corner2);
    }
    return rect;
}

/***************************************************************
** MARK: PUBLIC API
***************************************************************/

void rectangle_set_corners(rectangle_t* self, vec2 corner1, vec2 corner2)
{
    if (!self) return;

    glm_vec2_copy(corner1, self->corner1);
    glm_vec2_copy(corner2, self->corner2);
}

void rectangle_get_corner1(const rectangle_t* self, vec2 out_corner)
{
    if (!self) {
        glm_vec2_zero(out_corner);
        return;
    }
    glm_vec2_copy(self->corner1, out_corner);
}

void rectangle_get_corner2(const rectangle_t* self, vec2 out_corner)
{
    if (!self) {
        glm_vec2_zero(out_corner);
        return;
    }
    glm_vec2_copy(self->corner2, out_corner);
}

void rectangle_get_center(const rectangle_t* self, vec2 out_center)
{
    if (!self) {
        glm_vec2_zero(out_center);
        return;
    }

    /* Center = (corner1 + corner2) / 2 */
    glm_vec2_add(self->corner1, self->corner2, out_center);
    glm_vec2_scale(out_center, 0.5f, out_center);
}

float rectangle_width(const rectangle_t* self)
{
    if (!self) return 0.0f;
    return fabsf(self->corner2[0] - self->corner1[0]);
}

float rectangle_height(const rectangle_t* self)
{
    if (!self) return 0.0f;
    return fabsf(self->corner2[1] - self->corner1[1]);
}

float rectangle_area(const rectangle_t* self)
{
    if (!self) return 0.0f;
    return rectangle_width(self) * rectangle_height(self);
}

void rectangle_set_style(rectangle_t* self, vec4 color, float thickness, bool filled, bool construction)
{
    if (!self) return;

    glm_vec4_copy(color, self->color);
    self->thickness = thickness;
    self->filled = filled;
    self->construction = construction;
}

/***************************************************************
** MARK: OVERRIDDEN METHODS
***************************************************************/

void rectangle_debug_print(rectangle_t* self)
{
    if (!self) return;

    const char* name = object_get_name(RECTANGLE_AS_OBJECT(self));

    log_info("rectangle_t: name='%s', corner1=(%.2f,%.2f), corner2=(%.2f,%.2f), width=%.2f, height=%.2f",
             name ? name : "(null)",
             self->corner1[0], self->corner1[1],
             self->corner2[0], self->corner2[1],
             rectangle_width(self),
             rectangle_height(self));
}

json_t* rectangle_to_json(rectangle_t* self)
{
    if (!self) return json_null();

    /* Start with parent's JSON (includes children) */
    json_t* json = object_to_json(RECTANGLE_AS_OBJECT(self));

    /* Override type field */
    json_object_set_new(json, "type", json_string("rectangle_t"));

    /* Add rectangle-specific fields */
    json_t* corner1_array = json_array();
    json_array_append_new(corner1_array, json_real(self->corner1[0]));
    json_array_append_new(corner1_array, json_real(self->corner1[1]));
    json_object_set_new(json, "corner1", corner1_array);

    json_t* corner2_array = json_array();
    json_array_append_new(corner2_array, json_real(self->corner2[0]));
    json_array_append_new(corner2_array, json_real(self->corner2[1]));
    json_object_set_new(json, "corner2", corner2_array);

    json_t* color_array = json_array();
    json_array_append_new(color_array, json_real(self->color[0]));
    json_array_append_new(color_array, json_real(self->color[1]));
    json_array_append_new(color_array, json_real(self->color[2]));
    json_array_append_new(color_array, json_real(self->color[3]));
    json_object_set_new(json, "color", color_array);

    json_object_set_new(json, "thickness", json_real(self->thickness));
    json_object_set_new(json, "filled", json_boolean(self->filled));
    json_object_set_new(json, "construction", json_boolean(self->construction));

    return json;
}
