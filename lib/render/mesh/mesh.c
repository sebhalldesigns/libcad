/***************************************************************
**
** libcad Source File
**
** File         :  mesh.c
** Module       :  render/mesh
** Author       :  SH
** Created      :  2026-02-15 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad mesh rendering for solid bodies
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <string.h>

#include <util/log/log.h>
#include <render/gpu/gpu.h>

#include "mesh.h"

/***************************************************************
** MARK: SHADER RESOURCES
***************************************************************/

#ifdef USE_GLES
extern const unsigned char resources_shaders_mesh_body_fill_es_vs_glsl[];
extern const unsigned resources_shaders_mesh_body_fill_es_vs_glsl_size;
extern const unsigned char resources_shaders_mesh_body_fill_es_fs_glsl[];
extern const unsigned resources_shaders_mesh_body_fill_es_fs_glsl_size;
extern const unsigned char resources_shaders_mesh_body_edge_es_vs_glsl[];
extern const unsigned resources_shaders_mesh_body_edge_es_vs_glsl_size;
extern const unsigned char resources_shaders_mesh_body_edge_es_fs_glsl[];
extern const unsigned resources_shaders_mesh_body_edge_es_fs_glsl_size;
#else
extern const unsigned char resources_shaders_mesh_body_fill_core_vs_glsl[];
extern const unsigned resources_shaders_mesh_body_fill_core_vs_glsl_size;
extern const unsigned char resources_shaders_mesh_body_fill_core_fs_glsl[];
extern const unsigned resources_shaders_mesh_body_fill_core_fs_glsl_size;
extern const unsigned char resources_shaders_mesh_body_edge_core_vs_glsl[];
extern const unsigned resources_shaders_mesh_body_edge_core_vs_glsl_size;
extern const unsigned char resources_shaders_mesh_body_edge_core_fs_glsl[];
extern const unsigned resources_shaders_mesh_body_edge_core_fs_glsl_size;
#endif

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef struct {
    bool active;
    vertex_array_t fill_vao;
    buffer_t fill_vbo;
    buffer_t fill_ibo;
    uint32_t index_count;

    vertex_array_t edge_vao;
    buffer_t edge_vbo;
    uint32_t edge_vertex_count;

    vec4 fill_color;
    vec4 edge_color;
    mat4 model_matrix;
} mesh_gpu_entry_t;

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static mesh_gpu_entry_t s_entries[MESH_MAX_ENTRIES];
static uint32_t s_entry_count = 0;

static shader_t s_fill_shader = 0;
static uniform_t s_fill_u_mvp = -1;
static uniform_t s_fill_u_model = -1;
static uniform_t s_fill_u_fill_color = -1;
static uniform_t s_fill_u_light_dir = -1;

static shader_t s_edge_shader = 0;
static uniform_t s_edge_u_mvp = -1;
static uniform_t s_edge_u_edge_color = -1;

static bool s_initialized = false;

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

bool mesh_init(void)
{
    if (s_initialized) return true;

    memset(s_entries, 0, sizeof(s_entries));
    s_entry_count = 0;

    /* Compile fill shader */
#ifdef USE_GLES
    if (!gpu_compile_shader(
        (const char*)resources_shaders_mesh_body_fill_es_vs_glsl,
        resources_shaders_mesh_body_fill_es_vs_glsl_size,
        (const char*)resources_shaders_mesh_body_fill_es_fs_glsl,
        resources_shaders_mesh_body_fill_es_fs_glsl_size,
        &s_fill_shader))
#else
    if (!gpu_compile_shader(
        (const char*)resources_shaders_mesh_body_fill_core_vs_glsl,
        resources_shaders_mesh_body_fill_core_vs_glsl_size,
        (const char*)resources_shaders_mesh_body_fill_core_fs_glsl,
        resources_shaders_mesh_body_fill_core_fs_glsl_size,
        &s_fill_shader))
#endif
    {
        log_error("mesh_init: failed to compile fill shader");
        return false;
    }

    gpu_get_shader_uniform(s_fill_shader, "u_mvp", &s_fill_u_mvp);
    gpu_get_shader_uniform(s_fill_shader, "u_model", &s_fill_u_model);
    gpu_get_shader_uniform(s_fill_shader, "u_fill_color", &s_fill_u_fill_color);
    gpu_get_shader_uniform(s_fill_shader, "u_light_dir", &s_fill_u_light_dir);

    /* Compile edge shader */
#ifdef USE_GLES
    if (!gpu_compile_shader(
        (const char*)resources_shaders_mesh_body_edge_es_vs_glsl,
        resources_shaders_mesh_body_edge_es_vs_glsl_size,
        (const char*)resources_shaders_mesh_body_edge_es_fs_glsl,
        resources_shaders_mesh_body_edge_es_fs_glsl_size,
        &s_edge_shader))
#else
    if (!gpu_compile_shader(
        (const char*)resources_shaders_mesh_body_edge_core_vs_glsl,
        resources_shaders_mesh_body_edge_core_vs_glsl_size,
        (const char*)resources_shaders_mesh_body_edge_core_fs_glsl,
        resources_shaders_mesh_body_edge_core_fs_glsl_size,
        &s_edge_shader))
#endif
    {
        log_error("mesh_init: failed to compile edge shader");
        return false;
    }

    gpu_get_shader_uniform(s_edge_shader, "u_mvp", &s_edge_u_mvp);
    gpu_get_shader_uniform(s_edge_shader, "u_edge_color", &s_edge_u_edge_color);

    s_initialized = true;
    log_info("mesh_init: initialized successfully");
    return true;
}

