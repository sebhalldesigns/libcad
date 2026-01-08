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

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

struct ImGuiContext* ig_context = NULL;
struct ImGuiIO* ig_io = NULL;
struct ImDrawList* ig_drawlist = NULL;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

int lc_draw_init()
{
    ig_context = igCreateContext(NULL);
    ig_io = igGetIO_Nil();

    const char* glsl_version = "#version 330 core";
    ImGui_ImplOpenGL3_Init(glsl_version);

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
    ImDrawList_AddCircle(ig_drawlist, (ImVec2_c){center[0], center[1]}, radius, IM_COL32(200, 200, 200, 255), 12, 1.0f);
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

void lc_draw_rect_filled(vec2 start, vec2 end, uint32_t color)
{

    ImDrawList_AddRectFilled(ig_drawlist, 
        (ImVec2_c){start[0], start[1]}, 
        (ImVec2_c){end[0], end[1]},
        color,
        0.0f, 0
    );

}


void lc_draw_ellipse(vec2 center, vec2 size)
{

    ImDrawList_AddEllipseFilled(ig_drawlist, 
        (ImVec2_c){center[0], center[1]}, 
        (ImVec2_c){size[0] / 2.0f, size[1] / 2.0f}, 
        IM_COL32(255, 255, 255, 25),
        0.0f, 0  /* setting num_segments to 0 requests that imgui decide */
    );

    ImDrawList_AddEllipse(ig_drawlist, 
        (ImVec2_c){center[0], center[1]}, 
        (ImVec2_c){size[0] / 2.0f, size[1] / 2.0f},
        IM_COL32(255, 255, 255, 150), 
        0.0f, 0, 2.0f /* setting num_segments to 0 requests that imgui decide */
    );
    
    lc_draw_circle(center, 4.0f);

    vec2 corners[4];
    corners[0][0] = center[0];
    corners[0][1] = center[1] - size[1] / 2.0f;
    corners[1][0] = center[0] + size[0] / 2.0f;
    corners[1][1] = center[1];
    corners[2][0] = center[0];
    corners[2][1] = center[1] + size[1] / 2.0f;
    corners[3][0] = center[0] - size[0] / 2.0f;
    corners[3][1] = center[1];

    lc_draw_circle(corners[0], 4.0f);
    lc_draw_circle(corners[1], 4.0f);
    lc_draw_circle(corners[2], 4.0f);
    lc_draw_circle(corners[3], 4.0f);

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

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/






