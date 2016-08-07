// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef AUX_RANGE_H
#define AUX_RANGE_H

#include <cstddef>
#include <string>
#include <cstring>
#include <ostream>

namespace aux {

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
    Iter first;
    Iter last;
    Iter begin() const { return first; }
    Iter end() const { return last; }
    bool empty() const { return first == last; }
    std::size_t size() const { return last - first; }

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

} // namespace aux
#endif // header guard
