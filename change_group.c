#include "change_group.h"
#include <unistd.h>

int
dipsh_change_current_group(
    int new_group_id,
    int *old_group_id
)
{
    if (old_group_id) {
        *old_group_id = tcgetpgrp(0);
        if (-1 == *old_group_id) 
            return 1;
    } 
    int ret = tcsetpgrp(0, new_group_id);
    return -1 == ret; 
}
