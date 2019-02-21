// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_JSON_H
#define LIBCURV_JSON_H

#include <libcurv/value.h>
#include <ostream>

namespace curv {

void write_json_string(const char*, std::ostream&);
void write_json_value(Value, std::ostream&);

} // namespace curv
#endif // header guard
