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
#include <math.h>

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
** MARK: INLINE PICK SHADER SOURCE
***************************************************************/

#ifdef USE_GLES
static const char s_pick_vs_src[] =
    "#version 300 es\n"
    "precision highp float;\n"
    "layout (location = 0) in vec3 a_position;\n"
    "layout (location = 1) in vec3 a_normal;\n"
    "uniform mat4 u_mvp;\n"
    "void main() { gl_Position = u_mvp * vec4(a_position, 1.0); }\n";
static const char s_pick_fs_src[] =
    "#version 300 es\n"
    "precision highp float;\n"
    "uniform vec4 u_color;\n"
    "out vec4 frag_color;\n"
    "void main() { frag_color = u_color; }\n";
#else
static const char s_pick_vs_src[] =
    "#version 330 core\n"
    "layout (location = 0) in vec3 a_position;\n"
    "layout (location = 1) in vec3 a_normal;\n"
    "uniform mat4 u_mvp;\n"
    "void main() { gl_Position = u_mvp * vec4(a_position, 1.0); }\n";
static const char s_pick_fs_src[] =
    "#version 330 core\n"
    "uniform vec4 u_color;\n"
    "out vec4 frag_color;\n"
    "void main() { frag_color = u_color; }\n";
#endif

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef struct {
    bool active;
    bool visible;
    vertex_array_t fill_vao;
    buffer_t fill_vbo;
    buffer_t fill_ibo;
    uint32_t index_count;

    vertex_array_t edge_vao;
    buffer_t edge_vbo;
    uint32_t edge_segment_count; /* number of line segment instances */

    vec4 fill_color;
    vec4 edge_color;
    mat4 model_matrix;
} mesh_gpu_entry_t;

/***************************************************************
** MARK: QUAD GEOMETRY
***************************************************************/

static const float s_quad_vertices[] =
{
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f,  1.0f
};

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
static uniform_t s_edge_u_viewport = -1;
static uniform_t s_edge_u_line_width = -1;
static uniform_t s_edge_u_edge_color = -1;

static buffer_t s_edge_quad_buffer = 0;

static shader_t s_pick_shader = 0;
static uniform_t s_pick_u_mvp = -1;
static uniform_t s_pick_u_color = -1;

static float s_dpi_scale = 1.0f;

/* Hover / selection state (store entity IDs, 0x30000000 | handle) */
static uint32_t s_hovered_entity = 0xFFFFFFFF;
static uint32_t s_selected_entity = 0xFFFFFFFF;

/* Picking framebuffer */
static framebuffer_t s_pick_fbo = 0;
static texture_t s_pick_color_tex = 0;
static texture_t s_pick_depth_tex = 0;
static int s_pick_fbo_width = 0;
static int s_pick_fbo_height = 0;

static bool s_initialized = false;

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void mesh_set_dpi_scale(float scale)
{
    s_dpi_scale = (scale > 0.1f) ? scale : 1.0f;
}

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
    gpu_get_shader_uniform(s_edge_shader, "u_viewport", &s_edge_u_viewport);
    gpu_get_shader_uniform(s_edge_shader, "u_line_width", &s_edge_u_line_width);
    gpu_get_shader_uniform(s_edge_shader, "u_edge_color", &s_edge_u_edge_color);

    /* Create shared quad VBO for instanced edge rendering */
    s_edge_quad_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(s_edge_quad_buffer,
        (void*)s_quad_vertices, sizeof(s_quad_vertices), false);

    /* Compile flat-color pick shader (inline source) */
    if (!gpu_compile_shader(
        s_pick_vs_src, sizeof(s_pick_vs_src) - 1,
        s_pick_fs_src, sizeof(s_pick_fs_src) - 1,
        &s_pick_shader))
    {
        log_error("mesh_init: failed to compile pick shader");
        return false;
    }

    gpu_get_shader_uniform(s_pick_shader, "u_mvp", &s_pick_u_mvp);
    gpu_get_shader_uniform(s_pick_shader, "u_color", &s_pick_u_color);

    s_initialized = true;
    log_info("mesh_init: initialized successfully");
    return true;
}

