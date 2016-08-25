// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SHARED_H
#define CURV_SHARED_H

#include <aux/shared.h>

namespace curv {

/// Terse alias for aux::Shared in curv namespace
// template<typename T> using Shared = aux::Shared<T>;
using aux::Shared;

template<class T, class U>
inline Shared<T>
dynamic_shared_cast(Shared<U> p)
{
    return Shared<T>(dynamic_cast<T*>(p.get()));
}

} // namespace curv
#endif // header guard
