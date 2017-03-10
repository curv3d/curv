// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SHARED_H
#define CURV_SHARED_H

#include <aux/shared.h>

namespace curv {

using aux::Shared;
using aux::make;

template<class T, class U>
inline Shared<T>
cast(Shared<U> p)
{
    return Shared<T>(dynamic_cast<T*>(p.get()));
}

template<class T, class U>
inline bool
isa(Shared<U> p)
{
    return dynamic_cast<T*>(p.get()) != nullptr;
}

template<class T>
inline Shared<T>
share(T& obj)
{
    return Shared<T>(&obj);
}

template<class T>
inline T&
update_shared(Shared<const T>& p)
{
    if (p->use_count > 1)
        p = p->clone();
    return const_cast<T&>(*p);
}

} // namespace curv
#endif // header guard
