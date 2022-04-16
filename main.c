#include "cl_params.h"
#include "lexer.h"
#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <err.h>
#include <unistd.h>
#include <sys/wait.h>

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
dipshp_is_escaped_by_bs(
    const char *str,
    const char *char_pos
)
{
    --char_pos;
    int backslashes = 0;
    for (; *char_pos == '\\' && char_pos >= str; --char_pos)
        ++backslashes;
    return 1 == backslashes % 2;
}

static int
dipshp_num_non_escaped_quotes(
    const char *str
)
{
    int result = 0;
    const char *pos = str;
    while (NULL != (pos = strchr(pos, '"'))) {
        if (!dipshp_is_escaped_by_bs(str, pos))
            ++result;
        ++pos;
    }
    return result;
}

static char *
dipshp_read_next_line_from_stdin()
{
    int non_escaped_nl_found = 0, non_escaped_quotes = 0;
    char *read_str = calloc(1, 1), read_buf[DIPSHP_BUF_SIZE] = {0};
    while (!non_escaped_nl_found) {
        char *fgets_ret = fgets(read_buf, DIPSHP_BUF_SIZE, stdin);
        if (!fgets_ret) /* normally meaning EOF */
            return NULL;
        dipshp_append_buffer(&read_str, read_buf);
        non_escaped_quotes += dipshp_num_non_escaped_quotes(read_buf);
        char *nl_pos = strrchr(read_buf, '\n');
        if (nl_pos) {
            non_escaped_nl_found = 0 == non_escaped_quotes % 2 &&
                !dipshp_is_escaped_by_bs(read_buf, nl_pos);
        }
    }
    return read_str;
}

static char *
dipshp_escape_non_printables(
    const char *str
)
{
    char *result = malloc(4 * strlen(str) + 1), *res_pos = result;
    for (; *str; ++str) {
        if (isprint(*str)) {
            *res_pos = *str;
            ++res_pos;
        } else {
            res_pos[0] = '\\';
            switch (*str) {
            case '\a': res_pos[1] = 'a';  res_pos += 2; break;
            case '\b': res_pos[1] = 'b';  res_pos += 2; break;
            case '\f': res_pos[1] = 'f';  res_pos += 2; break;
            case '\n': res_pos[1] = 'n';  res_pos += 2; break;
            case '\r': res_pos[1] = 'r';  res_pos += 2; break;
            case '\t': res_pos[1] = 't';  res_pos += 2; break;
            case '\v': res_pos[1] = 'v';  res_pos += 2; break;
            case '\'': res_pos[1] = '\''; res_pos += 2; break;
            case '\"': res_pos[1] = '"';  res_pos += 2; break;
            case '\\': res_pos[1] = '\\'; res_pos += 2; break;
            case '\?': res_pos[1] = '?';  res_pos += 2; break;
            default:   
                sprintf(res_pos + 1, "%03o", *str); 
                res_pos += 4;
            }
        }
    }
    *res_pos = '\0';
    return result;
}

static void
dipshp_print_parse_subtree(
    const dipsh_symbol *root,
    int tabs
)
{
    for (int i = 0; i < tabs; ++i)
        printf("  ");
    printf("%s", dipsh_symbol_type_to_string(root->type));
    if (root->type & dipsh_symbol_nonterminal) {
        putchar('\n');
        dipsh_nonterminal_child *child = 
            ((const dipsh_nonterminal *)root)->children_list;
        while (child) {
            dipshp_print_parse_subtree(child->child, tabs + 1);
            child = child->next;
        }
    } else if (root->type & dipsh_symbol_terminal) {
        const dipsh_terminal *term_root = (const dipsh_terminal *)root;
        char *esc_val = dipshp_escape_non_printables(term_root->token.value);
        printf(": %s\n", esc_val);
        free(esc_val);
    } else {
        putchar('\n');
    }
}