void mesh_render(int width, int height, mat4 view_projection)
{
    if (!s_initialized) return;

    bool any_active = false;
    for (uint32_t i = 0; i < s_entry_count; i++) {
        if (s_entries[i].active) { any_active = true; break; }
    }
    if (!any_active) return;

    gpu_set_viewport(width, height);
    gpu_set_depth_test(true);
    gpu_set_depth_write(true);
    gpu_set_blending(false);

    vec3 light_dir = {0.3f, 0.7f, 0.5f};
    glm_vec3_normalize(light_dir);

    /* Pass 1: Fill triangles with polygon offset */
    gpu_use_shader(s_fill_shader);
    gpu_set_uniform_vec3(s_fill_u_light_dir, light_dir);
    gpu_set_polygon_offset(true, 1.0f, 1.0f);

    for (uint32_t i = 0; i < s_entry_count; i++) {
        mesh_gpu_entry_t* entry = &s_entries[i];
        if (!entry->active || entry->index_count == 0) continue;

        mat4 mvp;
        glm_mat4_mul(view_projection, entry->model_matrix, mvp);
        gpu_set_uniform_mat4(s_fill_u_mvp, mvp);
        gpu_set_uniform_mat4(s_fill_u_model, entry->model_matrix);
        gpu_set_uniform_vec4(s_fill_u_fill_color, entry->fill_color);

        gpu_draw_elements(entry->fill_vao, entry->index_count);
    }

    gpu_set_polygon_offset(false, 0.0f, 0.0f);

    /* Pass 2: Edge lines at original depth */
    gpu_use_shader(s_edge_shader);

    for (uint32_t i = 0; i < s_entry_count; i++) {
        mesh_gpu_entry_t* entry = &s_entries[i];
        if (!entry->active || entry->edge_vertex_count == 0) continue;

        mat4 mvp;
        glm_mat4_mul(view_projection, entry->model_matrix, mvp);
        gpu_set_uniform_mat4(s_edge_u_mvp, mvp);
        gpu_set_uniform_vec4(s_edge_u_edge_color, entry->edge_color);

        gpu_draw_lines(entry->edge_vao, 0, entry->edge_vertex_count);
    }
}

uint32_t mesh_create(const mesh_instance_t* instance)
{
    if (!s_initialized || !instance) return MESH_INVALID_HANDLE;

    /* Find a free slot */
    uint32_t handle = MESH_INVALID_HANDLE;
    for (uint32_t i = 0; i < s_entry_count; i++) {
        if (!s_entries[i].active) { handle = i; break; }
    }
    if (handle == MESH_INVALID_HANDLE) {
        if (s_entry_count >= MESH_MAX_ENTRIES) {
            log_error("mesh_create: max entries reached");
            return MESH_INVALID_HANDLE;
        }
        handle = s_entry_count++;
    }

    mesh_gpu_entry_t* entry = &s_entries[handle];
    memset(entry, 0, sizeof(*entry));
    entry->active = true;

    glm_vec4_copy(instance->fill_color, entry->fill_color);
    glm_vec4_copy(instance->edge_color, entry->edge_color);
    glm_mat4_copy(instance->model_matrix, entry->model_matrix);

    /* Upload fill mesh (interleaved pos+normal) */
    entry->fill_vao = gpu_create_vertex_array();
    entry->fill_vbo = gpu_create_buffer();
    entry->fill_ibo = gpu_create_buffer();
    entry->index_count = instance->index_count;

    gpu_bind_vertex_array(entry->fill_vao);
    gpu_upload_array_buffer_data(entry->fill_vbo,
        instance->vertices, instance->vertex_count * 6 * sizeof(float), false);

    /* position: location 0, 3 floats, stride 24, offset 0 */
    gpu_enable_vertex_attribute(0, 3, GPU_TYPE_FLOAT, false, 6 * sizeof(float), 0, 0);
    /* normal: location 1, 3 floats, stride 24, offset 12 */
    gpu_enable_vertex_attribute(1, 3, GPU_TYPE_FLOAT, false, 6 * sizeof(float), 3 * sizeof(float), 0);

    gpu_upload_index_buffer_data(entry->fill_ibo,
        instance->indices, instance->index_count * sizeof(uint32_t), false);

    /* Upload edge vertices (pos only) */
    entry->edge_vao = gpu_create_vertex_array();
    entry->edge_vbo = gpu_create_buffer();
    entry->edge_vertex_count = instance->edge_vertex_count;

    gpu_bind_vertex_array(entry->edge_vao);
    gpu_upload_array_buffer_data(entry->edge_vbo,
        instance->edge_vertices, instance->edge_vertex_count * 3 * sizeof(float), false);

    /* position: location 0, 3 floats, stride 12, offset 0 */
    gpu_enable_vertex_attribute(0, 3, GPU_TYPE_FLOAT, false, 3 * sizeof(float), 0, 0);

    gpu_bind_vertex_array(0);

    return handle;
}

void mesh_update(uint32_t handle, const mesh_instance_t* instance)
{
    if (!s_initialized || handle >= s_entry_count || !s_entries[handle].active || !instance) return;

    mesh_gpu_entry_t* entry = &s_entries[handle];
    glm_vec4_copy(instance->fill_color, entry->fill_color);
    glm_vec4_copy(instance->edge_color, entry->edge_color);
    glm_mat4_copy(instance->model_matrix, entry->model_matrix);
}

void mesh_destroy(uint32_t handle)
{
    if (!s_initialized || handle >= s_entry_count || !s_entries[handle].active) return;

    s_entries[handle].active = false;
    /* Note: GPU resources (VAOs, VBOs) are not deleted here for simplicity.
       In a production system these would be properly cleaned up. */
}
