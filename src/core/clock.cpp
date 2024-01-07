
#include "clock.h"

#include <esp_timer.h>
#include "soc/rtc.h"
extern "C" {
  #include <esp32/clk.h>
}

#include "esp32-hal-gpio.h" // OPEN_DRAIN

int64_t tmill() {
    return esp_timer_get_time() / 1000LL;
}

/* ------------------------------------------------------------------------------------------- *
 *  RTC-таймер, который не сбрасывается при deep sleep
 * ------------------------------------------------------------------------------------------- */
uint64_t utm() {
    return rtc_time_slowclk_to_us(rtc_time_get(), esp_clk_slowclk_cal_get());
}

/*
uint64_t utm_diff(uint64_t prev, uint64_t &curr) {
    curr = utm();
    return curr-prev;
}

uint64_t utm_diff(uint64_t prev) {
    uint64_t curr;
    return utm_diff(prev, curr);
}

uint32_t utm_diff32(uint32_t prev, uint32_t &curr) {
    curr = utm();
    return curr-prev;
}

uint32_t utm_diff32(uint32_t prev) {
    uint32_t curr;
    return utm_diff32(prev, curr);
}
*/

/*
    Есть проблема с отсчитыванием интервалов с использованием rtc_time.
    Само время в чистом виде не считается, но есть два параметра:
        1. Счётчик тиков таймера: rtc_time_get
        2. Калибровочное значение на данный момент: esp_clk_slowclk_cal_get
    Из двух этих параметров и получается в итоге время в микросекундах
    
    Счётчик rtc_time_get всегда последователен,
    однако esp_clk_slowclk_cal_get постоянно плавает, и при измерении времени
    с интервалом ~ 100мс может так случится, что предыдущее значение в мкс больше текущего
    Разница обычно не более 20000 мкс (20мс), но интервал получается отрицательным.

    В основном это происходит при использовании light sleep
    
    Поэтому, для измерения интервалов времени усложним логику.
    В качестве опорной величины будем использовать не время в мкс, а тики rtc_time_get.
    И уже к разнице тиков будем применять поправку esp_clk_slowclk_cal_get
*/

uint64_t utick() {
    return rtc_time_get();
}


uint64_t utm_diff(uint64_t utick_prev, uint64_t &utick_curr) {
    utick_curr = utick();
    return rtc_time_slowclk_to_us(utick_curr-utick_prev, esp_clk_slowclk_cal_get());
}

uint64_t utm_diff(uint64_t utick_prev) {
    uint64_t utick_curr;
    return utm_diff(utick_prev, utick_curr);
}

uint32_t utm_diff32(uint32_t utick_prev, uint32_t &utick_curr) {
    utick_curr = utick();
    return rtc_time_slowclk_to_us(utick_curr-utick_prev, esp_clk_slowclk_cal_get());
}

uint32_t utm_diff32(uint32_t utick_prev) {
    uint32_t utick_curr;
    return utm_diff32(utick_prev, utick_curr);
}
