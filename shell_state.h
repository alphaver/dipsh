#ifndef _DIPSH_SHELL_STATE_H_
#define _DIPSH_SHELL_STATE_H_

#include "command.h"

typedef struct dipsh_shell_state_tag
{
    int is_interactive;
    dipsh_command_status last_status;
}
dipsh_shell_state;

#endif /* _DIPSH_SHELL_STATE_H_ */
