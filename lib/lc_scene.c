/***************************************************************
**
** libcad Source File
**
** File         :  lc_scene.c
** Module       :  lc_scene
** Author       :  SH
** Created      :  2026-01-26 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad internal scene API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "lc_scene.h"

#include "lc_canvas.h"
#include "lc_draw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#else
#include <glad/glad.h>
#endif 

#include <cglm/cglm.h>


#include <libcad/libcad.h>



/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef struct 
{
    vec3 position;
    vec3 target;
    vec3 up;
} gm_camera_t;

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

#if __EMSCRIPTEN__

extern uint8_t resources_shaders_basic_basic_es_vs_glsl[];
extern uint32_t resources_shaders_basic_basic_es_vs_glsl_size;

extern uint8_t resources_shaders_basic_basic_es_fs_glsl[];
extern uint32_t resources_shaders_basic_basic_es_fs_glsl_size;

#else

extern uint8_t resources_shaders_basic_basic_core_vs_glsl[];
extern uint32_t resources_shaders_basic_basic_core_vs_glsl_size;

extern uint8_t resources_shaders_basic_basic_core_fs_glsl[];
extern uint32_t resources_shaders_basic_basic_core_fs_glsl_size;

#endif

static GLuint program = 0;
static GLuint VAO = 0;
static GLuint cubeEBO = 0;
static GLuint wireEBO = 0;

static mat4 modelMatrix;
static mat4 viewMatrix;
static mat4 projectionMatrix;

static float clipNear = 0.1f;
static float clipFar = 100.0f;

static gm_camera_t cam;
static bool orbitState = false;


float cube_vertices[] = {
    /* positions */
    -1.0,-1.0,-1.0,
    +1.0,-1.0,-1.0,
    +1.0,+1.0,-1.0,
    -1.0,+1.0,-1.0,
    -1.0,-1.0,+1.0,
    +1.0,-1.0,+1.0,
    +1.0,+1.0,+1.0,
    -1.0,+1.0,+1.0,
};

unsigned int cube_indices[] = {
    0, 1, 2,   2, 3, 0,
    4, 6, 5,   6, 7, 4,
    0, 3, 7,   7, 4, 0,
    1, 5, 6,   6, 2, 1,
    3, 2, 6,   6, 7, 3,
    0, 4, 5,   5, 1, 0
};

unsigned int cube_wireframe_indices[] = {
    0,1, 1,2, 2,3, 3,0,  /* front */
    4,5, 5,6, 6,7, 7,4,  /* back */
    0,4, 1,5, 2,6, 3,7   /* sides */
};

static vec4 cursor_pos = {0.0f, 0.0f, 0.0f, 0.0f};
static bool cursor_valid = false;

static vec2 pan_start_pos = {0.0f, 0.0f};
static bool pan_active = false;

/* multiple cube positions for the scene */
#define NUM_CUBES 5
static vec3 cube_positions[NUM_CUBES] = {
    {0.0f, 0.0f, 0.0f},      /* center cube */
    {0.0f, 0.0f, 3.0f},      /* back */
    {0.0f, 0.0f, -3.0f},     /* front */
    {0.0f, 3.0f, 0.0f},      /* top */
    {0.0f, -3.0f, 0.0f}      /* bottom */
};

static vec4 cube_colors[NUM_CUBES] = {
    {0.2f, 0.3f, 0.4f, 1.0f},   /* blue-gray */
    {0.4f, 0.2f, 0.3f, 1.0f},   /* purple-gray */
    {0.3f, 0.4f, 0.2f, 1.0f},   /* green-gray */
    {0.4f, 0.3f, 0.2f, 1.0f},   /* orange-gray */
    {0.2f, 0.4f, 0.4f, 1.0f}    /* teal */
};



