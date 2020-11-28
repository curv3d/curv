// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_LENS_H
#define LIBCURV_LENS_H

#include <libcurv/value.h>
#include <libcurv/context.h>

namespace curv {

// If slice is null, this is value@index.
// If slice is non-null, this is value.[index,...restofslice]
// where restofslice is described by the C++ range slice,endslice.
Value get_value_at_index(
    Value value, Value index,
    const Value* slice, const Value* endslice,
    const At_Syntax& cx);

// The 'slice' argument is unboxed to a list of index values.
Value get_value_at_boxed_slice(Value value, Value slice, const At_Syntax& cx);

} // namespace curv
#endif // header guard
