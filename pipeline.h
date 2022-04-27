#ifndef _DIPSH_PIPELINE_H_
#define _DIPSH_PIPELINE_H_

#include "parser.h"

typedef struct dipsh_pipeline_tag dipsh_pipeline;

dipsh_pipeline *
dipsh_pipeline_init(
    const dipsh_symbol *pipeline_tree
);

void
dipsh_pipeline_destroy(
    dipsh_pipeline *pipeline
);

int
dipsh_pipeline_execute(
    dipsh_pipeline *pipeline,
    dipsh_command_status *last_command_status
);

#endif /* _DIPSH_PIPELINE_H_ */
