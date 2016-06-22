// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_SHARED_H
#define CURV_SHARED_H

#include <aux/shared.h>

namespace curv {

/// Terse alias for aux::Shared_Ptr in curv namespace
template<typename T> using Shared = aux::Shared_Ptr<T>;

} // namespace curv
#endif // header guard
