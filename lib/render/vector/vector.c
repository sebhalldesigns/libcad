/***************************************************************
**
** libcad Source File
**
** File         :  vector.c
** Module       :  render/vector
** Author       :  SH
** Created      :  2026-02-06 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad vector API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <libcad/libcad.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#include <cglm/cglm.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <render/gpu/gpu.h>
#include <util/log/log.h>

#include "vector.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define INITIAL_ARENA_SIZE  (1024U)
#define INSTANCE_TYPE_OFFSET (32U)

#define DEBUG 1

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef struct
{
    void *data; /* raw data pointer */
    size_t used; /* number of bytes used of raw data */
    size_t capacity; /* allocated size of raw data */

    size_t instance_size; /* size of each instance in bytes */

    uint32_t *indices;
    uint32_t *reverse_indices;
    size_t max_handles; /* number of max_handles allocated */
} instance_arena_t;

/* Extended instance structures for picking (includes entity_id) */
typedef struct
{
    vector_line_instance_t base;
    float entity_id;
} vector_line_pick_instance_t;

typedef struct
{
    vector_shape_instance_t base;
    float entity_id;
} vector_shape_pick_instance_t;

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

#ifdef USE_GLES
extern uint8_t resources_shaders_vector_line_es_vs_glsl[];
extern uint32_t resources_shaders_vector_line_es_vs_glsl_size;

extern uint8_t resources_shaders_vector_line_es_fs_glsl[];
extern uint32_t resources_shaders_vector_line_es_fs_glsl_size;

extern uint8_t resources_shaders_vector_axis_es_vs_glsl[];
extern uint32_t resources_shaders_vector_axis_es_vs_glsl_size;

extern uint8_t resources_shaders_vector_axis_es_fs_glsl[];
extern uint32_t resources_shaders_vector_axis_es_fs_glsl_size;

extern uint8_t resources_shaders_vector_shape_es_vs_glsl[];
extern uint32_t resources_shaders_vector_shape_es_vs_glsl_size;

extern uint8_t resources_shaders_vector_shape_es_fs_glsl[];
extern uint32_t resources_shaders_vector_shape_es_fs_glsl_size;

extern uint8_t resources_shaders_vector_glyph_es_vs_glsl[];
extern uint32_t resources_shaders_vector_glyph_es_vs_glsl_size;

extern uint8_t resources_shaders_vector_glyph_es_fs_glsl[];
extern uint32_t resources_shaders_vector_glyph_es_fs_glsl_size;
#else
extern uint8_t resources_shaders_vector_line_core_vs_glsl[];
extern uint32_t resources_shaders_vector_line_core_vs_glsl_size;

extern uint8_t resources_shaders_vector_line_core_fs_glsl[];
extern uint32_t resources_shaders_vector_line_core_fs_glsl_size;

extern uint8_t resources_shaders_vector_axis_core_vs_glsl[];
extern uint32_t resources_shaders_vector_axis_core_vs_glsl_size;

extern uint8_t resources_shaders_vector_axis_core_fs_glsl[];
extern uint32_t resources_shaders_vector_axis_core_fs_glsl_size;

extern uint8_t resources_shaders_vector_shape_core_vs_glsl[];
extern uint32_t resources_shaders_vector_shape_core_vs_glsl_size;

extern uint8_t resources_shaders_vector_shape_core_fs_glsl[];
extern uint32_t resources_shaders_vector_shape_core_fs_glsl_size;

extern uint8_t resources_shaders_vector_glyph_core_vs_glsl[];
extern uint32_t resources_shaders_vector_glyph_core_vs_glsl_size;

extern uint8_t resources_shaders_vector_glyph_core_fs_glsl[];
extern uint32_t resources_shaders_vector_glyph_core_fs_glsl_size;
#endif

static const float quad_vertices[] =
{
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f,  1.0f
};

static instance_arena_t line_arena;
static instance_arena_t axis_arena;
static instance_arena_t shape_arena;
static instance_arena_t bezier_arena;
static instance_arena_t glyph_arena;

/* DPI scale for line thickness */
static float dpi_scale = 1.0f;

/* Hover state */
static uint32_t hovered_entity_id = VECTOR_INVALID_INSTANCE;
static uint32_t selected_entity_id = VECTOR_INVALID_INSTANCE;

/* LINE SHADER */
static shader_t line_shader;
static uniform_t line_shader_uniform_projection;
static uniform_t line_shader_uniform_viewport;
static vertex_array_t line_vertex_array;
static buffer_t line_quad_buffer;
static buffer_t line_instance_buffer;

/* AXIS OVERLAY SHADER */
static shader_t axis_shader;
static uniform_t axis_shader_uniform_projection;
static uniform_t axis_shader_uniform_viewport;
static vertex_array_t axis_vertex_array;
static buffer_t axis_quad_buffer;
static buffer_t axis_instance_buffer;

/* SHAPE SHADER */
static shader_t shape_shader;
static uniform_t shape_shader_uniform_projection;
static uniform_t shape_shader_uniform_viewport;
static vertex_array_t shape_vertex_array;
static buffer_t shape_quad_buffer;
static buffer_t shape_instance_buffer;

/* GLYPH SHADER */
static shader_t glyph_shader;
static uniform_t glyph_shader_uniform_projection;
static uniform_t glyph_shader_uniform_atlas;
static vertex_array_t glyph_vertex_array;
static buffer_t glyph_quad_buffer;
static buffer_t glyph_instance_buffer;
static texture_t glyph_atlas_texture;  /* TODO: texture_t typedef needed in gpu.h */

/* PICKING INFRASTRUCTURE */
static bool picking_initialized = false;

/* Picking shaders */
#ifdef USE_GLES
extern uint8_t resources_shaders_vector_line_pick_es_vs_glsl[];
extern uint32_t resources_shaders_vector_line_pick_es_vs_glsl_size;
extern uint8_t resources_shaders_vector_line_pick_es_fs_glsl[];
extern uint32_t resources_shaders_vector_line_pick_es_fs_glsl_size;

extern uint8_t resources_shaders_vector_shape_pick_es_vs_glsl[];
extern uint32_t resources_shaders_vector_shape_pick_es_vs_glsl_size;
extern uint8_t resources_shaders_vector_shape_pick_es_fs_glsl[];
extern uint32_t resources_shaders_vector_shape_pick_es_fs_glsl_size;
#else
extern uint8_t resources_shaders_vector_line_pick_core_vs_glsl[];
extern uint32_t resources_shaders_vector_line_pick_core_vs_glsl_size;
extern uint8_t resources_shaders_vector_line_pick_core_fs_glsl[];
extern uint32_t resources_shaders_vector_line_pick_core_fs_glsl_size;

extern uint8_t resources_shaders_vector_shape_pick_core_vs_glsl[];
extern uint32_t resources_shaders_vector_shape_pick_core_vs_glsl_size;
extern uint8_t resources_shaders_vector_shape_pick_core_fs_glsl[];
extern uint32_t resources_shaders_vector_shape_pick_core_fs_glsl_size;
#endif

static shader_t line_pick_shader;
static uniform_t line_pick_shader_uniform_projection;
static uniform_t line_pick_shader_uniform_viewport;
static vertex_array_t line_pick_vertex_array;
static buffer_t line_pick_quad_buffer;
static buffer_t line_pick_instance_buffer;

static shader_t shape_pick_shader;
static uniform_t shape_pick_shader_uniform_projection;
static uniform_t shape_pick_shader_uniform_viewport;
static vertex_array_t shape_pick_vertex_array;
static buffer_t shape_pick_quad_buffer;
static buffer_t shape_pick_instance_buffer;

