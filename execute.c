#include "execute.h"
#include "command.h"
#include "handler.h"
#include <string.h>
#include <err.h>

static int
dipshp_execute_script(
    const dipsh_symbol *ast
)
{
    dipsh_nonterminal_child *children =
        ((dipsh_nonterminal *)ast)->children_list;
    int ret = 0;
    while (0 == ret && children) {
        ret |= dipsh_execute_ast(children->child);
        children = children->next;
    }
    return ret;
}

static int
dipshp_execute_seq_bg_start(
    const dipsh_symbol *ast
)
{
    return 1;
}

static int
dipshp_execute_and_or(
    const dipsh_symbol *ast
)
{
    return 1;
}

static int
dipshp_execute_pipe(
    const dipsh_symbol *ast
)
{
    return 1;
}

static int
dipshp_execute_command(
    const dipsh_symbol *ast
)
{
    dipsh_command *command = dipsh_command_init(ast);
    if (!command) {
        warnx("command unexpectedly failed");
        return 0;
    }
    dipsh_command_status status;
    int command_ret = dipsh_command_execute(command, &status);
    if (dipsh_handler_ok == command_ret) {
        if (status.exited_normally && !status.exited_by_code) {
            warnx(
                "%s: caught signal SIG%s",
                *dipsh_command_get_argv(command),
                sigabbrev_np(status.signal_num)
            );
        }
    } 
    dipsh_command_destroy(command);
    return 0;
}

int
dipsh_execute_ast(
    const dipsh_symbol *ast
)
{
    switch (ast->type) {
    case dipsh_symbol_script:
        return dipshp_execute_script(ast);
    case dipsh_symbol_seq_bg_start:
        return dipshp_execute_seq_bg_start(ast);
    case dipsh_symbol_and_or:
        return dipshp_execute_and_or(ast);
    case dipsh_symbol_pipe:
        return dipshp_execute_pipe(ast);
    case dipsh_symbol_command:
        return dipshp_execute_command(ast);
    default: /* shouldn't happen */
        return 1;
    }
}
