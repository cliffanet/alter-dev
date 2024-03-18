/*
    Alt calculate
*/

#include "altcalc.h"
#include <math.h>

float press2alt(float pressgnd, float pressure) {
  return 44330 * (1.0 - pow(pressure / pressgnd, 0.1903));
}

AltCalc::VAvg::VAvg() :
    _interval(0),
    _alt(0),
    _speed(0)
{ }

AltCalc::VAvg::VAvg(float alt0, const src_t &src) :
    VAvg()
{
    // самый первый _interval в массиве - не должен входить в суммарный интервал при определении скорости
    double alt = alt0;
    for (const auto &d: src) {
        _interval += d.interval;
        alt += d.alt;
    }
    _alt = alt / (src.size()+1);
    _speed = (_interval > 0) ?
        // _interval м.б. равен нулю, даже при src.size()!=0,
        // но при _interval!=0 массив не может оказаться пустым
        (src.last().alt - alt0) * 1000 / _interval :
        0;
}

AltCalc::VApp::VApp(float alt0, const src_t &src) :
    VAvg()
{
    // считаем коэффициенты линейной аппроксимации
    // sy - единственный ненулевой элемент в нулевой точке - равен _alt0
    double sy = alt0, sxy = 0, sx = 0, sx2 = 0;
    // количество элементов на один больше, чем src.size(),
    // т.к. нулевой элемент находится не в src, а в alt0
    uint32_t x = 0, n = src.size() + 1;
    
    for (const auto &d: src) {
        if (d.interval == 0)
            continue;
        
        x   += d.interval;
        sx  += x;
        sx2 += x*x;
        sy  += d.alt;
        sxy += x*d.alt;
    }
    
    // коэфициенты
    double a = (sxy*n - (sx*sy)) / (sx2*n-(sx*sx));
    double b = (sy - (a*sx)) / n;

    _interval   = x;
    _alt        = a * x + b;
    _speed      = a * 1000;
}

AltCalc::VSavg::VSavg(const src_t &src, uint8_t sz) :
    VAvg()
{
    // самый первый _interval в массиве - не должен входить в суммарный интервал при определении скорости
    if (sz >= src.size())
        sz = src.size()-1;
    double alt = 0;
    uint8_t n = 0;
    for (const auto &d: src) {
        alt += d.alt;
        n++;
        if (n > sz) break;
        _interval += d.interval;
    }
    _alt = alt / n;
    _speed = (_interval > 0) ?
        // _interval м.б. равен нулю, даже при src.size()!=0,
        // но при _interval!=0 массив не может оказаться пустым
        (src.last().alt - src[sz].alt) * 1000 / _interval :
        0;
}

const uint32_t AltCalc::interval() const {
    uint32_t interval = 0;
    for (const auto &d: _data)
        interval += d.interval;
    return interval;
}

const double AltCalc::sqdiff() const {
    auto s = avg().speed();
    uint32_t x = 0;
    double sq = 0;
    for (const auto &d: _data) {
        x += d.interval;
        double altd = s * x / 1000 + _alt0 - d.alt;
        sq += altd * altd;
    }

    return sqrt(sq / _data.size());
}

void AltCalc::tick(float press, uint16_t tinterval) {
    bool full = _data.full();
    if (_data.empty()) {
        _pressgnd   = press;
        _press0     = press;
    }
    else
    if (full) {
        _press0 = _data.frst().press;
        _alt0   = _data.frst().alt;
    }

    _data.push({
        tinterval,
        press,
        press2alt(_pressgnd, press)
    });

    if (!full && _data.full())
        gndreset();
}

void AltCalc::gndreset() {
    // пересчёт _pressgnd
    double pr = 0;
    for (auto &d: _data)
        pr += d.press;
    _pressgnd = pr / _data.size();
    
    for (auto &d: _data) // т.к. мы пересчитали _pressgnd, то пересчитаем и alt
        d.alt = press2alt(_pressgnd, d.press);
    _alt0 = press2alt(_pressgnd, _press0);
}

void AltCalc::gndset(float press, uint16_t tinterval) {
    _pressgnd = press;
    // если мы делаем gndset ещё до полной инициализации,
    // забиваем массив данных текущим давлением, 
    // тем самым мы принудительно завершаем инициализацию
    while (!_data.full())
        _data.push({ tinterval, press, 0 });
}

/*******************************
 *          AltDirect
 *******************************/

void AltDirect::tick(const AltCalc &ac) {
    dir_t dir =
        ac.isinit() ?
            INIT :
        ac.app().speed() > AC_SPEED_FLAT ?
            UP :
        ac.app().speed() < -AC_SPEED_FLAT ?
            DOWN :
            FLAT;
    
    if (_mode == dir) {
        _cnt ++;
        _tm += ac.tm();
    }
    else {
        _mode = dir;
        _cnt = 0;
        _tm = 0;
    }
}

void AltDirect::reset() {
    _cnt = 0;
    _tm = 0;
}

/*******************************
 *          AltState
 *******************************/

