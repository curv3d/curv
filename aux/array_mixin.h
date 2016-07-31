// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef AUX_ARRAY_MIXIN_H
#define AUX_ARRAY_MIXIN_H

#include <iterator>
#include <stdexcept>

namespace aux {

/// This is used for defining a class that encapsulates an array.
///
/// It adds all of the C++ standard boilerplate that's expected of
/// a container class. The Base class defines the following:
/// * typename value_type
/// * value_type* data() noexcept
/// * size_t size() const noexcept
template <class Base>
struct Array_Mixin : public Base
{
    using Base::Base;

    // types:
    typedef typename Base::value_type& reference;
    typedef const typename Base::value_type& const_reference;
    typedef typename Base::value_type* iterator;
    typedef const typename Base::value_type* const_iterator;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef typename Base::value_type* pointer;
    typedef const typename Base::value_type* const_pointer;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    // iterators:
    iterator begin() noexcept { return Base::data(); }
    const_iterator begin() const noexcept { return Base::data(); }
    iterator end() noexcept { return Base::data() + Base::size(); }
    const_iterator end() const noexcept { return Base::data() + Base::size(); }

    reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }
    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }
    reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }
    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend() const noexcept { return end(); }
    const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    const_reverse_iterator crend() const noexcept { return rend(); }

    // predicate:
    bool empty() const noexcept { return Base::size() == 0; }

    // element access:
    reference operator[](size_type i) { return Base::data()[i]; }
    const_reference operator[](size_type i) const { return Base::data()[i]; }
    reference at(size_type i) {
        if (i < Base::size())
            return Base::data()[i];
        else
            throw std::out_of_range();
    }
    const_reference at(size_type i) const {
        if (i < Base::size())
            return Base::data()[i];
        else
            throw std::out_of_range();
    }

    reference front() { return (*this)[0]; }
    const_reference front() const { return (*this)[0]; }
    reference back() { return (*this)[Base::size()-1]; }
    const_reference back() const { return (*this)[Base::size()-1]; }

    const typename Base::value_type* data() const noexcept { return data(); }
};

} // namespace aux
#endif // header guard
