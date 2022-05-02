#include "pipeline.h"
#include "command.h"
#include "handler.h"
#include "change_group.h"
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <string.h>

struct dipsh_pipeline_tag
{
    dipsh_command **commands;
    int commands_len;

    int execute_blocks;
    int executed;
    dipsh_command_status last_command_status;
};

static const dipsh_command_traits dipshp_pipeline_command_traits = {
    .suspend_after_fork = 1,
    .run_in_separate_group = 0,
    .execute_blocks = 0
};

static const dipsh_command_traits dipshp_pipeline_first_command_traits = {
    .suspend_after_fork = 1,
    .run_in_separate_group = 1,
    .execute_blocks = 0
};

dipsh_pipeline *
dipsh_pipeline_init(
    const dipsh_symbol *pipeline_tree,
    int execute_blocks
)
{
    if (dipsh_symbol_pipe != pipeline_tree->type)
        return NULL;

    const dipsh_nonterminal *pipeline_nt = 
        (const dipsh_nonterminal *)pipeline_tree;
    const dipsh_nonterminal_child *children = pipeline_nt->children_list;

    dipsh_pipeline *result = calloc(sizeof(dipsh_pipeline), 1);
    if (!result)
        return NULL;
    result->execute_blocks = execute_blocks;
    const dipsh_nonterminal_child *curr = children;
    while (curr) {
        ++result->commands_len;
        curr = curr->next;
    }
    result->commands = calloc(sizeof(dipsh_command *), result->commands_len);
    if (!result->commands) {
        free(result);
        return NULL;
    }
    curr = children;
    int i = 0;
    while (curr) {
        result->commands[i] = dipsh_command_init(
            curr->child, 
            0 == i 
            ? &dipshp_pipeline_first_command_traits 
            : &dipshp_pipeline_command_traits
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
dipshp_pipeline_get_pipes(
    dipsh_pipeline *pipeline,
    int **pipes_fds
)
{
    *pipes_fds = malloc(2 * sizeof(int) * (pipeline->commands_len - 1));
    if (!*pipes_fds)
        return -1;
    for (int i = 0; i < pipeline->commands_len - 1; ++i) {
        int pipe_ret = pipe((*pipes_fds) + (2 * i));
        if (-1 == pipe_ret) {
            for (int j = 0; j < 2 * i; ++j)
                close((*pipes_fds)[j]);
            return -1;
        }
    }
    return 0;
}

static void
dipshp_pipeline_close_pipes(
    dipsh_pipeline *pipeline,
    int **pipes_fds
)
{
    if (!pipes_fds || !*pipes_fds)
        return;

    for (int i = 0; i < pipeline->commands_len - 1; ++i) {
        close((*pipes_fds)[2 * i]);
        close((*pipes_fds)[2 * i + 1]);
    }
    free(*pipes_fds);
    *pipes_fds = NULL;
}

static int
dipshp_pipeline_start_command(
    dipsh_pipeline *pipeline,
    int *pipes_fds,
    int command_idx
)
{
    dipsh_command *command = pipeline->commands[command_idx];
    for (int i = 0; i < pipeline->commands_len - 1; ++i) {
        if (command_idx - 1 == i) {
            dipsh_command_set_fd_redirect(
                command, dipsh_redir_in, 0, pipes_fds[2 * i]
            );
            dipsh_command_mark_fd_for_close(command, pipes_fds[2 * i + 1]);
        } else if (command_idx == i) {
            dipsh_command_set_fd_redirect(
                command, dipsh_redir_out, 1, pipes_fds[2 * i + 1]
            );
            dipsh_command_mark_fd_for_close(command, pipes_fds[2 * i]);
        } else {
            dipsh_command_mark_fd_for_close(command, pipes_fds[2 * i]);
            dipsh_command_mark_fd_for_close(command, pipes_fds[2 * i + 1]);
        }
    }
    return dipsh_command_execute(command);
}

static int
dipshp_pipeline_start_all_commands(
    dipsh_pipeline *pipeline,
    int *pipes_fds
)
{
    int ret = dipsh_handler_ok;
    int i;
    for (i = 0; i < pipeline->commands_len && dipsh_handler_ok == ret; ++i)
        ret = dipshp_pipeline_start_command(pipeline, pipes_fds, i);

    return ret;
}

static int
dipshp_pipeline_change_group(
    dipsh_pipeline *pipeline
)
{
    int new_pgid = dipsh_command_get_pid(pipeline->commands[0]);
    int ret = 0;
    for (int i = 1; i < pipeline->commands_len && 0 == ret; ++i)
        ret = setpgid(dipsh_command_get_pid(pipeline->commands[i]), new_pgid);
    return ret;
}

static int
dipshp_pipeline_run_all_commands(
    dipsh_pipeline *pipeline
)
{
    int ret = 0;
    for (int i = 0; i < pipeline->commands_len && 0 == ret; ++i)
        ret = dipsh_command_start_suspended(pipeline->commands[i]);
    return ret;
}

int
dipsh_pipeline_wait(
    dipsh_pipeline *pipeline
)
{
    for (int i = 0; i < pipeline->commands_len; ++i) {
        const dipsh_command_status *status = 
            dipsh_wait_for_command(pipeline->commands[i]);
        if (!status)
            return 1;
        if (pipeline->commands_len - 1 == i) {
            memcpy(
                &pipeline->last_command_status, status, 
                sizeof(dipsh_command_status)
            );
        }
    }
    pipeline->executed = 1;
    return 0;
}

const dipsh_command_status *
dipsh_pipeline_get_last_command_status(
    dipsh_pipeline *pipeline
)
{
    return pipeline->executed ? &pipeline->last_command_status : NULL;
}

int
dipsh_pipeline_execute(
    dipsh_pipeline *pipeline,
    int is_interactive_shell
)
{
    int *pipes_fds;
    int old_group;
    int ret = dipshp_pipeline_get_pipes(pipeline, &pipes_fds);
    if (0 != ret) {
        warn("pipeline failure: can't create enough pipes");
        return dipsh_handler_system_error;
    }
    ret = dipshp_pipeline_start_all_commands(
        pipeline, pipes_fds
    );
    if (dipsh_handler_ok != ret) {
        warn("pipeline failure: can't start all commands");
        goto cleanup;
    }
    dipshp_pipeline_close_pipes(pipeline, &pipes_fds);
    ret = dipshp_pipeline_change_group(pipeline);
    if (0 != ret) {
        warn("pipeline failure: can't change the group for all commands");
        goto cleanup;
    }
    if (is_interactive_shell) {
        ret = dipsh_change_current_group(
            dipsh_command_get_pid(pipeline->commands[0]), &old_group
        );
        if (0 != ret) {
            warn("pipeline failure: can't change current group");
            goto cleanup;
        }
    }
    ret = dipshp_pipeline_run_all_commands(pipeline);
    if (0 != ret) {
        warn("pipeline failure: can't run all commands");
        goto cleanup;
    }
    if (pipeline->execute_blocks) {
        ret = dipsh_pipeline_wait(pipeline);
        if (0 != ret) {
            warn("pipeline failure: can't wait for the pipeline to stop");
            goto cleanup;
        }
    }
    if (is_interactive_shell) {
        ret = dipsh_change_current_group(old_group, NULL);
        if (0 != ret) {
            warn("pipeline failure: can't restore current group");
            goto cleanup;
        }
    }

cleanup:
    dipshp_pipeline_close_pipes(pipeline, &pipes_fds);
    return ret;
}
