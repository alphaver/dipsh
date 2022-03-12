#include "cl_params.h"
#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <err.h>

#define DIPSHP_BUF_SIZE 128

static void
dipshp_append_buffer(
    char **read_str, 
    const char *buffer
)
{
    if (!*buffer)
        return;
    *read_str = realloc(*read_str, strlen(*read_str) + strlen(buffer) + 1);
    strcat(*read_str, buffer);
}

static int
dipshp_interactive_shell()
{
    for (;;) {
        printf("> ");
        int non_escaped_nl_found = 0;
        char *read_str = calloc(1, 1), read_buf[DIPSHP_BUF_SIZE] = {0};
        while (!non_escaped_nl_found) {
            char *fgets_ret = fgets(read_buf, DIPSHP_BUF_SIZE, stdin);
            if (!fgets_ret) { /* normally meaning EOF */
                putchar('\n');
                goto out;
            }
            dipshp_append_buffer(&read_str, read_buf);
            char *nl_pos = strrchr(read_buf, '\n');
            if (nl_pos) {
                int backslashes = 0;
                for (char *pos = nl_pos - 1; *pos == '\\' && pos >= read_buf; --pos)
                    ++backslashes;
                non_escaped_nl_found = 0 == backslashes % 2;
            }
        }
        dipsh_tokenize_error err;
        dipsh_token_list *list = dipsh_tokenize_string(read_str, &err);
        if (list) {
            int idx = 0;
            for (dipsh_token_list *curr = list; curr; curr = curr->next, ++idx) {
                printf(
                    "token %d: type %s, value [%s]\n", 
                    idx, dipsh_type_to_str(curr->token.type), curr->token.value 
                );
            }
        } else {
            warnx("line %d: %s", err.line, err.message);
            dipsh_tokenize_error_clean(&err);
        }
        dipsh_clean_token_list(list);
        free(read_str);
        if (feof(stdin))
            break;
    }
out:
    return 0;
}

int
main(int argc, char **argv)
{
    dipsh_cl_params *params = dipsh_read_cl_params(argc, argv);
    if (!params)
        return 1;
    if (NULL != params->script_file)
        errx(1, "can't work with script files yet");
    return dipshp_interactive_shell();
}
