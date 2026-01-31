/***************************************************************
**
** libcad Header File
**
** File         :  lc_geometry.h
** Module       :  libcad (geometry system)
** Author       :  SH
** Created      :  2026-01-30 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Geometric primitive definitions for B-Rep kernel.
**                 Provides immutable curve and surface definitions with
**                 evaluation functions. Curves and surfaces are stored
**                 in a registry separate from the entity system to
**                 enable sharing between multiple edges/faces.
**
***************************************************************/

#ifndef LC_GEOMETRY_H
#define LC_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <cglm/cglm.h>
#include <stdbool.h>
#include <stdint.h>

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* Curve type enumeration. */
typedef enum lc_curve_type_t
{
    LC_CURVE_INVALID = 0,
    LC_CURVE_LINE,              /* Straight line segment */
    LC_CURVE_CIRCLE,            /* Circular arc */
    LC_CURVE_ELLIPSE,           /* Elliptical arc */
    LC_CURVE_BSPLINE,           /* B-spline curve (future) */
} lc_curve_type_t;

/* Opaque curve handle (internal index into curve table) */
typedef uint32_t lc_curve_handle_t;
#define LC_CURVE_INVALID ((lc_curve_handle_t)0)

/* Line curve: parameterized as P(u) = origin + u * direction, u ∈ [0, 1] */
typedef struct lc_curve_line_t
{
    vec3 origin;                /* Line origin (u = 0) */
    vec3 direction;             /* Line direction (unit vector) */
    float length;               /* Line length (u = 1 corresponds to length) */
} lc_curve_line_t;

/* Circular arc curve: parameterized as P(u) = center + radius * (cos(u) * x_axis + sin(u) * y_axis) */
typedef struct lc_curve_circle_t
{
    vec3 center;                /* Circle center */
    vec3 x_axis;                /* Circle X-axis (unit vector) */
    vec3 y_axis;                /* Circle Y-axis (unit vector, perpendicular to x_axis) */
    float radius;               /* Circle radius */
    float u_start;              /* Start angle in radians */
    float u_end;                /* End angle in radians */
} lc_curve_circle_t;

/* Ellipse arc curve: similar to circle but with two radii */
typedef struct lc_curve_ellipse_t
{
    vec3 center;
    vec3 x_axis;                /* Major axis direction (unit vector) */
    vec3 y_axis;                /* Minor axis direction (unit vector) */
    float radius_major;
    float radius_minor;
    float u_start;              /* Start angle in radians */
    float u_end;                /* End angle in radians */
} lc_curve_ellipse_t;

/* Surface type enumeration. */
typedef enum lc_surface_type_t
{
    LC_SURFACE_INVALID = 0,
    LC_SURFACE_PLANE,           /* Infinite plane */
    LC_SURFACE_CYLINDER,        /* Cylindrical surface */
    LC_SURFACE_SPHERE,          /* Spherical surface */
    LC_SURFACE_CONE,            /* Conical surface */
    LC_SURFACE_TORUS,           /* Toroidal surface */
    LC_SURFACE_BSPLINE,         /* B-spline surface (future) */
} lc_surface_type_t;

/* Opaque surface handle (internal index into surface table) */
typedef uint32_t lc_surface_handle_t;
#define LC_SURFACE_INVALID ((lc_surface_handle_t)0)

/* Plane surface: parameterized as P(u,v) = origin + u * x_axis + v * y_axis */
typedef struct lc_surface_plane_t
{
    vec3 origin;                /* Plane origin */
    vec3 x_axis;                /* U-direction (unit vector) */
    vec3 y_axis;                /* V-direction (unit vector) */
    vec3 normal;                /* Plane normal (cross product of x_axis and y_axis) */
} lc_surface_plane_t;

/* Cylindrical surface: parameterized as P(u,v) = origin + v * axis + radius * (cos(u) * x_axis + sin(u) * y_axis) */
typedef struct lc_surface_cylinder_t
{
    vec3 origin;                /* Cylinder base center */
    vec3 axis;                  /* Cylinder axis (unit vector) */
    vec3 x_axis;                /* Radial X-axis (unit vector, perpendicular to axis) */
    vec3 y_axis;                /* Radial Y-axis (unit vector, perpendicular to axis and x_axis) */
    float radius;               /* Cylinder radius */
} lc_surface_cylinder_t;

/* Spherical surface: parameterized as P(u,v) = center + radius * (cos(v) * cos(u) * x_axis + cos(v) * sin(u) * y_axis + sin(v) * z_axis) */
typedef struct lc_surface_sphere_t
{
    vec3 center;                /* Sphere center */
    vec3 x_axis;                /* Sphere X-axis (unit vector) */
    vec3 y_axis;                /* Sphere Y-axis (unit vector) */
    vec3 z_axis;                /* Sphere Z-axis (unit vector) */
    float radius;               /* Sphere radius */
} lc_surface_sphere_t;

