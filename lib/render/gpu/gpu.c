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
#include <string.h>

#ifdef __EMSCRIPTEN__
    #include <emscripten/html5.h>
#endif

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

#define DEBUG 1


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
#ifdef __EMSCRIPTEN__
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;

    /* Use the canvas element with id="cad-canvas" */
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#cad-canvas", &attrs);
    if (ctx <= 0)
    {
        #ifdef DEBUG
        log_error("Failed to create WebGL context");
        #endif
        return false;
    }

    emscripten_webgl_make_context_current(ctx);
#elif !defined(USE_GLES)
    if (!gladLoadGL())
    {
        #ifdef DEBUG
        log_error("Failed to initialize GLAD\n");
        #endif
        return false;
    }
#endif

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
** MARK: FRAMEBUFFER OPERATIONS
***************************************************************/

framebuffer_t gpu_create_framebuffer(void)
{
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    return (framebuffer_t)fbo;
}

void gpu_bind_framebuffer(framebuffer_t fbo)
{
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)fbo);
}

texture_t gpu_create_texture_2d(int width, int height, bool rgba, bool depth)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    if (depth) {
        /* Depth texture */
        #ifdef USE_GLES
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0,
                     GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
        #else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0,
                     GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
        #endif
    } else {
        /* Color texture - use sized internal format for GLES 3.0+ compatibility */
        GLenum internal_format = rgba ? GL_RGBA8 : GL_RGB8;
        GLenum format = rgba ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, NULL);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    return (texture_t)texture;
}

void gpu_framebuffer_attach_texture(framebuffer_t fbo, texture_t texture, bool depth)
{
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)fbo);

    if (depth) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, (GLuint)texture, 0);
    } else {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, (GLuint)texture, 0);
    }
}

bool gpu_check_framebuffer_complete(framebuffer_t fbo)
{
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)fbo);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    return (status == GL_FRAMEBUFFER_COMPLETE);
}

void gpu_read_pixels(int x, int y, int width, int height, void *data)
{
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

/***************************************************************
** MARK: INDEX BUFFER & MESH DRAWING
***************************************************************/

void gpu_upload_index_buffer_data(buffer_t buffer, void *data, size_t data_size, bool dynamic)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data_size, data, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
}

void gpu_draw_elements(vertex_array_t vertex_array, uint32_t index_count)
{
    glBindVertexArray(vertex_array);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
}

void gpu_draw_lines(vertex_array_t vertex_array, uint32_t first, uint32_t count)
{
    glBindVertexArray(vertex_array);
    glDrawArrays(GL_LINES, first, count);
}

void gpu_set_polygon_offset(bool enable, float factor, float units)
{
    if (enable)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(factor, units);
    }
    else
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

void gpu_set_line_width(float width)
{
    glLineWidth(width);
}

void gpu_set_uniform_float(uniform_t uniform_handle, float value)
{
    glUniform1f(uniform_handle, value);
}

void gpu_set_uniform_vec3(uniform_t uniform_handle, vec3 value)
{
    glUniform3f(uniform_handle, value[0], value[1], value[2]);
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/






