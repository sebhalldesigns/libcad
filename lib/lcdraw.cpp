#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_DEFINE_MACROS
#define CIMGUI_USE_OPENGL3
#include <cimgui/cimgui.h>
#include <cimgui/cimgui_impl.h>

#ifndef IM_COL32_R_SHIFT
#ifdef IMGUI_USE_BGRA_PACKED_COLOR
#define IM_COL32_R_SHIFT    16
#define IM_COL32_G_SHIFT    8
#define IM_COL32_B_SHIFT    0
#define IM_COL32_A_SHIFT    24
#define IM_COL32_A_MASK     0xFF000000
#else
#define IM_COL32_R_SHIFT    0
#define IM_COL32_G_SHIFT    8
#define IM_COL32_B_SHIFT    16
#define IM_COL32_A_SHIFT    24
#define IM_COL32_A_MASK     0xFF000000
#endif
#endif
#define IM_COL32(R,G,B,A)    (((ImU32)(A)<<IM_COL32_A_SHIFT) | ((ImU32)(B)<<IM_COL32_B_SHIFT) | ((ImU32)(G)<<IM_COL32_G_SHIFT) | ((ImU32)(R)<<IM_COL32_R_SHIFT))
#define IM_COL32_WHITE       IM_COL32(255,255,255,255)  // Opaque white = 0xFFFFFFFF
#define IM_COL32_BLACK       IM_COL32(0,0,0,255)        // Opaque black
#define IM_COL32_BLACK_TRANS IM_COL32(0,0,0,0)          // Transparent black = 0x00000000

#define X(x) (x[0])
#define Y(y) (y[1])

#include <glad/glad.h>

#include <stdio.h>

#include "lcdraw.h"

static float zoom = 1.0f;

static float offsetX = 0.0f;
static float offsetY = 0.0f;
static float dragOffsetX = 0.0f;
static float dragOffsetY = 0.0f;

static int surfaceWidth = 0;
static int surfaceHeight = 0;

static vec2 origin = {0.0f, 0.0f};

static void draw_circle(vec2 center, float radius);
static void draw_grid(vec2 start, vec2 end, float spacing, uint32_t color);

static void draw_rect(vec2 center, vec2 size);
static void draw_ellipse(vec2 center, vec2 size);

struct ImGuiContext* ig_context = NULL;
struct ImGuiIO* ig_io = NULL;
struct ImDrawList* ig_drawlist = NULL;

void make_context()
{

    ig_context = igCreateContext(NULL);
    ig_io = igGetIO_Nil();

    const char* glsl_version = "#version 330 core";
    ImGui_ImplOpenGL3_Init(glsl_version);

}

void render(int x, int y, int vpw, int vph, int w, int h)
{
    ig_io->DisplaySize = ImVec2_c{(float)vpw, (float)vph};
    ig_io->DeltaTime = 1.0f;

    
    origin[0] = (vpw / 2.0f) + offsetX + dragOffsetX;
    origin[1] = (vph / 2.0f) + offsetY + dragOffsetY;

    ImGui_ImplOpenGL3_NewFrame();
    igNewFrame();

   ig_drawlist = igGetForegroundDrawList_ViewportPtr(igGetMainViewport());

    /* DRAW MINOR GRID */

    /* minor grid spacing from 5 to 50 px*/

    const float major_minor_ratio = 5.0f;
    
    const float minor_min = 10.0f;
    const float minor_max = 50.0f;
    const float base_spacing = 25.0f;

    float minor_spacing = zoom * base_spacing;

    while (minor_spacing < minor_min)
    {
        minor_spacing *= major_minor_ratio;
    }

    while (minor_spacing > minor_max)
    {
        minor_spacing /= major_minor_ratio;
    }
    
    vec2 minor_start = {fmodf(origin[0], minor_spacing) - minor_spacing, fmodf(origin[1], minor_spacing) - minor_spacing};
    vec2 grid_end = {(float)vpw, (float)vph};

    draw_grid(minor_start, grid_end, minor_spacing, IM_COL32(255, 255, 255, 13));

    /* DRAW MAJOR GRID */
    float major_spacing = minor_spacing * major_minor_ratio;
    vec2 major_start = {fmodf(origin[0], major_spacing) - major_spacing, fmodf(origin[1], major_spacing) - major_spacing};
    draw_grid(major_start, grid_end, major_spacing, IM_COL32(255, 255, 255, 25));

    /* draw axes */
    ImDrawList_AddLine(ig_drawlist, ImVec2_c{0, origin[1]}, ImVec2_c{(float)w, origin[1]}, IM_COL32(255, 255, 255, 150), 1.0f);
    ImDrawList_AddLine(ig_drawlist, ImVec2_c{origin[0], 0}, ImVec2_c{origin[0], (float)h}, IM_COL32(255, 255, 255, 150), 1.0f);

    draw_circle(origin, 4.0f);

    vec2 rect_origin = {150.0f, 100.0f};
    vec2 rect_size = {200.0f, 100.0f};

    rect_origin[0] *= zoom;
    rect_origin[1] *= zoom;

    rect_origin[0] += origin[0];
    rect_origin[1] += origin[1];

    rect_size[0] *= zoom;
    rect_size[1] *= zoom;

    draw_rect(rect_origin, rect_size);

    vec2 oval_origin = {-150.0f, 100.0f};
    vec2 oval_size = {100.0f, 200.0f};

    oval_origin[0] *= zoom;
    oval_origin[1] *= zoom;

    oval_origin[0] += origin[0];
    oval_origin[1] += origin[1];

    oval_size[0] *= zoom;
    oval_size[1] *= zoom;

    draw_ellipse(oval_origin, oval_size);

    igRender();
    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

}


