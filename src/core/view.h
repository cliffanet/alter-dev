/*
    View worker
*/

#ifndef _view_H
#define _view_H

#include "../../def.h"
#include <functional>
#include <U8g2lib.h>

#define str(ss, ...) snprintf_P(s, sizeof(s), PSTR(ss), ##__VA_ARGS__)

class View {
    public:
        typedef std::function<void (U8G2 &u8g2)> hnd_t;
        const hnd_t draw;

        View(hnd_t draw);
        ~View();
        bool activate();
        bool hide();
};

#endif // _view_H
