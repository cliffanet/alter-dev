/*
 *   Alt calculate
 */

#ifndef __altcalc_H
#define __altcalc_H

#include <stdint.h>
#include "ringsimple.h"

#define AC_DATA_COUNT           20

// ************************************************
//  Параметры для определения direct и state
//
// Порог скорости для режима FLAT (abs, m/s)
// Он же является порогом удержания для режима canopy
#define AC_SPEED_FLAT           1.5

float press2alt(float pressgnd, float pressure);

class AltCalc {
    typedef struct {
        uint16_t interval;
        float press;
        float alt;
    } data_t;
    typedef ring<data_t, AC_DATA_COUNT> src_t;
    src_t _data;
    float _pressgnd = 101325, _alt0 = 0, _press0 = 0;

public:

    class VAvg {
    public:
        VAvg();
        VAvg(float alt0, const src_t &src);
        const float     alt()       const { return _alt; }
        const float     speed()     const { return _speed; }
        const uint32_t  interval()  const { return _interval; }
    protected:
        float _alt, _speed, _interval;
    };

    class VApp : public VAvg {
    public:
        VApp(float alt0, const src_t &src);
    };

    class VSavg : public VAvg {
    public:
        VSavg(const src_t &src, uint8_t sz);
    };

    // Текущее давление у земли
    const float     pressgnd()  const { return _pressgnd; }
    const bool      isinit()    const { return !_data.full(); }
    // Текущее давление
    const float     press()     const { return _data.size() > 0 ? _data.last().press : 0; }
    // Текущая высота
    const float     alt()       const { return _data.size() > 0 ? _data.last().alt : 0; }
    // Интервал времени с предыдущего измерения
    const float     tm()        const { return _data.size() > 0 ? _data.last().interval : 0; }
    // суммарный интервал времени, за которое собраны данные
    const uint32_t  interval()  const;

    // средне-арифметические: высота и скорость
    const VAvg      avg()       const { return VAvg(_alt0, _data); }
    // по аппроксимации: высота и скорость
    const VApp      app()       const { return VApp(_alt0, _data); }
    // средне-арифметическое укороченное
    const VSavg     sav(uint8_t sz = 5) const { return VSavg(_data, sz); }
    // среднеквадратическое отклонение прямой скорости
    const double    sqdiff()    const;
        
    // очередное полученное значение давления и интервал в ms после предыдущего вычисления
    void tick(float press, uint16_t interval);
    // обновляет текущее состояние, вычисленное по коэфициентам
    // сбрасывает "ноль" высоты в текущие показания и обнуляет все состояния
    void gndreset();
    void gndset(float press, uint16_t interval = 100);
};

/**************************************************************************/
//  AltDirect       - Направление вертикального движения (вверх/вниз)
/**************************************************************************/

class AltDirect {
public:
    typedef enum {
        INIT = 0,
        UP,
        FLAT,
        DOWN
    } dir_t;

    void tick(const AltCalc &ac);

    // Текущий режим высоты
    const dir_t     mode()  const { return _mode; }
    // Время с предыдущего изменения режима высоты
    const uint32_t  cnt()   const { return _cnt; }
    const uint32_t  tm()    const { return _tm; }
    void reset();

private:
    dir_t _mode = INIT;
    uint32_t _cnt = 0, _tm = 0;
};

/**************************************************************************/
//  AltState    - определяется исходя из направления движения и скорости
//              т.е. это состояние на текущее мнгновение
/**************************************************************************/

#define AC_SPEED_CANOPY_I       4
// Порог срабатывания режима freefall безусловный (abs, m/s)
// т.е. если скорость снижения выше этой, то полюбому - ff
#define AC_SPEED_FREEFALL_I     30
// Порог удержания режима freefall,
// т.е. если текущий режим freefall, 
// то он будет удерживаться таким, пока скорость снижения выше этой
#define AC_SPEED_FREEFALL_O     20

class AltState {
public:
    typedef enum {
        INIT = 0,
        GROUND,
        TAKEOFF40,
        TAKEOFF,
        FREEFALL,
        CANOPY,
        LANDING
    } st_t;

    void tick(const AltCalc &ac);

    // Текущий режим высоты
    const mode_t    mode()  const { return _mode; }
    // Время с предыдущего изменения режима высоты
    const uint32_t  cnt()   const { return _cnt; }
    const uint32_t  tm()    const { return _tm; }
    void reset();

private:
    st_t _mode = INIT;
    uint32_t _cnt = 0, _tm = 0;
};

/**************************************************************************/
//  AltSqBig        - среднеквадратичное отклонение высот от прямой _speedavg
/**************************************************************************/

