/*
    Power functions
*/

#include "power.h"
#include "core/worker.h"
#include "core/btn.h"
#include "core/view.h"
#include "core/log.h"

#include "jump.h"

#include "esp_sleep.h"

void pwrinit();

typedef enum {
    PWR_OFF = 0,
    PWR_SLEEP,
    PWR_ACTIVE
} power_mode_t;

static RTC_DATA_ATTR power_mode_t mode = PWR_ACTIVE; //PWR_OFF;

static void pwroff() {
    CONSOLE("goto off");
    // ждём, пока будет отпущена кнопка
    while (digitalRead(PINBTN_PWR) == LOW) delay(100);
    esp_sleep_enable_ext0_wakeup(PINBTN_PWR, 0); //1 = High, 0 = Low
    CONSOLE("Going to deep sleep now");
    jumpStop(); // для перевода BMP280 в sleep-mode
    mode = PWR_OFF;
    esp_deep_sleep_start();
    CONSOLE("This will never be printed");
}

static void pwrsleep() {
    CONSOLE("goto sleep");
    esp_sleep_enable_ext0_wakeup(PINBTN_PWR, 0); //1 = High, 0 = Low
    esp_sleep_enable_timer_wakeup(1000000);
    CONSOLE("Going to deep sleep now");
    jumpStop(); // сохраняем давление для корректного авто-пробуждения из sleep
    mode = PWR_SLEEP;
    esp_deep_sleep_start();
    CONSOLE("This will never be printed");
}


static double fmap(uint32_t in, uint32_t in_min, uint32_t in_max, double out_min, double out_max) {
    return  (out_max - out_min) * (in - in_min) / (in_max - in_min) + out_min;
}

void pwrBattInit() {
    pinMode(HWPOWER_PIN_BATIN, INPUT);
    pinMode(HWPOWER_PIN_BATCHRG, INPUT_PULLUP);
}

uint16_t pwrBattRaw() {
    return analogRead(HWPOWER_PIN_BATIN);
}
double pwrBattValue() {
    return fmap(pwrBattRaw(), 0x0000, 0x0fff, 0.12, 3.35) * 3 / 2;
    /*
        raw 2400 = 3.02v
        raw 2500 = 3.14v
        raw 2600 = 3.26v
        raw 2700 = 3.37v
        raw 2800 = 3.49v
        raw 2900 = 3.61v
        raw 3000 = 3.73v
        raw 3100 = 3.85v
        raw 3200 = 3.97v
        raw 3300 = 4.08v
        raw 3400 = 4.20v
    */
}

uint8_t pwrBattLevel() {
    uint16_t bv = pwrBattRaw();
    
    return
        bv > 3150 ? 5 :
        bv > 3050 ? 4 :
        bv > 2950 ? 3 :
        bv > 2850 ? 2 :
        bv > 2750 ? 1 :
        0;
}

bool pwrBattCharge() {
    return digitalRead(HWPOWER_PIN_BATCHRG) == LOW;
}

/* ------------------------------------------------------------------------------------------- *
 *  процессинг
 * ------------------------------------------------------------------------------------------- */
static void _reset();
class _powerWrk : public Wrk {
    const uint8_t _tm[3] = { 15, 10, 20 };
    int8_t _ti = 0;
    uint16_t _c = 0;
    bool _l = false;
    const Btn _b = Btn(
        Btn::Hnd([this](){ close(); }),
        Btn::Hnd([this](){ click(); }),
        Btn::Hnd([this](){ close(); })
    );

    void close() {
        _ti = -100;
    }

    void click() {
        CONSOLE("click: i=%d, c=%d, l=%d", _ti, _c, _l);
        if (_ti < 0) return;
        if (!_l) {
            close();
            return;
        }

        _ti++;
        _c = 0;
    }

    View _w = View([this](U8G2 &u8g2) { draw(u8g2); });

