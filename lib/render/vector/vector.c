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
    printf("vector_set_dpi_scale: scale=%.2f, dpi_scale=%.2f\n", scale, dpi_scale);
}

void vector_render(int width, int height, mat4 projection)
{
    static int frame_count = 0;
    if (frame_count % 60 == 0) {
        printf("vector_render: frame %d, dpi_scale=%.2f\n", frame_count, dpi_scale);
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

    /* render axes in a dedicated final overlay pass with haze */
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

        gpu_set_depth_test(false);
        gpu_set_depth_write(false);

        gpu_bind_vertex_array(axis_vertex_array);

        if (scaled_axes)
        {
            scale_line_stroke_widths(axis_data, scaled_axes, axis_count, dpi_scale);
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
