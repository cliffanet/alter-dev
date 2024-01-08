
#include "btn.h"
#include "worker.h"
#include "clock.h"
#include "log.h"

#include <list>
#include "esp32-hal-gpio.h"

class _btnWrk : public Wrk {
    struct {
        uint8_t pin;
        int8_t pushed;
        bool click;
    } _ball[3] = {
        { PINBTN_UP, 0, false },
        { PINBTN_CE, 0, false }, 
        { PINBTN_DN, 0, false }
    };

    std::list<Btn*> _hnd;

    bool ispushed(uint8_t _pin) {
        return digitalRead(_pin) == LOW;
    }

public:
    _btnWrk() {
        for (auto &b: _ball) {
            pinMode(b.pin, INPUT);
            if (ispushed(b.pin))
                // Если мы стартовали, а кнопка уже нажата,
                // надо сделать так, чтобы click не сработал,
                // т.е. представим, что он уже сработал.
                // Актуально только при включении.
                b.click = true;
        }
    }
#ifdef FWVER_DEBUG
    ~_btnWrk() {
        CONSOLE("(0x%08x) destroy", this);
    }
#endif

    bool empty() const {
        return _hnd.empty();
    }

    void add(Btn *b, bool tofirst=false) {
        CONSOLE("[%d]: 0x%08x", _hnd.size(), b);
        if (tofirst)
            _hnd.push_front(b);
        else
            _hnd.push_back(b);
    }
    bool del(Btn *b) {
        CONSOLE("[%d]: 0x%08x", _hnd.size(), b);
        for (auto p = _hnd.begin(); p != _hnd.end(); p++)
            if (*p == b) {
                CONSOLE("found");
                _hnd.erase(p);
                return true;
            }

        return false;
    }

    state_t run() {
        if (_hnd.empty())
            return END;

        for (auto &b: _ball) {
            bool pushed = ispushed(b.pin);
            if (pushed && (b.pushed < 0))
                b.pushed = 0;

            if (!b.click) {
                const auto &h =
                    b.pin == PINBTN_UP ?
                        _hnd.back()->up :
                    b.pin == PINBTN_CE ?
                        _hnd.back()->ce :
                    b.pin == PINBTN_DN ?
                        _hnd.back()->dn :
                        Btn::Hnd(NULL);
                
                if (h.lng != NULL) {
                    // need wait long-click
                    if (pushed) {
                        b.pushed++;
                        if (b.pushed > 15) {
                            b.click = true;
                            h.lng();
                        }
                    }
                    else
                    if (b.pushed > 0) {
                        b.click = true;
                        if (h.sng != NULL)
                            h.sng();
                    }
                }
                else
                if (pushed) {
                    b.pushed++;
                    if (b.pushed > 0) {
                        b.click = true;
                        if (h.sng != NULL)
                            h.sng();
                    }
                }
            }

            if (!pushed) {
                if (b.pushed > 0)
                    b.pushed = 0;
                else
                if (b.pushed < -5)
                    b.pushed --;
                else
                    b.click = false;
            }
        }

        return DLY;
    }
};

static WrkProc<_btnWrk> _btn;

Btn::Btn(const Hnd &up, const Hnd &ce, const Hnd &dn) :
    up(up),
    ce(ce),
    dn(dn)
{
    if (!_btn.isrun())
        _btn = wrkRun<_btnWrk>();
    _btn->add(this);
}

Btn::~Btn() {
    if (!_btn.isrun())
        return;
    _btn->del(this);
    if (_btn->empty())
        _btn.term();
}

bool Btn::activate() {
    if (!_btn.isrun() || !_btn->del(this))
        return false;
    _btn->add(this);
    return true;
}

bool Btn::hide() {
    if (!_btn.isrun() || !_btn->del(this))
        return false;
    _btn->add(this, true);
    return true;
}
