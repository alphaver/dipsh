#include "token.h"
#include <string.h>

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
    token->value = (const char *)strdup(token_value);
    return 0;
}

void
dipsh_token_clean(
    dipsh_token *token
)
{
    free(token->value);
}
