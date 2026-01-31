/***************************************************************
**
** libcad Source File
**
** File         :  lc_geometry.c
** Module       :  libcad (geometry system)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Geometric primitive implementation. Provides curve and
**                 surface creation and evaluation. All geometry is stored
**                 in a flat registry separate from the entity system to
**                 enable sharing between multiple edges/faces.
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_geometry.h"
#include <math.h>
#include <string.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define LC_GEOMETRY_MAX_CURVES   65536
#define LC_GEOMETRY_MAX_SURFACES 65536

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* Curve storage entry (discriminated union). */
typedef struct lc_curve_entry_t
{
    lc_curve_type_t type;
    union
    {
        lc_curve_line_t line;
        lc_curve_circle_t circle;
        lc_curve_ellipse_t ellipse;
    } data;
} lc_curve_entry_t;

/* Surface storage entry (discriminated union). */
typedef struct lc_surface_entry_t
{
    lc_surface_type_t type;
    union
    {
        lc_surface_plane_t plane;
        lc_surface_cylinder_t cylinder;
        lc_surface_sphere_t sphere;
        lc_surface_cone_t cone;
        lc_surface_torus_t torus;
    } data;
} lc_surface_entry_t;

/* Geometry registry (static, not exposed). */
typedef struct lc_geometry_registry_t
{
    lc_curve_entry_t curves[LC_GEOMETRY_MAX_CURVES];
    uint32_t curve_count;

    lc_surface_entry_t surfaces[LC_GEOMETRY_MAX_SURFACES];
    uint32_t surface_count;

} lc_geometry_registry_t;

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static lc_geometry_registry_t g_registry;
static bool g_initialized = false;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/* Validate curve handle. */
static bool is_valid_curve(lc_curve_handle_t curve);

/* Validate surface handle. */
static bool is_valid_surface(lc_surface_handle_t surface);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void lc_geometry_init(void)
{
    if (g_initialized)
    {
        return;
    }

    memset(&g_registry, 0, sizeof(g_registry));

    /* Reserve index 0 as invalid handle. */
    g_registry.curve_count = 1;
    g_registry.surface_count = 1;

    g_initialized = true;
}

void lc_geometry_shutdown(void)
{
    if (!g_initialized)
    {
        return;
    }

    memset(&g_registry, 0, sizeof(g_registry));
    g_initialized = false;
}

lc_curve_handle_t lc_geometry_create_line(vec3 origin, vec3 direction, float length)
{
    if (!g_initialized)
    {
        return LC_CURVE_INVALID;
    }

    if (g_registry.curve_count >= LC_GEOMETRY_MAX_CURVES)
    {
        return LC_CURVE_INVALID;
    }

    lc_curve_handle_t handle = g_registry.curve_count++;
    lc_curve_entry_t *entry = &g_registry.curves[handle];

    entry->type = LC_CURVE_LINE;

    glm_vec3_copy(origin, entry->data.line.origin);
    glm_vec3_normalize_to(direction, entry->data.line.direction);
    entry->data.line.length = length;

    return handle;
}

lc_curve_handle_t lc_geometry_create_circle(vec3 center, vec3 x_axis, vec3 y_axis,
                                              float radius, float u_start, float u_end)
{
    if (!g_initialized)
    {
        return LC_CURVE_INVALID;
    }

    if (g_registry.curve_count >= LC_GEOMETRY_MAX_CURVES)
    {
        return LC_CURVE_INVALID;
    }

    lc_curve_handle_t handle = g_registry.curve_count++;
    lc_curve_entry_t *entry = &g_registry.curves[handle];

    entry->type = LC_CURVE_CIRCLE;

    glm_vec3_copy(center, entry->data.circle.center);
    glm_vec3_normalize_to(x_axis, entry->data.circle.x_axis);
    glm_vec3_normalize_to(y_axis, entry->data.circle.y_axis);
    entry->data.circle.radius = radius;
    entry->data.circle.u_start = u_start;
    entry->data.circle.u_end = u_end;

    return handle;
}

