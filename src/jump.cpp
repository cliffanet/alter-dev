
#include "jump.h"
#include "core/worker.h"
#include "core/clock.h"
#include "core/btn.h"
#include "core/view.h"
#include "core/log.h"
#include "display.h"
#include "power.h"

#include "altcalc.h"
#include <Adafruit_BMP280.h>

class _jmpWrk : public Wrk {
    Adafruit_BMP280 bmp = Adafruit_BMP280(PINBMP);
    AltCalc ac;
    uint64_t tck;
    bool ok = false;

    View _w = View([this](U8G2 &u8g2) { draw(u8g2); });

    Btn _b = Btn(
        Btn::Hnd(displayLightTgl),
        Btn::Hnd([](){ powerStart(false); }),
        Btn::Hnd(NULL)
    );

    void draw(U8G2 &u8g2) {
        const auto a = ac.avg();
        char s[20];
        
        u8g2.setFont(u8g2_font_logisoso62_tn);
        int16_t alt = round(a.alt());
        int16_t o = alt % ALT_STEP;
        alt -= o;
        if (abs(o) > ALT_STEP_ROUND) alt+= o >= 0 ? ALT_STEP : -ALT_STEP;

        snprintf_P(s, sizeof(s), PSTR("%d"), alt);
        u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s)-20, 80, s);

        u8g2.setFont(u8g2_font_ncenB08_tr);
        sprintf_P(s, PSTR("%.0f km/h"), round(abs(ac.speedapp() * 3.6)));
        u8g2.drawStr(10, u8g2.getDisplayHeight()-1, s);
    }

public:
    _jmpWrk() {
        if (!(ok = bmp.begin())) {   
            CONSOLE("Could not find a valid BMP280 sensor, check wiring!");
        }
        tck = utick();
    }
#ifdef FWVER_DEBUG
    ~_jmpWrk() {
        CONSOLE("(0x%08x) destroy", this);
        bmp.setSampling(Adafruit_BMP280::MODE_SLEEP);
    }
#endif

    state_t run() {
        if (!ok)
            return END;
        
        auto interval = utm_diff(tck, tck) / 1000;
        ac.tick(bmp.readPressure(), interval);

        //CONSOLE("alt: %0.1f", ac.alt());

        return DLY;
    }
};

static WrkProc<_jmpWrk> _jmp;

bool jumpStart() {
    if (_jmp.isrun())
        return false;
    
    _jmp = wrkRun<_jmpWrk>();

    return false;
}

bool jumpStop() {
    if (!_jmp.isrun())
        return false;
    
    _jmp.term();

    return false;
}