void mesh_render(int width, int height, mat4 view_projection)
{
    if (!s_initialized) return;

    bool any_visible = false;
    for (uint32_t i = 0; i < s_entry_count; i++) {
        if (s_entries[i].active && s_entries[i].visible) { any_visible = true; break; }
    }
    if (!any_visible) return;

    gpu_set_viewport(width, height);
    gpu_set_depth_test(true);
    gpu_set_depth_write(true);
    gpu_set_blending(false);

    vec3 light_dir = {0.3f, 0.7f, 0.5f};
    glm_vec3_normalize(light_dir);

    /* Pass 1: Fill triangles (no polygon offset - correct depth in buffer) */
    gpu_use_shader(s_fill_shader);
    gpu_set_uniform_vec3(s_fill_u_light_dir, light_dir);

    for (uint32_t i = 0; i < s_entry_count; i++) {
        mesh_gpu_entry_t* entry = &s_entries[i];
        if (!entry->active || !entry->visible || entry->index_count == 0) continue;

        uint32_t entity_id = 0x30000000u | i;
        vec4 fill_color;
        glm_vec4_copy(entry->fill_color, fill_color);

        /* Apply selection highlighting (orange tint) */
        if (entity_id == s_selected_entity) {
            fill_color[0] = fminf(fill_color[0] + 0.3f, 1.0f);
            fill_color[1] = fminf(fill_color[1] + 0.15f, 1.0f);
            fill_color[2] = fmaxf(fill_color[2] - 0.2f, 0.0f);
        }
        /* Apply hover highlighting (cyan tint, overrides selection) */
        if (entity_id == s_hovered_entity) {
            fill_color[0] = fmaxf(fill_color[0] - 0.2f, 0.0f);
            fill_color[1] = fminf(fill_color[1] + 0.2f, 1.0f);
            fill_color[2] = fminf(fill_color[2] + 0.2f, 1.0f);
        }

        mat4 mvp;
        glm_mat4_mul(view_projection, entry->model_matrix, mvp);
        gpu_set_uniform_mat4(s_fill_u_mvp, mvp);
        gpu_set_uniform_mat4(s_fill_u_model, entry->model_matrix);
        gpu_set_uniform_vec4(s_fill_u_fill_color, fill_color);

        gpu_draw_elements(entry->fill_vao, entry->index_count);
    }

    /* Pass 2: Edge lines as instanced quads (pull edges slightly closer) */
    gpu_use_shader(s_edge_shader);
    gpu_set_blending(true);
    gpu_set_depth_write(false);
    gpu_set_polygon_offset(true, -1.0f, -1.0f);

    vec2 viewport = {(float)width, (float)height};
    gpu_set_uniform_vec2(s_edge_u_viewport, viewport);
    gpu_set_uniform_float(s_edge_u_line_width, s_dpi_scale);

    for (uint32_t i = 0; i < s_entry_count; i++) {
        mesh_gpu_entry_t* entry = &s_entries[i];
        if (!entry->active || !entry->visible || entry->edge_segment_count == 0) continue;

        uint32_t entity_id = 0x30000000u | i;
        vec4 edge_color;
        glm_vec4_copy(entry->edge_color, edge_color);

        /* Highlight edge color for selected (orange) */
        if (entity_id == s_selected_entity) {
            edge_color[0] = 1.0f;
            edge_color[1] = 0.6f;
            edge_color[2] = 0.0f;
            edge_color[3] = 1.0f;
        }
        /* Highlight edge color for hovered (cyan, overrides selection) */
        if (entity_id == s_hovered_entity) {
            edge_color[0] = 0.0f;
            edge_color[1] = 1.0f;
            edge_color[2] = 1.0f;
            edge_color[3] = 1.0f;
        }

        mat4 mvp;
        glm_mat4_mul(view_projection, entry->model_matrix, mvp);
        gpu_set_uniform_mat4(s_edge_u_mvp, mvp);
        gpu_set_uniform_vec4(s_edge_u_edge_color, edge_color);

        gpu_draw_instances(entry->edge_vao, 0, 4, entry->edge_segment_count);
    }

    gpu_set_polygon_offset(false, 0.0f, 0.0f);
    gpu_set_depth_write(true);
    gpu_set_blending(false);
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
    entry->visible = true;

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

    /* Upload edge segments as instanced quad data.
    ** edge_vertices stores pairs of 3-float positions: [start, end, start, end, ...]
    ** Each pair = one line segment instance (6 floats per instance).
    ** Instance attributes: a_start (loc 1) + a_end (loc 2). */
    entry->edge_vao = gpu_create_vertex_array();
    entry->edge_vbo = gpu_create_buffer();
    entry->edge_segment_count = instance->edge_vertex_count / 2;

    gpu_bind_vertex_array(entry->edge_vao);

    /* Bind shared quad VBO to location 0 (per-vertex, advance=0) */
    gpu_upload_array_buffer_data(s_edge_quad_buffer,
        (void*)s_quad_vertices, sizeof(s_quad_vertices), false);
    gpu_enable_vertex_attribute(0, 2, GPU_TYPE_FLOAT, false, 2 * sizeof(float), 0, 0);

    /* Bind instance buffer with edge segment data (per-instance, advance=1) */
    gpu_upload_array_buffer_data(entry->edge_vbo,
        instance->edge_vertices, instance->edge_vertex_count * 3 * sizeof(float), false);
    /* a_start: location 1, 3 floats, stride 24, offset 0 */
    gpu_enable_vertex_attribute(1, 3, GPU_TYPE_FLOAT, false, 6 * sizeof(float), 0, 1);
    /* a_end: location 2, 3 floats, stride 24, offset 12 */
    gpu_enable_vertex_attribute(2, 3, GPU_TYPE_FLOAT, false, 6 * sizeof(float), 3 * sizeof(float), 1);

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

void mesh_set_visible(uint32_t handle, bool visible)
{
    if (!s_initialized || handle >= s_entry_count || !s_entries[handle].active) return;
    s_entries[handle].visible = visible;
}

void mesh_destroy(uint32_t handle)
{
    if (!s_initialized || handle >= s_entry_count || !s_entries[handle].active) return;

    s_entries[handle].active = false;
    /* Note: GPU resources (VAOs, VBOs) are not deleted here for simplicity.
       In a production system these would be properly cleaned up. */
}

/***************************************************************
** MARK: PICKING
***************************************************************/

static void mesh_ensure_pick_fbo(int width, int height)
{
    if (s_pick_fbo_width == width && s_pick_fbo_height == height && s_pick_fbo != 0) {
        return;
    }

    s_pick_fbo = gpu_create_framebuffer();
    s_pick_color_tex = gpu_create_texture_2d(width, height, true, false);
    s_pick_depth_tex = gpu_create_texture_2d(width, height, false, true);

    gpu_bind_framebuffer(s_pick_fbo);
    gpu_framebuffer_attach_texture(s_pick_fbo, s_pick_color_tex, false);
    gpu_framebuffer_attach_texture(s_pick_fbo, s_pick_depth_tex, true);

    if (!gpu_check_framebuffer_complete(s_pick_fbo)) {
        log_error("mesh pick FBO not complete");
        s_pick_fbo = 0;
        return;
    }

    gpu_bind_framebuffer(0);
    s_pick_fbo_width = width;
    s_pick_fbo_height = height;
}

uint32_t mesh_pick_entity(int screen_x, int screen_y, int width, int height, mat4 view_projection)
{
    if (!s_initialized) return MESH_INVALID_HANDLE;

    bool any_visible = false;
    for (uint32_t i = 0; i < s_entry_count; i++) {
        if (s_entries[i].active && s_entries[i].visible) { any_visible = true; break; }
    }
    if (!any_visible) return MESH_INVALID_HANDLE;

    mesh_ensure_pick_fbo(width, height);
    if (s_pick_fbo == 0) return MESH_INVALID_HANDLE;

    /* Bind pick FBO and clear */
    gpu_bind_framebuffer(s_pick_fbo);
    gpu_set_viewport(width, height);

    vec4 clear = {0.0f, 0.0f, 0.0f, 0.0f};
    gpu_clear_color_buffer(clear);
    gpu_clear_depth_buffer();

    gpu_set_blending(false);
    gpu_set_depth_test(true);
    gpu_set_depth_write(true);

    /* Render body fills using the pick shader (flat color = entity ID) */
    gpu_use_shader(s_pick_shader);

    for (uint32_t i = 0; i < s_entry_count; i++) {
        mesh_gpu_entry_t* entry = &s_entries[i];
        if (!entry->active || !entry->visible || entry->index_count == 0) continue;

        uint32_t entity_id = 0x30000000u | i;

        /* Encode entity ID as RGBA color */
        vec4 id_color;
        id_color[0] = (float)((entity_id >>  0) & 0xFF) / 255.0f;
        id_color[1] = (float)((entity_id >>  8) & 0xFF) / 255.0f;
        id_color[2] = (float)((entity_id >> 16) & 0xFF) / 255.0f;
        id_color[3] = (float)((entity_id >> 24) & 0xFF) / 255.0f;

        mat4 mvp;
        glm_mat4_mul(view_projection, entry->model_matrix, mvp);
        gpu_set_uniform_mat4(s_pick_u_mvp, mvp);
        gpu_set_uniform_vec4(s_pick_u_color, id_color);

        gpu_draw_elements(entry->fill_vao, entry->index_count);
    }

    /* Read pixel at click position */
    int gl_y = height - screen_y - 1;
    uint8_t pixel[4] = {0, 0, 0, 0};
    uint32_t entity_id = 0;

    /* Sample center first, then 1px neighborhood */
    for (int radius = 0; radius <= 1 && entity_id == 0; radius++) {
        for (int dy = -radius; dy <= radius && entity_id == 0; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (radius > 0 && abs(dx) != radius && abs(dy) != radius) continue;

                int sx = screen_x + dx;
                int sy = gl_y + dy;
                if (sx < 0 || sx >= width || sy < 0 || sy >= height) continue;

                gpu_read_pixels(sx, sy, 1, 1, pixel);
                uint32_t sample = ((uint32_t)pixel[0] <<  0) |
                                  ((uint32_t)pixel[1] <<  8) |
                                  ((uint32_t)pixel[2] << 16) |
                                  ((uint32_t)pixel[3] << 24);
                if (sample != 0) {
                    entity_id = sample;
                    break;
                }
            }
        }
    }

    gpu_bind_framebuffer(0);
    return entity_id == 0 ? MESH_INVALID_HANDLE : entity_id;
}

/***************************************************************
** MARK: HOVER / SELECTION
***************************************************************/

void mesh_set_hovered_entity(uint32_t entity_id)
{
    s_hovered_entity = entity_id;
}

uint32_t mesh_get_hovered_entity(void)
{
    return s_hovered_entity;
}

void mesh_set_selected_entity(uint32_t entity_id)
{
    s_selected_entity = entity_id;
}

uint32_t mesh_get_selected_entity(void)
{
    return s_selected_entity;
}
