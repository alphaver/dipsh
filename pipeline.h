#ifndef _DIPSH_PIPELINE_H_
#define _DIPSH_PIPELINE_H_

#include "parser.h"
#include "command.h"

typedef struct dipsh_pipeline_tag dipsh_pipeline;

dipsh_pipeline *
dipsh_pipeline_init(
    const dipsh_symbol *pipeline_tree,
    int execute_blocks
);

void
dipsh_pipeline_destroy(
    dipsh_pipeline *pipeline
);

int
dipsh_pipeline_get_pgid(
    const dipsh_pipeline *pipeline
);

int
dipsh_pipeline_wait(
    dipsh_pipeline *pipeline
);

const dipsh_command_status *
dipsh_pipeline_get_last_command_status(
    dipsh_pipeline *pipeline
);

int
dipsh_pipeline_execute(
    dipsh_pipeline *pipeline,
    int is_interactive_shell
);

#endif /* _DIPSH_PIPELINE_H_ */
