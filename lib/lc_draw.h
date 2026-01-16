/***************************************************************
**
** libcad Header File
**
** File         :  lc_draw.h
** Module       :  lc_draw
** Author       :  SH
** Created      :  2026-01-08 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad internal drawing API
**
***************************************************************/

#ifndef LC_DRAW_H
#define LC_DRAW_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_DEFINE_MACROS
#define CIMGUI_USE_OPENGL3
#include <cimgui/cimgui.h>
#include <cimgui/cimgui_impl.h>

#include <cglm/cglm.h>

#include <stdint.h>
#include <stdbool.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

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
#define WIDTH(x) (x[2])
#define HEIGHT(y) (y[3])

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

/* init the module, return  if ok */
int lc_draw_init();

void lc_draw_begin(int w, int h);
void lc_draw_end();

void lc_draw_line(vec2 start, vec2 end, uint32_t color);
void lc_draw_circle(vec2 center, float radius);
void lc_draw_grid(vec2 start, vec2 end, float spacing, uint32_t color);
void lc_draw_ellipse(vec2 center, vec2 size);

void lc_draw_rect_filled(vec2 start, vec2 end, uint32_t color);

void lc_draw_handle(vec2 pos, bool active);

#ifdef __cplusplus
}
#endif

#endif /* LC_DRAW_H */