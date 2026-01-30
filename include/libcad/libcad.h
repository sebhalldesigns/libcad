#ifndef LIBCAD_H
#define LIBCAD_H


#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXPORT
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#elif __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT __attribute__((visibility("default")))
#endif
#endif


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MOUSE_LEFT_BUTTON    1
#define MOUSE_MIDDLE_BUTTON  2
#define MOUSE_RIGHT_BUTTON   3

#define MODIFIER_CONTROL    1
#define MODIFIER_SHIFT      2
#define MODIFIER_ALT        3

#define CURSOR_NORMAL       0
#define CURSOR_MOVE         1
#define CURSOR_RESIZE_H     2
#define CURSOR_RESIZE_V     3
#define CURSOR_RESIZE_NWSE  4
#define CURSOR_RESIZE_NESW  5
#define CURSOR_POINTER      6

typedef uintptr_t cad_ctx_t;
typedef uintptr_t cad_model_t;

/* Entity handle types for document model */
typedef uint32_t cad_entity_t;
typedef uint32_t cad_sketch_t;
typedef uint32_t cad_body_t;

#define CAD_INVALID_ENTITY ((cad_entity_t)0)

EXPORT cad_ctx_t    cad_create_context();
EXPORT void         cad_destroy_context(cad_ctx_t ctx);

EXPORT bool         cad_load_model_file(cad_ctx_t ctx, cad_model_t* p_model, const char* filepath);
EXPORT bool         cad_load_model_data(cad_ctx_t ctx, cad_model_t* p_model, const uint8_t* data, size_t size);
EXPORT void         cad_unload_model(cad_ctx_t ctx);

EXPORT bool         cad_write_model_file(cad_ctx_t ctx, cad_model_t model, const char* format, const char* filepath);
EXPORT bool         cad_write_model_data(cad_ctx_t ctx, cad_model_t model, const char* format, uint8_t** p_data, size_t* p_size);

EXPORT void         cad_set_viewport(int x, int y, int width, int height, int window_width, int window_height);
EXPORT void         cad_render_viewport();
EXPORT void         cad_init_viewport();

EXPORT void         cad_set_cursor_pos(int x, int y);
EXPORT void         cad_cursor_lost();
EXPORT void         cad_set_cursor_button_state(int button, bool pressed);
EXPORT void         cad_set_modifier_state(int modifier, bool state);
EXPORT void         cad_axis_delta(int axis, float delta);

EXPORT void         cad_start_modal_tool(int tool_id);
EXPORT void         cad_clear_modal_tool();

EXPORT int          cad_get_cursor_type();

EXPORT void         cad_save_json(const char *path);
EXPORT void         cad_load_json(const char *path);

EXPORT const char*  cad_get_hovered_name(void);
EXPORT int          cad_get_hovered_id(void);

/* ---- Document Model (Phase 2) ---- */

/* Sketch creation and management */
EXPORT cad_sketch_t     cad_create_sketch(cad_ctx_t ctx);
EXPORT cad_entity_t     cad_sketch_add_line(cad_ctx_t ctx, cad_sketch_t sketch,
                                             float x1, float y1, float x2, float y2);
EXPORT cad_entity_t     cad_sketch_add_circle(cad_ctx_t ctx, cad_sketch_t sketch,
                                               float cx, float cy, float radius);
EXPORT cad_entity_t     cad_sketch_add_rect(cad_ctx_t ctx, cad_sketch_t sketch,
                                              float x1, float y1, float x2, float y2);
EXPORT void             cad_delete_entity(cad_ctx_t ctx, cad_entity_t entity);

/* Entity metadata */
EXPORT void             cad_set_entity_name(cad_ctx_t ctx, cad_entity_t entity, const char *name);
EXPORT const char*      cad_get_entity_name(cad_ctx_t ctx, cad_entity_t entity);

/* Selection */
EXPORT void             cad_select_entity(cad_ctx_t ctx, cad_entity_t entity);
EXPORT void             cad_deselect_all(cad_ctx_t ctx);
EXPORT int              cad_get_selection_count(cad_ctx_t ctx);

/* Undo/Redo */
EXPORT void             cad_undo(cad_ctx_t ctx);
EXPORT void             cad_redo(cad_ctx_t ctx);
EXPORT bool             cad_can_undo(cad_ctx_t ctx);
EXPORT bool             cad_can_redo(cad_ctx_t ctx);

/* ---- Constraint System (Phase 3) ---- */

/* Constraint creation */
EXPORT cad_entity_t     cad_constraint_distance_point_point(cad_ctx_t ctx,
                                                             cad_entity_t point1,
                                                             cad_entity_t point2,
                                                             float distance);
EXPORT cad_entity_t     cad_constraint_distance_point_line(cad_ctx_t ctx,
                                                            cad_entity_t point,
                                                            cad_entity_t line,
                                                            float distance);
EXPORT cad_entity_t     cad_constraint_angle_line_line(cad_ctx_t ctx,
                                                        cad_entity_t line1,
                                                        cad_entity_t line2,
                                                        float angle_radians);
