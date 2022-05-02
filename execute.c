#include "execute.h"
#include "command.h"
#include "handler.h"
#include "pipeline.h"
#include "change_group.h"
#include <string.h>
#include <err.h>

static void
dipshp_handle_command_result(
    const char *command_name,
    const dipsh_command_status *status
)
{
    if (status->exited_normally && !status->exited_by_code) {
        warnx(
            "%s: caught signal SIG%s",
            command_name,
            sigabbrev_np(status->signal_num)
        );
    }
}

static int
dipshp_execute_script(
    const dipsh_symbol *ast,
    dipsh_shell_state *state
)
{
    dipsh_nonterminal_child *children =
        ((dipsh_nonterminal *)ast)->children_list;
    int ret = 0;
    while (0 == ret && children) {
        ret |= dipsh_execute_ast(children->child, state);
        children = children->next;
    }
    return ret;
}

static int
dipshp_execute_seq_bg_start(
    const dipsh_symbol *ast,
    dipsh_shell_state *state
)
{
    return 1;
}

static int
dipshp_execute_and_or(
    const dipsh_symbol *ast,
    dipsh_shell_state *state
)
{
    return 1;
}

static int
dipshp_execute_pipe(
    const dipsh_symbol *ast,
    dipsh_shell_state *state
)
{
    dipsh_pipeline *pipeline = dipsh_pipeline_init(ast, 1);
    if (!pipeline) {
        warnx("pipeline unexpectedly failed");
        return 0;
    }
    const dipsh_command_status *status;
    int pipeline_ret = dipsh_pipeline_execute(pipeline, state->is_interactive);
    status = dipsh_pipeline_get_last_command_status(pipeline);
    if (0 == pipeline_ret)
        dipshp_handle_command_result("pipeline", status);
    memcpy(&state->last_status, status, sizeof(dipsh_command_status));
    dipsh_pipeline_destroy(pipeline);
    return 0;
}

static int
dipshp_execute_command(
    const dipsh_symbol *ast,
    dipsh_shell_state *state
)
{
    const dipsh_command_traits traits = {
        .suspend_after_fork = state->is_interactive,
        .run_in_separate_group = 1,
        .execute_blocks = 0
    };
    dipsh_command *command = dipsh_command_init(ast, &traits);
    if (!command) {
        warnx("command unexpectedly failed");
        return 0;
    }
    const dipsh_command_status *status;
    int old_group;
    int command_ret = dipsh_command_execute(command);
    if (dipsh_handler_ok == command_ret && state->is_interactive) {
        command_ret = dipsh_change_current_group(
            dipsh_command_get_pid(command), &old_group
        );
        if (0 != command_ret) {
            warn("couldn't change current group for a command");
            goto cleanup;
        }
        command_ret = dipsh_command_start_suspended(command);
    }
    status = dipsh_wait_for_command(command);
    if (dipsh_handler_ok == command_ret)
        dipshp_handle_command_result(*dipsh_command_get_argv(command), status);
    if (state->is_interactive) {
        command_ret = dipsh_change_current_group(old_group, NULL);
        if (0 != command_ret) {
            warn("couldn't restore current group for a command");
            goto cleanup;
        }
    }
    memcpy(&state->last_status, status, sizeof(dipsh_command_status));
cleanup:
    dipsh_command_destroy(command);
    return command_ret;
}

int
dipsh_execute_ast(
    const dipsh_symbol *ast,
    dipsh_shell_state *state
)
{
    switch (ast->type) {
    case dipsh_symbol_script:
        return dipshp_execute_script(ast, state);
    case dipsh_symbol_seq_bg_start:
        return dipshp_execute_seq_bg_start(ast, state);
    case dipsh_symbol_and_or:
        return dipshp_execute_and_or(ast, state);
    case dipsh_symbol_pipe:
        return dipshp_execute_pipe(ast, state);
    case dipsh_symbol_command:
        return dipshp_execute_command(ast, state);
    default: /* shouldn't happen */
        return 1;
    }
}
