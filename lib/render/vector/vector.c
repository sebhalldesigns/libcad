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

#else
extern uint8_t resources_shaders_vector_line_core_vs_glsl[];
extern uint32_t resources_shaders_vector_line_core_vs_glsl_size;

extern uint8_t resources_shaders_vector_line_core_fs_glsl[];
extern uint32_t resources_shaders_vector_line_core_fs_glsl_size;

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
static instance_arena_t shape_arena;
static instance_arena_t bezier_arena;
static instance_arena_t glyph_arena;

/* LINE SHADER */
static shader_t line_shader;
static uniform_t line_shader_uniform_projection;
static uniform_t line_shader_uniform_viewport;
static vertex_array_t line_vertex_array;
static buffer_t line_quad_buffer;
static buffer_t line_instance_buffer;

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

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

bool vector_init()
{

    /* create arenas */
    instance_arena_init(&line_arena, sizeof(vector_line_instance_t), INITIAL_ARENA_SIZE);
    instance_arena_init(&shape_arena, sizeof(vector_shape_instance_t), INITIAL_ARENA_SIZE);
    instance_arena_init(&glyph_arena, sizeof(vector_glyph_instance_t), INITIAL_ARENA_SIZE);

    /* LINE RENDERER SETUP */

    if (!gpu_compile_shader(
        (const char*)resources_shaders_vector_line_core_vs_glsl,
        resources_shaders_vector_line_core_vs_glsl_size,
        (const char*)resources_shaders_vector_line_core_fs_glsl,
        resources_shaders_vector_line_core_fs_glsl_size,
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

    /* SHAPE RENDERER SETUP */

    if (!gpu_compile_shader(
        (const char*)resources_shaders_vector_shape_core_vs_glsl,
        resources_shaders_vector_shape_core_vs_glsl_size,
        (const char*)resources_shaders_vector_shape_core_fs_glsl,
        resources_shaders_vector_shape_core_fs_glsl_size,
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
        (const char*)resources_shaders_vector_glyph_core_vs_glsl,
        resources_shaders_vector_glyph_core_vs_glsl_size,
        (const char*)resources_shaders_vector_glyph_core_fs_glsl,
        resources_shaders_vector_glyph_core_fs_glsl_size,
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

void vector_render(int width, int height)
{
    gpu_set_blending(true);
    gpu_set_viewport(width, height);

    /* enable depth testing for 3D */
    gpu_set_depth_test(true);
    gpu_set_depth_write(true);

    /* render lines */
    size_t line_count = instance_arena_count(&line_arena);
    if (line_count > 0)
    {
        gpu_use_shader(line_shader);

        mat4 projection;
        glm_ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f, projection);
        gpu_set_uniform_mat4(line_shader_uniform_projection, projection);

        vec2 viewport;
        viewport[0] = (float)width;
        viewport[1] = (float)height;
        gpu_set_uniform_vec2(line_shader_uniform_viewport, viewport);

        /* bind vertex array and upload instance data */
        gpu_bind_vertex_array(line_vertex_array);
        gpu_upload_array_buffer_data(line_instance_buffer,
                                     instance_arena_data(&line_arena),
                                     line_arena.used,
                                     true);

        /* draw all line instances */
        gpu_draw_instances(line_vertex_array, 0, 4, line_count);

        /* unbind vertex array */
        gpu_bind_vertex_array(0);
    }

    /* render shapes */
    size_t shape_count = instance_arena_count(&shape_arena);
    log_info("Rendering %zu shapes", shape_count);
    if (shape_count > 0)
    {
        gpu_use_shader(shape_shader);

        mat4 projection;
        glm_ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f, projection);
        gpu_set_uniform_mat4(shape_shader_uniform_projection, projection);

        vec2 viewport;
        viewport[0] = (float)width;
        viewport[1] = (float)height;
        gpu_set_uniform_vec2(shape_shader_uniform_viewport, viewport);

        /* bind vertex array and upload instance data */
        gpu_bind_vertex_array(shape_vertex_array);
        gpu_upload_array_buffer_data(shape_instance_buffer,
                                     instance_arena_data(&shape_arena),
                                     shape_arena.used,
                                     true);

        /* draw all shape instances */
        gpu_draw_instances(shape_vertex_array, 0, 4, shape_count);

        /* unbind vertex array */
        gpu_bind_vertex_array(0);
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

void vector_update_line(vector_instance_t instance, const vector_line_instance_t *data)
{
    instance_arena_update(&line_arena, instance, data);
}

void vector_destroy_line(vector_instance_t instance)
{
    instance_arena_remove(&line_arena, instance);
}

bool vector_create_shape(const vector_shape_instance_t *data, vector_instance_t *out_handle)
{
    if (!data || !out_handle)
        return false;

    uint32_t handle = instance_arena_add(&shape_arena, data);
    if (handle == UINT32_MAX)
        return false;

    *out_handle = handle;
    return true;
}

void vector_update_shape(vector_instance_t instance, const vector_shape_instance_t *data)
{
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
    
    arena->data = malloc(initial_capacity);
    arena->indices = malloc(initial_count * sizeof(uint32_t));
    arena->reverse_indices = malloc(initial_count * sizeof(uint32_t));
    
    if (arena->data && arena->indices && arena->reverse_indices)
    {
        arena->capacity = initial_capacity;
        arena->instance_size = instance_size;
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