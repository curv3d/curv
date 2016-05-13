/*
 * Copyright 2016 Doug Moen. See LICENCE.md file for terms of use.
 */
#ifndef CURV_PARSE_H
#define CURV_PARSE_H

#include <curv/expr.h>
#include <curv/script.h>

namespace curv {

std::unique_ptr<Expr> parse(const Script&);

}
#endif
