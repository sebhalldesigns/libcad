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

#include <render/gpu/gpu.h>
#include <util/log/log.h>

#include "vector.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define INITIAL_ARENA_SIZE  (1024U)
#define INSTANCE_TYPE_OFFSET (32U)

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
#endif

static shader_t vector_shader = 0;
static uniform_t projection_uniform = -1;

static const float quad_vertices[] =
{
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f,  1.0f
};

static vertex_array_t vector_vertex_array;
static buffer_t quad_buffer;
static buffer_t instance_buffer;

static instance_arena_t line_arena;
static instance_arena_t shape_arena;
static instance_arena_t bezier_arena;
static instance_arena_t glyph_arena;

static shader_t line_shader;
static uniform_t line_shader_uniform_projection;
static uniform_t line_shader_uniform_viewport;
static vertex_array_t line_vertex_array;
static buffer_t line_quad_buffer;
static buffer_t line_instance_buffer;


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

    log_info("buffer creation complete");

    return true;
}

void vector_render(int width, int height)
{
    gpu_set_blending(true);
    gpu_set_viewport(width, height);

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

    /* TODO: render shapes, beziers, glyphs */
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