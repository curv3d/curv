// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_ALIST_H
#define LIBCURV_ALIST_H

#include <libcurv/value.h>

namespace curv {

// Abstract_List is the abstract superclass of all concrete list classes.
// See Generic_List for an API that abstracts over all list values,
// both concrete and symbolic lists.
//
// At present, there are two Abstract_List subclasses: List and String.
// String exists for efficiency reasons: with the List representation,
// each character occupies 64 bits.
//
// In the future, we need more specialized list representations,
// for compactness and speed. Eg, bit lists, numeric ranges,
// and images, voxel grids, triangle meshes.
struct Abstract_List : public Ref_Value
{
    Abstract_List(int sty) : Ref_Value(ty_abstract_list, sty) {}
protected:
    size_t size_;
public:
    size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    virtual Value val_at(size_t i) const = 0;
    static const char name[];
};

#define ASSERT_SIZE(fl,rval,list,sz,cx) \
    if (list->size() != sz) { \
        FAIL(fl,rval,cx, \
            stringify("list ",list," does not have ",sz," elements")); \
    }

} // namespace curv
#endif // header guard