/* Conical surface: parameterized as P(u,v) = apex + v * axis + (v * tan(angle)) * (cos(u) * x_axis + sin(u) * y_axis) */
typedef struct lc_surface_cone_t
{
    vec3 apex;                  /* Cone apex */
    vec3 axis;                  /* Cone axis (unit vector) */
    vec3 x_axis;                /* Radial X-axis (unit vector) */
    vec3 y_axis;                /* Radial Y-axis (unit vector) */
    float angle;                /* Half-angle in radians (angle between axis and surface) */
} lc_surface_cone_t;

/* Toroidal surface: parameterized as P(u,v) = center + (major_radius + minor_radius * cos(v)) * (cos(u) * x_axis + sin(u) * y_axis) + minor_radius * sin(v) * z_axis */
typedef struct lc_surface_torus_t
{
    vec3 center;                /* Torus center */
    vec3 x_axis;                /* Major X-axis (unit vector) */
    vec3 y_axis;                /* Major Y-axis (unit vector) */
    vec3 z_axis;                /* Minor axis (unit vector) */
    float major_radius;         /* Distance from center to tube center */
    float minor_radius;         /* Tube radius */
} lc_surface_torus_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* Initialize geometry system.
 * Call once at startup before any geometry operations. */
void lc_geometry_init(void);

/* Shutdown geometry system and free all resources. */
void lc_geometry_shutdown(void);

/* Create curve definitions */

/* Create a line curve.
 * origin: Line start point (u = 0)
 * direction: Line direction (will be normalized internally)
 * length: Line length
 * Returns curve handle on success, LC_CURVE_INVALID on failure. */
lc_curve_handle_t lc_geometry_create_line(vec3 origin, vec3 direction, float length);

/* Create a circular arc curve.
 * center: Circle center
 * x_axis: Circle X-axis (will be normalized internally)
 * y_axis: Circle Y-axis (will be normalized internally, must be perpendicular to x_axis)
 * radius: Circle radius
 * u_start: Start angle in radians
 * u_end: End angle in radians
 * Returns curve handle on success, LC_CURVE_INVALID on failure. */
lc_curve_handle_t lc_geometry_create_circle(vec3 center, vec3 x_axis, vec3 y_axis,
                                              float radius, float u_start, float u_end);

/* Create an ellipse arc curve.
 * center: Ellipse center
 * x_axis: Major axis direction (will be normalized internally)
 * y_axis: Minor axis direction (will be normalized internally, must be perpendicular to x_axis)
 * radius_major: Major radius
 * radius_minor: Minor radius
 * u_start: Start angle in radians
 * u_end: End angle in radians
 * Returns curve handle on success, LC_CURVE_INVALID on failure. */
lc_curve_handle_t lc_geometry_create_ellipse(vec3 center, vec3 x_axis, vec3 y_axis,
                                               float radius_major, float radius_minor,
                                               float u_start, float u_end);

/* Create surface definitions */

/* Create a plane surface.
 * origin: Plane origin
 * x_axis: U-direction (will be normalized internally)
 * y_axis: V-direction (will be normalized internally, must be perpendicular to x_axis)
 * Returns surface handle on success, LC_SURFACE_INVALID on failure. */
lc_surface_handle_t lc_geometry_create_plane(vec3 origin, vec3 x_axis, vec3 y_axis);

/* Create a cylindrical surface.
 * origin: Cylinder base center
 * axis: Cylinder axis (will be normalized internally)
 * x_axis: Radial X-axis (will be normalized internally, must be perpendicular to axis)
 * radius: Cylinder radius
 * Returns surface handle on success, LC_SURFACE_INVALID on failure. */
lc_surface_handle_t lc_geometry_create_cylinder(vec3 origin, vec3 axis, vec3 x_axis, float radius);

/* Create a spherical surface.
 * center: Sphere center
 * x_axis: Sphere X-axis (will be normalized internally)
 * y_axis: Sphere Y-axis (will be normalized internally, must be perpendicular to x_axis)
 * z_axis: Sphere Z-axis (will be normalized internally, must be perpendicular to both)
 * radius: Sphere radius
 * Returns surface handle on success, LC_SURFACE_INVALID on failure. */
lc_surface_handle_t lc_geometry_create_sphere(vec3 center, vec3 x_axis, vec3 y_axis,
                                                vec3 z_axis, float radius);

