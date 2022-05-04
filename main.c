#include "cl_params.h"
#include "shell_modes.h"

int
main(
    int argc, char **argv
)
{
    dipsh_cl_params *params = dipsh_read_cl_params(argc, argv);
    if (!params)
        return 1;
    if (params->script_file) {
        return dipsh_execute_script(
            params->script_file, params->show_parsing_info
        );
    } else {
        return dipsh_interactive_shell(params->show_parsing_info);
    }
}
