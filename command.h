#ifndef _DIPSH_COMMAND_H_
#define _DIPSH_COMMAND_H_

#include "parser.h"

typedef enum dipsh_redir_type_tag
{
    dipsh_redir_in,
    dipsh_redir_out,
    dipsh_redir_app,
    dipsh_redir_close,
    dipsh_redir_unknown
}
dipsh_redir_type;

typedef struct dipsh_redirect_tag
{
    dipsh_redir_type type;
    int fd;
    int need_open_file;
    union
    {
        int inherited_fd;
        char *file_name;
    };
}
dipsh_redirect;

typedef struct dipsh_redirect_list_tag
{
    dipsh_redirect redir;
    struct dipsh_redirect_list_tag *next;
}
dipsh_redirect_list;

typedef struct dipsh_command_traits_tag
{
    int suspend_after_fork;
    int run_in_separate_group;
    int will_wait_for_group_change;
    int execute_blocks;
}
dipsh_command_traits;

typedef struct dipsh_command_tag dipsh_command;

dipsh_command *
dipsh_command_init(
    const dipsh_symbol *command_tree,
    const dipsh_command_traits *traits
);

void
dipsh_command_destroy(
    dipsh_command *command
);

enum
{
    dipsh_redir_set_ok,
    dipsh_redir_fd_already_taken,
    dipsh_redir_incorrect_fd
};

int
dipsh_command_set_fd_redirect(
    dipsh_command *command,
    dipsh_redir_type redir_type,
    int command_fd, 
    int fd_to_set
);

int
dipsh_command_mark_fd_for_close(
    dipsh_command *command,
    int fd_to_close
);

const dipsh_redirect *
dipsh_command_get_redirect(
    const dipsh_command *command,
    int command_fd
);

const dipsh_redirect_list *
dipsh_command_get_all_redirects(
    const dipsh_command *command
);

int
dipsh_command_get_argc(
    const dipsh_command *command
);

char **
dipsh_command_get_argv(
    dipsh_command *command
);

const dipsh_command_traits *
dipsh_command_get_traits(
    const dipsh_command *command
);

int
dipsh_command_get_pid(
    const dipsh_command *command
);

void
dipsh_command_set_pid(
    dipsh_command *command,
    int pid
);

int
dipsh_command_get_suspend_wait_fd(
    const dipsh_command *command
);

int
dipsh_command_is_builtin(
    const dipsh_command *command
);

int
dipsh_command_start_suspended(
    dipsh_command *command
);

int
dipsh_command_signal_group_change(
    dipsh_command *command
);

int
dipsh_command_wait_for_group_change(
    dipsh_command *command
);

typedef struct dipsh_command_status_tag
{
    int exited_normally;
    int exited_by_code;
    union
    {
        int exit_code;
        int signal_num;
    };
}
dipsh_command_status;

void
dipsh_wait_status_to_command_status(
    int exited_normally,
    int wait_status,
    dipsh_command_status *command_status
);

const dipsh_command_status *
dipsh_wait_for_command(
    dipsh_command *command
);

int
dipsh_command_execute(
    dipsh_command *command
);

#endif /* _DIPSH_COMMAND_H_ */
