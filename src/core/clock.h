/*
    Clock
*/

#ifndef _clock_H
#define _clock_H

#include "../../def.h"

#include <stdint.h>


/* ------------------------------------------------------------------------------------------- *
 *  операции со временем в формате tm_t
 * ------------------------------------------------------------------------------------------- */
int64_t tmill();
uint64_t utm();
uint64_t utick();
uint64_t utm_diff(uint64_t utick_prev, uint64_t &utick_curr);
uint64_t utm_diff(uint64_t utick_prev);
uint32_t utm_diff32(uint32_t utick_prev, uint32_t &utick_curr);
uint32_t utm_diff32(uint32_t utick_prev);

#endif // _clock_H
