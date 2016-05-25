// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef AUX_RANGE_H
#define AUX_RANGE_H

#include <cstddef>

namespace aux {

/// A Range references a contiguous subsequence of a collection
/// using a pair of iterators.
///
/// It's the simplest possible range implementation,
/// compatible with range-based for loops.
template <class Iter>
struct Range
{
    Range(Iter f, Iter l) : first(f), last(l) {}
    Iter first;
    Iter last;
    Iter begin() const { return first; }
    Iter end() const { return last; }
    bool empty() const { return first == last; }
    std::size_t size() const { return last - first; }
};

} // namespace aux
#endif // header guard
