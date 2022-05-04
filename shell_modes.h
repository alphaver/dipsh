#ifndef _DIPSH_SHELL_MODES_H_
#define _DIPSH_SHELL_MODES_H_

int
dipsh_execute_script(
    const char *script_name,
    int show_parsing_info
);

int
dipsh_interactive_shell(
    int show_parsing_info
);

#endif /* _DIPSH_SHELL_MODES_H_ */