/* Picking framebuffer */
static framebuffer_t pick_framebuffer;
static texture_t pick_texture;
static texture_t pick_depth_texture;
static int pick_fbo_width = 0;
static int pick_fbo_height = 0;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void instance_arena_init(instance_arena_t *arena, size_t instance_size, size_t initial_count);
static void instance_arena_destroy(instance_arena_t *arena);
static void instance_arena_clear(instance_arena_t *arena);
static uint32_t instance_arena_add(instance_arena_t *arena, const void *instance);
static void instance_arena_remove(instance_arena_t *arena, uint32_t handle);
static size_t instance_arena_count(instance_arena_t *arena);
static bool instance_arena_update(instance_arena_t *arena, uint32_t handle, const void *instance);
static void *instance_arena_get(instance_arena_t *arena, uint32_t handle);
static void *instance_arena_data(instance_arena_t *arena);

typedef struct
{
    float depth;
    uint32_t index;
} shape_sort_item_t;

static int compare_shape_depth_desc(const void *a, const void *b);
static void scale_line_stroke_widths(const vector_line_instance_t *src, vector_line_instance_t *dst, size_t count, float scale);
static void scale_shape_stroke_widths(const vector_shape_instance_t *src, vector_shape_instance_t *dst, size_t count, float scale);
static void apply_line_hover(vector_line_instance_t *instances, size_t count, uint32_t hover_id, uint32_t select_id, uint32_t type_bits);
static void apply_shape_hover(vector_shape_instance_t *instances, size_t count, uint32_t hover_id, uint32_t select_id);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