/* Create a conical surface.
 * apex: Cone apex
 * axis: Cone axis (will be normalized internally)
 * x_axis: Radial X-axis (will be normalized internally, must be perpendicular to axis)
 * angle: Half-angle in radians (angle between axis and surface)
 * Returns surface handle on success, LC_SURFACE_INVALID on failure. */
lc_surface_handle_t lc_geometry_create_cone(vec3 apex, vec3 axis, vec3 x_axis, float angle);

/* Create a toroidal surface.
 * center: Torus center
 * x_axis: Major X-axis (will be normalized internally)
 * y_axis: Major Y-axis (will be normalized internally, must be perpendicular to x_axis)
 * z_axis: Minor axis (will be normalized internally, must be perpendicular to both)
 * major_radius: Distance from center to tube center
 * minor_radius: Tube radius
 * Returns surface handle on success, LC_SURFACE_INVALID on failure. */
lc_surface_handle_t lc_geometry_create_torus(vec3 center, vec3 x_axis, vec3 y_axis,
                                               vec3 z_axis, float major_radius, float minor_radius);

/* Evaluation functions */

/* Evaluate curve at parameter u (returns 3D point).
 * curve: Curve handle
 * u: Parameter value (typically in [0, 1] for lines, angle in radians for circles)
 * out_point: Output 3D point */
void lc_geometry_eval_curve(lc_curve_handle_t curve, float u, vec3 out_point);

/* Evaluate curve tangent at parameter u (returns unit vector).
 * curve: Curve handle
 * u: Parameter value
 * out_tangent: Output tangent vector (normalized) */
void lc_geometry_eval_curve_tangent(lc_curve_handle_t curve, float u, vec3 out_tangent);

/* Evaluate surface at parameters (u, v) (returns 3D point).
 * surface: Surface handle
 * u: U parameter value
 * v: V parameter value
 * out_point: Output 3D point */
void lc_geometry_eval_surface(lc_surface_handle_t surface, float u, float v, vec3 out_point);

/* Evaluate surface normal at parameters (u, v) (returns unit vector).
 * surface: Surface handle
 * u: U parameter value
 * v: V parameter value
 * out_normal: Output normal vector (normalized) */
void lc_geometry_eval_surface_normal(lc_surface_handle_t surface, float u, float v, vec3 out_normal);

/* Get the type of a surface.
 * Returns LC_SURFACE_INVALID if handle is invalid. */
lc_surface_type_t lc_geometry_get_surface_type(lc_surface_handle_t surface);

/* Get the type of a curve.
 * Returns LC_CURVE_INVALID if handle is invalid. */
lc_curve_type_t lc_geometry_get_curve_type(lc_curve_handle_t curve);

/* Get surface data for a plane surface.
 * Returns true if surface is valid and is a plane, false otherwise. */
bool lc_geometry_get_plane_data(lc_surface_handle_t surface, lc_surface_plane_t *out_data);

/* Get surface data for a cylindrical surface.
 * Returns true if surface is valid and is a cylinder, false otherwise. */
bool lc_geometry_get_cylinder_data(lc_surface_handle_t surface, lc_surface_cylinder_t *out_data);

/* Get surface data for a spherical surface.
 * Returns true if surface is valid and is a sphere, false otherwise. */
bool lc_geometry_get_sphere_data(lc_surface_handle_t surface, lc_surface_sphere_t *out_data);

/* Compute curve length (exact for lines/arcs, approximate for splines).
 * Returns length or 0.0 if curve handle is invalid. */
float lc_geometry_curve_length(lc_curve_handle_t curve);

/* Project point onto curve (returns parameter u of closest point).
 * For Phase 4A: returns approximate u (not exact projection).
 * Returns 0.0 if curve handle is invalid. */
float lc_geometry_project_point_curve(lc_curve_handle_t curve, vec3 point);

/* Project point onto surface (returns parameters u, v of closest point).
 * For Phase 4A: returns approximate u, v (not exact projection).
 * out_u and out_v are set to 0.0 if surface handle is invalid. */
void lc_geometry_project_point_surface(lc_surface_handle_t surface, vec3 point,
                                         float *out_u, float *out_v);

/* Intersect two curves (returns parameter u1 on curve1 and u2 on curve2).
 * Returns true if intersection found, false otherwise.
 * For Phase 4A: only line-line and line-circle implemented. */
bool lc_geometry_intersect_curves(lc_curve_handle_t curve1,
                                   lc_curve_handle_t curve2,
                                   float *out_u1,
                                   float *out_u2);

#ifdef __cplusplus
}
#endif

#endif /* LC_GEOMETRY_H */
