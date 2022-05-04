#include "shell_state.h"
#include "execute.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

static int
dipshp_shell_state_bg_do_fork(
    dipsh_shell_state *state,
    const dipsh_symbol *ast,
    dipsh_shell_bg_command *bg_command
)
{
    bg_command->bg_pipe[0] = -1;
    bg_command->bg_pipe[1] = -1;
    int ret = pipe(bg_command->bg_pipe);
    if (-1 == ret)
        goto fail_exit;
    int pid = fork();
    if (0 == pid) {
        setpgid(0, 0);
        close(bg_command->bg_pipe[0]);
        state->is_interactive = 0;
        ret = dipsh_execute_ast(ast, state);
        write(
            bg_command->bg_pipe[1], &state->last_status, 
            sizeof(dipsh_command_status)
        );
        exit(ret);
    } else if (0 < pid) {
        close(bg_command->bg_pipe[1]);
        bg_command->bg_pid = pid;
        return 0;
    } else {
        goto fail_exit;
    }

fail_exit:
    if (-1 != bg_command->bg_pipe[0]) {
        close(bg_command->bg_pipe[0]);
        close(bg_command->bg_pipe[1]);
    }
    return 1; 
}

int
dipsh_shell_state_spawn_bg_command(
    dipsh_shell_state *state,
    const dipsh_symbol *ast,
    dipsh_spawned_bg_command_cb bg_cb
)
{
    dipsh_shell_bg_command_list *list_item = 
        calloc(sizeof(dipsh_shell_bg_command_list), 1);
    if (!list_item)
        goto fail;

    list_item->next = state->bg_commands;
    state->bg_commands = list_item;

    int ret = dipshp_shell_state_bg_do_fork(state, ast, &list_item->bg_command);
    if (0 != ret)
        goto fail;
    else {
        if (bg_cb)
            bg_cb(list_item->bg_command.bg_pid);
        return 0;
    }

fail:
    if (list_item && state->bg_commands == list_item) 
        state->bg_commands = list_item->next;
    free(list_item);
    return 1;
}

int
dipsh_shell_state_mark_finished_command(
    dipsh_shell_state *state,
    int bg_pid
)
{
    dipsh_shell_bg_command_list *bg_commands = state->bg_commands;
    while (bg_commands) {
        if (bg_commands->bg_command.bg_pid == bg_pid) {
            bg_commands->bg_command.finished = 1;
            return 0;
        }
        bg_commands = bg_commands->next;
    }
    return 1;
}


void
dipsh_shell_state_clear_finished_bg_commands(
    dipsh_shell_state *state,
    dipsh_finished_bg_command_cb bg_cb
)
{
    dipsh_shell_bg_command_list **bg_commands = &state->bg_commands;
    dipsh_command_status status;
    while (*bg_commands) {
        int wait_status;
        int wait_ret = waitpid(
            (*bg_commands)->bg_command.bg_pid, &wait_status, WNOHANG
        );
        if (0 == wait_ret) {
            bg_commands = &((*bg_commands)->next);
        } else {
            int read_ret = read(
                (*bg_commands)->bg_command.bg_pipe[0], &status, sizeof(status)
            );
            close((*bg_commands)->bg_command.bg_pipe[0]);
            if (bg_cb) {
                bg_cb(
                    (*bg_commands)->bg_command.bg_pid, 
                    sizeof(status) == read_ret ? &status : NULL
                ); 
            }
            dipsh_shell_bg_command_list *temp = *bg_commands;
            *bg_commands = (*bg_commands)->next;
            free(temp);
        }
    }
}
