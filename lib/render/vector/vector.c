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

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef struct
{
    vec2 pos;
    float radius;
    vec4 color;
} vector_instance_t;

typedef struct
{
    vec3 start;
    vec3 end;
    vec4 color;
    float stroke_width;
    float dash; /* encode into single float somehow */
} line_instance_t; /* 12 floats */

typedef struct
{
    vec3 center;
    vec3 normal;
    vec2 size;
    vec4 color;
    float rotation;
    float sides; /* 1 for ellipse, 2 invalid, 3 triangle etc. */
    float start_angle; /* for arcs, 0 for ellipse */
    float end_angle; /* for arcs, 0 for circle */
    float fill; /* normalise fill, -1 to 1. -ve fill from center, +ve fill from outside*/
    float stroke_width;
    float corner_radius;
    float dash;  /* encode into single float */
} shape_instance_t; /* 20 floats */

typedef struct
{
    vec3 p0, p1, p2, p3;
    vec4 color;
    float fill;
    float fill_param;
    float stroke_width;
    float dash; /* encode into single float */
} bezier_instance_t; /* 20 floats */

typedef struct
{
    vec2 position;
    vec2 size;
    vec2 uv_min;
    vec2 uv_max;
    vec4 color;
} glyph_instance_t; /* 12 floats */

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
extern uint8_t resources_shaders_vector_vector_core_vs_glsl[];
extern uint32_t resources_shaders_vector_vector_core_vs_glsl_size;

extern uint8_t resources_shaders_vector_vector_core_fs_glsl[];
extern uint32_t resources_shaders_vector_vector_core_fs_glsl_size;
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

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void instance_arena_init(instance_arena_t *arena, size_t capacity);
static void instance_arena_destroy(instance_arena_t *arena);
static void instance_arena_clear(instance_arena_t *arena);
static uint32_t instance_arena_add(instance_arena_t *arena, const void *instance);
static void instance_arena_remove(instance_arena_t *arena, uint32_t handle);
static size_t instance_arena_count(instance_arena_t *arena);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

bool vector_init()
{
    if (!gpu_compile_shader(
        (const char*)resources_shaders_vector_vector_core_vs_glsl,
        resources_shaders_vector_vector_core_vs_glsl_size,
        (const char*)resources_shaders_vector_vector_core_fs_glsl,
        resources_shaders_vector_vector_core_fs_glsl_size,
        &vector_shader
    ))
    {
        #ifdef DEBUG
            log_error("Failed to compile vector shader.");
        #endif
        return false;
    }

    log_info("compiled vector shader");

    if (!gpu_get_shader_uniform(vector_shader, "projection", &projection_uniform))
    {
        #ifdef DEBUG
            log_error("Failed to get projection uniform.");
        #endif
        return false;
    }

    vector_vertex_array = gpu_create_vertex_array();
    gpu_bind_vertex_array(vector_vertex_array);

    quad_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(quad_buffer, quad_vertices, sizeof(quad_vertices), false);

    gpu_enable_vertex_attribute(0, 2, GPU_TYPE_FLOAT, false, 2 * sizeof(float), 0, 0);

    vector_instance_t instances[2];
    instances[0].pos[0] = 100.0f;
    instances[0].pos[1] = 200.0f;
    instances[0].radius = 50.0f;
    instances[0].color[0] = 0.0f;
    instances[0].color[1] = 1.0f;
    instances[0].color[2] = 1.0f;
    instances[0].color[3] = 1.0f;
    instances[1].pos[0] = 300.0f;
    instances[1].pos[1] = 100.0f;
    instances[1].radius = 25.0f;
    instances[1].color[0] = 0.0f;
    instances[1].color[1] = 0.0f;
    instances[1].color[2] = 1.0f;
    instances[1].color[3] = 1.0f;

    int num_instances = 2;
    instance_buffer = gpu_create_buffer();
    gpu_upload_array_buffer_data(instance_buffer, instances, sizeof(instances), true);

    gpu_enable_vertex_attribute(1, 2, GPU_TYPE_FLOAT, false, sizeof(vector_instance_t), offsetof(vector_instance_t, pos), 1);
    gpu_enable_vertex_attribute(2, 1, GPU_TYPE_FLOAT, false, sizeof(vector_instance_t), offsetof(vector_instance_t, radius), 1);
    gpu_enable_vertex_attribute(3, 4, GPU_TYPE_FLOAT, false, sizeof(vector_instance_t), offsetof(vector_instance_t, color), 1);

    gpu_bind_vertex_array(0);

    log_info("buffer creation complete");

    return true;
}

void vector_render(int width, int height)
{
    gpu_set_blending(true);
    
    gpu_set_viewport(width, height);
    gpu_use_shader(vector_shader);

    mat4 projection;
    glm_ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f, projection);
    gpu_set_uniform_mat4(projection_uniform, projection);

    gpu_draw_instances(vector_vertex_array, 0, 4, 2);
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
    
    return true;
}

static size_t instance_arena_count(instance_arena_t *arena)
{
    return arena->used / arena->instance_size;
}


