// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIVEMODE_H
#define LIVEMODE_H

#include <libcurv/system.h>

// Start Curv's livemode
//
// For editor, you may either pass a (file)name (e.g. "gedit", "notepad++") or
// nullptr in case no editor instance should be opened.
int live_mode(curv::System& sys, const char* editor, const char* filename,
    curv::viewer::Viewer_Config&);

#endif // header guard
