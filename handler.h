#ifndef _DIPSH_HANDLER_H_
#define _DIPSH_HANDLER_H_

#include "command.h"

typedef int (*dipsh_command_handler)(dipsh_command *, dipsh_command_status *);

enum
{
    dipsh_handler_ok,
    dipsh_handler_no_such_command,
    dipsh_handler_system_error
};

dipsh_command_handler
dipsh_get_handler_by_name(
    const char *command_name
);

#endif /* _DIPSH_HANDLER_H_ */
