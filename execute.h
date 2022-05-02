#ifndef _DIPSH_EXECUTE_H_
#define _DIPSH_EXECUTE_H_

#include "parser.h"
#include "shell_state.h"

int
dipsh_execute_ast(
    const dipsh_symbol *ast,
    dipsh_shell_state *state
);

#endif /* _DIPSH_EXECUTE_H_ */
