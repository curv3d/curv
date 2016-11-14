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

using aux::make_shared;

template<class T, class U>
inline Shared<T>
dynamic_shared_cast(Shared<U> p)
{
    return Shared<T>(dynamic_cast<T*>(p.get()));
}

template<class T, class U>
inline bool
isa_shared(Shared<U> p)
{
    return dynamic_cast<T*>(p.get()) != nullptr;
}

template<class T>
inline Shared<T>
share(T& obj)
{
    return Shared<T>(&obj);
}

} // namespace curv
#endif // header guard
