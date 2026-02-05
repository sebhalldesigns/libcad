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

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

void gpu_init(void);

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


#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_GPU_H */