lc_curve_handle_t lc_geometry_create_ellipse(vec3 center, vec3 x_axis, vec3 y_axis,
                                               float radius_major, float radius_minor,
                                               float u_start, float u_end)
{
    if (!g_initialized)
    {
        return LC_CURVE_INVALID;
    }

    if (g_registry.curve_count >= LC_GEOMETRY_MAX_CURVES)
    {
        return LC_CURVE_INVALID;
    }

    lc_curve_handle_t handle = g_registry.curve_count++;
    lc_curve_entry_t *entry = &g_registry.curves[handle];

    entry->type = LC_CURVE_ELLIPSE;

    glm_vec3_copy(center, entry->data.ellipse.center);
    glm_vec3_normalize_to(x_axis, entry->data.ellipse.x_axis);
    glm_vec3_normalize_to(y_axis, entry->data.ellipse.y_axis);
    entry->data.ellipse.radius_major = radius_major;
    entry->data.ellipse.radius_minor = radius_minor;
    entry->data.ellipse.u_start = u_start;
    entry->data.ellipse.u_end = u_end;

    return handle;
}

lc_surface_handle_t lc_geometry_create_plane(vec3 origin, vec3 x_axis, vec3 y_axis)
{
    if (!g_initialized)
    {
        return LC_SURFACE_INVALID;
    }

    if (g_registry.surface_count >= LC_GEOMETRY_MAX_SURFACES)
    {
        return LC_SURFACE_INVALID;
    }

    lc_surface_handle_t handle = g_registry.surface_count++;
    lc_surface_entry_t *entry = &g_registry.surfaces[handle];

    entry->type = LC_SURFACE_PLANE;

    glm_vec3_copy(origin, entry->data.plane.origin);
    glm_vec3_normalize_to(x_axis, entry->data.plane.x_axis);
    glm_vec3_normalize_to(y_axis, entry->data.plane.y_axis);

    /* Compute normal as cross product. */
    glm_vec3_cross(entry->data.plane.x_axis, entry->data.plane.y_axis, entry->data.plane.normal);
    glm_vec3_normalize(entry->data.plane.normal);

    return handle;
}

lc_surface_handle_t lc_geometry_create_cylinder(vec3 origin, vec3 axis, vec3 x_axis, float radius)
{
    if (!g_initialized)
    {
        return LC_SURFACE_INVALID;
    }

    if (g_registry.surface_count >= LC_GEOMETRY_MAX_SURFACES)
    {
        return LC_SURFACE_INVALID;
    }

    lc_surface_handle_t handle = g_registry.surface_count++;
    lc_surface_entry_t *entry = &g_registry.surfaces[handle];

    entry->type = LC_SURFACE_CYLINDER;

    glm_vec3_copy(origin, entry->data.cylinder.origin);
    glm_vec3_normalize_to(axis, entry->data.cylinder.axis);
    glm_vec3_normalize_to(x_axis, entry->data.cylinder.x_axis);

    /* Compute y_axis as cross product to ensure orthogonality. */
    glm_vec3_cross(entry->data.cylinder.axis, entry->data.cylinder.x_axis, entry->data.cylinder.y_axis);
    glm_vec3_normalize(entry->data.cylinder.y_axis);

    entry->data.cylinder.radius = radius;

    return handle;
}

lc_surface_handle_t lc_geometry_create_sphere(vec3 center, vec3 x_axis, vec3 y_axis,
                                                vec3 z_axis, float radius)
{
    if (!g_initialized)
    {
        return LC_SURFACE_INVALID;
    }

    if (g_registry.surface_count >= LC_GEOMETRY_MAX_SURFACES)
    {
        return LC_SURFACE_INVALID;
    }

    lc_surface_handle_t handle = g_registry.surface_count++;
    lc_surface_entry_t *entry = &g_registry.surfaces[handle];

    entry->type = LC_SURFACE_SPHERE;

    glm_vec3_copy(center, entry->data.sphere.center);
    glm_vec3_normalize_to(x_axis, entry->data.sphere.x_axis);
    glm_vec3_normalize_to(y_axis, entry->data.sphere.y_axis);
    glm_vec3_normalize_to(z_axis, entry->data.sphere.z_axis);
    entry->data.sphere.radius = radius;

    return handle;
}

