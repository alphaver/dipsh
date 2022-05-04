#ifndef _DIPSH_SHELL_STATE_H_
#define _DIPSH_SHELL_STATE_H_

#include "command.h"
#include "parser.h"
#include <signal.h>

typedef struct dipsh_shell_bg_command_tag
{
    int bg_pid;
    int finished;
    int bg_pipe[2];   
}
dipsh_shell_bg_command;

typedef struct dipsh_shell_bg_command_list_tag
{
    dipsh_shell_bg_command bg_command;
    struct dipsh_shell_bg_command_list_tag *next;
}
dipsh_shell_bg_command_list;

typedef struct dipsh_shell_state_tag
{
    int is_interactive;
    dipsh_command_status last_status;
    dipsh_shell_bg_command_list *bg_commands;
}
dipsh_shell_state;

sighandler_t
dipsh_init_chld_handler(
    dipsh_shell_state *state
);

typedef void (*dipsh_spawned_bg_command_cb)(
    int pid
);

int
dipsh_shell_state_spawn_bg_command(
    dipsh_shell_state *state,
    const dipsh_symbol *ast,
    dipsh_spawned_bg_command_cb bg_cb
);

int
dipsh_shell_state_mark_finished_command(
    dipsh_shell_state *state,
    int bg_pid
);

typedef void (*dipsh_finished_bg_command_cb)(
    int pid, 
    const dipsh_command_status *status
);

void
dipsh_shell_state_clear_finished_bg_commands(
    dipsh_shell_state *state,
    dipsh_finished_bg_command_cb bg_cb
);

#endif /* _DIPSH_SHELL_STATE_H_ */
