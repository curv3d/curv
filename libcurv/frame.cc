// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/frame.h>

#include <libcurv/function.h>
#include <libcurv/phrase.h>

namespace curv {

Frame_Base::Frame_Base(Source_State& sstate, Frame* parent,
    Shared<const Function> caller, Shared<const Phrase> src)
:
    sstate_(sstate),
    parent_frame_(parent),
    caller_(caller),
    call_phrase_(move(src))
{
}

} // namespaces