lc_surface_handle_t lc_geometry_create_cone(vec3 apex, vec3 axis, vec3 x_axis, float angle)
{
    if (!g_initialized)
    {
        return LC_SURFACE_INVALID;
    }

    if (g_registry.surface_count >= LC_GEOMETRY_MAX_SURFACES)
    {
        return LC_SURFACE_INVALID;
    }

    lc_surface_handle_t handle = g_registry.surface_count++;
    lc_surface_entry_t *entry = &g_registry.surfaces[handle];

    entry->type = LC_SURFACE_CONE;

    glm_vec3_copy(apex, entry->data.cone.apex);
    glm_vec3_normalize_to(axis, entry->data.cone.axis);
    glm_vec3_normalize_to(x_axis, entry->data.cone.x_axis);

    /* Compute y_axis as cross product. */
    glm_vec3_cross(entry->data.cone.axis, entry->data.cone.x_axis, entry->data.cone.y_axis);
    glm_vec3_normalize(entry->data.cone.y_axis);

    entry->data.cone.angle = angle;

    return handle;
}

lc_surface_handle_t lc_geometry_create_torus(vec3 center, vec3 x_axis, vec3 y_axis,
                                               vec3 z_axis, float major_radius, float minor_radius)
{
    if (!g_initialized)
    {
        return LC_SURFACE_INVALID;
    }

    if (g_registry.surface_count >= LC_GEOMETRY_MAX_SURFACES)
    {
        return LC_SURFACE_INVALID;
    }

    lc_surface_handle_t handle = g_registry.surface_count++;
    lc_surface_entry_t *entry = &g_registry.surfaces[handle];

    entry->type = LC_SURFACE_TORUS;

    glm_vec3_copy(center, entry->data.torus.center);
    glm_vec3_normalize_to(x_axis, entry->data.torus.x_axis);
    glm_vec3_normalize_to(y_axis, entry->data.torus.y_axis);
    glm_vec3_normalize_to(z_axis, entry->data.torus.z_axis);
    entry->data.torus.major_radius = major_radius;
    entry->data.torus.minor_radius = minor_radius;

    return handle;
}

void lc_geometry_eval_curve(lc_curve_handle_t curve, float u, vec3 out_point)
{
    if (!is_valid_curve(curve))
    {
        glm_vec3_zero(out_point);
        return;
    }

    lc_curve_entry_t *entry = &g_registry.curves[curve];

    switch (entry->type)
    {
        case LC_CURVE_LINE:
        {
            /* P(u) = origin + u * direction * length */
            vec3 scaled_dir;
            glm_vec3_scale(entry->data.line.direction, u * entry->data.line.length, scaled_dir);
            glm_vec3_add(entry->data.line.origin, scaled_dir, out_point);
            break;
        }

        case LC_CURVE_CIRCLE:
        {
            /* P(u) = center + radius * (cos(u) * x_axis + sin(u) * y_axis) */
            float angle = entry->data.circle.u_start + u * (entry->data.circle.u_end - entry->data.circle.u_start);
            vec3 x_component, y_component;
            glm_vec3_scale(entry->data.circle.x_axis, cosf(angle) * entry->data.circle.radius, x_component);
            glm_vec3_scale(entry->data.circle.y_axis, sinf(angle) * entry->data.circle.radius, y_component);
            glm_vec3_add(entry->data.circle.center, x_component, out_point);
            glm_vec3_add(out_point, y_component, out_point);
            break;
        }

        case LC_CURVE_ELLIPSE:
        {
            /* P(u) = center + cos(u) * radius_major * x_axis + sin(u) * radius_minor * y_axis */
            float angle = entry->data.ellipse.u_start + u * (entry->data.ellipse.u_end - entry->data.ellipse.u_start);
            vec3 x_component, y_component;
            glm_vec3_scale(entry->data.ellipse.x_axis, cosf(angle) * entry->data.ellipse.radius_major, x_component);
            glm_vec3_scale(entry->data.ellipse.y_axis, sinf(angle) * entry->data.ellipse.radius_minor, y_component);
            glm_vec3_add(entry->data.ellipse.center, x_component, out_point);
            glm_vec3_add(out_point, y_component, out_point);
            break;
        }

        default:
            glm_vec3_zero(out_point);
            break;
    }
}

