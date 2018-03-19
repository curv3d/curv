// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

extern "C" {
#include "readlinex.h"
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
}
#include <iostream>
#include <fstream>

#include "progdir.h"
#include <curv/dtostr.h>
#include <curv/analyser.h>
#include <curv/context.h>
#include <curv/program.h>
#include <curv/exception.h>
#include <curv/file.h>
#include <curv/parser.h>
#include <curv/phrase.h>
#include <curv/shared.h>
#include <curv/system.h>
#include <curv/list.h>
#include <curv/record.h>
#include <curv/gl_compiler.h>
#include <curv/shape.h>
#include <curv/version.h>
#include <curv/die.h>

void export_stl(curv::Value value,
    curv::System& sys, const curv::Context& cx, std::ostream& out)
{
    (void)out;
    curv::Shape_Recognizer shape(cx);
    if (!shape.recognize(value))
        throw curv::Exception(cx, "not a shape");

    throw curv::Exception(cx, "STL export not supported yet");
}