void AltState::tick(const AltCalc &ac) {
    st_t st =
        ac.isinit() ?
            INIT :
        ac.app().speed() > AC_SPEED_FLAT ?
            (
                ac.app().alt() < 40 ?
                    TAKEOFF40 :
                    TAKEOFF
            ) :
        
        (ac.app().alt() < 50) && 
        (ac.avg().speed() < 0.5) &&
        (ac.avg().speed() > -0.5) ?
            GROUND :
        
        (ac.app().speed() < -AC_SPEED_FLAT) &&
        (ac.app().alt() < 100) ?
            LANDING :
        
        (ac.app().speed() < -AC_SPEED_FREEFALL_I) || 
        ((_mode == FREEFALL) && (ac.app().speed() < -AC_SPEED_FREEFALL_O)) ?
            FREEFALL :
        
        (ac.app().speed() < -AC_SPEED_CANOPY_I) ||
        ((_mode == CANOPY) && (ac.app().speed() < -AC_SPEED_FLAT)) ?
            CANOPY :

            _mode;
    
    if (_mode == st) {
        _cnt ++;
        _tm += ac.tm();
    }
    else {
        _mode = st;
        _cnt = 0;
        _tm = 0;
    }
}

void AltState::reset() {
    _cnt = 0;
    _tm = 0;
}

/*******************************
 *          AltSqBig
 *******************************/

void AltSqBig::tick(const AltCalc &ac) {
    _val = ac.sqdiff();
    // большая турбуленция (высокое среднеквадратическое отклонение)
    if (!_big && (_val >= AC_SQBIG_THRESH)) {
        _big = true;
        _cnt = 0;
        _tm = 0;
    }
    else
    if (_big && (_val < AC_SQBIG_MIN)) {
        _big = false;
        _cnt = 0;
        _tm = 0;
    }
    else {
        _cnt ++;
        _tm += ac.tm();
    }
}

void AltSqBig::reset() {
    _cnt = 0;
    _tm = 0;
}

/*******************************
 *          AltProfile
 *******************************/

AltProfile::AltProfile() :
    _prof(NULL),
    _sz(0),
    _icnt(0)
{ }

AltProfile::AltProfile(const prof_t *profile, uint8_t sz, uint8_t icnt) :
    _prof(profile),
    _sz(sz),
    _icnt(icnt)
{ }

void AltProfile::tick(const AltCalc::VAvg &avg, uint32_t tm) {
    if ((_prof == NULL) || (_sz == 0))
        return;
    
    if (_c == 0) {
        // стартуем определять вхождение в профиль по средней скорости
        const auto &p = _prof[_c];
        if ((avg.speed() >= p.min) && (avg.speed() <= p.max)) {
            _c ++;
            _alt = avg.alt();
            _tm = 0;
            _cnt = 0;
        }
        return;
    }

    // теперь считаем интервал от самого первого пункта в профиле
    _cnt ++;
    _tm += tm;

    if (_c >= _sz) return;

    // далее - каждые 10 тиков от старта будем проверять каждый следующий пункт профиля
    if ((_cnt/_c) < _icnt)
        return;

    auto alt = avg.alt() - _alt;
    const auto &p = _prof[_c];
    if ((alt >= p.min) && (alt <= p.max)) {
        // мы всё ещё соответствуем профилю начала прыга
        _c ++;
        _alt = avg.alt();
        return;
    }

    // мы вышли за пределы профиля
    if ((avg.speed() >= p.min) && (avg.speed() <= p.max)) {
        // но мы ещё в рамках старта профиля
        _c ++;
        _alt = avg.alt();
    }
    else {
        // выход из профиля полный - полный сброс процесса
        _c = 0;
        _alt = 0;
    }

    if (_cnt > 0) {
        _cnt = 0;
        _tm = 0;
    }
}

void AltProfile::reset() {
    _c      = 0;
    _alt    = 0;
    _cnt    = 0;
    _tm     = 0;
}

void AltProfile::clear() {
    reset();
    _prof   = NULL;
    _sz     = 0;
    _icnt   = 0;
}

/*******************************
 *          AltJmp
 *******************************/

