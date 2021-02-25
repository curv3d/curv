// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PARAMETRIC_H
#define LIBCURV_PARAMETRIC_H

#include <libcurv/function.h>
#include <libcurv/record.h>

namespace curv {

// construct a new instance of a parametric record
struct Parametric_Ctor : public Function
{
    Shared<const Closure> ctor_;
    Shared<const Record> defl_;
    Parametric_Ctor(Shared<const Closure> ctor, Shared<const Record> defl)
    :
        Function(ctor->nslots_),
        ctor_(ctor),
        defl_(defl)
    {}

    virtual Value call(Value arg, Fail, Frame& fm) const override;
};

} // namespace curv
#endif // header guard