EXPORT cad_entity_t     cad_constraint_parallel(cad_ctx_t ctx,
                                                 cad_entity_t line1,
                                                 cad_entity_t line2);
EXPORT cad_entity_t     cad_constraint_perpendicular(cad_ctx_t ctx,
                                                      cad_entity_t line1,
                                                      cad_entity_t line2);
EXPORT cad_entity_t     cad_constraint_horizontal(cad_ctx_t ctx, cad_entity_t line);
EXPORT cad_entity_t     cad_constraint_vertical(cad_ctx_t ctx, cad_entity_t line);
EXPORT cad_entity_t     cad_constraint_coincident_point_point(cad_ctx_t ctx,
                                                               cad_entity_t point1,
                                                               cad_entity_t point2);
EXPORT cad_entity_t     cad_constraint_coincident_point_line(cad_ctx_t ctx,
                                                              cad_entity_t point,
                                                              cad_entity_t line);
EXPORT cad_entity_t     cad_constraint_coincident_point_circle(cad_ctx_t ctx,
                                                                cad_entity_t point,
                                                                cad_entity_t circle);
EXPORT cad_entity_t     cad_constraint_tangent_line_circle(cad_ctx_t ctx,
                                                            cad_entity_t line,
                                                            cad_entity_t circle);
EXPORT cad_entity_t     cad_constraint_tangent_circle_circle(cad_ctx_t ctx,
                                                              cad_entity_t circle1,
                                                              cad_entity_t circle2);
EXPORT cad_entity_t     cad_constraint_equal_length(cad_ctx_t ctx,
                                                     cad_entity_t line1,
                                                     cad_entity_t line2);
EXPORT cad_entity_t     cad_constraint_equal_radius(cad_ctx_t ctx,
                                                     cad_entity_t circle1,
                                                     cad_entity_t circle2);
EXPORT cad_entity_t     cad_constraint_fix_point(cad_ctx_t ctx, cad_entity_t point);

/* Constraint management */
EXPORT bool             cad_constraint_delete(cad_ctx_t ctx, cad_entity_t constraint);
EXPORT bool             cad_constraint_set_value(cad_ctx_t ctx, cad_entity_t constraint, float value);
EXPORT float            cad_constraint_get_value(cad_ctx_t ctx, cad_entity_t constraint);
EXPORT float            cad_constraint_get_error(cad_ctx_t ctx, cad_entity_t constraint);
EXPORT bool             cad_constraint_is_satisfied(cad_ctx_t ctx, cad_entity_t constraint);

/* Sketch constraint analysis */
EXPORT int              cad_sketch_get_dof(cad_ctx_t ctx, cad_sketch_t sketch);
EXPORT bool             cad_sketch_is_fully_constrained(cad_ctx_t ctx, cad_sketch_t sketch);
EXPORT bool             cad_sketch_is_over_constrained(cad_ctx_t ctx, cad_sketch_t sketch);
EXPORT int              cad_sketch_get_constraint_count(cad_ctx_t ctx, cad_sketch_t sketch);

/* ---- B-Rep Solid Modeling (Phase 4) ---- */

/* Create a box solid (rectangular prism).
 * ox, oy, oz: origin corner at minimum x, y, z
 * dx, dy, dz: dimensions (width, height, depth)
 * Returns solid entity handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t     cad_create_box(cad_ctx_t ctx,
                                        float ox, float oy, float oz,
                                        float dx, float dy, float dz);

/* Create a cylinder solid.
 * cx, cy, cz: center of bottom circle
 * ax, ay, az: axis direction
 * radius: cylinder radius
 * height: cylinder height
 * segments: number of segments for circle approximation
 * Returns solid entity handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t     cad_create_cylinder(cad_ctx_t ctx,
                                             float cx, float cy, float cz,
                                             float ax, float ay, float az,
                                             float radius, float height,
                                             int segments);

/* Create a sphere solid.
 * cx, cy, cz: sphere center
 * radius: sphere radius
 * u_segments: longitudinal segments
 * v_segments: latitudinal segments
 * Returns solid entity handle on success, CAD_INVALID_ENTITY on failure. */
EXPORT cad_entity_t     cad_create_sphere(cad_ctx_t ctx,
                                           float cx, float cy, float cz,
                                           float radius,
                                           int u_segments, int v_segments);

/* Validate a solid: check topology integrity.
 * Returns true if solid passes Euler characteristic check. */
EXPORT bool             cad_validate_solid(cad_ctx_t ctx, cad_entity_t solid);

/* Get number of faces in a solid. */
EXPORT int              cad_solid_get_face_count(cad_ctx_t ctx, cad_entity_t solid);

/* Get number of edges in a solid. */
EXPORT int              cad_solid_get_edge_count(cad_ctx_t ctx, cad_entity_t solid);

/* Get number of vertices in a solid. */
EXPORT int              cad_solid_get_vertex_count(cad_ctx_t ctx, cad_entity_t solid);

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_H */