void AltJmp::tick(const AltCalc &ac) {
    auto m = _mode;
    auto tm = ac.tm();
    auto avg = ac.avg();

    if ((_mode != TAKEOFF) && !_ff.empty())
        _ff.clear();

    switch (_mode) {
        case INIT:
            if (!ac.isinit()) {
                m = GROUND;
                _c_cnt= 0;
                _c_tm = 0;
            }
            break;
        
        case GROUND:
            if (avg.speed() > AC_SPEED_FLAT) {
                _c_cnt++;
                _c_tm += tm;
                if ((_c_cnt >= AC_JMP_TOFF_COUNT) && (_c_tm >= AC_JMP_TOFF_TIME))
                    m = TAKEOFF;
            }
            else
            if (_c_cnt > 0) {
                _c_cnt= 0;
                _c_tm = 0;
            }
            break;
            
        case TAKEOFF: {
                static const AltProfile::prof_t profstr[] = {
                    { -50,  -15 },
                    { -100, -30 },
                    { -100, -40 },
                };
                static const AltProfile::prof_t profile[] = {
                    { -50,  -10 },
                    { -100, -23 },
                    { -100, -18 },
                    { -100, -8 },
                    { -100, -4 },
                    { -100, -2 }
                };
                if (_ff.empty())
                    _ff = _strict ?
                        AltProfile(profstr, 3) :
                        AltProfile(profile, 6);
                
                _ff.tick(ac.sav() /* avg */, tm);
                if (_ff.full()) {
                    // профиль закончился, принимаем окончательное решение
                    m = !_strict && (avg.speed() >= -AC_JMP_CNP_SPEED) ? CANOPY : FREEFALL;
                    // скорость средняя задерживается примерно на половину-весь размер буфера
                    _c_cnt= _ff.cnt() + 10 /* + AC_DATA_COUNT */;
                    _c_tm = _ff.tm();
                }
            }
            break;
            
        case FREEFALL:
            // Переход в режим CNP после начала прыга,
            // Дальше только окончание прыга может быть, даже если начнётся снова FF,
            // Для jmp только такой порядок переходов,
            // это гарантирует прибавление только одного прыга на счётчике при одном фактическом
            if (avg.speed() >= -AC_JMP_CNP_SPEED) {
                _c_cnt++;
                _c_tm += tm;
                if ((_c_cnt >= AC_JMP_CNP_COUNT) && (_c_tm >= AC_JMP_CNP_TIME))
                    m = CANOPY;
            }
            else
            if (_c_cnt > 0) {
                _c_cnt= 0;
                _c_tm = 0;
            }
            break;
            
        case CANOPY:
            if (
                    (avg.alt() < 50) &&
                    // разницы +-0.5 (между началом и концом = скорость)
                    // может быть недостаточно.
                    // некоторые датчики шумят чуть сильнее
                    (avg.speed() < 0.7) &&
                    (avg.speed() > -0.7)
                ) {
                _c_cnt++;
                _c_tm += tm;
                if ((_c_cnt >= AC_JMP_GND_COUNT) && (_c_tm >= AC_JMP_GND_TIME))
                    m = GROUND;
            }
            else
            if (_c_cnt > 0) {
                _c_cnt= 0;
                _c_tm = 0;
            }
            break;
    }

    if (m == _mode) {
        _cnt ++;
        _tm += tm;
    }
    else {
        _mode   = m;
        _cnt    = _c_cnt;
        _tm     = _c_tm;
        _c_cnt  = 0;
        _c_tm   = 0;
    }
}

void AltJmp::reset() {
    _mode   = INIT;
    _cnt    = 0;
    _tm     = 0;
    _c_cnt  = 0;
    _c_tm   = 0;
}

/*******************************
 *          AltStrict
 *******************************/

void AltStrict::tick(const AltCalc &ac) {
    auto tm = ac.tm();
    _prof.tick(ac.sav(5), tm);
    _dir.tick(ac);

    bool chg =
        _mode == AltJmp::INIT ?
            !ac.isinit() :
            _prof.full();
    
    if (!chg && (_mode > AltJmp::GROUND)) {
        auto avg = ac.avg();
        chg = 
            (avg.alt() < 50) && 
            (_dir.mode() == AltDirect::FLAT) &&
            (_dir.cnt() > 200);
        if (chg)
            _nxt = AltJmp::GROUND;
    }

    if (!chg) {
        _cnt ++;
        _tm += tm;
        return;
    }

    // профиль закончился успешно
    _mode   = _nxt;
    if (_prof.empty()) {
        _cnt    = 0;
        _tm     = 0;
    }
    else {
        // скорость средняя задерживается примерно на половину-весь размер буфера
        _cnt    = _prof.cnt() + 10 /* + AC_DATA_COUNT */;
        _tm     = _prof.tm();
    }

    switch (_mode) {
        case AltJmp::GROUND: {
                static const AltProfile::prof_t to_toff[] = {
                    { 1,  50 },
                    { 1,  50 },
                    { 2,  50 },
                    { 2,  50 },
                    { 2,  50 },
                };
                _prof   = AltProfile(to_toff, 5);
                _nxt    = AltJmp::TAKEOFF;
            }
            break;

        case AltJmp::TAKEOFF: {
                static const AltProfile::prof_t to_ff[] = {
                    { -50,  -10 },
                    { -100, -23 },
                    { -100, -35 },
                    { -100, -35 }
                };
                _prof   = AltProfile(to_ff, 4);
                _nxt    = AltJmp::FREEFALL;
            }
            break;

        case AltJmp::FREEFALL: {
                static const AltProfile::prof_t to_cnp[] = {
                    { -35,  10 },
                    { -15,  10 },
                    { -15,  10 },
                    { -15,  5 }
                };
                _prof   = AltProfile(to_cnp, 4);
                _nxt    = AltJmp::CANOPY;
            }
            break;

        case AltJmp::CANOPY: {
                _prof.clear();
                _nxt    = AltJmp::GROUND;
        }
    }
}
