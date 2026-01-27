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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
    float object_id; float _pad[3];       /* object ID for GPU picking, padded to 16-byte alignment */
} vector_instance_t;

/* shape type enum for documentation - matches shader switch cases */
typedef enum {
    SHAPE_CIRCLE         = 0,
    SHAPE_ROUND_RECT     = 1,
    SHAPE_LINE           = 2,
    SHAPE_ARC            = 3,
    SHAPE_TRIANGLE       = 4,
    SHAPE_POLYGON        = 5,
    SHAPE_ELLIPSE        = 6,
    SHAPE_RECTANGLE      = 7
} shape_type_t;

/* object type for hover info */
typedef enum {
    OBJ_NONE             = 0,
    OBJ_2D_CIRCLE        = 1,
    OBJ_2D_ROUND_RECT    = 2,
    OBJ_2D_LINE          = 3,
    OBJ_2D_ARC           = 4,
    OBJ_2D_TRIANGLE      = 5,
    OBJ_2D_POLYGON       = 6,
    OBJ_2D_ELLIPSE       = 7,
    OBJ_2D_RECTANGLE     = 8,
    OBJ_3D_CUBE          = 100,
    OBJ_3D_SPHERE        = 101
} object_type_t;

static const char* object_type_names[] = {
    "None",
    "Circle",
    "Rounded Rectangle",
    "Line",
    "Arc",
    "Triangle",
    "Polygon",
    "Ellipse",
    "Rectangle"
};

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

extern uint8_t resources_shaders_vector_vector_pick_fs_glsl[];
extern uint32_t resources_shaders_vector_vector_pick_fs_glsl_size;

#endif

static GLuint program = 0;
static GLuint pick_program = 0;
static GLuint vao = 0;
static GLuint vbo_quad = 0;
static GLuint vbo_instance = 0;

/* picking FBO */
static GLuint pick_fbo = 0;
static GLuint pick_color_tex = 0;
static GLuint pick_depth_rbo = 0;
static int pick_fbo_width = 0;
static int pick_fbo_height = 0;

static GLuint u_view_projection = 0;
static GLuint u_pick_view_projection = 0;

static mat4 view_projection;

/* hover state */
static int hovered_object_id = 0;
static int hovered_object_type = OBJ_NONE;
static vec2 cursor_pos = {0.0f, 0.0f};
static bool cursor_valid = false;

/* test shapes storage */
#define MAX_INSTANCES 64
static vector_instance_t instances[MAX_INSTANCES];
static int instance_count = 0;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void enable_attribute(GLuint loc, GLint n, GLsizei stride, size_t offset);
static int check_shader_compile(GLuint shader, const char* name);
static int check_program_link(GLuint prog, const char* name);
static void create_pick_fbo(int width, int height);
static void setup_test_shapes(void);
static int pick_object_at(int x, int y, float vp_width, float vp_height);

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

    if (!check_program_link(program, "main"))
    {
        free(vs_src);
        free(fs_src);
        return 0;
    }

    u_view_projection = glGetUniformLocation(program, "u_view_projection");

    /* create picking shader (uses same vertex shader) */
    char* pick_fs_src = (char*)malloc(resources_shaders_vector_vector_pick_fs_glsl_size + 1);
    memcpy(pick_fs_src, resources_shaders_vector_vector_pick_fs_glsl, resources_shaders_vector_vector_pick_fs_glsl_size);
    pick_fs_src[resources_shaders_vector_vector_pick_fs_glsl_size] = '\0';

    GLuint pick_fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(pick_fs, 1, (const GLchar**)&pick_fs_src, NULL);
    glCompileShader(pick_fs);

    if (!check_shader_compile(pick_fs, "pick fragment"))
    {
        free(vs_src);
        free(fs_src);
        free(pick_fs_src);
        return 0;
    }

    pick_program = glCreateProgram();
    glAttachShader(pick_program, vs);
    glAttachShader(pick_program, pick_fs);
    glLinkProgram(pick_program);

    if (!check_program_link(pick_program, "pick"))
    {
        free(vs_src);
        free(fs_src);
        free(pick_fs_src);
        return 0;
    }

    u_pick_view_projection = glGetUniformLocation(pick_program, "u_view_projection");

    free(pick_fs_src);

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
    enable_attribute(11, 1, S, offsetof(vector_instance_t, object_id));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* setup test shapes */
    setup_test_shapes();

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
    if (instance_count == 0) return;

    /* ensure pick FBO is the right size */
    int vp_w = (int)viewport_width;
    int vp_h = (int)viewport_height;
    if (pick_fbo == 0 || pick_fbo_width != vp_w || pick_fbo_height != vp_h)
    {
        create_pick_fbo(vp_w, vp_h);
    }

    /* prepare instance buffer with highlight applied */
    vector_instance_t render_instances[MAX_INSTANCES];
    memcpy(render_instances, instances, instance_count * sizeof(vector_instance_t));

    /* highlight hovered object */
    for (int i = 0; i < instance_count; i++)
    {
        if ((int)render_instances[i].object_id == hovered_object_id && hovered_object_id != 0)
        {
            /* brighten color for hover */
            render_instances[i].color[0] = fminf(render_instances[i].color[0] + 0.3f, 1.0f);
            render_instances[i].color[1] = fminf(render_instances[i].color[1] + 0.3f, 1.0f);
            render_instances[i].color[2] = fminf(render_instances[i].color[2] + 0.3f, 1.0f);
        }
    }

    /* upload instances */
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferData(GL_ARRAY_BUFFER, instance_count * sizeof(vector_instance_t), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, instance_count * sizeof(vector_instance_t), render_instances);

    /* --- picking pass --- */
    if (cursor_valid)
    {
        hovered_object_id = pick_object_at((int)cursor_pos[0], (int)cursor_pos[1], viewport_width, viewport_height);

        /* determine object type from ID */
        hovered_object_type = OBJ_NONE;
        for (int i = 0; i < instance_count; i++)
        {
            if ((int)instances[i].object_id == hovered_object_id)
            {
                hovered_object_type = (int)instances[i].type + 1; /* type 0=circle -> OBJ_2D_CIRCLE=1 */
                break;
            }
        }
    }

    /* --- main render pass --- */
    glUseProgram(program);
    glUniformMatrix4fv(u_view_projection, 1, GL_FALSE, (const GLfloat*)view_projection);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instance_count);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void lc_draw_set_view_matrix(mat4 matrix)
{
    glm_mat4_copy(matrix, view_projection);
}

