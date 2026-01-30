/***************************************************************
**
** libcad Header File
**
** File         :  lc_constraint.h
** Module       :  libcad (constraint system)
** Author       :  SH
** Created      :  2026-01-29 (YYYY-MM-DD)
** License      :  MIT
** Description  :  2D parametric constraint system. Provides constraint
**                 creation, evaluation, and dependency analysis for
**                 sketch entities.
**
***************************************************************/

#ifndef LC_CONSTRAINT_H
#define LC_CONSTRAINT_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_entity.h"
#include <stdbool.h>

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* Initialize constraint system.
 * Call once at startup before any constraint operations. */
void lc_constraint_init(void);

/* Shutdown constraint system.
 * Call once at shutdown. */
void lc_constraint_shutdown(void);

/* Create a distance constraint between two points.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_distance_point_point(
    lc_entity_handle_t point1,
    lc_entity_handle_t point2,
    float distance_value);

/* Create a distance constraint from a point to a line.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_distance_point_line(
    lc_entity_handle_t point,
    lc_entity_handle_t line,
    float distance_value);

/* Create an angle constraint between two lines.
 * angle_value is in radians.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_angle_line_line(
    lc_entity_handle_t line1,
    lc_entity_handle_t line2,
    float angle_radians);

/* Create a coincident constraint between two points.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_coincident_point_point(
    lc_entity_handle_t point1,
    lc_entity_handle_t point2);

/* Create a coincident constraint: point on line.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_coincident_point_line(
    lc_entity_handle_t point,
    lc_entity_handle_t line);

/* Create a coincident constraint: point on circle.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_coincident_point_circle(
    lc_entity_handle_t point,
    lc_entity_handle_t circle);

/* Create a parallel constraint between two lines.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_parallel(
    lc_entity_handle_t line1,
    lc_entity_handle_t line2);

/* Create a perpendicular constraint between two lines.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_perpendicular(
    lc_entity_handle_t line1,
    lc_entity_handle_t line2);

/* Create a horizontal constraint on a line.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_horizontal(
    lc_entity_handle_t line);

/* Create a vertical constraint on a line.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_vertical(
    lc_entity_handle_t line);

/* Create a tangent constraint: line-circle.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_tangent_line_circle(
    lc_entity_handle_t line,
    lc_entity_handle_t circle);

/* Create a tangent constraint: circle-circle.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_tangent_circle_circle(
    lc_entity_handle_t circle1,
    lc_entity_handle_t circle2);

/* Create an equal length constraint between two line segments.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_equal_length(
    lc_entity_handle_t line1,
    lc_entity_handle_t line2);

/* Create an equal radius constraint between two circles.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_equal_radius(
    lc_entity_handle_t circle1,
    lc_entity_handle_t circle2);

/* Create a fix constraint that locks a point position.
 * Returns constraint handle on success, LC_ENTITY_INVALID on failure. */
lc_entity_handle_t lc_constraint_create_fix_point(
    lc_entity_handle_t point);

/* Destroy a constraint entity.
 * Returns true if successful, false if constraint handle is invalid. */
bool lc_constraint_destroy(lc_entity_handle_t constraint);

/* Evaluate a single constraint: compute error/residual.
 * Returns error value (0.0 = satisfied, non-zero = violated).
 * Also updates constraint->error field.
 * Stub implementation for Phase 3B: returns 0.0 */
float lc_constraint_evaluate(lc_entity_handle_t constraint);

/* Get current error from constraint.
 * Returns error value or 0.0 if constraint is invalid. */
float lc_constraint_get_error(lc_entity_handle_t constraint);

/* Build constraint graph for a sketch.
 * Analyzes all geometry entities and constraints in the sketch.
 * Detects over-constrained, under-constrained, and fully-constrained entities.
 * Graph is cached until entities or constraints are added/removed/modified.
 * Returns true if graph was built successfully, false on error. */
bool lc_constraint_build_graph(lc_entity_handle_t sketch);

/* Invalidate constraint graph (call when entities or constraints change).
 * Forces rebuild on next call to lc_constraint_build_graph(). */
void lc_constraint_invalidate_graph(lc_entity_handle_t sketch);

/* Get degrees of freedom analysis for a sketch.
 * Returns total unconstrained DOF:
 *   > 0: under-constrained (needs more constraints)
 *   = 0: fully constrained (good)
 *   < 0: over-constrained (conflicting constraints)
 * Returns -1 on error (invalid sketch handle). */
int lc_constraint_get_dof(lc_entity_handle_t sketch);

/* Check if a sketch is fully constrained.
 * Returns true if DOF = 0, false otherwise. */
bool lc_constraint_is_fully_constrained(lc_entity_handle_t sketch);

/* Check if a sketch is over-constrained.
 * Returns true if DOF < 0, false otherwise. */
bool lc_constraint_is_over_constrained(lc_entity_handle_t sketch);

#ifdef __cplusplus
}
#endif

#endif /* LC_CONSTRAINT_H */
