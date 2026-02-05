/***************************************************************
**
** libcad Source File
**
** File         :  lc_draw.c
** Module       :  lc_draw
** Author       :  SH
** Created      :  2026-01-08 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad internal drawing API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_draw.h"

#include <string.h>

#ifdef EMSCRIPTEN
    #include <GLES3/gl3.h>
#else
    #include <glad/glad.h>
#endif


/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef struct 
{
    vec3 center;    float type;          // type stored as float, cast in shader

    vec3 axis_x;     float radius;        // for circle: radius in plane units (same as vP)
    vec3 axis_y;     float corner_radius;  // for rounded rect: corner radius in plane units

    vec2 half_size;   float thickness;   float filled; // filled: 1.0 fill, 0.0 stroke-only
    vec4 color;                           // premultiplied or straight alpha; your choice
} vector_instance_t;

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

struct ImGuiContext* ig_context = NULL;
struct ImGuiIO* ig_io = NULL;
struct ImDrawList* ig_drawlist = NULL;

#if __EMSCRIPTEN__


#else

extern uint8_t resources_shaders_vector_vector_core_vs_glsl[];
extern uint32_t resources_shaders_vector_vector_core_vs_glsl_size;

extern uint8_t resources_shaders_vector_vector_core_fs_glsl[];
extern uint32_t resources_shaders_vector_vector_core_fs_glsl_size;

#endif

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vbo_quad = 0;
static GLuint vbo_instance = 0;

static GLuint u_view_projection = 0;
static GLuint u_viewport = 0;

static mat4 view_projection;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void enable_attribute(GLuint loc, GLint n, GLsizei stride, size_t offset);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

int lc_draw_init()
{
    

    
    return true;

#if __EMSCRIPTEN__
    
#else
    char* vs_src = (char*)malloc(resources_shaders_vector_vector_core_vs_glsl_size + 1);
    memcpy(vs_src, resources_shaders_vector_vector_core_vs_glsl, resources_shaders_vector_vector_core_vs_glsl_size);
    vs_src[resources_shaders_vector_vector_core_vs_glsl_size] = '\0';

    char* fs_src = (char*)malloc(resources_shaders_vector_vector_core_fs_glsl_size + 1);
    memcpy(fs_src, resources_shaders_vector_vector_core_fs_glsl, resources_shaders_vector_vector_core_fs_glsl_size);
    fs_src[resources_shaders_vector_vector_core_fs_glsl_size] = '\0';
#endif

    printf("ABOUT TO CREATE SHADERS INIT\n");

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, (const GLchar**)&vs_src, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, (const GLchar**)&fs_src, NULL);
    glCompileShader(fs);

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);

    if (!ok)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "lc_draw: Shader program linking failed: %s\n", infoLog);
        return 0;
    }
    else
    {
        printf("lc_draw: Shader program linked successfully\n");
    }
    
    u_view_projection = glGetUniformLocation(program, "u_view_projection");

    // Unit quad [-1,1]^2 as two triangles
    const float quadVerts[12] = {
        -1.f, -1.f,
         1.f, -1.f,
         1.f,  1.f,
        -1.f, -1.f,
         1.f,  1.f,
        -1.f,  1.f
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo_quad);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_quad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

    glGenBuffers(1, &vbo_instance);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STREAM_DRAW);

    const GLsizei S = (GLsizei)sizeof(vector_instance_t);

    enable_attribute(1, 3, S, offsetof(vector_instance_t, center));
    enable_attribute(2, 1, S, offsetof(vector_instance_t, type));

    enable_attribute(3, 3, S, offsetof(vector_instance_t, axis_x));
    enable_attribute(4, 1, S, offsetof(vector_instance_t, radius));

    enable_attribute(5, 3, S, offsetof(vector_instance_t, axis_y));
    enable_attribute(6, 1, S, offsetof(vector_instance_t, corner_radius));

    enable_attribute(7, 2, S, offsetof(vector_instance_t, half_size));
    enable_attribute(8, 1, S, offsetof(vector_instance_t, thickness));
    enable_attribute(9, 1, S, offsetof(vector_instance_t, filled));

    enable_attribute(10, 4, S, offsetof(vector_instance_t, color));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void lc_draw_begin(int w, int h)
{
    

}

void lc_draw_end()
{
   
}

void lc_draw_line(vec2 start, vec2 end, uint32_t color)
{
   
}

void lc_draw_circle(vec2 center, float radius)
{
    
}


void lc_draw_grid(vec2 start, vec2 end, float spacing, uint32_t color)
{

    
}

void lc_draw_rect(vec2 start, vec2 end, uint32_t color)
{
    
}

void lc_draw_rect_filled(vec2 start, vec2 end, uint32_t color)
{

   
}


void lc_draw_ellipse(vec2 center, vec2 size, uint32_t color)
{
    
}

void lc_draw_handle(vec2 pos, bool active)
{
   
}

void lc_draw_text(vec2 pos, const char *text, float size, uint32_t color)
{
   
}

void lc_draw_render(float viewport_width, float viewport_height)
{
   

}

void lc_draw_set_view_matrix(mat4 matrix)
{   
    glm_mat4_copy(matrix, view_projection);
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/


static void enable_attribute(GLuint loc, GLint n, GLsizei stride, size_t offset)
{

}
