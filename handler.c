#include "handler.h"
#include "command.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

static int
dipshp_open_file_redir(
    const dipsh_redirect *redir
)
{
    errno = 0;
    if (!redir || !redir->need_open_file)
        return -1;
    
    int open_flags;
    switch (redir->type) {
    case dipsh_redir_in:  
        open_flags = O_RDONLY; 
        break;
    case dipsh_redir_out: 
        open_flags = O_WRONLY | O_CREAT | O_TRUNC; 
        break;
    case dipsh_redir_app:
        open_flags = O_WRONLY | O_CREAT | O_TRUNC | O_APPEND;
        break;
    default:
        return -1;
    }

    return open(redir->file_name, open_flags, 0666);
}

#define DIPSHP_CD_USAGE                                                        \
    "cd -- change working directory\n\n"                                       \
    "Usage:\n"                                                                 \
    "   cd [-h|--help] [DIR]\n\n"                                              \
    "Description:\n"                                                           \
    "Changes working directory to DIR. If DIR is not specified, then the "     \
    "value from HOME environment variable will be used.\n\n"                   \
    "Parameters:\n"                                                            \
    "   DIR         the directory to change to\n"                              \
    "   -h, --help  this help message\n"                                        

#define DIPSHP_PWD_USAGE                                                       \
    "pwd -- print working directory\n\n"                                       \
    "Usage:\n"                                                                 \
    "   pwd [-h|--help]\n\n"                                                   \
    "Description:\n"                                                           \
    "Prints absolute path to the current working directory.\n\n"               \
    "Parameters:\n"                                                            \
    "   -h, --help  this help message\n"                                        

static int
dipshp_is_help_arg(
    const char *arg
)
{
    return 0 == strcmp(arg, "--help") || 0 == strcmp(arg, "-h");
}

static int
dipshp_write_to_command_fd(
    dipsh_command *command,
    dipsh_command_status *status,
    int command_fd,
    const char *msg
)
{
    const char *command_name = *dipsh_command_get_argv(command);
    status->exited_normally = 1;
    status->exited_by_code = 1;
    status->exit_code = 0;

    int result_fd;
    const dipsh_redirect *redir = dipsh_command_get_redirect(
        command, command_fd
    );
   
    int should_close_result_fd = 0; 
    if (redir) {
        if (redir->need_open_file) {
            result_fd = dipshp_open_file_redir(redir);
            should_close_result_fd = 1;
        } else {
            result_fd = redir->inherited_fd;
        }
    } else {
        result_fd = command_fd;
    }

    if (-1 == result_fd) {
        if (0 == errno)
            warnx("%s: incorrect output file descriptor", command_name);
        else
            warn("%s: can't open file", command_name);
        status->exit_code = 1;
        return dipsh_handler_ok;
    }
    int msg_len = strlen(msg);
    int write_ret = write(result_fd, msg, msg_len);
    if (msg_len != write_ret) {
        warn("%s: writing to file failed", command_name);
        status->exit_code = 1;
    }
    if (should_close_result_fd)
        close(result_fd);
    return dipsh_handler_ok;
}

static int
dipshp_write_fmt_to_command_fd(
    dipsh_command *command,
    dipsh_command_status *status,
    int command_fd,
    const char *fmt, ...
)
{
    va_list ap;
    va_start(ap, fmt);
    char *res_str;
    int ret = vasprintf(&res_str, fmt, ap);
    va_end(ap);

    if (ret <= 0) {
        status->exited_normally = 0;
        return dipsh_handler_system_error; 
    }
    ret = dipshp_write_to_command_fd(command, status, command_fd, res_str);
    free(res_str);
    return ret;
}

#define DIPSHP_PRINT_ERROR_TO_STDERR(command, status, err)             \
    do {                                                               \
        int ret = dipshp_write_to_command_fd(command, status, 2, err); \
        if (dipsh_handler_ok == ret)                                   \
            status->exit_code = 1;                                     \
        return ret;                                                    \
    } while (0)

#define DIPSHP_PRINT_FMT_ERROR_TO_STDERR(command, status, fmt, ...)    \
    do {                                                               \
        int ret = dipshp_write_fmt_to_command_fd(                      \
            command, status, 2, fmt, __VA_ARGS__                       \
        );                                                             \
        if (dipsh_handler_ok == ret)                                   \
            status->exit_code = 1;                                     \
        return ret;                                                    \
    } while (0)

