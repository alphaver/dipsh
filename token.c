#include "token.h"
#include <string.h>
#include <stdlib.h>

const char *
dipsh_type_to_str(
    dipsh_token_type type
)
{
    switch (type) {
    case dipsh_token_word:
        return "word";
    case dipsh_token_amp:
        return "amp";
    case dipsh_token_dbl_amp:
        return "dbl_amp";
    case dipsh_token_bar:
        return "bar";
    case dipsh_token_dbl_bar:
        return "dbl_bar";
    case dipsh_token_semicolon:
        return "semicolon";
    case dipsh_token_lt:
        return "lt";
    case dipsh_token_gt:
        return "gt";
    case dipsh_token_dbl_gt:
        return "dbl_gt";
    case dipsh_token_open_paren:
        return "open_paren";
    case dipsh_token_close_paren:
        return "close_paren";
    case dipsh_token_open_brace:
        return "open_brace";
    case dipsh_token_close_brace:
        return "close_brace";
    default:
        return "error";
    }
}

dipsh_token_type
dipsh_delim_to_type(
    char delim
)
{
    switch (delim) {
    case '&':
        return dipsh_token_amp;
    case '|':
        return dipsh_token_bar;
    case ';':
        return dipsh_token_semicolon;
    case '<':
        return dipsh_token_lt;
    case '>':
        return dipsh_token_gt;
    case '(':    
        return dipsh_token_open_paren;
    case ')':
        return dipsh_token_close_paren;
    case '{':
        return dipsh_token_open_brace;
    case '}':
        return dipsh_token_close_brace;
    default:
        return dipsh_token_error;
    }
}

dipsh_token_type
dipsh_dbl_delim_to_type(
    const char *delim
)
{
    if (0 == strcmp("&&", delim))
        return dipsh_token_dbl_amp;
    else if (0 == strcmp("||", delim))
        return dipsh_token_dbl_bar;
    else if (0 == strcmp(">>", delim))
        return dipsh_token_dbl_gt;
    else
        return dipsh_token_error;
}

int
dipsh_token_init(
    dipsh_token *token,
    dipsh_token_type type,
    int token_line,
    const char *token_value
)
{
    if (!token)
        return 1;
    token->type = type;
    token->line = token_line;
    token->value = strdup(token_value);
    return 0;
}

void
dipsh_token_clean(
    dipsh_token *token
)
{
    free(token->value);
}
