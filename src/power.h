/*
    Power functions
*/

#ifndef _power_H
#define _power_H

#include "../def.h"
#include <stdint.h>

#define HWPOWER_PIN_BATIN   36
#define HWPOWER_PIN_BATCHRG 12

void pwrBattInit();
uint16_t pwrBattRaw();
double pwrBattValue();
uint8_t pwrBattLevel();
bool pwrBattCharge();

bool powerStart(bool pwron);
bool powerStop();
void powerSleep();

#endif // _power_H
