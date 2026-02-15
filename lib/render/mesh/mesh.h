/***************************************************************
**
** libcad Header File
**
** File         :  mesh.h
** Module       :  render/mesh
** Author       :  SH
** Created      :  2026-02-15 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad mesh rendering for solid bodies
**
***************************************************************/

#ifndef LIBCAD_MESH_H
#define LIBCAD_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <cglm/cglm.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define MESH_INVALID_HANDLE 0xFFFFFFFF
#define MESH_MAX_ENTRIES 256

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef struct {
    float* vertices;           /* interleaved pos+normal: 6 floats per vert */
    uint32_t* indices;         /* triangle indices */
    float* edge_vertices;      /* edge line positions: 3 floats per vert */
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t edge_vertex_count;
    vec4 fill_color;
    vec4 edge_color;
    mat4 model_matrix;
} mesh_instance_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

bool mesh_init(void);
void mesh_set_dpi_scale(float scale);
void mesh_render(int width, int height, mat4 view_projection);

uint32_t mesh_create(const mesh_instance_t* instance);
void mesh_update(uint32_t handle, const mesh_instance_t* instance);
void mesh_destroy(uint32_t handle);
void mesh_set_visible(uint32_t handle, bool visible);

/* Picking */
uint32_t mesh_pick_entity(int screen_x, int screen_y, int width, int height, mat4 view_projection);

/* Hover / selection */
void mesh_set_hovered_entity(uint32_t entity_id);
uint32_t mesh_get_hovered_entity(void);
void mesh_set_selected_entity(uint32_t entity_id);
uint32_t mesh_get_selected_entity(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_MESH_H */
