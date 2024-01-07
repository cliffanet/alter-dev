
#include "view.h"
#include "worker.h"
#include "clock.h"
#include "log.h"

#include <list>

class _viewWrk : public Wrk {
    U8G2_ST75256_JLX19296_F_4W_HW_SPI u8g2;
    std::list<View*> _hnd;

public:
    _viewWrk() :
        u8g2(U8G2_R0, /* cs=*/ 26, /* dc=*/ 25, /* reset=*/ 33)
    {
        u8g2.begin();
    }
#ifdef FWVER_DEBUG
    ~_viewWrk() {
        CONSOLE("(0x%08x) destroy", this);
        u8g2.clearDisplay();
        u8g2.setPowerSave(true);
    }
#endif

    bool empty() const {
        return _hnd.empty();
    }

    void add(View *b, bool tofirst=false) {
        CONSOLE("[%d]: 0x%08x", _hnd.size(), b);
        if (tofirst)
            _hnd.push_front(b);
        else {
            _hnd.push_back(b);
            draw(true);
        }
    }
    bool del(View *b) {
        CONSOLE("[%d]: 0x%08x", _hnd.size(), b);
        bool iscur = (_hnd.size() > 0) && ( _hnd.back() == b );
        for (auto p = _hnd.begin(); p != _hnd.end(); p++)
            if (*p == b) {
                CONSOLE("found");
                _hnd.erase(p);
                if (iscur)
                    draw(true);
                return true;
            }

        return false;
    }

    state_t run() {
        if (_hnd.empty())
            return END;

        draw();

        return DLY;
    }

    void draw(bool clr = false) {
        if (clr)
            clear();
        if (_hnd.empty())
            return;

        auto d = _hnd.back()->draw;
        u8g2.firstPage();
        do {
            d(u8g2);
        }  while( u8g2.nextPage() );
    }

    void clear() {
        u8g2.clearDisplay();
    }

    void visible(bool vis) {
        u8g2.setPowerSave(!vis);
        if (vis) clear();
    }
};

static WrkProc<_viewWrk> _view;

View::View(hnd_t draw) :
    draw(draw)
{
    if (!_view.isrun())
        _view = wrkRun<_viewWrk>();
    _view->add(this);
}

View::~View() {
    if (!_view.isrun())
        return;
    _view->del(this);
    if (_view->empty())
        _view.term();
}

bool View::activate() {
    if (!_view.isrun() || !_view->del(this))
        return false;
    _view->add(this);
    return true;
}

bool View::hide() {
    if (!_view.isrun() || !_view->del(this))
        return false;
    _view->add(this, true);
    return true;
}