bool vector_init()
{

    /* create arenas */
    instance_arena_init(&line_arena, sizeof(vector_line_instance_t), INITIAL_ARENA_SIZE);
    instance_arena_init(&axis_arena, sizeof(vector_line_instance_t), INITIAL_ARENA_SIZE);
    instance_arena_init(&shape_arena, sizeof(vector_shape_instance_t), INITIAL_ARENA_SIZE);
    instance_arena_init(&glyph_arena, sizeof(vector_glyph_instance_t), INITIAL_ARENA_SIZE);

    /* LINE RENDERER SETUP */


    if (!gpu_compile_shader(
#if USE_GLES
        (const char*)resources_shaders_vector_line_es_vs_glsl,
        resources_shaders_vector_line_es_vs_glsl_size,
        (const char*)resources_shaders_vector_line_es_fs_glsl,
        resources_shaders_vector_line_es_fs_glsl_size,
#else
        (const char*)resources_shaders_vector_line_core_vs_glsl,
        resources_shaders_vector_line_core_vs_glsl_size,
        (const char*)resources_shaders_vector_line_core_fs_glsl,
        resources_shaders_vector_line_core_fs_glsl_size,
#endif
        &line_shader
    ))
    {
        #ifdef DEBUG
            log_error("Failed to compile line shader.");
        #endif
        return false;
    }

    log_info("compiled line shader");

    if (
        !gpu_get_shader_uniform(line_shader, "projection", &line_shader_uniform_projection)
    ||  !gpu_get_shader_uniform(line_shader, "viewport", &line_shader_uniform_viewport)
    )
    {
        #ifdef DEBUG
            log_error("Failed to get line shader uniform.");
        #endif
        return false;
    }

    line_vertex_array = gpu_create_vertex_array();
    gpu_bind_vertex_array(line_vertex_array);

    /* set up quad buffer (per-vertex data) */
    line_quad_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(line_quad_buffer, quad_vertices, sizeof(quad_vertices), false);
    gpu_enable_vertex_attribute(0, 2, GPU_TYPE_FLOAT, false, 2 * sizeof(float), 0, 0);

    /* create and bind instance buffer (per-instance data) */
    line_instance_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(line_instance_buffer, NULL, 0, true); /* bind the buffer */

    /* set up per-instance vertex attributes */
    gpu_enable_vertex_attribute(1, 3, GPU_TYPE_FLOAT, false, sizeof(vector_line_instance_t), offsetof(vector_line_instance_t, start), 1);
    gpu_enable_vertex_attribute(2, 3, GPU_TYPE_FLOAT, false, sizeof(vector_line_instance_t), offsetof(vector_line_instance_t, end), 1);
    gpu_enable_vertex_attribute(3, 4, GPU_TYPE_FLOAT, false, sizeof(vector_line_instance_t), offsetof(vector_line_instance_t, color), 1);
    gpu_enable_vertex_attribute(4, 1, GPU_TYPE_FLOAT, false, sizeof(vector_line_instance_t), offsetof(vector_line_instance_t, stroke_width), 1);
    gpu_enable_vertex_attribute(5, 1, GPU_TYPE_FLOAT, false, sizeof(vector_line_instance_t), offsetof(vector_line_instance_t, dash), 1);

    gpu_bind_vertex_array(0);

    /* AXIS OVERLAY RENDERER SETUP */

    if (!gpu_compile_shader(
#if USE_GLES
        (const char*)resources_shaders_vector_axis_es_vs_glsl,
        resources_shaders_vector_axis_es_vs_glsl_size,
        (const char*)resources_shaders_vector_axis_es_fs_glsl,
        resources_shaders_vector_axis_es_fs_glsl_size,
#else
        (const char*)resources_shaders_vector_axis_core_vs_glsl,
        resources_shaders_vector_axis_core_vs_glsl_size,
        (const char*)resources_shaders_vector_axis_core_fs_glsl,
        resources_shaders_vector_axis_core_fs_glsl_size,
#endif
        &axis_shader
    ))
    {
        #ifdef DEBUG
            log_error("Failed to compile axis shader.");
        #endif
        return false;
    }

    if (
        !gpu_get_shader_uniform(axis_shader, "projection", &axis_shader_uniform_projection)
    ||  !gpu_get_shader_uniform(axis_shader, "viewport", &axis_shader_uniform_viewport)
    )
    {
        #ifdef DEBUG
            log_error("Failed to get axis shader uniform.");
        #endif
        return false;
    }

    axis_vertex_array = gpu_create_vertex_array();
    gpu_bind_vertex_array(axis_vertex_array);

    axis_quad_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(axis_quad_buffer, quad_vertices, sizeof(quad_vertices), false);
    gpu_enable_vertex_attribute(0, 2, GPU_TYPE_FLOAT, false, 2 * sizeof(float), 0, 0);

    axis_instance_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(axis_instance_buffer, NULL, 0, true);

    gpu_enable_vertex_attribute(1, 3, GPU_TYPE_FLOAT, false, sizeof(vector_line_instance_t), offsetof(vector_line_instance_t, start), 1);
    gpu_enable_vertex_attribute(2, 3, GPU_TYPE_FLOAT, false, sizeof(vector_line_instance_t), offsetof(vector_line_instance_t, end), 1);
    gpu_enable_vertex_attribute(3, 4, GPU_TYPE_FLOAT, false, sizeof(vector_line_instance_t), offsetof(vector_line_instance_t, color), 1);
    gpu_enable_vertex_attribute(4, 1, GPU_TYPE_FLOAT, false, sizeof(vector_line_instance_t), offsetof(vector_line_instance_t, stroke_width), 1);
    gpu_enable_vertex_attribute(5, 1, GPU_TYPE_FLOAT, false, sizeof(vector_line_instance_t), offsetof(vector_line_instance_t, dash), 1);

    gpu_bind_vertex_array(0);

    /* SHAPE RENDERER SETUP */

    if (!gpu_compile_shader(
#if USE_GLES
        (const char*)resources_shaders_vector_shape_es_vs_glsl,
        resources_shaders_vector_shape_es_vs_glsl_size,
        (const char*)resources_shaders_vector_shape_es_fs_glsl,
        resources_shaders_vector_shape_es_fs_glsl_size,
#else
        (const char*)resources_shaders_vector_shape_core_vs_glsl,
        resources_shaders_vector_shape_core_vs_glsl_size,
        (const char*)resources_shaders_vector_shape_core_fs_glsl,
        resources_shaders_vector_shape_core_fs_glsl_size,
#endif
        &shape_shader
    ))
    {
        #ifdef DEBUG
            log_error("Failed to compile shape shader.");
        #endif
        return false;
    }

    log_info("compiled shape shader (shader id: %u)", shape_shader);

    if (
        !gpu_get_shader_uniform(shape_shader, "projection", &shape_shader_uniform_projection)
    ||  !gpu_get_shader_uniform(shape_shader, "viewport", &shape_shader_uniform_viewport)
    )
    {
        #ifdef DEBUG
            log_error("Failed to get shape shader uniform.");
        #endif
        return false;
    }

    shape_vertex_array = gpu_create_vertex_array();
    gpu_bind_vertex_array(shape_vertex_array);

    /* set up quad buffer (per-vertex data) */
    shape_quad_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(shape_quad_buffer, quad_vertices, sizeof(quad_vertices), false);
    gpu_enable_vertex_attribute(0, 2, GPU_TYPE_FLOAT, false, 2 * sizeof(float), 0, 0);

    /* create and bind instance buffer (per-instance data) */
    shape_instance_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(shape_instance_buffer, NULL, 0, true);

    /* set up per-instance vertex attributes for shapes */
    gpu_enable_vertex_attribute(1, 3, GPU_TYPE_FLOAT, false, sizeof(vector_shape_instance_t), offsetof(vector_shape_instance_t, center), 1);
    gpu_enable_vertex_attribute(2, 3, GPU_TYPE_FLOAT, false, sizeof(vector_shape_instance_t), offsetof(vector_shape_instance_t, normal), 1);
    gpu_enable_vertex_attribute(3, 2, GPU_TYPE_FLOAT, false, sizeof(vector_shape_instance_t), offsetof(vector_shape_instance_t, size), 1);
    gpu_enable_vertex_attribute(4, 4, GPU_TYPE_FLOAT, false, sizeof(vector_shape_instance_t), offsetof(vector_shape_instance_t, color), 1);
    gpu_enable_vertex_attribute(5, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_instance_t), offsetof(vector_shape_instance_t, rotation), 1);
    gpu_enable_vertex_attribute(6, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_instance_t), offsetof(vector_shape_instance_t, sides), 1);
    gpu_enable_vertex_attribute(7, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_instance_t), offsetof(vector_shape_instance_t, start_angle), 1);
    gpu_enable_vertex_attribute(8, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_instance_t), offsetof(vector_shape_instance_t, end_angle), 1);
    gpu_enable_vertex_attribute(9, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_instance_t), offsetof(vector_shape_instance_t, fill), 1);
    gpu_enable_vertex_attribute(10, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_instance_t), offsetof(vector_shape_instance_t, stroke_width), 1);
    gpu_enable_vertex_attribute(11, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_instance_t), offsetof(vector_shape_instance_t, corner_radius), 1);
    gpu_enable_vertex_attribute(12, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_instance_t), offsetof(vector_shape_instance_t, dash), 1);

    gpu_bind_vertex_array(0);

    /* GLYPH RENDERER SETUP */

    if (!gpu_compile_shader(
#if USE_GLES
        (const char*)resources_shaders_vector_glyph_es_vs_glsl,
        resources_shaders_vector_glyph_es_vs_glsl_size,
        (const char*)resources_shaders_vector_glyph_es_fs_glsl,
        resources_shaders_vector_glyph_es_fs_glsl_size,
#else
        (const char*)resources_shaders_vector_glyph_core_vs_glsl,
        resources_shaders_vector_glyph_core_vs_glsl_size,
        (const char*)resources_shaders_vector_glyph_core_fs_glsl,
        resources_shaders_vector_glyph_core_fs_glsl_size,
#endif
        &glyph_shader
    ))
    {
        #ifdef DEBUG
            log_error("Failed to compile glyph shader.");
        #endif
        return false;
    }

    log_info("compiled glyph shader (shader id: %u)", glyph_shader);

    if (
        !gpu_get_shader_uniform(glyph_shader, "projection", &glyph_shader_uniform_projection)
    ||  !gpu_get_shader_uniform(glyph_shader, "atlas_texture", &glyph_shader_uniform_atlas)
    )
    {
        #ifdef DEBUG
            log_error("Failed to get glyph shader uniform.");
        #endif
        return false;
    }

    glyph_vertex_array = gpu_create_vertex_array();
    gpu_bind_vertex_array(glyph_vertex_array);

    /* set up quad buffer (per-vertex data) */
    glyph_quad_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(glyph_quad_buffer, quad_vertices, sizeof(quad_vertices), false);
    gpu_enable_vertex_attribute(0, 2, GPU_TYPE_FLOAT, false, 2 * sizeof(float), 0, 0);

    /* create and bind instance buffer (per-instance data) */
    glyph_instance_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(glyph_instance_buffer, NULL, 0, true);

    /* set up per-instance vertex attributes for glyphs */
    gpu_enable_vertex_attribute(1, 2, GPU_TYPE_FLOAT, false, sizeof(vector_glyph_instance_t), offsetof(vector_glyph_instance_t, position), 1);
    gpu_enable_vertex_attribute(2, 2, GPU_TYPE_FLOAT, false, sizeof(vector_glyph_instance_t), offsetof(vector_glyph_instance_t, size), 1);
    gpu_enable_vertex_attribute(3, 2, GPU_TYPE_FLOAT, false, sizeof(vector_glyph_instance_t), offsetof(vector_glyph_instance_t, uv_min), 1);
    gpu_enable_vertex_attribute(4, 2, GPU_TYPE_FLOAT, false, sizeof(vector_glyph_instance_t), offsetof(vector_glyph_instance_t, uv_max), 1);
    gpu_enable_vertex_attribute(5, 4, GPU_TYPE_FLOAT, false, sizeof(vector_glyph_instance_t), offsetof(vector_glyph_instance_t, color), 1);

    gpu_bind_vertex_array(0);

    log_info("buffer creation complete");

    return true;
}

void vector_set_dpi_scale(float scale)
{
    dpi_scale = fmaxf(scale, 0.1f); /* Clamp to minimum 0.1 */
    //printf("vector_set_dpi_scale: scale=%.2f, dpi_scale=%.2f\n", scale, dpi_scale);
}

void vector_set_hovered_entity(uint32_t entity_id)
{
    hovered_entity_id = entity_id;
}

uint32_t vector_get_hovered_entity(void)
{
    return hovered_entity_id;
}

void vector_set_selected_entity(uint32_t entity_id)
{
    selected_entity_id = entity_id;
}

uint32_t vector_get_selected_entity(void)
{
    return selected_entity_id;
}

/***************************************************************
** MARK: PICKING INITIALIZATION
***************************************************************/

