#include "token.h"
#include <string.h>
#include <stdlib.h>

typedef struct dipshp_token_traits_tag
{
    dipsh_token_type type;
    const char *name;
    const char *value;
}
dipshp_token_traits;

const dipshp_token_traits token_traits[] = {
    { dipsh_token_word, "word", "" },
    { dipsh_token_amp, "amp", "&" },
    { dipsh_token_dbl_amp, "dbl_amp", "&&" },
    { dipsh_token_bar, "bar", "|" },
    { dipsh_token_dbl_bar, "dbl_bar", "||" },
    { dipsh_token_semicolon, "semicolon", ";" },
    { dipsh_token_lt, "lt", "<" },
    { dipsh_token_gt, "gt", ">" },
    { dipsh_token_dbl_gt, "dbl_gt", ">>" },
    { dipsh_token_digits_lt, "digits_lt", "" },
    { dipsh_token_digits_gt, "digits_gt", "" },
    { dipsh_token_digits_dbl_gt, "digits_dbl_gt", "" },
    { dipsh_token_open_paren, "open_paren", "(" },
    { dipsh_token_close_paren, "close_paren", ")" },
    { dipsh_token_open_brace, "open_brace", "{" },
    { dipsh_token_close_brace, "close_brace", "}" },
    { dipsh_token_newline, "newline", "\n" },
    { dipsh_token_error, "error", NULL },
};

const char *
dipsh_type_to_str(
    dipsh_token_type type
)
{
    return token_traits[type].name;
}

dipsh_token_type
dipsh_delim_to_type(
    char delim
)
{
    for (const dipshp_token_traits *pos = token_traits; pos->value; ++pos) {
        if (1 == strlen(pos->value) && *pos->value == delim)
            return pos->type;
    }
    return dipsh_token_error;
}

dipsh_token_type
dipsh_dbl_delim_to_type(
    const char *delim
)
{
    if (0 == strcmp(token_traits[dipsh_token_dbl_amp].value, delim))
        return dipsh_token_dbl_amp;
    else if (0 == strcmp(token_traits[dipsh_token_dbl_bar].value, delim))
        return dipsh_token_dbl_bar;
    else if (0 == strcmp(token_traits[dipsh_token_dbl_gt].value, delim))
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
