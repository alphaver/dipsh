#include "cl_params.h"
    
int
main(int argc, char **argv)
{
    dipsh_cl_params *params = dipsh_read_cl_params(argc, argv);
    if (!params)
        return 1;
    if (NULL != params->script_file)
        errx(1, "can't work with script files yet");

}