static bool vector_init_picking(void)
{
    if (picking_initialized) {
        return true;
    }

    /* Compile picking shaders */
    if (!gpu_compile_shader(
#ifdef USE_GLES
        (const char*)resources_shaders_vector_line_pick_es_vs_glsl,
        resources_shaders_vector_line_pick_es_vs_glsl_size,
        (const char*)resources_shaders_vector_line_pick_es_fs_glsl,
        resources_shaders_vector_line_pick_es_fs_glsl_size,
#else
        (const char*)resources_shaders_vector_line_pick_core_vs_glsl,
        resources_shaders_vector_line_pick_core_vs_glsl_size,
        (const char*)resources_shaders_vector_line_pick_core_fs_glsl,
        resources_shaders_vector_line_pick_core_fs_glsl_size,
#endif
        &line_pick_shader))
    {
        log_error("Failed to compile line picking shader");
        return false;
    }

    if (!gpu_compile_shader(
#ifdef USE_GLES
        (const char*)resources_shaders_vector_shape_pick_es_vs_glsl,
        resources_shaders_vector_shape_pick_es_vs_glsl_size,
        (const char*)resources_shaders_vector_shape_pick_es_fs_glsl,
        resources_shaders_vector_shape_pick_es_fs_glsl_size,
#else
        (const char*)resources_shaders_vector_shape_pick_core_vs_glsl,
        resources_shaders_vector_shape_pick_core_vs_glsl_size,
        (const char*)resources_shaders_vector_shape_pick_core_fs_glsl,
        resources_shaders_vector_shape_pick_core_fs_glsl_size,
#endif
        &shape_pick_shader))
    {
        log_error("Failed to compile shape picking shader");
        return false;
    }

    /* Get uniforms */
    gpu_get_shader_uniform(line_pick_shader, "projection", &line_pick_shader_uniform_projection);
    gpu_get_shader_uniform(line_pick_shader, "viewport", &line_pick_shader_uniform_viewport);
    gpu_get_shader_uniform(shape_pick_shader, "projection", &shape_pick_shader_uniform_projection);
    gpu_get_shader_uniform(shape_pick_shader, "viewport", &shape_pick_shader_uniform_viewport);

    /* Set up line picking vertex array */
    line_pick_vertex_array = gpu_create_vertex_array();
    gpu_bind_vertex_array(line_pick_vertex_array);

    line_pick_quad_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(line_pick_quad_buffer, quad_vertices, sizeof(quad_vertices), false);
    gpu_enable_vertex_attribute(0, 2, GPU_TYPE_FLOAT, false, 2 * sizeof(float), 0, 0);

    line_pick_instance_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(line_pick_instance_buffer, NULL, 0, true);

    /* Line pick instance attributes */
    gpu_enable_vertex_attribute(1, 3, GPU_TYPE_FLOAT, false, sizeof(vector_line_pick_instance_t), offsetof(vector_line_pick_instance_t, base.start), 1);
    gpu_enable_vertex_attribute(2, 3, GPU_TYPE_FLOAT, false, sizeof(vector_line_pick_instance_t), offsetof(vector_line_pick_instance_t, base.end), 1);
    gpu_enable_vertex_attribute(3, 4, GPU_TYPE_FLOAT, false, sizeof(vector_line_pick_instance_t), offsetof(vector_line_pick_instance_t, base.color), 1);
    gpu_enable_vertex_attribute(4, 1, GPU_TYPE_FLOAT, false, sizeof(vector_line_pick_instance_t), offsetof(vector_line_pick_instance_t, base.stroke_width), 1);
    gpu_enable_vertex_attribute(5, 1, GPU_TYPE_FLOAT, false, sizeof(vector_line_pick_instance_t), offsetof(vector_line_pick_instance_t, base.dash), 1);
    gpu_enable_vertex_attribute(6, 1, GPU_TYPE_FLOAT, false, sizeof(vector_line_pick_instance_t), offsetof(vector_line_pick_instance_t, entity_id), 1);

    gpu_bind_vertex_array(0);

    /* Set up shape picking vertex array */
    shape_pick_vertex_array = gpu_create_vertex_array();
    gpu_bind_vertex_array(shape_pick_vertex_array);

    shape_pick_quad_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(shape_pick_quad_buffer, quad_vertices, sizeof(quad_vertices), false);
    gpu_enable_vertex_attribute(0, 2, GPU_TYPE_FLOAT, false, 2 * sizeof(float), 0, 0);

    shape_pick_instance_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(shape_pick_instance_buffer, NULL, 0, true);

    /* Shape pick instance attributes */
    gpu_enable_vertex_attribute(1, 3, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, base.center), 1);
    gpu_enable_vertex_attribute(2, 3, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, base.normal), 1);
    gpu_enable_vertex_attribute(3, 2, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, base.size), 1);
    gpu_enable_vertex_attribute(4, 4, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, base.color), 1);
    gpu_enable_vertex_attribute(5, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, base.rotation), 1);
    gpu_enable_vertex_attribute(6, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, base.sides), 1);
    gpu_enable_vertex_attribute(7, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, base.start_angle), 1);
    gpu_enable_vertex_attribute(8, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, base.end_angle), 1);
    gpu_enable_vertex_attribute(9, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, base.fill), 1);
    gpu_enable_vertex_attribute(10, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, base.stroke_width), 1);
    gpu_enable_vertex_attribute(11, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, base.corner_radius), 1);
    gpu_enable_vertex_attribute(12, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, base.dash), 1);
    gpu_enable_vertex_attribute(13, 1, GPU_TYPE_FLOAT, false, sizeof(vector_shape_pick_instance_t), offsetof(vector_shape_pick_instance_t, entity_id), 1);

    gpu_bind_vertex_array(0);

    picking_initialized = true;
    log_info("Picking infrastructure initialized");
    return true;
}

static void vector_ensure_pick_framebuffer(int width, int height)
{
    /* Resize framebuffer if needed */
    if (pick_fbo_width != width || pick_fbo_height != height) {
        if (pick_framebuffer != 0) {
            /* TODO: Add cleanup functions to GPU API */
        }

        pick_framebuffer = gpu_create_framebuffer();
        pick_texture = gpu_create_texture_2d(width, height, true, false);
        pick_depth_texture = gpu_create_texture_2d(width, height, false, true);

        gpu_bind_framebuffer(pick_framebuffer);
        gpu_framebuffer_attach_texture(pick_framebuffer, pick_texture, false);
        gpu_framebuffer_attach_texture(pick_framebuffer, pick_depth_texture, true);

        if (!gpu_check_framebuffer_complete(pick_framebuffer)) {
            printf("ERROR: Picking framebuffer not complete!\n");
            log_error("Picking framebuffer not complete");
            pick_framebuffer = 0;
            return;
        }

        gpu_bind_framebuffer(0);
        pick_fbo_width = width;
        pick_fbo_height = height;

        printf("Created picking framebuffer: %dx%d\n", width, height);
        log_info("Created picking framebuffer: %dx%d", width, height);
    }
}

/***************************************************************
** MARK: PICKING IMPLEMENTATION
***************************************************************/