static void
dipshp_print_parse_tree(
    const dipsh_symbol *root
)
{
    dipshp_print_parse_subtree(root, 0);
}

/* TODO: throw away the following fuckery (up to FUCKERY_END) */

static const dipsh_symbol *
dipshp_get_to_command(
    const dipsh_symbol *root
)
{
    if (dipsh_symbol_script != root->type)
        return NULL;
    const dipsh_nonterminal *nonterm_symb = (const dipsh_nonterminal *)root;
    const dipsh_nonterminal_child *children = nonterm_symb->children_list;
    if (children->next && children->next->next) {
        warnx("we don't support multiple strings as for now");
        return NULL;
    }
    nonterm_symb = (const dipsh_nonterminal *)children->child;
    children = nonterm_symb->children_list;
    while (dipsh_symbol_command != nonterm_symb->symb.type) {
        nonterm_symb = (const dipsh_nonterminal *)children->child;
        if (dipsh_symbol_command == nonterm_symb->symb.type)
            break;
        children = nonterm_symb->children_list;
        if (children->next) {
            warnx("you use an unsupported feature of dipsh (&&, ||, |, &, ;)");
            return NULL;
        }
    }
    return (dipsh_symbol *)nonterm_symb;
}

static void
dipshp_append_string_to_argv(
    const char *str,
    char ***argv,
    int *argv_len,
    int *argv_cap
)
{
    if (!*argv_cap) {
        *argv = calloc(sizeof(char *) * 2, 1);
        *argv_cap = 2;
    }
    if (*argv_len + 2 > *argv_cap) {
        *argv_cap *= 2;
        char **new_argv = calloc(sizeof(char *) * (*argv_cap), 1);
        memcpy(new_argv, *argv, sizeof(char *) * (*argv_len));
        free(*argv);
        *argv = new_argv;
    }
    ++(*argv_len);
    (*argv)[(*argv_len) - 1] = strdup(str);
}

static void
dipshp_clean_argv(
    char **argv
)
{
    for (char **pos = argv; *pos; ++pos)
        free(*pos);
    free(argv);
}

static void
dipshp_build_argv_from_command_subtree(
    const dipsh_symbol *cmd_root,
    char ***argv,
    int *argv_len,
    int *argv_cap
)
{
    const dipsh_nonterminal_child *cmd_children = 
        ((const dipsh_nonterminal *)cmd_root)->children_list;
    const dipsh_nonterminal_child *next_child = cmd_children->next;
    if (!next_child) {
        dipshp_append_string_to_argv(
            ((const dipsh_terminal *)cmd_children->child)->token.value, 
            argv, argv_len, argv_cap
        );
    } else if (dipsh_symbol_word == next_child->child->type) {
        dipshp_build_argv_from_command_subtree(
            cmd_children->child, argv, argv_len, argv_cap
        );
        if (*argv) {
            dipshp_append_string_to_argv(
                ((const dipsh_terminal *)next_child->child)->token.value, 
                argv, argv_len, argv_cap
            );
        }
    } else {
        warnx("we don't support redirections yet");
        if (*argv) {
            dipshp_clean_argv(*argv);
            *argv = NULL;
        }
    }
}

static char **
dipshp_create_argv(
    const dipsh_symbol *cmd_root
)
{
    char **argv = NULL;
    int argv_len = 0, argv_cap = 0;
    dipshp_build_argv_from_command_subtree(
        cmd_root, &argv, &argv_len, &argv_cap
    );
    return argv;
}

static void
dipshp_execute_tree(
    const dipsh_symbol *root
)
{
    const dipsh_symbol *command_root = dipshp_get_to_command(root);
    if (!command_root)
        return;
    char **argv = dipshp_create_argv(command_root);
    if (!argv)
        return;
    int proc_pid = fork();
    if (0 < proc_pid) {
        int wst;
        waitpid(proc_pid, &wst, 0);
    } else if (0 == proc_pid) {
        execvp(argv[0], argv);
        err(1, argv[0]);
    } else {
        perror("fork");
    }

    dipshp_clean_argv(argv);
}

