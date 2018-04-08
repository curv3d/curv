// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef READLINEX_H
#define READLINEX_H

#ifdef __cplusplus
extern "C" {
#endif

enum RLXResult { rlx_okay, rlx_eof, rlx_interrupt };

/// A wrapper around GNU readline() which flushes its state and input
/// and returns immediately if a keyboard interrupt is sent.
///
/// If the line has any text in it, save it on the history.
///
/// This matches the behaviour of bash and many other programs that use
/// readline. The necessary code is arcane, hence the wrapper.
char *readlinex(const char* prompt, enum RLXResult* result);

#ifdef __cplusplus
}
#endif
#endif // header guard
