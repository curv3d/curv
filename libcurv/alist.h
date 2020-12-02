// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_ALIST_H
#define LIBCURV_ALIST_H

#include <libcurv/value.h>

namespace curv {

struct Abstract_List : public Ref_Value
{
    using Ref_Value::Ref_Value;
protected:
    size_t size_;
public:
    size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    virtual Value val_at(size_t i) const = 0;
};

} // namespace curv
#endif // header guard
