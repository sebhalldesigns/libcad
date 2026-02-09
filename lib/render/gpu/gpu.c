/***************************************************************
**
** libcad Source File
**
** File         :  gpu.c
** Module       :  render/gpu
** Author       :  SH
** Created      :  2026-01-26 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad GPU API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <libcad/libcad.h>
#include <stdio.h>

#ifdef USE_GLES
    #include <GLES3/gl3.h>
#else
    #include <glad/glad.h>
#endif

#include <cglm/cglm.h>

#include <util/log/log.h>

#include "gpu.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define INFO_LOG_SIZE (512U)

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static int success;
static char info_log[INFO_LOG_SIZE];

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

bool gpu_init(void)
{
    if (!gladLoadGL())
    {
        #if DEBUG
        log_error("Failed to initialize GLAD\n");
        #endif
        return false;
    }

    return true;
}

void gpu_set_viewport(int width, int height)
{
    glViewport(0, 0, width, height);
}

bool gpu_compile_shader(
    const char *vertex_src, size_t vertex_src_size, 
    const char *fragment_src, size_t fragment_src_size, 
    shader_t *shader_handle
)
{   
    char* vertex_src_terminated = (char*)malloc(vertex_src_size + 1);
    memcpy(vertex_src_terminated, vertex_src, vertex_src_size);
    vertex_src_terminated[vertex_src_size] = '\0';

    char* fragment_src_terminated = (char*)malloc(fragment_src_size + 1);
    memcpy(fragment_src_terminated, fragment_src, fragment_src_size);
    fragment_src_terminated[fragment_src_size] = '\0';

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, (const GLchar**)&vertex_src_terminated, NULL);
    glCompileShader(vertex_shader);    
    
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex_shader, INFO_LOG_SIZE, NULL, info_log);
        #ifdef DEBUG
            log_error("Failed to compile vertex shader: %s\n", info_log);
        #endif
        glDeleteShader(vertex_shader);
        return false;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, (const GLchar**)&fragment_src_terminated, NULL);
    glCompileShader(fragment_shader);

        
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment_shader, INFO_LOG_SIZE, NULL, info_log);
        #ifdef DEBUG
            log_error("Failed to compile fragment shader: %s\n", info_log);
        #endif
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }


    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, INFO_LOG_SIZE, NULL, info_log);
        #ifdef DEBUG
            log_error("Failed to link shader program: %s\n", info_log);
        #endif
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        glDeleteProgram(program);
        return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    *shader_handle = program;

    return true;
}

void gpu_clear_color_buffer(vec4 color)
{
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
}

void gpu_clear_depth_buffer(void)
{
    glClear(GL_DEPTH_BUFFER_BIT);
}

void gpu_use_shader(shader_t shader_handle)
{
    glUseProgram(shader_handle);
}

bool gpu_get_shader_uniform(shader_t shader_handle, const char *uniform, uniform_t *uniform_handle)
{
    int uniform_loc = glGetUniformLocation(shader_handle, uniform);
    
    if (uniform_loc < 0)
    {
        #ifdef DEBUG
            log_error("Failed get uniform: %s\n", uniform);
        #endif
        return false;
    }

    *uniform_handle = uniform_loc;
    
    return true;
}

void gpu_set_uniform_vec2(uniform_t uniform_handle, vec2 value)
{
    glUniform2f(uniform_handle, value[0], value[1]);
}

void gpu_set_uniform_vec4(uniform_t uniform_handle, vec4 value)
{
    glUniform4f(uniform_handle, value[0], value[1], value[2], value[3]);
}

void gpu_set_uniform_mat4(uniform_t uniform_handle, mat4 value)
{
    glUniformMatrix4fv(uniform_handle, 1, GL_FALSE, (float*)value);
}

vertex_array_t gpu_create_vertex_array()
{
    GLuint vertex_array;
    glGenVertexArrays(1, &vertex_array);
    return vertex_array;
}

void gpu_bind_vertex_array(vertex_array_t vertex_array)
{
    glBindVertexArray(vertex_array);
}

buffer_t gpu_create_buffer()
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    return buffer;
}

void gpu_upload_array_buffer_data(buffer_t buffer, void *data, size_t data_size, bool dynamic)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, data_size, data, dynamic ? GL_STATIC_DRAW : GL_STATIC_DRAW);
}

void gpu_enable_vertex_attribute(
    uint32_t location, uint32_t components, 
    gpu_type_t type, bool normalized, 
    size_t stride, size_t offset,
    size_t advance
)
{
    uint32_t gl_type;

    switch (type)
    {
        default:
        {
            gl_type = GL_FLOAT;
        } break;
    }

    glVertexAttribPointer(
        location,
        components,
        gl_type,
        normalized ? GL_TRUE : GL_FALSE,
        stride,
        (void*)offset
    );

    glEnableVertexAttribArray(location);
    glVertexAttribDivisor(location, advance);
}

void gpu_draw_instances(
    vertex_array_t vertex_array, 
    uint32_t first_index, uint32_t vertex_count, 
    size_t instance_count
)
{
    glBindVertexArray(vertex_array);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, first_index, vertex_count, instance_count);
}

void gpu_set_blending(bool blending)
{   
    if (blending)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    }
    else
    {
        glDisable(GL_BLEND);
    }
}

void gpu_set_depth_test(bool depth_test)
{
    if (depth_test)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void gpu_set_depth_write(bool depth_write)
{
    glDepthMask(depth_write);
}


/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/






