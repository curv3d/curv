// Copyright Doug Moen 2016-2018.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SLOT_H
#define CURV_SLOT_H

namespace curv {

/// slot_t is the unsigned type of an index into a Frame's slot array,
/// or the size of a slot array.
/// It's not size_t, because 64 bits is much larger than necessary, and there is
/// a benefit in being able to fit several slot_t's into a single 64 bit word.
using slot_t = unsigned;

} // namespace curv
#endif // header guard
