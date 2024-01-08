/*
    Button worker
*/

#ifndef _btn_H
#define _btn_H

#include <functional>

#define PINBTN_UP   39
#define PINBTN_CE   34
#define PINBTN_DN   35

#define PINBTN_PWR  GPIO_NUM_34

class Btn {
    public:
        typedef std::function<void (void)> hnd_t;

        class Hnd {
        public:
            const hnd_t sng, lng;
            Hnd(hnd_t sng, hnd_t lng = NULL) :
                sng(sng), lng(lng) {}
            Hnd(const Hnd &h) :
                sng(h.sng), lng(h.lng) {}
        };

        const Hnd up, ce, dn;

        Btn(const Hnd &up, const Hnd &ce, const Hnd &dn);
        ~Btn();
        bool activate();
        bool hide();
};

uint32_t btnIdle();

#endif // _btn_H
