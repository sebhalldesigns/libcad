/***************************************************************
**
** libcad Source File
**
** File         :  lc_gpu.c
** Module       :  lc_gpu
** Author       :  SH
** Created      :  2026-01-26 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad GPU API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_gpu.h"
#include <libcad/libcad.h>
#include <stdio.h>
#include <string.h>

#ifdef EMSCRIPTEN
    #include <GLES3/gl3.h>
#else
    #include <glad/glad.h>
#endif

#include <cglm/cglm.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define LC_GPU_MAX_BUFFERS  256
#define LC_GPU_MAX_SHADERS  64
#define LC_GPU_MAX_PROGRAMS 32
#define LC_GPU_MAX_VAOS     32
#define LC_GPU_MAX_TEXTURES 64
#define LC_GPU_MAX_FBOS     16

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* FBO data stores both the framebuffer and depth renderbuffer */
typedef struct {
    GLuint fbo;
    GLuint depth_rbo;
} lc_gpu_fbo_data_t;

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

/* Each slot stores the raw OpenGL handle (GLuint).
 * Slot 0 is reserved (handle 0 = invalid). */
static GLuint buffer_registry[LC_GPU_MAX_BUFFERS];
static int buffer_count = 0;

static GLuint shader_registry[LC_GPU_MAX_SHADERS];
static int shader_count = 0;

static GLuint program_registry[LC_GPU_MAX_PROGRAMS];
static int program_count = 0;

static GLuint vao_registry[LC_GPU_MAX_VAOS];
static int vao_count = 0;

static GLuint texture_registry[LC_GPU_MAX_TEXTURES];
static int texture_count = 0;

static lc_gpu_fbo_data_t fbo_registry[LC_GPU_MAX_FBOS];
static int fbo_count = 0;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static GLenum lc_gpu_buffer_usage_to_gl(lc_gpu_buffer_usage_t usage);
static GLenum lc_gpu_shader_type_to_gl(lc_gpu_shader_type_t type);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

/* Initialize GPU module */
int lc_gpu_init(void) {
    /* Zero out all registries */
    memset(buffer_registry, 0, sizeof(buffer_registry));
    memset(shader_registry, 0, sizeof(shader_registry));
    memset(program_registry, 0, sizeof(program_registry));
    memset(vao_registry, 0, sizeof(vao_registry));
    memset(texture_registry, 0, sizeof(texture_registry));
    memset(fbo_registry, 0, sizeof(fbo_registry));

    /* Reserve slot 0 (handle 0 = invalid) */
    buffer_count = 1;
    shader_count = 1;
    program_count = 1;
    vao_count = 1;
    texture_count = 1;
    fbo_count = 1;

    return 0;
}

/* Shutdown GPU module, release all resources */
void lc_gpu_shutdown(void) {
    int i;

    /* Destroy all buffers */
    for (i = 1; i < buffer_count && i < LC_GPU_MAX_BUFFERS; i++) {
        if (buffer_registry[i] != 0) {
            glDeleteBuffers(1, &buffer_registry[i]);
        }
    }

    /* Destroy all shaders */
    for (i = 1; i < shader_count && i < LC_GPU_MAX_SHADERS; i++) {
        if (shader_registry[i] != 0) {
            glDeleteShader(shader_registry[i]);
        }
    }

    /* Destroy all programs */
    for (i = 1; i < program_count && i < LC_GPU_MAX_PROGRAMS; i++) {
        if (program_registry[i] != 0) {
            glDeleteProgram(program_registry[i]);
        }
    }

    /* Destroy all VAOs */
    for (i = 1; i < vao_count && i < LC_GPU_MAX_VAOS; i++) {
        if (vao_registry[i] != 0) {
            glDeleteVertexArrays(1, &vao_registry[i]);
        }
    }

    /* Destroy all textures */
    for (i = 1; i < texture_count && i < LC_GPU_MAX_TEXTURES; i++) {
        if (texture_registry[i] != 0) {
            glDeleteTextures(1, &texture_registry[i]);
        }
    }

    /* Destroy all FBOs and depth renderbuffers */
    for (i = 1; i < fbo_count && i < LC_GPU_MAX_FBOS; i++) {
        if (fbo_registry[i].fbo != 0) {
            glDeleteFramebuffers(1, &fbo_registry[i].fbo);
        }
        if (fbo_registry[i].depth_rbo != 0) {
            glDeleteRenderbuffers(1, &fbo_registry[i].depth_rbo);
        }
    }

    /* Reset counts */
    buffer_count = 0;
    shader_count = 0;
    program_count = 0;
    vao_count = 0;
    texture_count = 0;
    fbo_count = 0;
}

