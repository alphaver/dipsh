#ifndef _DIPSH_CL_PARAMS_H_
#define _DIPSH_CL_PARAMS_H_

typedef struct dipsh_cl_params_tag 
{
    int show_parsing_info;
    char *script_file;
}
dipsh_cl_params;

dipsh_cl_params *
dipsh_read_cl_params(
    int argc, 
    char **argv
);

#endif /* _DIPSH_CL_PARAMS_H_ */