void lc_geometry_eval_curve_tangent(lc_curve_handle_t curve, float u, vec3 out_tangent)
{
    if (!is_valid_curve(curve))
    {
        glm_vec3_zero(out_tangent);
        return;
    }

    lc_curve_entry_t *entry = &g_registry.curves[curve];

    switch (entry->type)
    {
        case LC_CURVE_LINE:
        {
            /* Tangent is constant: direction */
            glm_vec3_copy(entry->data.line.direction, out_tangent);
            break;
        }

        case LC_CURVE_CIRCLE:
        {
            /* Tangent: dP/du = radius * (-sin(u) * x_axis + cos(u) * y_axis) */
            float angle = entry->data.circle.u_start + u * (entry->data.circle.u_end - entry->data.circle.u_start);
            vec3 x_component, y_component;
            glm_vec3_scale(entry->data.circle.x_axis, -sinf(angle), x_component);
            glm_vec3_scale(entry->data.circle.y_axis, cosf(angle), y_component);
            glm_vec3_add(x_component, y_component, out_tangent);
            glm_vec3_normalize(out_tangent);
            break;
        }

        case LC_CURVE_ELLIPSE:
        {
            /* Tangent: dP/du = -sin(u) * radius_major * x_axis + cos(u) * radius_minor * y_axis */
            float angle = entry->data.ellipse.u_start + u * (entry->data.ellipse.u_end - entry->data.ellipse.u_start);
            vec3 x_component, y_component;
            glm_vec3_scale(entry->data.ellipse.x_axis, -sinf(angle) * entry->data.ellipse.radius_major, x_component);
            glm_vec3_scale(entry->data.ellipse.y_axis, cosf(angle) * entry->data.ellipse.radius_minor, y_component);
            glm_vec3_add(x_component, y_component, out_tangent);
            glm_vec3_normalize(out_tangent);
            break;
        }

        default:
            glm_vec3_zero(out_tangent);
            break;
    }
}

