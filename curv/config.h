// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_CONFIG_H
#define CURV_CONFIG_H

#include <libcurv/context.h>
#include <libcurv/record.h>

curv::Shared<const curv::Record> get_config(
    const curv::Context&,
    curv::Shared<const curv::String>&);

#endif // header guard
