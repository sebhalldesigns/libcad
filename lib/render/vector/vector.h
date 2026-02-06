/***************************************************************
**
** libcad Header File
**
** File         :  vector.h
** Module       :  render/vector
** Author       :  SH
** Created      :  2026-02-06 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad vector API
**
***************************************************************/

#ifndef LIBCAD_VECTOR_H
#define LIBCAD_VECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include <cglm/cglm.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

bool vector_init(void);

void vector_render(int width, int height);

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_VECTOR_H */