static int
dipshp_handle_cd(
    dipsh_command *command,
    dipsh_command_status *status
)
{
    int argc = dipsh_command_get_argc(command);
    char **argv = dipsh_command_get_argv(command);
    if (argc > 2 || (argc == 2 && dipshp_is_help_arg(argv[1]))) 
        return dipshp_write_to_command_fd(command, status, 2, DIPSHP_CD_USAGE);
    
    status->exited_normally = 1;
    status->exited_by_code = 1;
    status->exit_code = 0;

    const char *new_wd = argc == 2 ? argv[1] : getenv("HOME");
    if (!new_wd) 
        DIPSHP_PRINT_ERROR_TO_STDERR(command, status, "cd: unknown HOME\n");
    int chdir_ret = chdir(new_wd);
    if (-1 == chdir_ret) {
        DIPSHP_PRINT_FMT_ERROR_TO_STDERR(
            command, status,
            "cd: can't change working directory: %s\n", 
            strerror(errno)
        );
    }
    return dipsh_handler_ok;
}

static int
dipshp_handle_pwd(
    dipsh_command *command,
    dipsh_command_status *status
)
{
    int argc = dipsh_command_get_argc(command);
    if (argc > 1) 
        return dipshp_write_to_command_fd(command, status, 2, DIPSHP_PWD_USAGE);
    
    char *pwd_buf = get_current_dir_name();
    if (!pwd_buf) {
        DIPSHP_PRINT_FMT_ERROR_TO_STDERR(
            command, status,
            "pwd: can't get the current directory: %s\n", 
            strerror(errno)
        );
    }
    int ret = dipshp_write_to_command_fd(command, status, 1, pwd_buf);
    ret = dipshp_write_to_command_fd(command, status, 1, "\n");
    free(pwd_buf);
    return ret;
}

static void
dipshp_execute_external_command(
    dipsh_command *command
)
{
    char **argv = dipsh_command_get_argv(command);
    const dipsh_redirect_list *redirs = dipsh_command_get_all_redirects(command);
    while (redirs) {
        int fd_to_dup;
        if (redirs->redir.need_open_file)
            fd_to_dup = dipshp_open_file_redir(&redirs->redir);
        else
            fd_to_dup = redirs->redir.inherited_fd;
        int dup_ret = dup2(fd_to_dup, redirs->redir.fd);
        if (-1 == dup_ret)
            err(1, "%s: failure in dup2()", argv[0]);
        close(fd_to_dup);
        redirs = redirs->next;
    }
    execvp(argv[0], argv);
}

static int
dipshp_run_external_command(
    dipsh_command *command,
    int *command_status
)
{
    int pid = fork();
    char *command_name = *dipsh_command_get_argv(command);
    if (0 == pid) {
        dipshp_execute_external_command(command);
        err(1, "%s: can't execute command", command_name);
    } else if (0 < pid) {
        int wait_ret = waitpid(pid, command_status, 0);
        if (-1 == wait_ret) {
            warn("%s: can't wait for the command", command_name);
            return dipsh_handler_system_error;
        }
        return dipsh_handler_ok;
    } else {
        warn("%s: failure in fork()", command_name);
        return dipsh_handler_system_error;
    }
}

static int
dipshp_handle_external_command(
    dipsh_command *command,
    dipsh_command_status *status
)
{
    int command_status;
    int ret = dipshp_run_external_command(command, &command_status);
    if (dipsh_handler_system_error == ret) {
        status->exited_normally = 0;
    } else {
        status->exited_normally = 1;
        status->exited_by_code = WIFEXITED(command_status);
        if (status->exited_by_code)
            status->exit_code = WEXITSTATUS(command_status);
        else if (WIFSIGNALED(command_status))
            status->exit_code = WTERMSIG(command_status);
        else /* something weird */
            status->exited_normally = 0;
    }
    return ret;
}

typedef struct dipshp_handler_traits
{
    const char *name;
    dipsh_command_handler handler;
}
dipshp_handler_traits;

static const dipshp_handler_traits
dipshp_handlers[] = {
    { "cd", dipshp_handle_cd },
    { "pwd", dipshp_handle_pwd },
    { NULL, dipshp_handle_external_command }
};

dipsh_command_handler
dipsh_get_handler_by_name(
    const char *command_name
)
{
    const dipshp_handler_traits *traits = dipshp_handlers;
    while (traits->name) {
        if (0 == strcmp(traits->name, command_name))
            return traits->handler;
        ++traits;
    }
    return dipshp_handle_external_command;
}
