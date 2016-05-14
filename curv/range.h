/*
 * Copyright 2016 Doug Moen. See LICENCE.md file for terms of use.
 */
#ifndef CURV_RANGE_H
#define CURV_RANGE_H

#include <cstddef>

namespace curv {

/// \brief A Range is a pair of iterators.
///
/// Compatible with range-based for loop.
/// TODO: fully implement the Container concept?
template <class Iter>
struct Range
{
    Range(Iter f, Iter l) : first(f), last(l) {}
    Iter first;
    Iter last;
    Iter begin() const { return first; }
    Iter end() const { return last; }
    std::size_t size() const { return last - first; }
};

} // namespace curv
#endif // header guard