/* Buffer management */
lc_gpu_buffer_t lc_gpu_create_buffer(size_t size, const void *data, lc_gpu_buffer_usage_t usage) {
    GLuint gl_buffer;
    GLenum gl_usage;

    if (buffer_count >= LC_GPU_MAX_BUFFERS) {
        fprintf(stderr, "lc_gpu_create_buffer: buffer registry full\n");
        return 0;
    }

    gl_usage = lc_gpu_buffer_usage_to_gl(usage);

    glGenBuffers(1, &gl_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, gl_buffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)size, data, gl_usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    buffer_registry[buffer_count] = gl_buffer;
    buffer_count++;

    return (lc_gpu_buffer_t)(buffer_count - 1);
}

void lc_gpu_update_buffer(lc_gpu_buffer_t buf, size_t offset, size_t size, const void *data) {
    GLuint gl_buffer;

    if (buf == 0 || buf >= (lc_gpu_buffer_t)buffer_count) {
        fprintf(stderr, "lc_gpu_update_buffer: invalid buffer handle %u\n", buf);
        return;
    }

    gl_buffer = buffer_registry[buf];
    if (gl_buffer == 0) {
        fprintf(stderr, "lc_gpu_update_buffer: buffer handle %u is destroyed\n", buf);
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, gl_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)offset, (GLsizeiptr)size, data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void lc_gpu_destroy_buffer(lc_gpu_buffer_t buf) {
    GLuint gl_buffer;

    if (buf == 0 || buf >= (lc_gpu_buffer_t)buffer_count) {
        return;
    }

    gl_buffer = buffer_registry[buf];
    if (gl_buffer != 0) {
        glDeleteBuffers(1, &gl_buffer);
        buffer_registry[buf] = 0;
    }
}

/* Shader management */
lc_gpu_shader_t lc_gpu_compile_shader(lc_gpu_shader_type_t type, const char *source) {
    GLuint gl_shader;
    GLenum gl_type;
    GLint success;
    GLchar info_log[512];

    if (shader_count >= LC_GPU_MAX_SHADERS) {
        fprintf(stderr, "lc_gpu_compile_shader: shader registry full\n");
        return 0;
    }

    if (source == NULL) {
        fprintf(stderr, "lc_gpu_compile_shader: source is NULL\n");
        return 0;
    }

    gl_type = lc_gpu_shader_type_to_gl(type);
    gl_shader = glCreateShader(gl_type);

    glShaderSource(gl_shader, 1, &source, NULL);
    glCompileShader(gl_shader);

    glGetShaderiv(gl_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(gl_shader, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "lc_gpu_compile_shader: compilation failed:\n%s\n", info_log);
        glDeleteShader(gl_shader);
        return 0;
    }

    shader_registry[shader_count] = gl_shader;
    shader_count++;

    return (lc_gpu_shader_t)(shader_count - 1);
}

lc_gpu_program_t lc_gpu_link_program(lc_gpu_shader_t vertex, lc_gpu_shader_t fragment) {
    GLuint gl_program;
    GLuint gl_vertex;
    GLuint gl_fragment;
    GLint success;
    GLchar info_log[512];

    if (program_count >= LC_GPU_MAX_PROGRAMS) {
        fprintf(stderr, "lc_gpu_link_program: program registry full\n");
        return 0;
    }

    if (vertex == 0 || vertex >= (lc_gpu_shader_t)shader_count) {
        fprintf(stderr, "lc_gpu_link_program: invalid vertex shader handle %u\n", vertex);
        return 0;
    }

    if (fragment == 0 || fragment >= (lc_gpu_shader_t)shader_count) {
        fprintf(stderr, "lc_gpu_link_program: invalid fragment shader handle %u\n", fragment);
        return 0;
    }

    gl_vertex = shader_registry[vertex];
    gl_fragment = shader_registry[fragment];

    if (gl_vertex == 0 || gl_fragment == 0) {
        fprintf(stderr, "lc_gpu_link_program: one or both shaders are destroyed\n");
        return 0;
    }

    gl_program = glCreateProgram();
    glAttachShader(gl_program, gl_vertex);
    glAttachShader(gl_program, gl_fragment);
    glLinkProgram(gl_program);

    glGetProgramiv(gl_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(gl_program, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "lc_gpu_link_program: linking failed:\n%s\n", info_log);
        glDeleteProgram(gl_program);
        return 0;
    }

    program_registry[program_count] = gl_program;
    program_count++;

    return (lc_gpu_program_t)(program_count - 1);
}

int lc_gpu_get_uniform_location(lc_gpu_program_t prog, const char *name) {
    GLuint gl_program;

    if (prog == 0 || prog >= (lc_gpu_program_t)program_count) {
        fprintf(stderr, "lc_gpu_get_uniform_location: invalid program handle %u\n", prog);
        return -1;
    }

    gl_program = program_registry[prog];
    if (gl_program == 0) {
        fprintf(stderr, "lc_gpu_get_uniform_location: program handle %u is destroyed\n", prog);
        return -1;
    }

    if (name == NULL) {
        fprintf(stderr, "lc_gpu_get_uniform_location: name is NULL\n");
        return -1;
    }

    return glGetUniformLocation(gl_program, name);
}

void lc_gpu_destroy_shader(lc_gpu_shader_t shader) {
    GLuint gl_shader;

    if (shader == 0 || shader >= (lc_gpu_shader_t)shader_count) {
        return;
    }

    gl_shader = shader_registry[shader];
    if (gl_shader != 0) {
        glDeleteShader(gl_shader);
        shader_registry[shader] = 0;
    }
}

void lc_gpu_destroy_program(lc_gpu_program_t prog) {
    GLuint gl_program;

    if (prog == 0 || prog >= (lc_gpu_program_t)program_count) {
        return;
    }

    gl_program = program_registry[prog];
    if (gl_program != 0) {
        glDeleteProgram(gl_program);
        program_registry[prog] = 0;
    }
}

/* VAO management */
lc_gpu_vao_t lc_gpu_create_vao(void) {
    GLuint gl_vao;

    if (vao_count >= LC_GPU_MAX_VAOS) {
        fprintf(stderr, "lc_gpu_create_vao: VAO registry full\n");
        return 0;
    }

    glGenVertexArrays(1, &gl_vao);

    vao_registry[vao_count] = gl_vao;
    vao_count++;

    return (lc_gpu_vao_t)(vao_count - 1);
}

void lc_gpu_destroy_vao(lc_gpu_vao_t vao) {
    GLuint gl_vao;

    if (vao == 0 || vao >= (lc_gpu_vao_t)vao_count) {
        return;
    }

    gl_vao = vao_registry[vao];
    if (gl_vao != 0) {
        glDeleteVertexArrays(1, &gl_vao);
        vao_registry[vao] = 0;
    }
}

/* Texture management */
lc_gpu_texture_t lc_gpu_create_texture_2d(int width, int height, int internal_format, int format, int type, const void *data) {
    GLuint gl_texture;

    if (texture_count >= LC_GPU_MAX_TEXTURES) {
        fprintf(stderr, "lc_gpu_create_texture_2d: texture registry full\n");
        return 0;
    }

    glGenTextures(1, &gl_texture);
    glBindTexture(GL_TEXTURE_2D, gl_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)internal_format, (GLsizei)width, (GLsizei)height, 0, (GLenum)format, (GLenum)type, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    texture_registry[texture_count] = gl_texture;
    texture_count++;

    return (lc_gpu_texture_t)(texture_count - 1);
}

void lc_gpu_destroy_texture(lc_gpu_texture_t tex) {
    GLuint gl_texture;

    if (tex == 0 || tex >= (lc_gpu_texture_t)texture_count) {
        return;
    }

    gl_texture = texture_registry[tex];
    if (gl_texture != 0) {
        glDeleteTextures(1, &gl_texture);
        texture_registry[tex] = 0;
    }
}

/* FBO management */
lc_gpu_fbo_t lc_gpu_create_fbo(void) {
    GLuint gl_fbo;

    if (fbo_count >= LC_GPU_MAX_FBOS) {
        fprintf(stderr, "lc_gpu_create_fbo: FBO registry full\n");
        return 0;
    }

    glGenFramebuffers(1, &gl_fbo);

    fbo_registry[fbo_count].fbo = gl_fbo;
    fbo_registry[fbo_count].depth_rbo = 0;
    fbo_count++;

    return (lc_gpu_fbo_t)(fbo_count - 1);
}

void lc_gpu_fbo_attach_texture(lc_gpu_fbo_t fbo, lc_gpu_texture_t tex) {
    GLuint gl_fbo;
    GLuint gl_texture;

    if (fbo == 0 || fbo >= (lc_gpu_fbo_t)fbo_count) {
        fprintf(stderr, "lc_gpu_fbo_attach_texture: invalid FBO handle %u\n", fbo);
        return;
    }

    if (tex == 0 || tex >= (lc_gpu_texture_t)texture_count) {
        fprintf(stderr, "lc_gpu_fbo_attach_texture: invalid texture handle %u\n", tex);
        return;
    }

    gl_fbo = fbo_registry[fbo].fbo;
    gl_texture = texture_registry[tex];

    if (gl_fbo == 0) {
        fprintf(stderr, "lc_gpu_fbo_attach_texture: FBO handle %u is destroyed\n", fbo);
        return;
    }

    if (gl_texture == 0) {
        fprintf(stderr, "lc_gpu_fbo_attach_texture: texture handle %u is destroyed\n", tex);
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_texture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void lc_gpu_fbo_attach_depth(lc_gpu_fbo_t fbo, int width, int height) {
    GLuint gl_fbo;
    GLuint gl_rbo;

    if (fbo == 0 || fbo >= (lc_gpu_fbo_t)fbo_count) {
        fprintf(stderr, "lc_gpu_fbo_attach_depth: invalid FBO handle %u\n", fbo);
        return;
    }

    gl_fbo = fbo_registry[fbo].fbo;

    if (gl_fbo == 0) {
        fprintf(stderr, "lc_gpu_fbo_attach_depth: FBO handle %u is destroyed\n", fbo);
        return;
    }

    /* Delete existing depth renderbuffer if present */
    if (fbo_registry[fbo].depth_rbo != 0) {
        glDeleteRenderbuffers(1, &fbo_registry[fbo].depth_rbo);
    }

    glGenRenderbuffers(1, &gl_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, gl_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, (GLsizei)width, (GLsizei)height);

    glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gl_rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    fbo_registry[fbo].depth_rbo = gl_rbo;
}

int lc_gpu_fbo_check_complete(lc_gpu_fbo_t fbo) {
    GLuint gl_fbo;
    GLenum status;

    if (fbo == 0 || fbo >= (lc_gpu_fbo_t)fbo_count) {
        fprintf(stderr, "lc_gpu_fbo_check_complete: invalid FBO handle %u\n", fbo);
        return 0;
    }

    gl_fbo = fbo_registry[fbo].fbo;

    if (gl_fbo == 0) {
        fprintf(stderr, "lc_gpu_fbo_check_complete: FBO handle %u is destroyed\n", fbo);
        return 0;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo);
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "lc_gpu_fbo_check_complete: framebuffer is not complete, status=0x%X\n", status);
        return 0;
    }

    return 1;
}

void lc_gpu_destroy_fbo(lc_gpu_fbo_t fbo) {
    GLuint gl_fbo;
    GLuint gl_rbo;

    if (fbo == 0 || fbo >= (lc_gpu_fbo_t)fbo_count) {
        return;
    }

    gl_fbo = fbo_registry[fbo].fbo;
    gl_rbo = fbo_registry[fbo].depth_rbo;

    if (gl_fbo != 0) {
        glDeleteFramebuffers(1, &gl_fbo);
        fbo_registry[fbo].fbo = 0;
    }

    if (gl_rbo != 0) {
        glDeleteRenderbuffers(1, &gl_rbo);
        fbo_registry[fbo].depth_rbo = 0;
    }
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

/* Convert lc_gpu_buffer_usage_t to OpenGL enum */
static GLenum lc_gpu_buffer_usage_to_gl(lc_gpu_buffer_usage_t usage) {
    switch (usage) {
        case LC_GPU_STATIC:
            return GL_STATIC_DRAW;
        case LC_GPU_DYNAMIC:
            return GL_DYNAMIC_DRAW;
        case LC_GPU_STREAM:
            return GL_STREAM_DRAW;
        default:
            return GL_STATIC_DRAW;
    }
}

/* Convert lc_gpu_shader_type_t to OpenGL enum */
static GLenum lc_gpu_shader_type_to_gl(lc_gpu_shader_type_t type) {
    switch (type) {
        case LC_GPU_VERTEX_SHADER:
            return GL_VERTEX_SHADER;
        case LC_GPU_FRAGMENT_SHADER:
            return GL_FRAGMENT_SHADER;
        default:
            return GL_VERTEX_SHADER;
    }
}






