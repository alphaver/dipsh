#include "pipeline.h"
#include "command.h"
#include "handler.h"
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

struct dipsh_pipeline_tag
{
    dipsh_command **commands;
    int commands_len;
};

const dipsh_command_traits dipshp_pipeline_command_traits = {
    .suspend_after_fork = 1,
    .run_in_separate_group = 0
};

dipsh_pipeline *
dipsh_pipeline_init(
    const dipsh_symbol *pipeline_tree
)
{
    if (dipsh_symbol_pipeline != pipeline_tree->type)
        return NULL;

    const dipsh_nonterminal *pipeline_nt = 
        (const dipsh_nonterminal *)pipeline_tree;
    const dipsh_nonterminal_child *children = pipeline_nt->children_list;

    dipsh_pipeline *result = calloc(sizeof(dipsh_pipeline), 1);
    if (!result)
        return NULL;
    const dipsh_nonterminal_child *curr = children;
    while (curr) {
        ++result->commands_len;
        curr = curr->next;
    }
    result->commands = calloc(sizeof(dipsh_command *), result->commands_len);
    if (!result->commands)
        return NULL;
    curr = children;
    int i = 0;
    while (curr) {
        result->commands[i] = dipsh_command_init(
            curr->child, &dipshp_pipeline_command_traits
        );
        if (!result->commands[i]) {
            dipsh_pipeline_destroy(result);
            return NULL;
        }
        ++i;
        curr = curr->next;
    }
    return result;
}

void
dipsh_pipeline_destroy(
    dipsh_pipeline *pipeline
)
{
    if (!pipeline)
        return;
    for (int i = 0; i < pipeline->commands_len; ++i)
        dipsh_command_destroy(pipeline->commands[i]);
    free(pipeline);
}

static int
dipshp_get_pipes(
    dipsh_pipeline *pipeline,
    int **pipes_fds
)
{
    *pipes_fds = malloc(2 * sizeof(int) * (pipeline->commands_len - 1));
    if (!*pipes_fds)
        return -1;
    for (int i = 0; i < pipeline->commands_len - 1; ++i) {
        int pipe_ret = pipe(*pipes_fds + (2 * i));
        if (-1 == pipe_ret) {
            for (int j = 0; j < 2 * i; ++j)
                close(*pipes_fds[j]);
            return -1;
        }
    }
    return 0;
}

static int
dipshp_start_command(
    dipsh_command *command,
    int stdin_fd,
    int stdout_fd,
    dipsh_command_status *last_command_status
)
{
    if (-1 != stdin_fd)
        dipsh_command_set_fd_redirect(command, dipsh_redir_in, 0, stdin_fd);
    if (-1 != stdout_fd)
        dipsh_command_set_fd_redirect(command, dipsh_redir_out, 1, stdout_fd);
    return dipsh_command_execute(command, last_command_status);
}

int
dipsh_pipeline_execute(
    dipsh_pipeline *pipeline,
    dipsh_command_status *last_command_status
)
{
    int *pipes_fds;
    int ret = dipshp_get_pipes(pipeline, &pipes_fds);
    if (0 != ret) {
        warn("pipeline failure: can't create enough pipes");
        return dipsh_handler_system_error;
    }
    for (int i = 0; i < pipeline->commands_len; ++i) {
        ret = dipshp_pipeline_start_command(
            pipeline->commands[i],
            0 == i ? -1 : pipes_fds[2 * (i - 1)],
            pipeline->commands_len - 1 == i ? -1 : pipes_fds[2 * i + 1],
            pipeline->commands_len - 1 == i ? last_command_status : NULL
        );
        if (dipsh_handler_ok != ret) 
            goto cleanup;
    }

cleanup:
    for (int i = 0; i < 2 * (pipeline->commands_len - 1); ++i)
        close(pipes_fds[i]);
    return ret;
}