uint32_t vector_pick_entity(int screen_x, int screen_y, int width, int height, mat4 projection)
{
    /* Lazy initialize picking */
    if (!picking_initialized) {
        if (!vector_init_picking()) {
            printf("Failed to initialize picking\n");
            return VECTOR_INVALID_INSTANCE;
        }
        printf("Picking initialized successfully\n");
    }

    /* Ensure framebuffer is ready */
    vector_ensure_pick_framebuffer(width, height);
    if (pick_framebuffer == 0) {
        printf("Picking framebuffer not ready\n");
        return VECTOR_INVALID_INSTANCE;
    }

    /* Bind picking framebuffer */
    gpu_bind_framebuffer(pick_framebuffer);
    gpu_set_viewport(width, height);

    /* Clear to background (ID 0) */
    vec4 clear_color = {0.0f, 0.0f, 0.0f, 0.0f};
    gpu_clear_color_buffer(clear_color);
    gpu_clear_depth_buffer();

    gpu_set_blending(false);
    gpu_set_depth_test(true);
    gpu_set_depth_write(true);

    vec2 viewport = {(float)width, (float)height};

    size_t shape_count = instance_arena_count(&shape_arena);
    size_t line_count = instance_arena_count(&line_arena);
    size_t axis_count = instance_arena_count(&axis_arena);
    printf("Pick pass: shapes=%zu, lines=%zu, axes=%zu\n", shape_count, line_count, axis_count);

    /* Pass 1: render lines/axes only (explicit priority over shapes) */
    if (line_count > 0) {
        vector_line_instance_t *line_data = (vector_line_instance_t *)instance_arena_data(&line_arena);
        vector_line_pick_instance_t *pick_lines = malloc(line_count * sizeof(vector_line_pick_instance_t));

        if (pick_lines) {
            for (size_t i = 0; i < line_count; i++) {
                pick_lines[i].base = line_data[i];
                /* Entity ID encoding: type (0x0) in upper 4 bits, instance index in lower 28 bits */
                union { uint32_t u; float f; } id_converter;
                id_converter.u = 0x00000000 | (uint32_t)i;
                pick_lines[i].entity_id = id_converter.f;
            }

            gpu_use_shader(line_pick_shader);
            gpu_set_uniform_mat4(line_pick_shader_uniform_projection, projection);
            gpu_set_uniform_vec2(line_pick_shader_uniform_viewport, viewport);

            gpu_bind_vertex_array(line_pick_vertex_array);
            gpu_upload_array_buffer_data(line_pick_instance_buffer,
                                         pick_lines,
                                         line_count * sizeof(vector_line_pick_instance_t),
                                         true);
            gpu_draw_instances(line_pick_vertex_array, 0, 4, line_count);
            gpu_bind_vertex_array(0);

            free(pick_lines);
        }
    }

    if (axis_count > 0) {
        vector_line_instance_t *axis_data = (vector_line_instance_t *)instance_arena_data(&axis_arena);
        vector_line_pick_instance_t *pick_axes = malloc(axis_count * sizeof(vector_line_pick_instance_t));

        if (pick_axes) {
            for (size_t i = 0; i < axis_count; i++) {
                pick_axes[i].base = axis_data[i];
                /* Entity ID encoding: type (0x1) in upper 4 bits, instance index in lower 28 bits */
                union { uint32_t u; float f; } id_converter;
                id_converter.u = 0x10000000 | (uint32_t)i;
                pick_axes[i].entity_id = id_converter.f;
            }

            gpu_use_shader(line_pick_shader);
            gpu_set_uniform_mat4(line_pick_shader_uniform_projection, projection);
            gpu_set_uniform_vec2(line_pick_shader_uniform_viewport, viewport);

            gpu_bind_vertex_array(line_pick_vertex_array);
            gpu_upload_array_buffer_data(line_pick_instance_buffer,
                                         pick_axes,
                                         axis_count * sizeof(vector_line_pick_instance_t),
                                         true);
            gpu_draw_instances(line_pick_vertex_array, 0, 4, axis_count);
            gpu_bind_vertex_array(0);

            free(pick_axes);
        }
    }

    /* Read priority pixel (lines/axes) before rendering any shapes */
    int gl_y = height - screen_y - 1;
    uint8_t pixel[4] = {0, 0, 0, 0};
    uint32_t priority_entity_id = 0;
    int priority_sample_x = screen_x;
    int priority_sample_y = gl_y;

    /* Sample center first, then 1px neighborhood to avoid one-sided misses */
    for (int radius = 0; radius <= 1 && priority_entity_id == 0; radius++) {
        for (int dy = -radius; dy <= radius && priority_entity_id == 0; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (radius > 0 && abs(dx) != radius && abs(dy) != radius) {
                    continue;
                }

                int sample_x = screen_x + dx;
                int sample_y = gl_y + dy;
                if (sample_x < 0 || sample_x >= width || sample_y < 0 || sample_y >= height) {
                    continue;
                }

                gpu_read_pixels(sample_x, sample_y, 1, 1, pixel);
                uint32_t sample_id = ((uint32_t)pixel[0] << 0) |
                                     ((uint32_t)pixel[1] << 8) |
                                     ((uint32_t)pixel[2] << 16) |
                                     ((uint32_t)pixel[3] << 24);
                if (sample_id != 0) {
                    priority_entity_id = sample_id;
                    priority_sample_x = sample_x;
                    priority_sample_y = sample_y;
                    break;
                }
            }
        }
    }

    if (priority_entity_id != 0) {
        gpu_bind_framebuffer(0);
        printf("Picked priority line/axis (%d,%d): RGBA=(%u,%u,%u,%u) -> ID=0x%08X\n",
               priority_sample_x, height - priority_sample_y - 1,
               pixel[0], pixel[1], pixel[2], pixel[3], priority_entity_id);
        return priority_entity_id;
    }

    /* Pass 2: clear and render shapes only */
    gpu_clear_color_buffer(clear_color);
    gpu_clear_depth_buffer();

    if (shape_count > 0) {
        vector_shape_instance_t *shape_data = (vector_shape_instance_t *)instance_arena_data(&shape_arena);
        vector_shape_pick_instance_t *pick_shapes = malloc(shape_count * sizeof(vector_shape_pick_instance_t));

        if (pick_shapes) {
            /* Build picking instances with entity IDs */
            for (size_t i = 0; i < shape_count; i++) {
                pick_shapes[i].base = shape_data[i];
                /* Entity ID encoding: type (0x2) in upper 4 bits, instance index in lower 28 bits */
                /* Use union to preserve bit pattern when converting uint to float */
                union { uint32_t u; float f; } id_converter;
                id_converter.u = 0x20000000 | (uint32_t)i;
                pick_shapes[i].entity_id = id_converter.f;
            }

            union { uint32_t u; float f; } first_id;
            first_id.f = pick_shapes[0].entity_id;
            printf("Rendering %zu shapes for picking, first ID=0x%08X\n",
                   shape_count, first_id.u);

            gpu_use_shader(shape_pick_shader);
            gpu_set_uniform_mat4(shape_pick_shader_uniform_projection, projection);
            gpu_set_uniform_vec2(shape_pick_shader_uniform_viewport, viewport);

            gpu_bind_vertex_array(shape_pick_vertex_array);
            gpu_upload_array_buffer_data(shape_pick_instance_buffer,
                                         pick_shapes,
                                         shape_count * sizeof(vector_shape_pick_instance_t),
                                         true);
            gpu_draw_instances(shape_pick_vertex_array, 0, 4, shape_count);
            gpu_bind_vertex_array(0);

            free(pick_shapes);
        }
    }

    /* Debug: Read center pixel to check if anything rendered in shape pass */
    uint8_t center_pixel[4] = {0, 0, 0, 0};
    gpu_read_pixels(width/2, height/2, 1, 1, center_pixel);
    uint32_t center_id = ((uint32_t)center_pixel[0] << 0) |
                         ((uint32_t)center_pixel[1] << 8) |
                         ((uint32_t)center_pixel[2] << 16) |
                         ((uint32_t)center_pixel[3] << 24);
    if (center_id != 0) {
        printf("Center pixel (%d,%d) has ID=0x%08X\n", width/2, height/2, center_id);
    }

    /* Read pixel at click location from shapes pass */
    gpu_read_pixels(screen_x, gl_y, 1, 1, pixel);

    /* Decode entity ID from pixel color */
    uint32_t entity_id = ((uint32_t)pixel[0] << 0) |
                         ((uint32_t)pixel[1] << 8) |
                         ((uint32_t)pixel[2] << 16) |
                         ((uint32_t)pixel[3] << 24);

    /* Unbind picking framebuffer */
    gpu_bind_framebuffer(0);

    if (entity_id != 0) {
        printf("Picked pixel (%d,%d): RGBA=(%u,%u,%u,%u) -> ID=0x%08X\n",
               screen_x, screen_y, pixel[0], pixel[1], pixel[2], pixel[3], entity_id);
    }

    return entity_id;
}

