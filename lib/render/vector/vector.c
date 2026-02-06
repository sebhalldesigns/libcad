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

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

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