void set_zoom(float z)
{
    zoom = z;
}

void set_offset(vec2 vec)
{
    offsetX = vec[0];
    offsetY = vec[1];
}




static void draw_circle(vec2 center, float radius)
{
    ImDrawList_AddCircle(ig_drawlist, ImVec2_c{center[0], center[1]}, radius, IM_COL32(200, 200, 200, 255), 12, 1.0f);
}


static void draw_grid(vec2 start, vec2 end, float spacing, uint32_t color)
{

    for (float x = start[0]; x <= end[0]; x += spacing)
    {
        ImDrawList_AddLine(ig_drawlist, ImVec2_c{x, start[1]}, ImVec2_c{x, end[1]}, color, 1.0f);
    }

    for (float y = start[1]; y <= end[1]; y += spacing)
    {
        ImDrawList_AddLine(ig_drawlist, ImVec2_c{start[0], y}, ImVec2_c{end[0], y}, color, 1.0f);

    }
}


static void draw_rect(vec2 center, vec2 size)
{

    ImDrawList_AddRectFilled(ig_drawlist, ImVec2_c{center[0] - size[0] / 2.0f, center[1] - size[1] / 2.0f}, ImVec2_c{center[0] + size[0] / 2.0f, center[1] + size[1] / 2.0f}, IM_COL32(255, 255, 255, 25), 0.0f, 0);
    ImDrawList_AddRect(ig_drawlist, ImVec2_c{center[0] - size[0] / 2.0f, center[1] - size[1] / 2.0f}, ImVec2_c{center[0] + size[0] / 2.0f, center[1] + size[1] / 2.0f}, IM_COL32(255, 255, 255, 150), 0.0f, 0, 2.0f);



    draw_circle(center, 4.0f);

    vec2 corners[4];
    corners[0][0] = center[0] - size[0] / 2.0f;
    corners[0][1] = center[1] - size[1] / 2.0f;
    corners[1][0] = center[0] + size[0] / 2.0f;
    corners[1][1] = center[1] - size[1] / 2.0f;
    corners[2][0] = center[0] - size[0] / 2.0f;
    corners[2][1] = center[1] + size[1] / 2.0f;
    corners[3][0] = center[0] + size[0] / 2.0f;
    corners[3][1] = center[1] + size[1] / 2.0f;

    draw_circle(corners[0], 4.0f);
    draw_circle(corners[1], 4.0f);
    draw_circle(corners[2], 4.0f);
    draw_circle(corners[3], 4.0f);


}


static void draw_ellipse(vec2 center, vec2 size)
{

    ImDrawList_AddEllipseFilled(ig_drawlist, ImVec2_c{center[0], center[1]}, ImVec2_c{size[0] / 2.0f, size[1] / 2.0f}, IM_COL32(255, 255, 255, 25), 0.0f, 0);


    ImDrawList_AddEllipse(ig_drawlist, ImVec2_c{center[0], center[1]}, ImVec2_c{size[0] / 2.0f, size[1] / 2.0f}, IM_COL32(255, 255, 255, 150), 0.0f, 0, 2.0f);
    
    draw_circle(center, 4.0f);

    vec2 corners[4];
    corners[0][0] = center[0];
    corners[0][1] = center[1] - size[1] / 2.0f;
    corners[1][0] = center[0] + size[0] / 2.0f;
    corners[1][1] = center[1];
    corners[2][0] = center[0];
    corners[2][1] = center[1] + size[1] / 2.0f;
    corners[3][0] = center[0] - size[0] / 2.0f;
    corners[3][1] = center[1];

    draw_circle(corners[0], 4.0f);
    draw_circle(corners[1], 4.0f);
    draw_circle(corners[2], 4.0f);
    draw_circle(corners[3], 4.0f);

}
