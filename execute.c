#include "execute.h"
#include "command.h"
#include "handler.h"
#include "pipeline.h"
#include "change_group.h"
#include <string.h>
#include <stdio.h>
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

static void
dipshp_bg_command_spawned_cb(
    int pid
)
{
    printf("[%d] Spawned\n", pid);
}

static int
dipshp_execute_seq_bg_start(
    const dipsh_symbol *ast,
    dipsh_shell_state *state
)
{
    dipsh_nonterminal_child *children =
        ((dipsh_nonterminal *)ast)->children_list;
    int seq_bg_ret = 0;
    int is_bg_op = 0;
    const dipsh_symbol *command;
    const dipsh_symbol *op;
    while (0 == seq_bg_ret && children) {
        command = children->child;
        op = children->next ? children->next->child : NULL;
        is_bg_op = op && dipsh_symbol_bg == op->type;
        if (is_bg_op) {
            seq_bg_ret = dipsh_shell_state_spawn_bg_command(
                state, command, dipshp_bg_command_spawned_cb
            );
        } else {
            seq_bg_ret = dipsh_execute_ast(command, state);
        }
        if (op)
            children = children->next->next;
        else
            children = NULL;
    }
    return seq_bg_ret;
}

static int
dipshp_execute_and_or(
    const dipsh_symbol *ast,
    dipsh_shell_state *state
)
{
    dipsh_nonterminal_child *children =
        ((dipsh_nonterminal *)ast)->children_list;
    int and_or_ret = 0; 
    int curr_status = 0;
    const dipsh_symbol *command;
    const dipsh_symbol *op;
    while (0 == and_or_ret && children) {
        command = children->child;
        op = children->next ? children->next->child : NULL;
        and_or_ret = dipsh_execute_ast(command, state);
        if (0 != and_or_ret || 
            !state->last_status.exited_normally || 
            !state->last_status.exited_by_code) {
            and_or_ret = 1;
            break;
        }
        curr_status = state->last_status.exit_code;
        if (op) {
            int is_and_op = dipsh_symbol_and == op->type;
            if ((curr_status && is_and_op) ||
                (!curr_status && !is_and_op)) {
                break;
            }
            children = children->next->next;
        } else {
            children = NULL;
        }
    }
    return and_or_ret;
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
    if (0 == pipeline_ret && state->is_interactive)
        dipshp_handle_command_result("pipeline", status);
    if (0 != pipeline_ret) 
        goto cleanup;
    memcpy(&state->last_status, status, sizeof(dipsh_command_status));
cleanup:
    dipsh_pipeline_destroy(pipeline);
    return pipeline_ret;
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
    if (dipsh_handler_ok == command_ret && state->is_interactive &&
        !dipsh_command_is_builtin(command)) {
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
    if (dipsh_handler_ok == command_ret && state->is_interactive)
        dipshp_handle_command_result(*dipsh_command_get_argv(command), status);
    if (state->is_interactive && !dipsh_command_is_builtin(command)) {
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