void lc_draw_set_cursor_pos(float x, float y)
{
    cursor_pos[0] = x;
    cursor_pos[1] = y;
    cursor_valid = true;
}

void lc_draw_set_cursor_lost(void)
{
    cursor_valid = false;
    hovered_object_id = 0;
    hovered_object_type = OBJ_NONE;
}

int lc_draw_get_hovered_id(void)
{
    return hovered_object_id;
}

const char* lc_draw_get_hovered_name(void)
{
    if (hovered_object_type >= OBJ_3D_CUBE)
    {
        if (hovered_object_type == OBJ_3D_CUBE) return "Cube";
        if (hovered_object_type == OBJ_3D_SPHERE) return "Sphere";
        return "3D Object";
    }

    if (hovered_object_type > 0 && hovered_object_type <= 8)
    {
        return object_type_names[hovered_object_type];
    }

    return object_type_names[0]; /* "None" */
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

static int check_program_link(GLuint prog, const char* name)
{
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char info_log[512];
        glGetProgramInfoLog(prog, 512, NULL, info_log);
        fprintf(stderr, "lc_draw: %s shader program linking failed: %s\n", name, info_log);
        return 0;
    }
    printf("lc_draw: %s shader program linked successfully\n", name);
    return 1;
}

static void create_pick_fbo(int width, int height)
{
    /* delete old FBO if it exists */
    if (pick_fbo != 0)
    {
        glDeleteFramebuffers(1, &pick_fbo);
        glDeleteTextures(1, &pick_color_tex);
        glDeleteRenderbuffers(1, &pick_depth_rbo);
    }

    pick_fbo_width = width;
    pick_fbo_height = height;

    glGenFramebuffers(1, &pick_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, pick_fbo);

    /* integer texture for object IDs */
    glGenTextures(1, &pick_color_tex);
    glBindTexture(GL_TEXTURE_2D, pick_color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, width, height, 0, GL_RED_INTEGER, GL_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pick_color_tex, 0);

    /* depth buffer */
    glGenRenderbuffers(1, &pick_depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, pick_depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pick_depth_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "lc_draw: Pick FBO not complete!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    printf("lc_draw: Created pick FBO %dx%d\n", width, height);
}

static int pick_object_at(int x, int y, float vp_width, float vp_height)
{
    if (pick_fbo == 0) return 0;

    /* render to pick FBO */
    glBindFramebuffer(GL_FRAMEBUFFER, pick_fbo);
    glViewport(0, 0, pick_fbo_width, pick_fbo_height);

    /* clear with ID = 0 */
    GLint clear_val = 0;
    glClearBufferiv(GL_COLOR, 0, &clear_val);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(pick_program);
    glUniformMatrix4fv(u_pick_view_projection, 1, GL_FALSE, (const GLfloat*)view_projection);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instance_count);

    /* read pixel at cursor (flip Y for OpenGL coords) */
    int gl_y = pick_fbo_height - y - 1;
    GLint picked_id = 0;

    if (x >= 0 && x < pick_fbo_width && gl_y >= 0 && gl_y < pick_fbo_height)
    {
        glReadPixels(x, gl_y, 1, 1, GL_RED_INTEGER, GL_INT, &picked_id);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    return picked_id;
}

static void setup_test_shapes(void)
{
    instance_count = 0;
    float padding = 0.15f;

    /* Shape 0: Circle (red) - left side */
    {
        float radius = 1.2f;
        float quad_size = radius + padding;
        glm_vec3_copy((vec3){-5.0f, 2.0f, 0.0f}, instances[instance_count].center);
        instances[instance_count].type = (float)SHAPE_CIRCLE;
        glm_vec3_copy((vec3){quad_size, 0.0f, 0.0f}, instances[instance_count].axis_x);
        glm_vec3_copy((vec3){0.0f, quad_size, 0.0f}, instances[instance_count].axis_y);
        instances[instance_count].radius = radius;
        instances[instance_count].half_size[0] = quad_size;
        instances[instance_count].half_size[1] = quad_size;
        instances[instance_count].corner_radius = 0.0f;
        instances[instance_count].thickness = 2.0f;
        instances[instance_count].filled = 0.0f;
        glm_vec4_copy((vec4){1.0f, 0.3f, 0.3f, 1.0f}, instances[instance_count].color);
        instances[instance_count].object_id = (float)(instance_count + 1);
        instance_count++;
    }

    /* Shape 1: Filled Circle (pink) */
    {
        float radius = 0.8f;
        float quad_size = radius + padding;
        glm_vec3_copy((vec3){-5.0f, -1.0f, 0.0f}, instances[instance_count].center);
        instances[instance_count].type = (float)SHAPE_CIRCLE;
        glm_vec3_copy((vec3){quad_size, 0.0f, 0.0f}, instances[instance_count].axis_x);
        glm_vec3_copy((vec3){0.0f, quad_size, 0.0f}, instances[instance_count].axis_y);
        instances[instance_count].radius = radius;
        instances[instance_count].half_size[0] = quad_size;
        instances[instance_count].half_size[1] = quad_size;
        instances[instance_count].corner_radius = 0.0f;
        instances[instance_count].thickness = 2.0f;
        instances[instance_count].filled = 1.0f;
        glm_vec4_copy((vec4){1.0f, 0.5f, 0.7f, 1.0f}, instances[instance_count].color);
        instances[instance_count].object_id = (float)(instance_count + 1);
        instance_count++;
    }

    /* Shape 2: Rounded Rectangle (blue) */
    {
        float half_w = 1.2f, half_h = 0.8f;
        float quad_w = half_w + padding, quad_h = half_h + padding;
        glm_vec3_copy((vec3){-2.5f, 2.0f, 0.0f}, instances[instance_count].center);
        instances[instance_count].type = (float)SHAPE_ROUND_RECT;
        glm_vec3_copy((vec3){quad_w, 0.0f, 0.0f}, instances[instance_count].axis_x);
        glm_vec3_copy((vec3){0.0f, quad_h, 0.0f}, instances[instance_count].axis_y);
        instances[instance_count].radius = 0.0f;
        instances[instance_count].half_size[0] = half_w;
        instances[instance_count].half_size[1] = half_h;
        instances[instance_count].corner_radius = 0.2f;
        instances[instance_count].thickness = 2.0f;
        instances[instance_count].filled = 0.0f;
        glm_vec4_copy((vec4){0.3f, 0.5f, 1.0f, 1.0f}, instances[instance_count].color);
        instances[instance_count].object_id = (float)(instance_count + 1);
        instance_count++;
    }

    /* Shape 3: Rectangle (cyan) */
    {
        float half_w = 1.0f, half_h = 0.6f;
        float quad_w = half_w + padding, quad_h = half_h + padding;
        glm_vec3_copy((vec3){-2.5f, -1.0f, 0.0f}, instances[instance_count].center);
        instances[instance_count].type = (float)SHAPE_RECTANGLE;
        glm_vec3_copy((vec3){quad_w, 0.0f, 0.0f}, instances[instance_count].axis_x);
        glm_vec3_copy((vec3){0.0f, quad_h, 0.0f}, instances[instance_count].axis_y);
        instances[instance_count].radius = 0.0f;
        instances[instance_count].half_size[0] = half_w;
        instances[instance_count].half_size[1] = half_h;
        instances[instance_count].corner_radius = 0.0f;
        instances[instance_count].thickness = 2.0f;
        instances[instance_count].filled = 1.0f;
        glm_vec4_copy((vec4){0.3f, 0.8f, 0.8f, 1.0f}, instances[instance_count].color);
        instances[instance_count].object_id = (float)(instance_count + 1);
        instance_count++;
    }

    /* Shape 4: Triangle (green) */
    {
        float size = 1.0f;
        float quad_size = size + padding;
        glm_vec3_copy((vec3){2.5f, 2.0f, 0.0f}, instances[instance_count].center);
        instances[instance_count].type = (float)SHAPE_TRIANGLE;
        glm_vec3_copy((vec3){quad_size, 0.0f, 0.0f}, instances[instance_count].axis_x);
        glm_vec3_copy((vec3){0.0f, quad_size, 0.0f}, instances[instance_count].axis_y);
        instances[instance_count].radius = 0.0f;
        instances[instance_count].half_size[0] = size;
        instances[instance_count].half_size[1] = size;
        instances[instance_count].corner_radius = 0.0f;
        instances[instance_count].thickness = 2.0f;
        instances[instance_count].filled = 0.0f;
        glm_vec4_copy((vec4){0.3f, 0.9f, 0.3f, 1.0f}, instances[instance_count].color);
        instances[instance_count].object_id = (float)(instance_count + 1);
        instance_count++;
    }

    /* Shape 5: Hexagon (orange) - 6 sided polygon */
    {
        float radius = 1.0f;
        float quad_size = radius + padding;
        glm_vec3_copy((vec3){2.5f, -1.0f, 0.0f}, instances[instance_count].center);
        instances[instance_count].type = (float)SHAPE_POLYGON;
        glm_vec3_copy((vec3){quad_size, 0.0f, 0.0f}, instances[instance_count].axis_x);
        glm_vec3_copy((vec3){0.0f, quad_size, 0.0f}, instances[instance_count].axis_y);
        instances[instance_count].radius = radius;
        instances[instance_count].half_size[0] = quad_size;
        instances[instance_count].half_size[1] = quad_size;
        instances[instance_count].corner_radius = 6.0f; /* n sides */
        instances[instance_count].thickness = 2.0f;
        instances[instance_count].filled = 1.0f;
        glm_vec4_copy((vec4){1.0f, 0.6f, 0.2f, 1.0f}, instances[instance_count].color);
        instances[instance_count].object_id = (float)(instance_count + 1);
        instance_count++;
    }

    /* Shape 6: Ellipse (purple) */
    {
        float half_w = 1.3f, half_h = 0.7f;
        float quad_w = half_w + padding, quad_h = half_h + padding;
        glm_vec3_copy((vec3){5.0f, 2.0f, 0.0f}, instances[instance_count].center);
        instances[instance_count].type = (float)SHAPE_ELLIPSE;
        glm_vec3_copy((vec3){quad_w, 0.0f, 0.0f}, instances[instance_count].axis_x);
        glm_vec3_copy((vec3){0.0f, quad_h, 0.0f}, instances[instance_count].axis_y);
        instances[instance_count].radius = 0.0f;
        instances[instance_count].half_size[0] = half_w;
        instances[instance_count].half_size[1] = half_h;
        instances[instance_count].corner_radius = 0.0f;
        instances[instance_count].thickness = 2.0f;
        instances[instance_count].filled = 0.0f;
        glm_vec4_copy((vec4){0.7f, 0.3f, 0.9f, 1.0f}, instances[instance_count].color);
        instances[instance_count].object_id = (float)(instance_count + 1);
        instance_count++;
    }

    /* Shape 7: Pentagon (yellow) - 5 sided polygon */
    {
        float radius = 0.9f;
        float quad_size = radius + padding;
        glm_vec3_copy((vec3){5.0f, -1.0f, 0.0f}, instances[instance_count].center);
        instances[instance_count].type = (float)SHAPE_POLYGON;
        glm_vec3_copy((vec3){quad_size, 0.0f, 0.0f}, instances[instance_count].axis_x);
        glm_vec3_copy((vec3){0.0f, quad_size, 0.0f}, instances[instance_count].axis_y);
        instances[instance_count].radius = radius;
        instances[instance_count].half_size[0] = quad_size;
        instances[instance_count].half_size[1] = quad_size;
        instances[instance_count].corner_radius = 5.0f; /* n sides */
        instances[instance_count].thickness = 2.0f;
        instances[instance_count].filled = 0.0f;
        glm_vec4_copy((vec4){0.9f, 0.9f, 0.2f, 1.0f}, instances[instance_count].color);
        instances[instance_count].object_id = (float)(instance_count + 1);
        instance_count++;
    }

    printf("lc_draw: Created %d test shapes\n", instance_count);
}