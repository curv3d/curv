// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_TERNARY_H
#define LIBCURV_TERNARY_H

namespace curv {

struct Ternary
{
    enum T { False, True, Unknown } _;
    Ternary(T val) : _(val) {}
    Ternary(bool val) : _((T)(int)val) {}
    bool to_bool() const { return (bool)(int)_; }
    bool operator==(Ternary rhs) const { return _ == rhs._; }
    bool operator!=(Ternary rhs) const { return _ != rhs._; }
    Ternary operator!() const {
        if (_ == Unknown) return {Unknown}; else return {!to_bool()};
    }
    Ternary operator&(Ternary rhs) const {
        if (_ == False || rhs._ == False) return False;
        if (_ == Unknown || rhs._ == Unknown) return Unknown;
        return True;
    }
    void operator&=(Ternary rhs) { *this = (*this & rhs); }
};

} // namespace curv
#endif // header guard