/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void camera_init(gm_camera_t* camera);
static void camera_pan(gm_camera_t* camera, vec2 delta);
static void camera_orbit(gm_camera_t* camera, vec2 delta);
static void camera_zoom(gm_camera_t* camera, float delta);
static float camera_distance(gm_camera_t* camera);
static void camera_right(gm_camera_t* camera, vec3 right);
static void camera_forward(gm_camera_t* camera, vec3 forward);
static void camera_view_matrix(gm_camera_t* camera, mat4 view);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void lc_scene_init()
{
    camera_init(&cam);

    printf("LC SCENE INIT\n");

#if __EMSCRIPTEN__
    char* vs_src = (char*)malloc(resources_shaders_basic_basic_es_vs_glsl_size + 1);
    memcpy(vs_src, resources_shaders_basic_basic_es_vs_glsl, resources_shaders_basic_basic_es_vs_glsl_size);
    vs_src[resources_shaders_basic_basic_es_vs_glsl_size] = '\0';

    char* fs_src = (char*)malloc(resources_shaders_basic_basic_es_fs_glsl_size + 1);
    memcpy(fs_src, resources_shaders_basic_basic_es_fs_glsl, resources_shaders_basic_basic_es_fs_glsl_size);
    fs_src[resources_shaders_basic_basic_es_fs_glsl_size] = '\0';
#else
    char* vs_src = (char*)malloc(resources_shaders_basic_basic_core_vs_glsl_size + 1);
    memcpy(vs_src, resources_shaders_basic_basic_core_vs_glsl, resources_shaders_basic_basic_core_vs_glsl_size);
    vs_src[resources_shaders_basic_basic_core_vs_glsl_size] = '\0';

    char* fs_src = (char*)malloc(resources_shaders_basic_basic_core_fs_glsl_size + 1);
    memcpy(fs_src, resources_shaders_basic_basic_core_fs_glsl, resources_shaders_basic_basic_core_fs_glsl_size);
    fs_src[resources_shaders_basic_basic_core_fs_glsl_size] = '\0';
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
    
    printf("Created shaders\n");

    /* create buffers */
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &cubeEBO);
    glGenBuffers(1, &wireEBO);

    GLuint VBO;
    glGenBuffers(1,&VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wireEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_wireframe_indices), cube_wireframe_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0); 
}

void lc_scene_render(float viewport_width, float viewport_height)
{

    glClearColor(0.5f, 0.2f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);

    /* restore depth buffer */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    /* clear depth buffer */
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);

    glm_mat4_identity(viewMatrix);
    glm_mat4_identity(projectionMatrix);
    glm_mat4_identity(modelMatrix);

    vec3 translateVec = {0.0f, 0.0f, 0.0f};
    glm_translate(modelMatrix, translateVec);

    glm_perspective(glm_rad(45.0f), (float)viewport_width / (float)viewport_height, clipNear, clipFar, projectionMatrix);

    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, (float*)modelMatrix);

    camera_view_matrix(&cam, viewMatrix);

    /* CANVAS PROJECTION -> CONVERTS FROM NDC TO WINDOW COORDS */
    mat4 canvas_translate;
    glm_mat4_identity(canvas_translate);
    glm_translate(canvas_translate, (vec3){1.0f, 1.0f, 0.0f});

    mat4 canvas_scale;
    glm_mat4_identity(canvas_scale);
    glm_scale(canvas_scale, (vec3){2.0f / viewport_width, 2.0f / viewport_height, 1.0f});

    mat4 canvas_projection;
    glm_mat4_identity(canvas_projection);
    glm_mat4_mul(canvas_scale, canvas_translate, canvas_projection);

    /* CANVAS MODEL -> MOVE CANVAS POSITION WITHIN NDC */
    mat4 canvas_model;
    glm_mat4_identity(canvas_model);
    glm_translate(canvas_model, (vec3){0.0f, 0.0f, -1.0f});

    /* CANVAS TRANSFORM -> FINAL TRANSFORM */
    mat4 canvas_transform;
    glm_mat4_identity(canvas_transform);
    glm_mat4_mul(canvas_transform, canvas_projection, canvas_transform);
    glm_mat4_mul(canvas_model, canvas_transform, canvas_transform);
    glm_mat4_mul(viewMatrix, canvas_transform, canvas_transform);
    glm_mat4_mul(projectionMatrix, canvas_transform, canvas_transform);

    lc_canvas_set_view_matrix(canvas_transform);

    mat4 view_projection;
    glm_mat4_identity(view_projection);
    glm_mat4_mul(projectionMatrix, viewMatrix, view_projection);
    lc_draw_set_view_matrix(view_projection);
    

    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, (float*)viewMatrix);
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, (float*)projectionMatrix);

    glBindVertexArray(VAO);

    /* draw all cubes */
    for (int i = 0; i < NUM_CUBES; i++)
    {
        mat4 cube_model;
        glm_mat4_identity(cube_model);
        glm_translate(cube_model, cube_positions[i]);

        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, (float*)cube_model);
        glUniform4fv(glGetUniformLocation(program, "u_color"), 1, cube_colors[i]);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
        glDrawElements(GL_TRIANGLES, sizeof(cube_indices)/sizeof(unsigned int), GL_UNSIGNED_INT, 0);

#ifndef __EMSCRIPTEN__
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0f, -1.0f);
        glLineWidth(2.0f);
#endif

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wireEBO);
        glUniform4f(glGetUniformLocation(program, "u_color"), 0.8f, 0.8f, 0.8f, 1.0f);
        glDrawElements(GL_LINES, sizeof(cube_wireframe_indices)/sizeof(unsigned int), GL_UNSIGNED_INT, 0);

