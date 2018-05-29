// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SLOT_H
#define LIBCURV_SLOT_H

namespace curv {

/// slot_t is the unsigned type of an index into a Frame's slot array,
/// or the size of a slot array.
/// It's not size_t, because 64 bits is much larger than necessary, and there is
/// a benefit in being able to fit several slot_t's into a single 64 bit word.
using slot_t = unsigned;

} // namespace curv
#endif // header guard