void vector_render(int width, int height, mat4 projection)
{
    static int frame_count = 0;
    if (frame_count % 60 == 0) {
        //printf("vector_render: frame %d, dpi_scale=%.2f\n", frame_count, dpi_scale);
    }
    frame_count++;

    gpu_set_blending(true);
    gpu_set_viewport(width, height);

    /* enable depth testing for 3D */
    gpu_set_depth_test(true);
    gpu_set_depth_write(true);

    /* render shapes back-to-front for stable transparency */
    size_t shape_count = instance_arena_count(&shape_arena);
    if (shape_count > 0)
    {
        vector_shape_instance_t *shape_data = (vector_shape_instance_t *)instance_arena_data(&shape_arena);
        shape_sort_item_t *sort_items = (shape_sort_item_t *)malloc(shape_count * sizeof(shape_sort_item_t));
        vector_shape_instance_t *sorted_shapes = (vector_shape_instance_t *)malloc(shape_count * sizeof(vector_shape_instance_t));
        vector_shape_instance_t *scaled_shapes = (vector_shape_instance_t *)malloc(shape_count * sizeof(vector_shape_instance_t));

        if (shape_data && sort_items && sorted_shapes && scaled_shapes)
        {
            for (size_t i = 0; i < shape_count; i++)
            {
                vec4 clip;
                glm_mat4_mulv(projection,
                              (vec4){shape_data[i].center[0], shape_data[i].center[1], shape_data[i].center[2], 1.0f},
                              clip);
                float depth = (fabsf(clip[3]) > 1e-6f) ? (clip[2] / clip[3]) : 1.0f;
                sort_items[i].depth = depth;
                sort_items[i].index = (uint32_t)i;
            }

            qsort(sort_items, shape_count, sizeof(shape_sort_item_t), compare_shape_depth_desc);

            for (size_t i = 0; i < shape_count; i++)
            {
                sorted_shapes[i] = shape_data[sort_items[i].index];
            }

            /* Scale stroke widths by DPI */
            scale_shape_stroke_widths(sorted_shapes, scaled_shapes, shape_count, dpi_scale);

            /* Apply hover and selection highlighting */
            apply_shape_hover(scaled_shapes, shape_count, hovered_entity_id, selected_entity_id);

            gpu_use_shader(shape_shader);
            gpu_set_uniform_mat4(shape_shader_uniform_projection, projection);

            vec2 viewport;
            viewport[0] = (float)width;
            viewport[1] = (float)height;
            gpu_set_uniform_vec2(shape_shader_uniform_viewport, viewport);

            gpu_set_depth_test(true);
            gpu_set_depth_write(false);

            gpu_bind_vertex_array(shape_vertex_array);
            gpu_upload_array_buffer_data(shape_instance_buffer,
                                         scaled_shapes,
                                         shape_count * sizeof(vector_shape_instance_t),
                                         true);
            gpu_draw_instances(shape_vertex_array, 0, 4, shape_count);
            gpu_bind_vertex_array(0);

            gpu_set_depth_write(true);
        }
        else
        {
            /* Fallback to unsorted draw if allocation fails */
            vector_shape_instance_t *scaled_shapes_fallback = (vector_shape_instance_t *)malloc(shape_count * sizeof(vector_shape_instance_t));

            gpu_use_shader(shape_shader);
            gpu_set_uniform_mat4(shape_shader_uniform_projection, projection);

            vec2 viewport;
            viewport[0] = (float)width;
            viewport[1] = (float)height;
            gpu_set_uniform_vec2(shape_shader_uniform_viewport, viewport);

            gpu_set_depth_test(true);
            gpu_set_depth_write(false);

            gpu_bind_vertex_array(shape_vertex_array);

            if (scaled_shapes_fallback)
            {
                scale_shape_stroke_widths(shape_data, scaled_shapes_fallback, shape_count, dpi_scale);
                gpu_upload_array_buffer_data(shape_instance_buffer,
                                             scaled_shapes_fallback,
                                             shape_count * sizeof(vector_shape_instance_t),
                                             true);
                free(scaled_shapes_fallback);
            }
            else
            {
                /* Last resort: upload without scaling */
                gpu_upload_array_buffer_data(shape_instance_buffer,
                                             instance_arena_data(&shape_arena),
                                             shape_arena.used,
                                             true);
            }

            gpu_draw_instances(shape_vertex_array, 0, 4, shape_count);
            gpu_bind_vertex_array(0);

            gpu_set_depth_write(true);
        }

        free(scaled_shapes);
        free(sorted_shapes);
        free(sort_items);
    }

    /* render lines as overlay (always on top) */
    size_t line_count = instance_arena_count(&line_arena);
    if (line_count > 0)
    {
        vector_line_instance_t *line_data = (vector_line_instance_t *)instance_arena_data(&line_arena);
        vector_line_instance_t *scaled_lines = (vector_line_instance_t *)malloc(line_count * sizeof(vector_line_instance_t));

        gpu_use_shader(line_shader);
        gpu_set_uniform_mat4(line_shader_uniform_projection, projection);

        vec2 viewport;
        viewport[0] = (float)width;
        viewport[1] = (float)height;
        gpu_set_uniform_vec2(line_shader_uniform_viewport, viewport);

        gpu_set_depth_test(false);
        gpu_set_depth_write(false);

        gpu_bind_vertex_array(line_vertex_array);

        if (scaled_lines)
        {
            scale_line_stroke_widths(line_data, scaled_lines, line_count, dpi_scale);
            apply_line_hover(scaled_lines, line_count, hovered_entity_id, selected_entity_id, 0x00000000);
            gpu_upload_array_buffer_data(line_instance_buffer,
                                         scaled_lines,
                                         line_count * sizeof(vector_line_instance_t),
                                         true);
            free(scaled_lines);
        }
        else
        {
            /* Fallback: upload without scaling */
            gpu_upload_array_buffer_data(line_instance_buffer,
                                         line_data,
                                         line_arena.used,
                                         true);
        }

        gpu_draw_instances(line_vertex_array, 0, 4, line_count);
        gpu_bind_vertex_array(0);

        gpu_set_depth_test(true);
        gpu_set_depth_write(true);
    }

    /* render glyphs */
    size_t glyph_count = instance_arena_count(&glyph_arena);
    if (glyph_count > 0)
    {
        gpu_use_shader(glyph_shader);

        mat4 projection;
        glm_ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f, projection);
        gpu_set_uniform_mat4(glyph_shader_uniform_projection, projection);

        /* bind atlas texture to texture unit 0 */
        /* TODO: Need to implement gpu_bind_texture() and gpu_set_uniform_int() in gpu.h */
        //gpu_bind_texture(glyph_atlas_texture, 0);
        //gpu_set_uniform_int(glyph_shader_uniform_atlas, 0);

        /* bind vertex array and upload instance data */
        gpu_bind_vertex_array(glyph_vertex_array);
        gpu_upload_array_buffer_data(glyph_instance_buffer,
                                     instance_arena_data(&glyph_arena),
                                     glyph_arena.used,
                                     true);

        /* draw all glyph instances */
        gpu_draw_instances(glyph_vertex_array, 0, 4, glyph_count);

        /* unbind vertex array */
        gpu_bind_vertex_array(0);
    }

    /* render axes in a dedicated final overlay pass (depth tested to be occluded by bodies) */
    size_t axis_count = instance_arena_count(&axis_arena);
    if (axis_count > 0)
    {
        vector_line_instance_t *axis_data = (vector_line_instance_t *)instance_arena_data(&axis_arena);
        vector_line_instance_t *scaled_axes = (vector_line_instance_t *)malloc(axis_count * sizeof(vector_line_instance_t));

        gpu_use_shader(axis_shader);
        gpu_set_uniform_mat4(axis_shader_uniform_projection, projection);

        vec2 viewport;
        viewport[0] = (float)width;
        viewport[1] = (float)height;
        gpu_set_uniform_vec2(axis_shader_uniform_viewport, viewport);

        gpu_set_depth_test(true);
        gpu_set_depth_write(false);

        gpu_bind_vertex_array(axis_vertex_array);

        if (scaled_axes)
        {
            scale_line_stroke_widths(axis_data, scaled_axes, axis_count, dpi_scale);
            apply_line_hover(scaled_axes, axis_count, hovered_entity_id, selected_entity_id, 0x10000000);
            gpu_upload_array_buffer_data(axis_instance_buffer,
                                         scaled_axes,
                                         axis_count * sizeof(vector_line_instance_t),
                                         true);
            free(scaled_axes);
        }
        else
        {
            /* Fallback: upload without scaling */
            gpu_upload_array_buffer_data(axis_instance_buffer,
                                         axis_data,
                                         axis_arena.used,
                                         true);
        }

        gpu_draw_instances(axis_vertex_array, 0, 4, axis_count);
        gpu_bind_vertex_array(0);

        gpu_set_depth_test(true);
        gpu_set_depth_write(true);
    }

    /* TODO: render beziers */
}