/* FUCKERY_END */

static int
dipshp_interactive_shell()
{
    for (;;) {
        printf("> ");
        char *read_str = dipshp_read_next_line_from_stdin();
        if (!read_str) {
            putchar('\n');
            goto out;
        }
        dipsh_tokenize_error err;
        dipsh_token_list *list = dipsh_tokenize_string(read_str, &err);
        puts("lexical analysis results:");
        if (list) {
            int idx = 0;
            for (const dipsh_token_list *curr = list; curr; curr = curr->next, ++idx) {
                char *esc_val = dipshp_escape_non_printables(curr->token.value);
                printf(
                    "token %d: type %s, value [%s]\n", 
                    idx, dipsh_type_to_str(curr->token.type), esc_val 
                );
                free(esc_val);
            }
        } else {
            char *esc_msg = dipshp_escape_non_printables(err.message);
            warnx("line %d: %s", err.line, esc_msg);
            free(esc_msg);
            dipsh_tokenize_error_clean(&err);
            continue;
        }

        dipsh_symbol *root = NULL;
        char *parser_err = NULL;
        int parser_ret = dipsh_parse_token_list(list, &root, &parser_err);
        puts("parsing results:");
        if (dipsh_parser_accepted == parser_ret) {
            dipsh_make_ast(&root);
            dipshp_print_parse_tree(root);
        } else {
            char *esc_msg = dipshp_escape_non_printables(parser_err);
            warnx(esc_msg);
            free(esc_msg);
            free(parser_err);
        }
        if (root) {
            /* dipshp_execute_tree(root); */
            dipsh_symbol_clear(root); 
        }
        dipsh_clean_token_list(list);
        free(read_str);
        if (feof(stdin))
            break;
    }
out:
    return 0;
}

static int
dipshp_execute_script(
    const char *script_name
)
{
    FILE *script = fopen(script_name, "r");
    if (!script)
        err(1, "can't open file '%s'", script_name);
    int ret = 1;
    dipsh_tokenize_error err;
    dipsh_token_list *list = dipsh_tokenize_stream(script, &err);
    puts("lexical analysis results:");
    if (list) {
        int idx = 0;
        for (const dipsh_token_list *curr = list; curr; curr = curr->next, ++idx) {
            char *esc_val = dipshp_escape_non_printables(curr->token.value);
            printf(
                "token %d: type %s, value [%s]\n", 
                idx, dipsh_type_to_str(curr->token.type), esc_val 
            );
            free(esc_val);
        }
    } else {
        char *esc_msg = dipshp_escape_non_printables(err.message);
        warnx("line %d: %s", err.line, esc_msg);
        free(esc_msg);
        dipsh_tokenize_error_clean(&err);
        goto exit;
    }

    dipsh_symbol *root = NULL;
    char *parser_err = NULL;
    int parser_ret = dipsh_parse_token_list(list, &root, &parser_err);
    puts("parsing results:");
    if (dipsh_parser_accepted == parser_ret) {
        dipsh_make_ast(&root);
        dipshp_print_parse_tree(root);
    } else {
        char *esc_msg = dipshp_escape_non_printables(parser_err);
        warnx(esc_msg);
        free(esc_msg);
        free(parser_err);
        goto exit;
    }
    /* dipshp_execute_tree(root); */
    ret = 0;
exit:
    dipsh_symbol_clear(root); 
    dipsh_clean_token_list(list);
    fclose(script);
    return ret;
}

int
main(int argc, char **argv)
{
    dipsh_cl_params *params = dipsh_read_cl_params(argc, argv);
    if (!params)
        return 1;
    if (NULL != params->script_file)
        return dipshp_execute_script(params->script_file);
    else
        return dipshp_interactive_shell();
}
