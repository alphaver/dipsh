#include "command.h"
#include "handler.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

struct dipsh_command_tag
{
    char **argv;
    int argv_len;
    int argv_cap;
    
    int pid_set;
    int pid;
    int no_system_error;

    int suspend_pipe_fds[2];
    int group_signal_pipe_fds[2];

    dipsh_redirect_list *redir_list;
    dipsh_command_traits traits;

    int wait_performed;
    int wait_failed;
    dipsh_command_status status;
   
    int is_builtin; 
    dipsh_command_handler handler;
};

static void
dipshp_append_word_to_argv(
    dipsh_command *command,
    const char *word
)
{
    if (command->argv_len + 1 > command->argv_cap) {
        char **old_argv = command->argv;
        command->argv_cap *= 2;
        command->argv = calloc(sizeof(char *), command->argv_cap);
        memcpy(command->argv, old_argv, sizeof(char *) * command->argv_len);
        free(old_argv);
    }
    command->argv[command->argv_len - 1] = strdup(word);
    ++command->argv_len;
}

static void
dipshp_clear_argv(
    dipsh_command *command
)
{
    for (int i = 0; i < command->argv_len; ++i)
        free(command->argv[i]);
    free(command->argv);
}

static int
dipshp_insert_file_redir(
    dipsh_redirect_list **redir_list,
    dipsh_redir_type type,
    int fd,
    const char *file_name
)
{
    if (fd < 0)
        return dipsh_redir_incorrect_fd;
    dipsh_redirect_list **curr = redir_list;
    while (*curr) {
        if ((*curr)->redir.fd == fd)
            return dipsh_redir_fd_already_taken;
        curr = &((*curr)->next);
    }
    *curr = calloc(sizeof(dipsh_redirect_list), 1);
    (*curr)->redir.fd = fd;
    (*curr)->redir.type = type;
    (*curr)->redir.need_open_file = 1;
    (*curr)->redir.file_name = strdup(file_name);
    return dipsh_redir_set_ok;
}

static int
dipshp_insert_fd_redir(
    dipsh_redirect_list **redir_list,
    dipsh_redir_type type,
    int fd,
    int fd_to_set
)
{
    if (fd < 0 && dipsh_redir_close != type)
        return dipsh_redir_incorrect_fd;
    dipsh_redirect_list **curr = redir_list;
    while (*curr) {
        if ((*curr)->redir.fd == fd && 
            dipsh_redir_close != type && 
            dipsh_redir_close != (*curr)->redir.type) {
            return dipsh_redir_fd_already_taken;
        }
        curr = &((*curr)->next);
    }
    /* when a fork is performed, fds remain the same, so, if the new fd and the
     * old fd are equal, there is no need to do any redirect */
    if (fd != fd_to_set) {
        *curr = calloc(sizeof(dipsh_redirect_list), 1);
        (*curr)->redir.fd = fd;
        (*curr)->redir.type = type;
        (*curr)->redir.need_open_file = 0;
        (*curr)->redir.inherited_fd = fd_to_set;
    }
    return dipsh_redir_set_ok;
}

static void
dipshp_clear_file_redirs(
    dipsh_redirect_list *redir_list
)
{
    while (redir_list) {
        dipsh_redirect_list *temp = redir_list;
        redir_list = redir_list->next;
        if (temp->redir.need_open_file)
            free(temp->redir.file_name);
        free(temp);
    }
}

static dipsh_redir_type
dipshp_string_to_redir_type(
    const char *str
)
{
    if (0 == strcmp(str, "<")) 
        return dipsh_redir_in;
    else if (0 == strcmp(str, ">"))
        return dipsh_redir_out;
    else if (0 == strcmp(str, ">>"))
        return dipsh_redir_app;
    else
        return dipsh_redir_unknown;
}