float vector_build_dash(float period, float ratio)
{
    /* Pack into 23-bit mantissa: 12 bits period + 11 bits duty */
    uint32_t period_packed = (uint32_t)(fminf(period, 4095.0f));    /* 12 bits: 0-4095 */
    uint32_t ratio_packed = (uint32_t)(ratio * 2047.0f);            /* 11 bits: 0-2047 */

    union {
        uint32_t u;
        float f;
    } converter;

    /* Create normal float: exponent=127 (2^0), pack data in mantissa */
    converter.u = (127 << 23) | ((period_packed & 0xFFFu) << 11) | (ratio_packed & 0x7FFu);

    return converter.f;
}

bool vector_create_line(const vector_line_instance_t *data, vector_instance_t *out_handle)
{
    if (!data || !out_handle)
        return false;

    uint32_t handle = instance_arena_add(&line_arena, data);
    if (handle == UINT32_MAX)
        return false;

    *out_handle = handle;
    return true;
}

bool vector_create_axis_line(const vector_line_instance_t *data, vector_instance_t *out_handle)
{
    if (!data || !out_handle)
        return false;

    uint32_t handle = instance_arena_add(&axis_arena, data);
    if (handle == UINT32_MAX)
        return false;

    *out_handle = handle;
    return true;
}

void vector_update_line(vector_instance_t instance, const vector_line_instance_t *data)
{
    instance_arena_update(&line_arena, instance, data);
}

void vector_update_axis_line(vector_instance_t instance, const vector_line_instance_t *data)
{
    instance_arena_update(&axis_arena, instance, data);
}

void vector_destroy_line(vector_instance_t instance)
{
    instance_arena_remove(&line_arena, instance);
}

void vector_destroy_axis_line(vector_instance_t instance)
{
    instance_arena_remove(&axis_arena, instance);
}

bool vector_create_shape(const vector_shape_instance_t *data, vector_instance_t *out_handle)
{
    if (!data || !out_handle)
        return false;

    uint32_t handle = instance_arena_add(&shape_arena, data);
    if (handle == UINT32_MAX)
        return false;

    *out_handle = handle;

    #ifdef DEBUG
    log_info("vector_create_shape: handle=%u center=(%.1f,%.1f,%.1f) normal=(%.1f,%.1f,%.1f) size=(%.1f,%.1f) color=(%.2f,%.2f,%.2f,%.2f) sides=%.0f stroke=%.1f",
             handle,
             data->center[0], data->center[1], data->center[2],
             data->normal[0], data->normal[1], data->normal[2],
             data->size[0], data->size[1],
             data->color[0], data->color[1], data->color[2], data->color[3],
             data->sides, data->stroke_width);
    #endif

    return true;
}

void vector_update_shape(vector_instance_t instance, const vector_shape_instance_t *data)
{
    #ifdef DEBUG
    log_info("vector_update_shape: handle=%u center=(%.1f,%.1f,%.1f) normal=(%.1f,%.1f,%.1f) color=(%.2f,%.2f,%.2f,%.2f)",
             instance,
             data->center[0], data->center[1], data->center[2],
             data->normal[0], data->normal[1], data->normal[2],
             data->color[0], data->color[1], data->color[2], data->color[3]);
    #endif

    instance_arena_update(&shape_arena, instance, data);
}

void vector_destroy_shape(vector_instance_t instance)
{
    instance_arena_remove(&shape_arena, instance);
}

bool vector_create_glyph(const vector_glyph_instance_t *data, vector_instance_t *out_handle)
{
    if (!data || !out_handle)
        return false;

    uint32_t handle = instance_arena_add(&glyph_arena, data);
    if (handle == UINT32_MAX)
        return false;

    *out_handle = handle;
    return true;
}

void vector_update_glyph(vector_instance_t instance, const vector_glyph_instance_t *data)
{
    instance_arena_update(&glyph_arena, instance, data);
}

void vector_destroy_glyph(vector_instance_t instance)
{
    instance_arena_remove(&glyph_arena, instance);
}


/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static void instance_arena_init(instance_arena_t *arena, size_t instance_size, size_t initial_count)
{
   size_t initial_capacity = initial_count * instance_size;

    /* Always set instance_size to avoid divide-by-zero */
    arena->instance_size = instance_size;

    arena->data = malloc(initial_capacity);
    arena->indices = malloc(initial_count * sizeof(uint32_t));
    arena->reverse_indices = malloc(initial_count * sizeof(uint32_t));

    if (arena->data && arena->indices && arena->reverse_indices)
    {
        arena->capacity = initial_capacity;
        arena->used = 0;
        arena->max_handles = initial_count;
    }
    else
    {
        #ifdef DEBUG
            log_error("Failed to allocate instance arena.");
        #endif

        /* clean up partial allocation */
        free(arena->data);
        free(arena->indices);
        free(arena->reverse_indices);
        arena->data = NULL;
        arena->indices = NULL;
        arena->reverse_indices = NULL;
        arena->capacity = 0;
        arena->used = 0;
        arena->max_handles = 0;
    }
}

static void instance_arena_destroy(instance_arena_t *arena)
{
    free(arena->data);
    free(arena->indices);
    free(arena->reverse_indices);
    
    arena->data = NULL;
    arena->indices = NULL;
    arena->reverse_indices = NULL;
    arena->used = 0;
    arena->capacity = 0;
    arena->max_handles = 0;
}

static void instance_arena_clear(instance_arena_t *arena)
{
    arena->used = 0;
}

