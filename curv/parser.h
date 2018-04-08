// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_PARSER_H
#define CURV_PARSER_H

#include <curv/phrase.h>
#include <curv/scanner.h>

namespace curv {

Shared<Program_Phrase> parse_program(Scanner&);

} // namespace curv
#endif // header guard
