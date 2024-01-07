/*
    Display functions
*/

#include "display.h"
#include "core/log.h"

#include "esp32-hal-gpio.h"

static bool lght = false;

static void displayLightUpd() {
    CONSOLE("lght: %d", lght);
    pinMode(LIGHT_PIN, OUTPUT);
    digitalWrite(LIGHT_PIN, lght ? LOW : HIGH);
}

void displayLightTgl() {
    lght = not lght;
    displayLightUpd();
}

bool displayLight() {
    return lght;
}
