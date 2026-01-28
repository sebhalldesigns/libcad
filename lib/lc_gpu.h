/***************************************************************
**
** libcad Header File
**
** File         :  lc_gpu.h
** Module       :  lc_gpu
** Author       :  SH
** Created      :  2026-01-26 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad GPU API
**
**  Module: lc_gpu
**  Responsibility: GPU resource management abstraction layer.
**    Will provide handle-based API for buffers, textures, shaders,
**    and framebuffers. Currently a stub.
**  Owns: (planned) GPU resource handles and lifecycle.
**  Uses: OpenGL (via glad/GLES3).
**  Does NOT own: Scene/canvas/draw state.
**
***************************************************************/

#ifndef LC_GPU_H
#define LC_GPU_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* Opaque handles for GPU resources */
typedef uint32_t lc_gpu_buffer_t;
typedef uint32_t lc_gpu_shader_t;
typedef uint32_t lc_gpu_program_t;
typedef uint32_t lc_gpu_vao_t;
typedef uint32_t lc_gpu_texture_t;
typedef uint32_t lc_gpu_fbo_t;

/* Buffer usage hints */
typedef enum {
    LC_GPU_STATIC  = 0,
    LC_GPU_DYNAMIC = 1,
    LC_GPU_STREAM  = 2
} lc_gpu_buffer_usage_t;

/* Shader types */
typedef enum {
    LC_GPU_VERTEX_SHADER   = 0,
    LC_GPU_FRAGMENT_SHADER = 1
} lc_gpu_shader_type_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* Initialize GPU module */
int lc_gpu_init(void);

/* Shutdown GPU module, release all resources */
void lc_gpu_shutdown(void);

/* Buffer management */
lc_gpu_buffer_t lc_gpu_create_buffer(size_t size, const void *data, lc_gpu_buffer_usage_t usage);
void lc_gpu_update_buffer(lc_gpu_buffer_t buf, size_t offset, size_t size, const void *data);
void lc_gpu_destroy_buffer(lc_gpu_buffer_t buf);

/* Shader management */
lc_gpu_shader_t lc_gpu_compile_shader(lc_gpu_shader_type_t type, const char *source);
lc_gpu_program_t lc_gpu_link_program(lc_gpu_shader_t vertex, lc_gpu_shader_t fragment);
int lc_gpu_get_uniform_location(lc_gpu_program_t prog, const char *name);
void lc_gpu_destroy_shader(lc_gpu_shader_t shader);
void lc_gpu_destroy_program(lc_gpu_program_t prog);

/* VAO management */
lc_gpu_vao_t lc_gpu_create_vao(void);
void lc_gpu_destroy_vao(lc_gpu_vao_t vao);

/* Texture management */
lc_gpu_texture_t lc_gpu_create_texture_2d(int width, int height, int internal_format, int format, int type, const void *data);
void lc_gpu_destroy_texture(lc_gpu_texture_t tex);

/* FBO management */
lc_gpu_fbo_t lc_gpu_create_fbo(void);
void lc_gpu_fbo_attach_texture(lc_gpu_fbo_t fbo, lc_gpu_texture_t tex);
void lc_gpu_fbo_attach_depth(lc_gpu_fbo_t fbo, int width, int height);
int lc_gpu_fbo_check_complete(lc_gpu_fbo_t fbo);
void lc_gpu_destroy_fbo(lc_gpu_fbo_t fbo)


#ifdef __cplusplus
}
#endif

#endif /* LC_GPU_H */