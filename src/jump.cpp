
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
    uint64_t tck;
    AltCalc ac;
    AltJmp jmp;
    AltSqBig sq;
    bool ok = false;
    uint8_t page = 0;

    typedef struct {
        int alt;
        bool ismode;
        bool mchg;
    } log_t;
    ring<log_t, 3*60*10> log;
    int lmin = 0, lmax = 0;

    View _w = View([this](U8G2 &u8g2) { draw(u8g2); });

    Btn _b = Btn(
        Btn::Hnd(displayLightTgl),
        Btn::Hnd([](){ powerStart(false); }),
        Btn::Hnd([this](){ page++; page %= 2; })
    );

    const char *strtm(char *s, uint32_t tm) {
        tm /= 1000;
        auto sec = tm % 60;
        tm -= sec;

        if (tm <= 0) {
            snprintf_P(s, 20, PSTR("%d s"), sec);
            return s;
        }

        tm /= 60;
        auto min = tm % 60;
        tm -= min;

        if (tm <= 0) {
            snprintf_P(s, 20, PSTR("%d:%02d"), min, sec);
            return s;
        }

        snprintf_P(s, 20, PSTR("%d:%02d:%02d"), tm/60, min, sec);
        return s;
    }

    int16_t alt() const {
        int16_t alt = round(ac.avg().alt());
        int16_t o = alt % ALT_STEP;
        alt -= o;
        if (abs(o) > ALT_STEP_ROUND) alt+= o >= 0 ? ALT_STEP : -ALT_STEP;
        return alt;
    }

    void draw(U8G2 &u8g2) {
        drawBatt(u8g2);

        char s[30], t[20];

        auto m = jmp.mode();
        const char *ms =
            m == AltJmp::INIT       ? PSTR("INIT") :
            m == AltJmp::GROUND     ? PSTR("GND") :
            m == AltJmp::TAKEOFF    ? PSTR("TOFF") :
            m == AltJmp::FREEFALL   ? PSTR("FF") :
            m == AltJmp::CANOPY     ? PSTR("CNP") : PSTR("-");
        
        switch (page) {
            case 0: {
                // alt
                u8g2.setFont(u8g2_font_logisoso62_tn);
                str("%d", alt());
                u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), 80, s);

                // info
                u8g2.setFont(u8g2_font_6x13B_tr);

                uint8_t y = 40;
                str("%.1f m/s", ac.speedapp());
                u8g2.drawStr(0, y, s);

                y += u8g2.getAscent()+2;
                str("%s (%s)", ms, strtm(t, jmp.tm()));
                u8g2.drawStr(0, y, s);

                y += u8g2.getAscent()+2;
                if (jmp.newtm() > 0) {
                    str("new: %s", strtm(t, jmp.newtm()));
                    u8g2.drawStr(0, y, s);
                }
            }
            break;
            case 1: {
                // alt
                u8g2.setFont(u8g2_font_fub20_tr);

                int y = u8g2.getAscent();
                str("%d", alt());
                u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);

                u8g2.setFont(u8g2_font_6x13B_tr);

                // info
                y += u8g2.getAscent()+5;
                str("sq: %0.1f (%s)", sq.val(), strtm(t, sq.tm()));
                u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);

                y += u8g2.getAscent()+2;
                str("%s (%s)", ms, strtm(t, jmp.tm()));
                u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);

                y += u8g2.getAscent()+2;
                str("%.1f m/s", ac.speedapp());
                u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);

                // вертикальная шкала
                u8g2.drawLine(0, 0, 5, 0);
                u8g2.drawLine(0, u8g2.getDisplayHeight()-1, 5, u8g2.getDisplayHeight()-1);
                u8g2.drawLine(0, u8g2.getDisplayHeight()/2, 5, u8g2.getDisplayHeight()/2);

                str("%d", lmin);
                u8g2.drawStr(5, u8g2.getDisplayHeight()-1, s);
                str("%d", lmax);
                u8g2.drawStr(5, u8g2.getAscent(), s);
                str("%d", lmin+(lmax-lmin)/2);
                u8g2.drawStr(5, (u8g2.getDisplayHeight()+u8g2.getAscent()-1)/2, s);

                // рисуем графики
#define LPADD 12
                double dx = static_cast<double>(u8g2.getDisplayWidth()-LPADD) / log.capacity();
                double xd = u8g2.getDisplayWidth();
                double dy = static_cast<double>(u8g2.getDisplayHeight()) / (lmax-lmin);
                for (const auto &l : log) {
                    // точки графика
                    int x = static_cast<int>(round(xd));
                    int y = static_cast<int>(round(dy * (l.alt - lmin)));
                    if ((y >= 0) && (y < u8g2.getDisplayHeight())) {
                        y = u8g2.getDisplayHeight()-y-1;
                        u8g2.drawPixel(x, y);
                    
                        // высчитанное место изменения режима
                        if (l.ismode)
                            u8g2.drawDisc(x, y, 2);
                    }
                    
                    if (l.mchg)
                        // точка изменения режима (принятия реешения)
                        for (int y = 0; y < u8g2.getDisplayHeight(); y+=3)
                            u8g2.drawPixel(x, y);

                    xd -= dx;
                }
            }
            break;
        }
    }

    void drawBatt(U8G2 &u8g2) {
        auto blev = pwrBattLevel();
            u8g2.setFont(u8g2_font_battery19_tn);
            u8g2.setFontDirection(1);
            u8g2.drawGlyph(50, 0, '0' + blev);
            u8g2.setFontDirection(0);
        if (pwrBattCharge()) {
            u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
            u8g2.drawGlyph(40, 9, 'C');
        }
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
        auto m = jmp.mode();
        jmp.tick(ac);
        const bool chgmode = m != jmp.mode();
        sq.tick(ac);

        // добавление в log
        log.push({ ac.alt(), false, chgmode });
        if (chgmode && (jmp.cnt() < log.size()))
            log[jmp.cnt()].ismode = true;

        lmin = log[0].alt;
        lmax = log[0].alt;
        for (const auto &l : log) {
            if (lmin > l.alt)
                lmin = l.alt;
            if (lmax < l.alt)
                lmax = l.alt;
        }

        auto y = lmin - (lmin % 300);
        if (y > lmin+5) y -= 300; // +5, чтобы случайные -1-2m не уводили в -300
        lmin = y;
        for (; y <= 15000; y += 300)
            if (y > lmax) break;
        lmax = y;
        // -------------------

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