void lc_geometry_eval_surface(lc_surface_handle_t surface, float u, float v, vec3 out_point)
{
    if (!is_valid_surface(surface))
    {
        glm_vec3_zero(out_point);
        return;
    }

    lc_surface_entry_t *entry = &g_registry.surfaces[surface];

    switch (entry->type)
    {
        case LC_SURFACE_PLANE:
        {
            /* P(u,v) = origin + u * x_axis + v * y_axis */
            vec3 u_component, v_component;
            glm_vec3_scale(entry->data.plane.x_axis, u, u_component);
            glm_vec3_scale(entry->data.plane.y_axis, v, v_component);
            glm_vec3_add(entry->data.plane.origin, u_component, out_point);
            glm_vec3_add(out_point, v_component, out_point);
            break;
        }

        case LC_SURFACE_CYLINDER:
        {
            /* P(u,v) = origin + v * axis + radius * (cos(u) * x_axis + sin(u) * y_axis) */
            vec3 v_component, radial_x, radial_y;
            glm_vec3_scale(entry->data.cylinder.axis, v, v_component);
            glm_vec3_scale(entry->data.cylinder.x_axis, cosf(u) * entry->data.cylinder.radius, radial_x);
            glm_vec3_scale(entry->data.cylinder.y_axis, sinf(u) * entry->data.cylinder.radius, radial_y);
            glm_vec3_add(entry->data.cylinder.origin, v_component, out_point);
            glm_vec3_add(out_point, radial_x, out_point);
            glm_vec3_add(out_point, radial_y, out_point);
            break;
        }

        case LC_SURFACE_SPHERE:
        {
            /* P(u,v) = center + radius * (cos(v) * cos(u) * x_axis + cos(v) * sin(u) * y_axis + sin(v) * z_axis) */
            float cos_u = cosf(u);
            float sin_u = sinf(u);
            float cos_v = cosf(v);
            float sin_v = sinf(v);
            vec3 x_component, y_component, z_component;
            glm_vec3_scale(entry->data.sphere.x_axis, entry->data.sphere.radius * cos_v * cos_u, x_component);
            glm_vec3_scale(entry->data.sphere.y_axis, entry->data.sphere.radius * cos_v * sin_u, y_component);
            glm_vec3_scale(entry->data.sphere.z_axis, entry->data.sphere.radius * sin_v, z_component);
            glm_vec3_add(entry->data.sphere.center, x_component, out_point);
            glm_vec3_add(out_point, y_component, out_point);
            glm_vec3_add(out_point, z_component, out_point);
            break;
        }

        case LC_SURFACE_CONE:
        {
            /* P(u,v) = apex + v * axis + (v * tan(angle)) * (cos(u) * x_axis + sin(u) * y_axis) */
            float radial_scale = v * tanf(entry->data.cone.angle);
            vec3 v_component, radial_x, radial_y;
            glm_vec3_scale(entry->data.cone.axis, v, v_component);
            glm_vec3_scale(entry->data.cone.x_axis, cosf(u) * radial_scale, radial_x);
            glm_vec3_scale(entry->data.cone.y_axis, sinf(u) * radial_scale, radial_y);
            glm_vec3_add(entry->data.cone.apex, v_component, out_point);
            glm_vec3_add(out_point, radial_x, out_point);
            glm_vec3_add(out_point, radial_y, out_point);
            break;
        }

        case LC_SURFACE_TORUS:
        {
            /* P(u,v) = center + (major_radius + minor_radius * cos(v)) * (cos(u) * x_axis + sin(u) * y_axis) + minor_radius * sin(v) * z_axis */
            float major_component = entry->data.torus.major_radius + entry->data.torus.minor_radius * cosf(v);
            vec3 xy_component, z_component, temp_x, temp_y;
            glm_vec3_scale(entry->data.torus.x_axis, cosf(u) * major_component, temp_x);
            glm_vec3_scale(entry->data.torus.y_axis, sinf(u) * major_component, temp_y);
            glm_vec3_add(temp_x, temp_y, xy_component);
            glm_vec3_scale(entry->data.torus.z_axis, entry->data.torus.minor_radius * sinf(v), z_component);
            glm_vec3_add(entry->data.torus.center, xy_component, out_point);
            glm_vec3_add(out_point, z_component, out_point);
            break;
        }

        default:
            glm_vec3_zero(out_point);
            break;
    }
}

