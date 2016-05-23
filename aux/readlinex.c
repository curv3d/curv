// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <aux/readlinex.h>
#include <readline/readline.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

sigjmp_buf rlx_env;

// It's normally very dangerous to longjmp out of a signal handler.
// However, the strategy is recommended by the maintainer of readline(),
// and this interrupt handler is only installed while readline() is running.
void rlx_interrupt_handler(int sig)
{
    siglongjmp(rlx_env, 1);
}

char*
readlinex(const char* prompt, enum RLXResult* result)
{
    struct sigaction interrupt_action, old_action;
    char *line = NULL;
    *result = rlx_okay;

    if (sigsetjmp(rlx_env, 1) == 0) {
        // direct return from sigsetjmp
        memset((void*)&interrupt_action, 0, sizeof(interrupt_action));
        interrupt_action.sa_handler = rlx_interrupt_handler;
        sigaction(SIGINT, &interrupt_action, &old_action);
        line = readline(prompt);
        sigaction(SIGINT, &old_action, NULL);
        *result = (line == NULL ? rlx_eof : rlx_okay);
    } else {
        // jumped here from rlx_interrupt_handler
        // https://lists.gnu.org/archive/html/bug-readline/2016-04/msg00071.html
        rl_free_line_state();
        rl_cleanup_after_signal();
        RL_UNSETSTATE(RL_STATE_ISEARCH|RL_STATE_NSEARCH|RL_STATE_VIMOTION
            |RL_STATE_NUMERICARG|RL_STATE_MULTIKEY);
        rl_done = 1;
        line = NULL;
        *result = rlx_interrupt;
    }

    return line;
}