    void draw(U8G2 &u8g2) {
        _l = (_ti >= 0) && (_ti < sizeof(_tm)) && (_c > _tm[_ti]) && (_c < _tm[_ti]+40);
        if (!_l) {
            if ((_ti == 0) && ison)
                drawBatt(u8g2);
            return;
        }

        char s[20];
        u8g2.setFont(u8g2_font_fub20_tf);
        strncpy_P(s, PSTR("<--- PRESS!"), sizeof(s));
        u8g2.drawStr(10, u8g2.getDisplayHeight()/2 + 20, s);

        u8g2.setFont(u8g2_font_ncenB08_tr);
        strncpy_P(s, ison ? PSTR("to pwr ON") : PSTR("to pwr OFF"), sizeof(s));
        u8g2.drawStr(10, u8g2.getDisplayHeight()-1, s);
    }

    void drawBatt(U8G2 &u8g2) {
        u8g2.setFont(u8g2_font_battery24_tr);
        uint8_t blev = pwrBattLevel();
        if (blev > 0) blev--;
        u8g2.drawGlyph((u8g2.getDisplayWidth()-20)/2, u8g2.getDisplayHeight()/2 + 24/2, 0x30+blev);
        
        if (pwrBattCharge()) {
            u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
            u8g2.drawGlyph((u8g2.getDisplayWidth()-20)/2+22, u8g2.getDisplayHeight()/2 + 24/2, 'C');
        }
    };

public:
    const bool ison;
    _powerWrk(bool pwron) :
        ison(pwron)
    {
    }
    ~_powerWrk() {
        CONSOLE("(0x%08x) destroy", this);
    }

    state_t run() {
        if (_ti < 0) {
            // отрицательные значения _ti
            // означают ожидание завершения процесса
            _ti --;
            return _ti < -60 ? END : DLY;
        }

        if (_ti >= sizeof(_tm)/sizeof(_tm[0]))
            // положительные значения _ti
            // означают индекс для массива _tm[]
            return END;
        
        _c ++;

        if ((_c > (_tm[_ti] + 1)) && !_l)
            _ti = -100;

        return DLY;
    }

    void end() {
        _reset();
        CONSOLE("ison=%d", ison);

        if (_ti <= -100) {
            CONSOLE("fail");
            if (ison) pwroff();
            return;
        }

        if (ison) {
            pwrinit();
            mode = PWR_ACTIVE;
        }
        else
            pwroff();
    }
};
static WrkProc<_powerWrk> _pwr;

/* ------------------------------------------------------------------------------------------- *
 *  Запуск / остановка
 * ------------------------------------------------------------------------------------------- */

bool powerStart(bool pwron) {
    if (_pwr.isrun())
        return false;
    
    if (pwron && (mode == PWR_SLEEP)) {
        // Проверяем, почему мы вышли из спящего режима

#ifdef FWVER_DEBUG
        switch(esp_sleep_get_wakeup_cause()) {
            case ESP_SLEEP_WAKEUP_EXT0      : CONSOLE("Wakeup: RTC_IO"); break;
            case ESP_SLEEP_WAKEUP_EXT1      : CONSOLE("Wakeup: RTC_CNTL"); break;
            case ESP_SLEEP_WAKEUP_TIMER     : CONSOLE("Wakeup: timer"); break;
            case ESP_SLEEP_WAKEUP_TOUCHPAD  : CONSOLE("Wakeup: touchpad"); break;
            case ESP_SLEEP_WAKEUP_ULP       : CONSOLE("Wakeup: ULP program"); break;
            default : CONSOLE("Wakeup unknown: %d", esp_sleep_get_wakeup_cause()); break;
        }
#endif // FWVER_DEBUG

        if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
            CONSOLE("resume from sleep by btn");
            pwrinit();
            mode = PWR_ACTIVE;
            return true;
        }
        else
        if (jumpSleepTakeoff()) {
            CONSOLE("resume from sleep by Takeoff");
            pwrinit();
            mode = PWR_ACTIVE;
            return true;
        }
        else {
            pwrsleep();
        }
    }

    if (pwron && (mode == PWR_ACTIVE)) {
        CONSOLE("mode = active");
        pwrinit();
        return true;
    }
    
    _pwr = wrkRun<_powerWrk>(pwron);
    
    return true;
}

bool powerStop() {
    if (!_pwr.isrun())
        return false;

    _pwr.term();

    return true;
}

void powerSleep() {
    pwrsleep();
}

static void _reset() {
    _pwr.reset();
}
