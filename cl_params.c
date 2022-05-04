#include "cl_params.h"
#include <string.h>
#include <err.h>

void
dipshp_print_usage(
    const char *command_name
)
{
    warnx("usage: %s [--parse-info] [SCRIPT]", command_name);
}

dipsh_cl_params *
dipsh_read_cl_params(
    int argc, 
    char **argv
)
{
    static dipsh_cl_params params;

    int should_show_parsing_info = 
        argc >= 2 && 0 == strcmp("--parse-info", argv[1]);

    switch (argc) {
    case 1: 
        params.show_parsing_info = 0;
        params.script_file = NULL;
        break;

    case 2:
        params.show_parsing_info = should_show_parsing_info;
        params.script_file = should_show_parsing_info ? NULL : argv[1];
        break;

    case 3:
        if (!should_show_parsing_info) {
            dipshp_print_usage(argv[0]);
            return NULL;
        }
        params.show_parsing_info = 1;
        params.script_file = argv[2];
        break;

    default:
        dipshp_print_usage(argv[0]);
        return NULL;    
    }
    return &params;
}