#ifndef __EMSCRIPTEN__
        glDisable(GL_POLYGON_OFFSET_LINE);
#endif
    }

}

void lc_scene_set_cursor_pos(float x, float y)
{
    cursor_pos[0] = x;
    cursor_pos[1] = y;
    cursor_valid = true;

    if (pan_active)
    {
        vec2 delta;
        glm_vec2_sub(cursor_pos, pan_start_pos, delta);

        if (orbitState)
        {
            camera_orbit(&cam, delta);
        }
        else
        {
            camera_pan(&cam, delta);
        }

        
        
        glm_vec2_copy(cursor_pos, pan_start_pos);
    }
}

void lc_scene_set_cursor_lost()
{
    cursor_valid = false;
}

void lc_scene_set_cursor_button_state(int button, bool pressed)
{
    switch (button)
    {
        case MOUSE_MIDDLE_BUTTON:
        {
            if (pressed)
            {
                glm_vec2_copy(cursor_pos, pan_start_pos);
                pan_active = true;
            }
            else
            {
                pan_active = false;
            }

        } break;
    }
}

void lc_scene_set_modifier_state(int modifier, bool pressed)
{
    orbitState = (modifier == 2) ? pressed : orbitState;
}

void lc_scene_axis_delta(int axis, float delta)
{
    camera_zoom(&cam, delta);
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static void camera_init(gm_camera_t* camera)
{

    glm_vec3_zero(camera->position);
    glm_vec3_zero(camera->target);
    glm_vec3_zero(camera->up);

    camera->position[2] = 5.0f;
    camera->up[1] = 1.0f;

}

static void camera_pan(gm_camera_t* camera, vec2 delta)
{
    float sensitivity = 0.002f * camera_distance(camera);
    
    vec3 right;
    camera_right(camera, right);

    vec3 translate = {0.0f, 0.0f, 0.0f};
    translate[0] = (right[0] * delta[0] + camera->up[0] * delta[1]) * sensitivity;
    translate[1] = (right[1] * delta[0] + camera->up[1] * delta[1]) * sensitivity;
    translate[2] = (right[2] * delta[0] + camera->up[2] * delta[1]) * sensitivity;
    
    glm_vec3_add(camera->position, translate, camera->position);
    glm_vec3_add(camera->target, translate, camera->target);
}

static void camera_orbit(gm_camera_t* camera, vec2 delta)
{
    float sensitivity = 0.005f;

    float yaw = delta[0] * sensitivity * -1.0f;
    float pitch = delta[1] * sensitivity * -1.0f;

    vec3 offset;
    glm_vec3_sub(camera->position, camera->target, offset);
    float distance = glm_vec3_norm(offset);

    vec3 direction;
    glm_vec3_divs(offset, distance, direction);
    float theta = atan2f(direction[0], direction[2]);
    float phi = acosf(direction[1]);

    theta += yaw;
    phi = glm_clamp(phi + pitch, 0.01f, 3.13f); /* prevent gimbal lock */
    
    direction[0] = sinf(phi) * sinf(theta);
    direction[1] = cosf(phi);
    direction[2] = sinf(phi) * cosf(theta);

    vec3 newOffset;
    glm_vec3_scale(direction, distance, newOffset);
    glm_vec3_add(camera->target, newOffset, camera->position);
}

static void camera_zoom(gm_camera_t* camera, float delta)
{
    /* wheel > 0  → zoom in,  wheel < 0  → zoom out */
    const float factor = (delta > 0.0f) ? 0.9f : 1.1111f;  /* 10% per notch */

    vec3 forward;
    camera_forward(camera, forward);  /* target - eye (unit) */

    float cur_dist = camera_distance(camera);
    float new_dist = cur_dist * factor;
    new_dist = glm_max(new_dist, 0.01f);  /* never collapse */

    /* eye = target – forward * new_dist */
    vec3 offset;
    glm_vec3_scale(forward, new_dist, offset);
    glm_vec3_sub(camera->target, offset, camera->position);
}

static float camera_distance(gm_camera_t* camera)
{
    vec3 diff;
    glm_vec3_sub(camera->position, camera->target, diff);
    return glm_vec3_norm(diff);
}

static void camera_right(gm_camera_t* camera, vec3 right)
{
    vec3 diff;
    glm_vec3_sub(camera->position, camera->target, diff);

    glm_vec3_cross(diff, camera->up, right);
    glm_vec3_normalize(right);
}

static void camera_forward(gm_camera_t* camera, vec3 forward)
{
    glm_vec3_sub(camera->target, camera->position, forward);
    glm_vec3_normalize(forward);
}

static void camera_view_matrix(gm_camera_t* camera, mat4 view)
{
    glm_lookat(camera->position, camera->target, camera->up, view);
}
