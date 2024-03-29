// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PARSER_H
#define LIBCURV_PARSER_H

#include <libcurv/phrase.h>
#include <libcurv/scanner.h>

namespace curv {

Shared<Program_Phrase> parse_program(Scanner&);

} // namespace curv
#endif // header guard
