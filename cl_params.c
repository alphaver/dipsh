#include "cl_params.h"
#include <stddef.h>

dipsh_cl_params *
dipsh_read_cl_params(int argc, char **argv)
{
    static dipsh_cl_params params;

    if (argc > 2)
        return NULL;
    params.script_file = argc == 2 ? argv[1] : NULL;
    return &params;
}