static int
dipshp_redir_to_type_fd(
    const char *redir,
    dipsh_redir_type *type,
    int *fd
)
{
    char *endptr; 
    errno = 0;
    *fd = strtol(redir, &endptr, 10);
    if (endptr != redir && errno)
        return errno;
    *type = dipshp_string_to_redir_type(endptr);
    if (endptr == redir) {
        switch (*type) {
        case dipsh_redir_in:  *fd = 0; break;
        case dipsh_redir_out: *fd = 1; break;
        case dipsh_redir_app: *fd = 1; break;
        default:                       return 1;
        }
    }
    return 0;
}

static int
dipshp_add_redir(
    dipsh_command *command,
    const dipsh_symbol *redir_child
)
{
    const dipsh_nonterminal *redir_symb = 
        (const dipsh_nonterminal *)redir_child;
    const dipsh_terminal *redir_type = 
        (const dipsh_terminal *)redir_symb->children_list->child;
    const dipsh_terminal *redir_file =
        (const dipsh_terminal *)redir_symb->children_list->next->child;
    dipsh_redir_type type;
    int fd;
    int ret = dipshp_redir_to_type_fd(
        redir_type->token.value, &type, &fd
    );
    if (0 == ret) {
        ret = dipshp_insert_file_redir(
            &command->redir_list, type, fd, redir_file->token.value
        );
    }
    return ret;
}

static void
dipshp_set_default_trait_values(
    dipsh_command_traits *traits
)
{
    traits->suspend_after_fork = 0;
    traits->run_in_separate_group = 1;
    traits->will_wait_for_group_change = 0;
    traits->execute_blocks = 1;
}

dipsh_command *
dipsh_command_init(
    const dipsh_symbol *command_tree,
    const dipsh_command_traits *traits
)
{
    if (dipsh_symbol_command != command_tree->type)
        return NULL;

    dipsh_command *result = calloc(sizeof(dipsh_command), 1);
    result->argv = calloc(sizeof(char*), 1);
    result->argv_len = 1;
    result->argv_cap = 1;

    const dipsh_nonterminal_child *children = 
        ((dipsh_nonterminal *)command_tree)->children_list;
    while (children) {
        if (dipsh_symbol_word == children->child->type) {
            dipshp_append_word_to_argv(
                result, ((dipsh_terminal *)children->child)->token.value
            );
        } else if (dipsh_symbol_redir == children->child->type) {
            int not_ok = dipshp_add_redir(result, children->child);
            if (not_ok) 
                goto fail;
        }
        children = children->next;
    }
    
    result->handler = dipsh_get_handler_by_name(result->argv[0]);
    result->is_builtin = dipsh_has_builtin_handler(result->argv[0]);
    
    if (traits)
        memcpy(&result->traits, traits, sizeof(dipsh_command_traits));
    else
        dipshp_set_default_trait_values(&result->traits);

    result->suspend_pipe_fds[0] = -1;
    result->suspend_pipe_fds[1] = -1;
    if (result->traits.suspend_after_fork) {
        int not_ok = pipe2(result->suspend_pipe_fds, O_CLOEXEC);
        if (-1 == not_ok)
            goto fail;
    }
    result->group_signal_pipe_fds[0] = -1;
    result->group_signal_pipe_fds[1] = -1;
    if (result->traits.run_in_separate_group && 
        result->traits.will_wait_for_group_change) {
        int not_ok = pipe2(result->group_signal_pipe_fds, O_CLOEXEC);
        if (-1 == not_ok)
            goto fail;
    }

    return result;

fail:
    dipsh_command_destroy(result);
    return NULL;
}

void
dipsh_command_destroy(
    dipsh_command *command
)
{
    if (!command)
        return;
    if (!command->traits.execute_blocks)
        dipsh_wait_for_command(command);
    dipshp_clear_argv(command);
    dipshp_clear_file_redirs(command->redir_list);
    free(command);
}

