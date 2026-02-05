/***************************************************************
**
** libcad Header File
**
** File         :  log.h
** Module       :  util/log
** Author       :  SH
** Created      :  2026-02-05 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad logging API
**
***************************************************************/

#ifndef LIBCAD_LOG_H
#define LIBCAD_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdbool.h>
#include <stdint.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

void log_info(const char* message, ...);
void log_warning(const char* message, ...);
void log_error(const char* message, ...);

#ifdef __cplusplus
}
#endif

#endif /* LIBCAD_LOG_H */