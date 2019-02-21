// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_RANGE_H
#define LIBCURV_RANGE_H

#include <cstddef>
#include <string>
#include <cstring>
#include <ostream>

namespace curv {

/// A Range references a contiguous subsequence of a collection
/// using a pair of iterators.
///
/// Compatible with range-based for loops.
/// TODO: rbegin(), possibly others.
///
/// It's the simplest possible range implementation
/// (boost/iterator_range.hpp is 40,000 lines of code).
template <class Iter>
struct Range
{
    Range(Iter f, Iter l) : first(f), last(l) {}
    Range(Iter p, std::size_t sz) : first(p), last(p+sz) {}
    Iter first;
    Iter last;
    Iter begin() const { return first; }
    Iter end() const { return last; }
    bool empty() const { return first == last; }
    std::size_t size() const { return last - first; }
    auto operator[](size_t i) const -> decltype(first[i]) { return first[i]; }

    /// convert Range<const char*> to std::string
    operator std::string() { return std::string(begin(), size()); }
};

std::ostream& operator<<(std::ostream&, const Range<const char*>&);

inline bool operator==(Range<const char*> r, const char* s)
{
    size_t size = r.size();
    if (size != strlen(s))
        return false;
    return memcmp(r.begin(), s, size) == 0;
}

} // namespace curv
#endif // header guard