int
dipsh_command_set_fd_redirect(
    dipsh_command *command,
    dipsh_redir_type redir_type,
    int command_fd,
    int fd_to_set
)
{
    return dipshp_insert_fd_redir(
        &command->redir_list, redir_type, command_fd, fd_to_set
    );
}

int
dipsh_command_mark_fd_for_close(
    dipsh_command *command,
    int fd_to_close
)
{
    return dipshp_insert_fd_redir(
        &command->redir_list, dipsh_redir_close, 0, fd_to_close
    );
}

const dipsh_redirect *
dipsh_command_get_redirect(
    const dipsh_command *command,
    int command_fd
)
{
    dipsh_redirect_list *pos = command->redir_list;
    while (pos) {
        if (pos->redir.fd == command_fd)
            return (dipsh_redirect *)pos;
    }
    return NULL;
}

const dipsh_redirect_list *
dipsh_command_get_all_redirects(
    const dipsh_command *command
)
{
    return command->redir_list;
}

int
dipsh_command_get_argc(
    const dipsh_command *command
)
{
    return command->argv_len - 1;
}

char **
dipsh_command_get_argv(
    dipsh_command *command
)
{
    return command->argv;
}

const dipsh_command_traits *
dipsh_command_get_traits(
    const dipsh_command *command
)
{
    return &command->traits;
}

int
dipsh_command_get_pid(
    const dipsh_command *command
)
{
    return command->pid;
}

void
dipsh_command_set_pid(
    dipsh_command *command,
    int pid
)
{
    if (command->pid_set)
        return;

    command->pid_set = 1;
    command->pid = pid;
}

int
dipsh_command_get_suspend_wait_fd(
    const dipsh_command *command
)
{
    return command->suspend_pipe_fds[0]; 
}

int
dipsh_command_is_builtin(
    const dipsh_command *command
)
{
    return command->is_builtin;
}

int
dipsh_command_start_suspended(
    dipsh_command *command
)
{
    if (!command->traits.suspend_after_fork)
        return 1;

    int ret = write(command->suspend_pipe_fds[1], "", 1);
    close(command->suspend_pipe_fds[0]);
    close(command->suspend_pipe_fds[1]);
    return 1 == ret ? 0 : 1;
}

int
dipsh_command_signal_group_change(
    dipsh_command *command
)
{
    int ret = write(command->group_signal_pipe_fds[1], "", 1);
    return 1 == ret ? 0 : 1;
}

int
dipsh_command_wait_for_group_change(
    dipsh_command *command
)
{
    char dummy;
    int ret = read(command->group_signal_pipe_fds[0], &dummy, 1);
    close(command->group_signal_pipe_fds[0]);
    close(command->group_signal_pipe_fds[1]);
    return 1 == ret ? 0 : 1;
}

void
dipsh_wait_status_to_command_status(
    int exited_normally,
    int wait_status,
    dipsh_command_status *command_status
)
{
    if (!command_status) 
       return;

    command_status->exited_normally = exited_normally;
    if (exited_normally) {
        command_status->exited_by_code = WIFEXITED(wait_status);
        if (command_status->exited_by_code)
            command_status->exit_code = WEXITSTATUS(wait_status);
        else if (WIFSIGNALED(wait_status))
            command_status->signal_num = WTERMSIG(wait_status);
        else /* something weird */
            command_status->exited_normally = 0;
    }
}

const dipsh_command_status *
dipsh_wait_for_command(
    dipsh_command *command
)
{
    if (command->wait_performed)
        return command->wait_failed ? NULL : &command->status;

    command->wait_performed = 1;
    if (!command->is_builtin) {
        int wait_status;
        int wait_ret = waitpid(command->pid, &wait_status, 0);
        if (-1 == wait_ret) {
            command->wait_failed = 1;
            return NULL;
        }
        dipsh_wait_status_to_command_status(1, wait_status, &command->status);
    }
    return &command->status;
}

int
dipsh_command_execute(
    dipsh_command *command
)
{
    return command->handler(command, &command->status);
}
