/***************************************************************
**
** libcad Header File
**
** File         :  window.h
** Module       :  render/window
** Author       :  SH
** Created      :  2026-02-05 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad Window API
**
***************************************************************/

#ifndef LIBCAD_WINDOW_H
#define LIBCAD_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdbool.h>

#include <cglm/cglm.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef uintptr_t window_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/


window_t window_create(const char *title, int width, int height);
void window_destroy(window_t window);

/* return false if should quit */
bool window_update();

void window_get_size(window_t window, vec2 *size);

void window_swap_buffers(window_t window);

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_WINDOW_H */