static uint32_t instance_arena_add(instance_arena_t *arena, const void *instance)
{
    /* Check if arena is valid (initialization succeeded) */
    if (!arena->data || arena->instance_size == 0 || arena->capacity == 0)
    {
        #ifdef DEBUG
            log_error("Attempted to add instance to uninitialized arena");
        #endif
        return UINT32_MAX;
    }

    /* check if we need to grow the data array */
    if (arena->used + arena->instance_size > arena->capacity)
    {
        size_t new_capacity = arena->capacity * 2;
        void *new_data = realloc(arena->data, new_capacity);
        
        if (!new_data)
        {
            #ifdef DEBUG
                log_error("Failed to reallocate instance arena data.");
            #endif
            return UINT32_MAX;
        }
        
        arena->data = new_data;
        arena->capacity = new_capacity;
    }
    
    /* check if we need to grow the index arrays */
    if ((arena->capacity / arena->instance_size) >= arena->max_handles)
    {
        size_t new_max = arena->max_handles * 2;
        
        uint32_t *new_indices = realloc(arena->indices, new_max * sizeof(uint32_t));
        uint32_t *new_reverse = realloc(arena->reverse_indices, new_max * sizeof(uint32_t));
        
        if (!new_indices || !new_reverse)
        {
            #ifdef DEBUG
                log_error("Failed to reallocate instance arena indices.");
            #endif
            free(new_indices);
            free(new_reverse);
            return UINT32_MAX;
        }
        
        arena->indices = new_indices;
        arena->reverse_indices = new_reverse;
        arena->max_handles = new_max;
    }
    
    /* Add instance to dense array */
    uint32_t handle = arena->used / arena->instance_size; 
    uint32_t data_index = handle;  /* Same value initially */
    
    void *dst = (uint8_t *)arena->data + arena->used;
    memcpy(dst, instance, arena->instance_size);
    
    arena->indices[handle] = data_index;
    arena->reverse_indices[data_index] = handle;
    
    arena->used += arena->instance_size;
    
    return handle;
}

static void instance_arena_remove(instance_arena_t *arena, uint32_t handle)
{
    if (handle >= arena->max_handles)
    {
        #ifdef DEBUG
            log_warning("Call to remove invalid instance handle");
        #endif
        return;
    }
        
    
    uint32_t data_index = arena->indices[handle];
    size_t active_count = arena->used / arena->instance_size;
    
    if (data_index == UINT32_MAX || data_index >= active_count)
    {
        #ifdef DEBUG
            log_warning("Call to remove invalid instance handle");
        #endif
        return;
    }
       
    
    uint32_t last_data_index = active_count - 1;
    
    if (data_index != last_data_index)
    {
        void *dst = (uint8_t *)arena->data + (data_index * arena->instance_size);
        void *src = (uint8_t *)arena->data + (last_data_index * arena->instance_size);
        memcpy(dst, src, arena->instance_size);
        
        uint32_t swapped_handle = arena->reverse_indices[last_data_index];
        arena->indices[swapped_handle] = data_index;
        arena->reverse_indices[data_index] = swapped_handle;
    }
    
    arena->indices[handle] = UINT32_MAX;
    arena->used -= arena->instance_size;
    
}

static size_t instance_arena_count(instance_arena_t *arena)
{
    return arena->used / arena->instance_size;
}

static bool instance_arena_update(instance_arena_t *arena, uint32_t handle, const void *instance)
{
    if (handle >= arena->max_handles)
        return false;
    
    uint32_t data_index = arena->indices[handle];
    size_t active_count = arena->used / arena->instance_size;
    
    if (data_index == UINT32_MAX || data_index >= active_count)
        return false;
    
    void *dst = (uint8_t *)arena->data + (data_index * arena->instance_size);
    memcpy(dst, instance, arena->instance_size);
    
    return true;
}

static void *instance_arena_get(instance_arena_t *arena, uint32_t handle)
{
    if (handle >= arena->max_handles)
        return NULL;
    
    uint32_t data_index = arena->indices[handle];
    size_t active_count = arena->used / arena->instance_size;
    
    if (data_index == UINT32_MAX || data_index >= active_count)
        return NULL;
    
    return (uint8_t *)arena->data + (data_index * arena->instance_size);
}

static void *instance_arena_data(instance_arena_t *arena)
{
    return arena->data;
}

static int compare_shape_depth_desc(const void *a, const void *b)
{
    const shape_sort_item_t *sa = (const shape_sort_item_t *)a;
    const shape_sort_item_t *sb = (const shape_sort_item_t *)b;
    if (sa->depth < sb->depth) return 1;
    if (sa->depth > sb->depth) return -1;
    return 0;
}

static void scale_line_stroke_widths(const vector_line_instance_t *src, vector_line_instance_t *dst, size_t count, float scale)
{
    static int log_once = 0;
    if (log_once < 3 && count > 0) {
        printf("scale_line_stroke_widths: count=%zu, scale=%.2f, original_width=%.2f, scaled_width=%.2f\n",
               count, scale, src[0].stroke_width, src[0].stroke_width * scale);
        log_once++;
    }

    for (size_t i = 0; i < count; i++)
    {
        dst[i] = src[i];
        dst[i].stroke_width *= scale;
    }
}

static void scale_shape_stroke_widths(const vector_shape_instance_t *src, vector_shape_instance_t *dst, size_t count, float scale)
{
    for (size_t i = 0; i < count; i++)
    {
        dst[i] = src[i];
        dst[i].stroke_width *= scale;
    }
}

/* Apply hover and selection highlighting to lines */
static void apply_line_hover(vector_line_instance_t *instances, size_t count, uint32_t hover_id, uint32_t select_id, uint32_t type_bits)
{
    uint32_t expected_type = (type_bits >> 28) & 0xF;

    /* Apply selection highlighting (orange) */
    if (select_id != VECTOR_INVALID_INSTANCE) {
        uint32_t select_type = (select_id >> 28) & 0xF;
        uint32_t select_index = select_id & 0x0FFFFFFF;

        if (select_type == expected_type && select_index < count) {
            instances[select_index].color[0] = 1.0f;  /* Orange for selection */
            instances[select_index].color[1] = 0.6f;
            instances[select_index].color[2] = 0.0f;
            instances[select_index].color[3] = 1.0f;
            instances[select_index].stroke_width *= 2.0f;
        }
    }

    /* Apply hover highlighting (cyan) - overrides selection if hovering over selected item */
    if (hover_id != VECTOR_INVALID_INSTANCE) {
        uint32_t hover_type = (hover_id >> 28) & 0xF;
        uint32_t hover_index = hover_id & 0x0FFFFFFF;

        if (hover_type == expected_type && hover_index < count) {
            instances[hover_index].color[0] = 0.0f;  /* Cyan for hover */
            instances[hover_index].color[1] = 1.0f;
            instances[hover_index].color[2] = 1.0f;
            instances[hover_index].color[3] = 1.0f;
            instances[hover_index].stroke_width *= 2.0f;
        }
    }
}

/* Apply hover and selection highlighting to shapes */
static void apply_shape_hover(vector_shape_instance_t *instances, size_t count, uint32_t hover_id, uint32_t select_id)
{
    /* Apply selection highlighting (orange) */
    if (select_id != VECTOR_INVALID_INSTANCE) {
        uint32_t select_type = (select_id >> 28) & 0xF;
        uint32_t select_index = select_id & 0x0FFFFFFF;

        if (select_type == 0x2 && select_index < count) {  /* Shape type */
            instances[select_index].color[0] = 1.0f;  /* Orange for selection */
            instances[select_index].color[1] = 0.6f;
            instances[select_index].color[2] = 0.0f;
            instances[select_index].color[3] = 0.8f;
            instances[select_index].stroke_width *= 2.0f;
        }
    }

    /* Apply hover highlighting (cyan) - overrides selection if hovering over selected item */
    if (hover_id != VECTOR_INVALID_INSTANCE) {
        uint32_t hover_type = (hover_id >> 28) & 0xF;
        uint32_t hover_index = hover_id & 0x0FFFFFFF;

        if (hover_type == 0x2 && hover_index < count) {  /* Shape type */
            /* Apply cyan color and thicker stroke */
            instances[hover_index].color[0] = 0.0f;
            instances[hover_index].color[1] = 1.0f;
            instances[hover_index].color[2] = 1.0f;
            instances[hover_index].color[3] = 0.8f;
            instances[hover_index].stroke_width *= 2.0f;
        }
    }
}
