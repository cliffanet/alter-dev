/*
    Display functions
*/

#include "display.h"

#include "esp32-hal-gpio.h"

static bool lght = false;

static void displayLightUpd() {
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
