
#include "def.h"

#include "src/core/log.h"
#include "src/core/clock.h"
#include "src/core/worker.h"

#include "src/power.h"
#include "src/jump.h"

//------------------------------------------------------------------------------
void setup() {
#ifdef FWVER_DEBUG
    Serial.begin(115200);
#endif // FWVER_DEBUG
    pwrBattInit();
    powerStart(true);
}

void pwrinit() {
    CONSOLE("Firmware " FWVER_FILENAME "; Build Date: " __DATE__);
    jumpStart();
    
    CONSOLE("init finish");
}

//------------------------------------------------------------------------------

void loop() {
    uint64_t u = utick();
    wrkProcess(100);
    u = utm_diff(u);
    if (u < 90000)
        delay((100000-u) / 1000);
}
