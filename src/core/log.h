/*
    Log
*/

#ifndef _log_H
#define _log_H

#include "../../def.h"

#ifdef FWVER_DEBUG

#include <pgmspace.h>   // PSTR()

const char * pathToFileName_P(const char * path);
#define LOG_FMT(s)        PSTR(" [%s:%u] %s(): " s), pathToFileName_P(PSTR(__FILE__)), __LINE__, __FUNCTION__
//#define LOG_FMT(s)        PSTR(" [%s:%u] %s(): " s), __FILE__, __LINE__, __FUNCTION__

void conslog_P(const char *s, ...);
#define CONSOLE(txt, ...) conslog_P(LOG_FMT(txt), ##__VA_ARGS__)

#else // FWVER_DEBUG

#define CONSOLE(txt, ...)

#endif // FWVER_DEBUG

#define ESPDO(func, ...)    do { \
        CONSOLE(TOSTRING(func)); \
        esp_err_t err = func; \
        if (err != ESP_OK) { \
            CONSOLE(TOSTRING(func) ": [%d] %s", err, esp_err_to_name(err)); \
            __VA_ARGS__; \
        } \
    } while (0)


#endif // _log_H