void lc_geometry_eval_surface_normal(lc_surface_handle_t surface, float u, float v, vec3 out_normal)
{
    if (!is_valid_surface(surface))
    {
        glm_vec3_zero(out_normal);
        return;
    }

    lc_surface_entry_t *entry = &g_registry.surfaces[surface];

    switch (entry->type)
    {
        case LC_SURFACE_PLANE:
        {
            /* Normal is constant: plane normal */
            glm_vec3_copy(entry->data.plane.normal, out_normal);
            break;
        }

        case LC_SURFACE_CYLINDER:
        {
            /* Normal points radially outward: cos(u) * x_axis + sin(u) * y_axis */
            vec3 radial_x, radial_y;
            glm_vec3_scale(entry->data.cylinder.x_axis, cosf(u), radial_x);
            glm_vec3_scale(entry->data.cylinder.y_axis, sinf(u), radial_y);
            glm_vec3_add(radial_x, radial_y, out_normal);
            glm_vec3_normalize(out_normal);
            break;
        }

        case LC_SURFACE_SPHERE:
        {
            /* Normal points radially outward from center: (P - center) / radius */
            vec3 point;
            lc_geometry_eval_surface(surface, u, v, point);
            glm_vec3_sub(point, entry->data.sphere.center, out_normal);
            glm_vec3_normalize(out_normal);
            break;
        }

        case LC_SURFACE_CONE:
        {
            /* Compute tangent vectors and cross product. */
            vec3 du, dv;
            /* dP/du = (v * tan(angle)) * (-sin(u) * x_axis + cos(u) * y_axis) */
            float radial_scale = v * tanf(entry->data.cone.angle);
            vec3 du_x, du_y;
            glm_vec3_scale(entry->data.cone.x_axis, -sinf(u) * radial_scale, du_x);
            glm_vec3_scale(entry->data.cone.y_axis, cosf(u) * radial_scale, du_y);
            glm_vec3_add(du_x, du_y, du);

            /* dP/dv = axis + tan(angle) * (cos(u) * x_axis + sin(u) * y_axis) */
            vec3 dv_radial_x, dv_radial_y;
            glm_vec3_scale(entry->data.cone.x_axis, cosf(u) * tanf(entry->data.cone.angle), dv_radial_x);
            glm_vec3_scale(entry->data.cone.y_axis, sinf(u) * tanf(entry->data.cone.angle), dv_radial_y);
            glm_vec3_add(entry->data.cone.axis, dv_radial_x, dv);
            glm_vec3_add(dv, dv_radial_y, dv);

            glm_vec3_cross(du, dv, out_normal);
            glm_vec3_normalize(out_normal);
            break;
        }

        case LC_SURFACE_TORUS:
        {
            /* Compute tangent vectors and cross product. */
            vec3 du, dv;
            /* dP/du = (major_radius + minor_radius * cos(v)) * (-sin(u) * x_axis + cos(u) * y_axis) */
            float major_component = entry->data.torus.major_radius + entry->data.torus.minor_radius * cosf(v);
            vec3 du_x, du_y;
            glm_vec3_scale(entry->data.torus.x_axis, -sinf(u) * major_component, du_x);
            glm_vec3_scale(entry->data.torus.y_axis, cosf(u) * major_component, du_y);
            glm_vec3_add(du_x, du_y, du);

            /* dP/dv = -minor_radius * sin(v) * (cos(u) * x_axis + sin(u) * y_axis) + minor_radius * cos(v) * z_axis */
            vec3 dv_xy_x, dv_xy_y, dv_z;
            glm_vec3_scale(entry->data.torus.x_axis, -entry->data.torus.minor_radius * sinf(v) * cosf(u), dv_xy_x);
            glm_vec3_scale(entry->data.torus.y_axis, -entry->data.torus.minor_radius * sinf(v) * sinf(u), dv_xy_y);
            glm_vec3_scale(entry->data.torus.z_axis, entry->data.torus.minor_radius * cosf(v), dv_z);
            glm_vec3_add(dv_xy_x, dv_xy_y, dv);
            glm_vec3_add(dv, dv_z, dv);

            glm_vec3_cross(du, dv, out_normal);
            glm_vec3_normalize(out_normal);
            break;
        }

        default:
            glm_vec3_zero(out_normal);
            break;
    }
}

lc_surface_type_t lc_geometry_get_surface_type(lc_surface_handle_t surface)
{
    if (!is_valid_surface(surface))
    {
        return LC_SURFACE_INVALID;
    }

    return g_registry.surfaces[surface].type;
}

lc_curve_type_t lc_geometry_get_curve_type(lc_curve_handle_t curve)
{
    if (!is_valid_curve(curve))
    {
        return LC_CURVE_INVALID;
    }

    return g_registry.curves[curve].type;
}

bool lc_geometry_get_plane_data(lc_surface_handle_t surface, lc_surface_plane_t *out_data)
{
    if (!is_valid_surface(surface))
    {
        return false;
    }

    lc_surface_entry_t *entry = &g_registry.surfaces[surface];
    if (entry->type != LC_SURFACE_PLANE)
    {
        return false;
    }

    *out_data = entry->data.plane;
    return true;
}

bool lc_geometry_get_cylinder_data(lc_surface_handle_t surface, lc_surface_cylinder_t *out_data)
{
    if (!is_valid_surface(surface))
    {
        return false;
    }

    lc_surface_entry_t *entry = &g_registry.surfaces[surface];
    if (entry->type != LC_SURFACE_CYLINDER)
    {
        return false;
    }

    *out_data = entry->data.cylinder;
    return true;
}

