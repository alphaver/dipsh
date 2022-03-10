#include "lexer.h"
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

typedef enum dipshp_parse_state_tag
{
    dipshp_waiting_token,
    dipshp_reading_word,
    dipshp_read_one_amp_bar_gt,
    dipshp_reading_escape_char,
    dipshp_end_of_stream
}
dipshp_parse_state;

struct dipsh_lexer_state_tag
{
    char *word;
    int word_capacity;
    int line;
    char *error;
    dipshp_parse_state parse_state;
};

static const char dipshp_ws[] = " \t\v\n";
static const char dipshp_non_ws_delims[] = "&|;<>(){}\n";

static int
dipshp_is_ws(
    char c
)
{
    return strchr(dipshp_ws, c);
}

static int
dipshp_is_delim(
    char c
)
{
    return strchr(dipshp_ws, c) || strchr(dipshp_non_ws_delims, c);
}

dipsh_lexer_state *
dipsh_lexer_state_init()
{
    dipsh_lexer_state *st = calloc(sizeof(dipsh_lexer_state), 1);
    st->line = 1;
    st->word = calloc(1, 1);
    st->word_capacity = 1;
    st->parse_state = dipshp_waiting_token;
    st->error = NULL;
    return st;
}

void
dipsh_lexer_state_destroy(
    dipsh_lexer_state *state
)
{
    free(state->word);
    free(state->error);
    free(state);
}

int
dipsh_lexer_state_get_line(
    const dipsh_lexer_state *state
)
{
    return state->line;
}

const char *
dipsh_lexer_state_get_error(
    const dipsh_lexer_state *state
)
{
    return state->error;
}

static void
dipshp_append_character(
    dipsh_lexer_state *state,
    int c
)
{

}

static int
dipshp_handle_waiting_token(
    dipsh_lexer_state *state,
    int c,
    dipsh_token *token
)
{
    if (EOF == c || dipshp_is_ws(c)) {
        return dipsh_lexer_no_token;
    } else if ('&' == c || '|' == c || '>' == c) {
        dipshp_append_character(state, c);
        state = dipshp_read_one_amp_bar_gt;
        return dipsh_lexer_no_token;
    } else if (isprint(c)) {
        dipshp_append_character(state, c);
    }
}

static int
dipshp_handle_reading_word(
    dipsh_lexer_state *state,
    int c,
    dipsh_token *token
)
{
}

static int
dipshp_handle_read_one_amp_bar_gt(
    dipsh_lexer_state *state,
    int c,
    dipsh_token *token
)
{
}

static int
dipshp_handle_reading_escape_char(
    dipsh_lexer_state *state,
    int c,
    dipsh_token *token
)
{
}


int
dipsh_lexer_next_token(
    dipsh_lexer_state *state,
    int c,
    dipsh_token *token
)
{
    if ('\n' == c)
        ++state->line;
    switch (state->parse_state) {
    case dipshp_waiting_token:
        return dipshp_handle_waiting_token(state, c, token);
    case dipshp_reading_word:
        return dipshp_handle_reading_word(state, c, token);
    case dipshp_read_one_amp_bar_gt:
        return dipshp_handle_read_one_amp_bar_gt(state, c, token);
    case dipshp_reading_escape_char:
        return dipshp_handle_reading_escape_char(state, c, token);
    default:

    }
}

int
dipsh_tokenize_error_set(
    dipsh_tokenize_error *err,
    int line,
    const char *format, ...
)
{
    if (!err)
        return 1;
    err->line = line;
    
    va_list ap;
    va_start(ap, format);
    ret = vasprintf(&err->message, format, ap);
    va_end(ap);
    if (-1 == ret) { 
        err->message = NULL;
        return 1;
    }
    return 0;
}

void
dipsh_tokenize_error_clean(
    dipsh_tokenize_error *err
)
{
    free(err->message);
}

static void
dipshp_insert_new_token(
    dipsh_token_list **first, 
    dipsh_token_list **last,
    dipsh_token_list *new_token
)
{
    if (!*last) {
        *first = new_token;
        *last = new_token;
    } else {
        *last->next = new_token;
        new_token->next = NULL;
    }
}

static int
dipshp_add_char(
    dipsh_lexer_state *state,
    int c,
    dipsh_token_list **first, 
    dipsh_token_list **last,
    dipsh_tokenize_error *err
)
{
    dipsh_token_list *new_token = malloc(sizeof(dipsh_token_list));
    int next_token_ret = dipsh_lexer_new_token(state, c, &new_token->token);
    switch (next_token_ret) {

    case dipsh_lexer_no_token: {
        free(new_token);
        break;
    }

    case dipsh_lexer_new_token: {
        dipshp_insert_new_token(first, last, new_token);
        break;
    }

    case dipsh_lexer_error: {
        dipsh_tokenize_error_set(
            err, dipsh_lexer_state_get_line(state),
            dipsh_lexer_state_get_error(state)
        );
        return 1;
    }

    default: {  /* shouldn't happen */
        dipsh_tokenize_error_set(
            err, dipsh_lexer_state_get_line(state),
            "unknown lexer error"
        );
    }

    }

    return 0;
}

dipsh_token_list *
dipsh_tokenize_string(
    const char *str,
    dipsh_tokenize_error *err
)
{
    FILE *str_stream = fmemopen((void*)str, strlen(str), "r");
    if (!str_stream) {
        dipsh_tokenize_error_set(
            err, 0, "fmemopen failed: %s", strerrordesc_np(errno)
        );
        return NULL;
    }
    dipsh_token_list *list = dipsh_tokenize_stream(str_stream, err);
    fclose(str_stream);
    return list;
}

dipsh_token_list *
dipsh_tokenize_stream(
    FILE *stream,
    dipsh_tokenize_error *err
)
{
    dipsh_token_list *first = NULL, *last = NULL;
    dipsh_lexer_state *state = dipsh_lexer_state_init();
    if (!state) {
        dipsh_tokenize_error_set(err, 0, "can't initialize the tokenizer");
        goto fail;
    }

    int c;
    while ((c = fgetc(stream)) != EOF) {
        int not_ok = dipshp_add_char(state, c, &first, &last, err);
        if (not_ok)
            goto fail;
    }
    int not_ok = dipshp_add_char(state, EOF, &first, &last, err);
    if (not_ok)
        goto fail;

    return first;

fail:
    dipsh_lexer_state_destroy(state);
    dipsh_clean_list(first);
    return NULL;
}

void
dipsh_clean_list(
    dipsh_token_list *list
)
{
    dipsh_token_list *pos = list;
    while (pos) {
        dipsh_token_list *temp = pos;
        pos = pos->next;
        dipsh_token_clean(&temp->token);
        free(temp);
    }
}
