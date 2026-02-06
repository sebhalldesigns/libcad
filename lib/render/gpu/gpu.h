/***************************************************************
**
** libcad Header File
**
** File         :  gpu.h
** Module       :  render/gpu
** Author       :  SH
** Created      :  2026-01-26 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad GPU API
**
***************************************************************/

#ifndef LIBCAD_GPU_H
#define LIBCAD_GPU_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include <cglm/cglm.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef uint32_t shader_t;
typedef int32_t uniform_t;
typedef uint32_t vertex_array_t;
typedef uint32_t buffer_t;

typedef enum
{
    GPU_TYPE_FLOAT
} gpu_type_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

bool gpu_init(void);

void gpu_set_viewport(int width, int height);

bool gpu_compile_shader(
    const char *vertex, size_t vertex_size, 
    const char *fragment, size_t fragment_size, 
    shader_t *shader_handle
);

void gpu_clear_color_buffer(vec4 color);
void gpu_clear_depth_buffer(void);

void gpu_use_shader(shader_t shader_handle);

bool gpu_get_shader_uniform(shader_t shader_handle, const char *uniform, uniform_t *uniform_handle);

void gpu_set_uniform_vec4(uniform_t uniform_handle, vec4 value);
void gpu_set_uniform_mat4(uniform_t uniform_handle, mat4 value);

vertex_array_t gpu_create_vertex_array();
void gpu_bind_vertex_array(vertex_array_t vertex_array);

buffer_t gpu_create_buffer();
void gpu_upload_array_buffer_data(buffer_t buffer, void *data, size_t data_size, bool dynamic);

void gpu_enable_vertex_attribute(
    uint32_t location, uint32_t components, 
    gpu_type_t type, bool normalized, 
    size_t stride, size_t offset,
    size_t instance_advance
);

void gpu_draw_instances(
    vertex_array_t vertex_array, 
    uint32_t first_index, uint32_t vertex_count, 
    size_t instance_count
);

void gpu_set_blending(bool blending);

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_GPU_H */