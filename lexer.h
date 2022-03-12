#ifndef _DIPSH_LEXER_H_
#define _DIPSH_LEXER_H_

#include "token.h"
#include <stdio.h>

/* lexer state and low-level lexical analysis functions */

typedef struct dipsh_lexer_state_tag dipsh_lexer_state;

dipsh_lexer_state *
dipsh_lexer_state_init();

void
dipsh_lexer_state_destroy(
    dipsh_lexer_state *state
);

int
dipsh_lexer_state_is_idle(
    const dipsh_lexer_state *state
);

int
dipsh_lexer_state_get_line(
    const dipsh_lexer_state *state
);

const char *
dipsh_lexer_state_get_error(
    const dipsh_lexer_state *state
);

enum
{
    dipsh_lexer_no_token,
    dipsh_lexer_new_token,
    dipsh_lexer_error
};

/* accepts a character and returns a token (if avaliable), or reports an error
 * parameters:
 *     state - lexer's state
 *     c     - character to be accepted by the lexer (EOF if the previous 
 *     character was the last one
 *     token - where to store the returned token
 * return values:
 *     dipsh_lexer_no_token    - the character was accepted, but it didn't 
 *         produce a token
 *     dipsh_lexer_new_token   - the character was accepted, and a new token is
 *         produced (returned via token parameter) 
 *     dipsh_lexer_error - the character wasn't accepted for some reason 
 *         (can be found out via dipsh_lexer_state_get_error) */

int
dipsh_lexer_next_token(
    dipsh_lexer_state *state,
    int c,
    dipsh_token *token
);

/* string and stream tokenizing functions */

typedef struct dipsh_token_list_tag
{
    dipsh_token token;
    struct dipsh_token_list_tag *next;
}
dipsh_token_list;

typedef struct dipsh_tokenize_error_tag
{
    int line;
    char *message;
}
dipsh_tokenize_error;

int
dipsh_tokenize_error_set(
    dipsh_tokenize_error *err,
    int line,
    const char *format, ...
);

void
dipsh_tokenize_error_clean(
    dipsh_tokenize_error *err
);

dipsh_token_list *
dipsh_tokenize_string(
    const char *str,
    dipsh_tokenize_error *err
);

dipsh_token_list *
dipsh_tokenize_stream(
    FILE *stream,
    dipsh_tokenize_error *err
);

void
dipsh_clean_token_list(
    dipsh_token_list *list
);

#endif /* _DIPSH_LEXER_H_ */
