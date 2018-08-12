// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_ANSI_COLOUR_H
#define CURV_ANSI_COLOUR_H

// Escape sequences for setting the foreground text colour in a terminal.

#define AC_RED     "\x1b[31m"
#define AC_GREEN   "\x1b[32m"
#define AC_YELLOW  "\x1b[33m"
#define AC_BLUE    "\x1b[34m"
#define AC_MAGENTA "\x1b[35m"
#define AC_CYAN    "\x1b[36m"

#define AC_Black            "\x1b[0;30m"
#define AC_Blue             "\x1b[0;34m"
#define AC_Green            "\x1b[0;32m"
#define AC_Cyan             "\x1b[0;36m"
#define AC_Red              "\x1b[0;31m"
#define AC_Purple           "\x1b[0;35m"
#define AC_Brown            "\x1b[0;33m"
#define AC_Grey             "\x1b[0;37m"
#define AC_Dark_Grey        "\x1b[1;30m"
#define AC_Light_Blue       "\x1b[1;34m"
#define AC_Light_Green      "\x1b[1;32m"
#define AC_Light_Cyan       "\x1b[1;36m"
#define AC_Light_Red        "\x1b[1;31m"
#define AC_Light_Purple     "\x1b[1;35m"
#define AC_Yellow           "\x1b[1;33m"
#define AC_White            "\x1b[1;37m"

#define AC_RESET    "\x1b[m"
#define AC_BOLD     "\x1b[1m"

// Curv colour scheme.
// Instead of the usual "unicorn barf",
// I want a simple, coordinated colour scheme.
#define AC_PROMPT   AC_Dark_Grey
#define AC_LINENO   AC_Dark_Grey
#define AC_MESSAGE  AC_BOLD
#define AC_CARET    AC_Light_Red

#endif
