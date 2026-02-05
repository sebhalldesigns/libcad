/***************************************************************
**
** libcad Source File
**
** File         :  log.c
** Module       :  util/log
** Author       :  SH
** Created      :  2026-02-05 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad logging API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "log.h"

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void log_info(const char* message, ...)
{
    time_t currentTime = time(0);
    struct tm* timeInfo = localtime(&currentTime);

    #ifndef EMSCRIPTEN
        printf("[%02d:%02d:%02d] ", timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
        printf("\033[0;94mINFO\033[0m ");
    #else
        printf("[%02d:%02d:%02d] INFO ", timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
    #endif

    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    printf("\n");
}

void log_warning(const char* message, ...)
{
    time_t currentTime = time(0);
    struct tm* timeInfo = localtime(&currentTime);

    #ifndef EMSCRIPTEN
        printf("[%02d:%02d:%02d] ", timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
        printf("\033[0;93mWARNING\033[0m ");
     #else
        printf("[%02d:%02d:%02d] WARNING ", timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
    #endif

    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    printf("\n");
}

void log_error(const char* message, ...)
{
    time_t currentTime = time(0);
    struct tm* timeInfo = localtime(&currentTime);

    #ifndef EMSCRIPTEN
        printf("[%02d:%02d:%02d] ", timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
        printf("\033[0;91mERROR\033[0m ");
    #else
        printf("[%02d:%02d:%02d] ERROR ", timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
    #endif

    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    printf("\n");
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/