// порог переключения в sqbig (сильные турбуленции)
#define AC_SQBIG_THRESH         12
#define AC_SQBIG_MIN            4

class AltSqBig {
    double _val = 0;
    bool _big = false;
    uint32_t _cnt = 0, _tm = 0;

public:
    void tick(const AltCalc &ac);

    // Текущее численное значение
    const double    val()   const { return _val; }
    // Текущее относительное значение (большая величина или нет)
    const bool      isbig() const { return _big; }
    // Время с предыдущего изменения isbig
    const uint32_t  cnt()   const { return _cnt; }
    const uint32_t  tm()    const { return _tm; }
    void reset();
};

/**************************************************************************/
//  AltProfile      - Проверка следования профайлу
/**************************************************************************/

class AltProfile {
public:
    typedef struct {
        int8_t min;
        int8_t max;
    } prof_t;

    AltProfile();
    AltProfile(const prof_t *profile, uint8_t sz, uint8_t icnt = 10);
    void tick(const AltCalc::VAvg &avg, uint32_t tm);

    const bool  empty()     const { return (_prof == NULL) || (_sz == 0); }
    const bool  active()    const { return _c > 0; }
    const bool  full()      const { return (_sz > 0) && (_c >= _sz); }
    const uint8_t num()     const { return _c; }
    // Время со старта работы профайла
    const uint32_t  cnt()   const { return _cnt; }
    const uint32_t  tm()    const { return _tm; }
    void reset();
    void clear();

private:
    const prof_t *_prof;
    uint8_t _c = 0, _sz, _icnt;
    float _alt = 0;
    uint32_t _cnt = 0, _tm = 0;
};

/**************************************************************************/
//  AltJmp          - интеллектуальное определение режимов прыжка
/**************************************************************************/

// Время (кол-во тиков), которое должен удерживаться state() > ACST_GROUND для определения, что это подъём
#define AC_JMP_TOFF_COUNT       10
// Время (мс), оторое должен удерживаться state() > ACST_GROUND для определения, что это подъём
#define AC_JMP_TOFF_TIME        7000

// Скорость, при котором будет переход из FF в CNP
#define AC_JMP_CNP_SPEED        12
// Время (кол-во тиков), которое должна удерживаться скорость AC_JMP_CNP_SPEED для перехода из FF в CNP
#define AC_JMP_CNP_COUNT        6
// Время (мс), которое должна удерживаться скорость AC_JMP_CNP_SPEED для перехода из FF в CNP
#define AC_JMP_CNP_TIME         6000
// Время (кол-во тиков), которое должен удерживаться state() == ACST_GROUND для перехода в NONE

#define AC_JMP_GND_COUNT        6
// Время (мс), которое должен удерживаться state() == ACST_GROUND для перехода в NONE
#define AC_JMP_GND_TIME         6000

class AltJmp {
public:
    typedef enum {
        INIT = -1,
        GROUND,
        TAKEOFF,
        FREEFALL,
        CANOPY
    } mode_t;

    void tick(const AltCalc &ac);

    // Текущий режим
    const mode_t    mode()  const { return _mode; }
    // Время с предыдущего изменения режима
    const uint32_t  cnt()   const { return _cnt; }
    const uint32_t  tm()    const { return _tm; }
    const AltProfile &ff()  const { return _ff; }
    void reset();

    const uint32_t  newtm() const { return _c_tm; }
    const uint32_t  newcnt() const { return _c_cnt; }

private:
    mode_t _mode = INIT;
    uint32_t _cnt = 0, _tm = 0, _c_cnt = 0, _c_tm = 0;
    AltProfile _ff;
};

/**************************************************************************/
//  AltStrict       - более строгое определение режимов прыжка
/**************************************************************************/

// режим AC_STRICT
// при котором более строго профилируется начало подъёма и начала прыжка.
// Особенности:
// - профилируется (сверяется с профилем) любое изменение режима
// - начало прыжка - всегда FF (т.е. раскрытие под бортом уже не прокатит)
// - более укороченное принятие решение о начале прыжка

class AltStrict {
public:

    void tick(const AltCalc &ac);

    // Текущий режим
    const AltJmp::mode_t    mode()  const { return _mode; }
    // Время с предыдущего изменения режима
    const uint32_t  cnt()   const { return _cnt; }
    const uint32_t  tm()    const { return _tm; }
    const AltProfile &prof() const { return _prof; }

private:
    AltJmp::mode_t _mode = AltJmp::INIT, _nxt = AltJmp::GROUND;
    uint32_t _cnt = 0, _tm = 0;
    AltProfile _prof;
    AltDirect _dir;
};

#endif // __altcalc_H
