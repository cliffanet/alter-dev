#ifndef RINGSIMPLE_H
#define RINGSIMPLE_H

/*********************************************
 * Упращённая версия кольцевого буфера.
 * Умеет только добавление в конец, for
 * и получение свежих данных по индексу.
 *********************************************/

#include <stddef.h>
#include <stdlib.h>
#include <cassert>
#include <type_traits>
#include <stdexcept>

template <class T, size_t _dsz = 10>
class ring {
    using value_type = T;
    using reference = T & ;
    using const_ref = const T &;
    using size_type = size_t;

    value_type _data[_dsz];
    size_type _cur = _dsz-1;
    size_type _csz = 0;

    inline
    size_type posfrst() const {
        return _csz <= 0 ? _cur : pos(_csz-1);
    }

    size_type pos(size_type index) const {
        if (index < 0)
            return posfrst();
        if (_csz <= 0)
            return _cur;
        if (index <= _cur)
            return _cur - index;
        if (index >= _csz)
            index = _csz-1;
        index -= _cur;
        return _dsz - index;
    }

public:
    reference   last()            { return _data[_cur]; }
    const_ref   last()      const { return _data[_cur]; }
    reference   frst()            { return _data[ posfrst() ]; }
    const_ref   frst()      const { return _data[ posfrst() ]; }
    size_type   size()      const { return _csz; }
    size_type   capacity()  const { return _dsz; }
    bool        empty()     const { return _csz == 0; }
    bool        full()      const { return _csz >= _dsz; }

    reference operator*() {
        return _data[ _cur ];
    }
    const_ref operator*() const {
        return _data[ _cur ];
    }
    reference operator[](size_type index) {
        return _data[ pos(index) ];
    }
    const_ref operator[](size_type index) const {
        return _data[ pos(index) ];
    }

    void clear() {
        _cur = _dsz-1;
        _csz = 0;
    }
    void push(const value_type &val) {
        _cur ++;
        _cur %= _dsz;
        _data[_cur] = val;
        if (_csz < _dsz)
            _csz ++;
    }
    
    template <bool isconst = false>
    struct my_iterator
    {
        using reference = typename std::conditional< isconst, T const &, T & >::type;
        using pointer = typename std::conditional< isconst, T const *, T * >::type;
        using src_ref = typename std::conditional<isconst, ring<T, _dsz> const *, ring<T, _dsz> *>::type;
    private:
        src_ref _s;
        size_type _i;

    public:
        my_iterator(src_ref src, size_type ind = 0) : _s(src), _i(ind) {}
        my_iterator(const my_iterator<false>& it) :
            _s(it._s),
            _i(it._i) {}
        reference operator*() {
            return **_s;
        }
        reference operator[](size_type index) {
            return (*_s)[index];
        }
        pointer operator->() { return &(operator *()); }

        my_iterator& operator++ () {
            ++_i;
            return *this;
        }
        my_iterator operator ++(int) {
            auto orig = *this;
            ++_i;
            return orig;
        }
        my_iterator& operator --() {
            --_i;
            return *this;
        }
        my_iterator operator --(int) {
            auto orig = *this;
            --_i;
            return orig;
        }
        friend my_iterator operator+(my_iterator lhs, int rhs) {
            lhs._i += rhs;
            return lhs;
        }
        friend my_iterator operator+(int lhs, my_iterator rhs) {
            rhs._i += lhs;
            return rhs;
        }
        my_iterator& operator+=(int n) {
            _i += n;
            return *this;
        }
        friend my_iterator operator-(my_iterator lhs, int rhs) {
            lhs._i -= rhs;
            return lhs;
        }
        my_iterator& operator-=(int n) {
            _i -= n;
            return *this;
        }
        bool operator==(const my_iterator &other) {
            return (_s == other._s) && (_i == other._i);
        }
        bool operator!=(const my_iterator &other) {
            return (_s != other._s) || (_i != other._i);
        }
        bool operator<(const my_iterator &other) {
            return (_s == other._s) && (_i < other._i);
        }
        bool operator<=(const my_iterator &other) {
            return (_s == other._s) && (_i <= other._i);
        }
        bool operator >(const my_iterator &other) {
            return (_s == other._s) && (_i > other._i);
        }
        bool operator>=(const my_iterator &other) {
            return (_s == other._s) && (_i >= other._i);
        }
        friend class ring<T>;
    };

    using iterator = my_iterator<false>;
    using const_it = my_iterator<true>;
    iterator begin() {
        return iterator(this);
    }
    const_it begin() const {
        return const_it(this);
    }
    const_it cbegin() const {
        return const_it(this);
    }
    iterator end() {
        return iterator(this, _csz);
    }
    const_it end() const {
        return const_it(this, _csz);
    }
    const_it cend() const {
        return const_it(this, _csz);
    }
};

#endif // RINGSIMPLE_H
