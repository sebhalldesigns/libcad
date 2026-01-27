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
    vec3 center;    float type;           /* type stored as float, cast in shader */

    vec3 axis_x;     float radius;        /* for circle: radius in plane units (same as vP) */
    vec3 axis_y;     float corner_radius; /* for rounded rect: corner radius in plane units */

    vec2 half_size;   float thickness;   float filled; /* filled: 1.0 fill, 0.0 stroke-only */
    vec4 color;                           /* premultiplied or straight alpha; your choice */
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
static int check_shader_compile(GLuint shader, const char* name);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

int lc_draw_init()
{
    /* initialize view_projection to identity as safe default */
    glm_mat4_identity(view_projection);

    ig_context = igCreateContext(NULL);
    ig_io = igGetIO_Nil();

    #ifdef EMSCRIPTEN
    const char* glsl_version = "#version 300 es";
    #else
    const char* glsl_version = "#version 330 core";
    #endif
    ImGui_ImplOpenGL3_Init(glsl_version);

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

    if (!check_shader_compile(vs, "vertex"))
    {
        free(vs_src);
        free(fs_src);
        return 0;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, (const GLchar**)&fs_src, NULL);
    glCompileShader(fs);

    if (!check_shader_compile(fs, "fragment"))
    {
        free(vs_src);
        free(fs_src);
        return 0;
    }

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

    /* unit quad [-1,1]^2 as two triangles */
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
    ig_io->DisplaySize = (ImVec2_c){(float)w, (float)h};
    ig_io->DeltaTime = 1.0f;

    ImGui_ImplOpenGL3_NewFrame();
    igNewFrame();

    ig_drawlist = igGetForegroundDrawList_ViewportPtr(igGetMainViewport());
    

}

void lc_draw_end()
{
    igRender();
    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

    ig_drawlist = NULL;
}

void lc_draw_line(vec2 start, vec2 end, uint32_t color)
{
    ImDrawList_AddLine(ig_drawlist, (ImVec2_c){start[0], start[1]}, (ImVec2_c){end[0], end[1]}, color, 1.0f);
}

void lc_draw_circle(vec2 center, float radius)
{
    ImDrawList_AddCircle(ig_drawlist, (ImVec2_c){center[0], center[1]}, radius, IM_COL32(200, 200, 200, 255), 0, 1.0f);
}


void lc_draw_grid(vec2 start, vec2 end, float spacing, uint32_t color)
{

    for (float x = start[0]; x <= end[0]; x += spacing)
    {
        ImDrawList_AddLine(ig_drawlist, (ImVec2_c){x, start[1]}, (ImVec2_c){x, end[1]}, color, 1.0f);
    }

    for (float y = start[1]; y <= end[1]; y += spacing)
    {
        ImDrawList_AddLine(ig_drawlist, (ImVec2_c){start[0], y}, (ImVec2_c){end[0], y}, color, 1.0f);

    }
}

void lc_draw_rect(vec2 start, vec2 end, uint32_t color)
{
    ImDrawList_AddRect(ig_drawlist, 
        (ImVec2_c){start[0], start[1]}, 
        (ImVec2_c){end[0], end[1]},
        color,
        0.0f, 0, 1.0f
    );
}

void lc_draw_rect_filled(vec2 start, vec2 end, uint32_t color)
{

    ImDrawList_AddRectFilled(ig_drawlist, 
        (ImVec2_c){start[0], start[1]}, 
        (ImVec2_c){end[0], end[1]},
        color,
        0.0f, 0
    );

}


void lc_draw_ellipse(vec2 center, vec2 size, uint32_t color)
{
    ImDrawList_AddEllipse(ig_drawlist, 
        (ImVec2_c){center[0], center[1]}, 
        (ImVec2_c){size[0] / 2.0f, size[1] / 2.0f},
        color, 
        0.0f, 0, 2.0f /* setting num_segments to 0 requests that imgui decide */
    );
}

