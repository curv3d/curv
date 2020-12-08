// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_ALIST_H
#define LIBCURV_ALIST_H

#include <libcurv/value.h>

namespace curv {

// An Abstract_List is any Curv value that denotes a sequence of values.
// (Right now, except for the case of Reactive_Value.)
//
// At present, there are two Abstract_List subclasses: List and String.
// String exists for efficiency reasons: with the List representation,
// each character occupies 64 bits.
//
// In the future, we need more specialized list representations,
// for compactness and speed. Eg, bit lists, numeric ranges,
// and images, voxel grids, triangle meshes.
//
// Right now, a lot of code has special cases for List and String (faster
// than repeatedly calling val_at). If we want more list representations,
// we need a better way to write efficient, abstract list code.
//  * Extend the Abstract_List interface with efficient bulk operations.
//  * Extend the Reactive_Value API so that any RV that is a list is also
//    an Abstract_List. We need to preserve the identity of primitive
//    vectorized operations from SPIR-V/WGSL.
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