bool lc_geometry_get_sphere_data(lc_surface_handle_t surface, lc_surface_sphere_t *out_data)
{
    if (!is_valid_surface(surface))
    {
        return false;
    }

    lc_surface_entry_t *entry = &g_registry.surfaces[surface];
    if (entry->type != LC_SURFACE_SPHERE)
    {
        return false;
    }

    *out_data = entry->data.sphere;
    return true;
}

float lc_geometry_curve_length(lc_curve_handle_t curve)
{
    if (!is_valid_curve(curve))
    {
        return 0.0f;
    }

    lc_curve_entry_t *entry = &g_registry.curves[curve];

    switch (entry->type)
    {
        case LC_CURVE_LINE:
            return entry->data.line.length;

        case LC_CURVE_CIRCLE:
        {
            float angle_range = entry->data.circle.u_end - entry->data.circle.u_start;
            return entry->data.circle.radius * angle_range;
        }

        case LC_CURVE_ELLIPSE:
        {
            /* Approximate ellipse arc length using Ramanujan approximation. */
            float a = entry->data.ellipse.radius_major;
            float b = entry->data.ellipse.radius_minor;
            float h = ((a - b) * (a - b)) / ((a + b) * (a + b));
            float circumference = 3.14159265359f * (a + b) * (1.0f + (3.0f * h) / (10.0f + sqrtf(4.0f - 3.0f * h)));
            float angle_range = entry->data.ellipse.u_end - entry->data.ellipse.u_start;
            return circumference * (angle_range / (2.0f * 3.14159265359f));
        }

        default:
            return 0.0f;
    }
}

float lc_geometry_project_point_curve(lc_curve_handle_t curve, vec3 point)
{
    if (!is_valid_curve(curve))
    {
        return 0.0f;
    }

    /* Stub implementation: return midpoint parameter.
     * Proper projection requires iterative solver. */
    return 0.5f;
}

void lc_geometry_project_point_surface(lc_surface_handle_t surface, vec3 point,
                                         float *out_u, float *out_v)
{
    if (!is_valid_surface(surface))
    {
        *out_u = 0.0f;
        *out_v = 0.0f;
        return;
    }

    /* Stub implementation: return center parameters.
     * Proper projection requires iterative solver. */
    *out_u = 0.0f;
    *out_v = 0.0f;
}

bool lc_geometry_intersect_curves(lc_curve_handle_t curve1,
                                   lc_curve_handle_t curve2,
                                   float *out_u1,
                                   float *out_u2)
{
    if (!is_valid_curve(curve1) || !is_valid_curve(curve2))
    {
        return false;
    }

    /* Stub implementation: only line-line intersection.
     * Full implementation requires handling line-circle, circle-circle, etc. */

    lc_curve_entry_t *entry1 = &g_registry.curves[curve1];
    lc_curve_entry_t *entry2 = &g_registry.curves[curve2];

    if (entry1->type == LC_CURVE_LINE && entry2->type == LC_CURVE_LINE)
    {
        /* Line-line intersection in 3D (approximate: check if lines are coplanar). */
        /* For Phase 4A, return false (not implemented). */
        return false;
    }

    return false;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static bool is_valid_curve(lc_curve_handle_t curve)
{
    if (!g_initialized)
    {
        return false;
    }

    if (curve == LC_CURVE_INVALID || curve >= g_registry.curve_count)
    {
        return false;
    }

    if (g_registry.curves[curve].type == LC_CURVE_INVALID)
    {
        return false;
    }

    return true;
}

static bool is_valid_surface(lc_surface_handle_t surface)
{
    if (!g_initialized)
    {
        return false;
    }

    if (surface == LC_SURFACE_INVALID || surface >= g_registry.surface_count)
    {
        return false;
    }

    if (g_registry.surfaces[surface].type == LC_SURFACE_INVALID)
    {
        return false;
    }

    return true;
}