void lc_draw_handle(vec2 pos, bool active)
{
    const float HANDLE_SIZE = 5.0f;

    ImDrawList_AddCircleFilled(ig_drawlist, 
        (ImVec2_c){pos[0], pos[1]}, 
        HANDLE_SIZE, 
        active ? IM_COL32(100, 100, 255, 255) : IM_COL32(100, 255, 100, 255),
        0
    );
    
    ImDrawList_AddCircle(ig_drawlist, (ImVec2_c){pos[0], pos[1]}, HANDLE_SIZE, IM_COL32(255, 255, 255, 255), 0, 1.0f);
}

void lc_draw_text(vec2 pos, const char *text, float size, uint32_t color)
{
    ImDrawList_AddText_Vec2(ig_drawlist, 
        (ImVec2_c){pos[0], pos[1]},
        color, 
        text, 
        NULL
    );
}

void lc_draw_render(float viewport_width, float viewport_height)
{
    glUseProgram(program);

    glUniformMatrix4fv(u_view_projection, 1, GL_FALSE, (const GLfloat*)view_projection);

    /* if test? */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);

    vector_instance_t instances[2];

    /* circle - on XY plane, to the left of the cube */
    /* add padding to quad axes for stroke thickness */
    float circle_radius = 1.5f;
    float circle_padding = 0.15f;  /* extra space for stroke */
    float circle_quad_size = circle_radius + circle_padding;

    glm_vec3_copy((vec3){-3.0f, 0.0f, 0.0f}, instances[0].center);
    instances[0].type = 0.0f;  /* type 0 = circle in shader */

    glm_vec3_copy((vec3){circle_quad_size, 0.0f, 0.0f}, instances[0].axis_x);
    glm_vec3_copy((vec3){0.0f, circle_quad_size, 0.0f}, instances[0].axis_y);

    instances[0].radius = circle_radius;
    instances[0].half_size[0] = circle_quad_size;  /* drives v_plane - must match axis */
    instances[0].half_size[1] = circle_quad_size;
    instances[0].corner_radius = 0.0f;

    instances[0].thickness = 2.0f;
    instances[0].filled = 0.0f;
    glm_vec4_copy((vec4){1.0f, 0.2f, 0.2f, 1.0f}, instances[0].color);

    /* rectangle - on XY plane, to the right of the cube */
    float rect_half_w = 1.5f;
    float rect_half_h = 1.0f;
    float rect_padding = 0.15f;  /* extra space for stroke */
    float rect_quad_w = rect_half_w + rect_padding;
    float rect_quad_h = rect_half_h + rect_padding;

    glm_vec3_copy((vec3){3.0f, 0.0f, 0.0f}, instances[1].center);
    instances[1].type = 1.0f;  /* type 1 = rectangle */
    glm_vec3_copy((vec3){rect_quad_w, 0.0f, 0.0f}, instances[1].axis_x);
    glm_vec3_copy((vec3){0.0f, rect_quad_h, 0.0f}, instances[1].axis_y);
    instances[1].half_size[0] = rect_half_w;  /* SDF uses actual shape size */
    instances[1].half_size[1] = rect_half_h;
    instances[1].corner_radius = 0.2f;
    instances[1].thickness = 2.0f;
    instances[1].filled = 0.0f;
    glm_vec4_copy((vec4){0.2f, 0.5f, 1.0f, 1.0f}, instances[1].color);

    glBufferData(GL_ARRAY_BUFFER, sizeof(instances), NULL, GL_STREAM_DRAW); /* orphan */
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(instances), instances);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, sizeof(instances) / sizeof(vector_instance_t));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);

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
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, n, GL_FLOAT, GL_FALSE, stride, (void*)offset);
    glVertexAttribDivisor(loc, 1);
}

static int check_shader_compile(GLuint shader, const char* name)
{
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        fprintf(stderr, "lc_draw: %s shader compilation failed:\n%s\n", name, info_log);
        return 0;
    }
    printf("lc_draw: %s shader compiled successfully\n", name);
    return 1;
}