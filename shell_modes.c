#include "shell_modes.h"
#include "shell_state.h"
#include "lexer.h"
#include "parser.h"
#include "execute.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <err.h>
#include <signal.h>

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

static int
dipshp_handle_parsed_list(
    dipsh_token_list *list,
    dipsh_tokenize_error *err,
    dipsh_shell_state *state
)
{
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
        char *esc_msg = dipshp_escape_non_printables(err->message);
        warnx("line %d: %s", err->line, esc_msg);
        free(esc_msg);
        dipsh_tokenize_error_clean(err);
        return 1;
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
        return 1;
    }
    if (root) {
        int exec_ret = dipsh_execute_ast(root, state);
        if (0 != exec_ret)
            warnx("you used an unsupported feature of dipsh (&&, ||, &, |, ;)");
        dipsh_symbol_clear(root); 
    }
    dipsh_clean_token_list(list);
    return 0;
}

int
dipsh_interactive_shell()
{
    dipsh_shell_state state = {
        .is_interactive = 1
    };
    signal(SIGTTOU, SIG_IGN);
    for (;;) {
        printf("> ");
        char *read_str = dipshp_read_next_line_from_stdin();
        if (!read_str) {
            putchar('\n');
            goto out;
        }
        dipsh_tokenize_error err;
        dipsh_token_list *list = dipsh_tokenize_string(read_str, &err);
        dipshp_handle_parsed_list(list, &err, &state);
        free(read_str);
        if (feof(stdin))
            break;
    }
out:
    return 0;
}

int
dipsh_execute_script(
    const char *script_name
)
{
    dipsh_shell_state state = {
        .is_interactive = 0
    };
    FILE *script = fopen(script_name, "r");
    if (!script)
        err(1, "can't open file '%s'", script_name);
    dipsh_tokenize_error err;
    dipsh_token_list *list = dipsh_tokenize_stream(script, &err);
    int ret = dipshp_handle_parsed_list(list, &err, &state);
    fclose(script);
    return ret;
}

