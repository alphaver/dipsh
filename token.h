#ifndef _DIPSH_TOKEN_H_
#define _DIPSH_TOKEN_H_

typedef enum dipsh_token_type_tag
{
    dipsh_token_word,           /* anything not from the list below  */
    dipsh_token_amp,            /* & */
    dipsh_token_dbl_amp,        /* && */
    dipsh_token_bar,            /* | */
    dipsh_token_dbl_bar,        /* || */
    dipsh_token_semicolon,      /* ; */
    dipsh_token_lt,             /* < */
    dipsh_token_gt,             /* > */
    dipsh_token_dbl_gt,         /* >> */
    dipsh_token_open_paren,     /* ( */
    dipsh_token_close_paren,    /* ) */
    dipsh_token_open_brace,     /* { */
    dipsh_token_close_brace,    /* } */
    dipsh_token_newline,        /* \n */
}
dipsh_token_type;

typedef struct dipsh_token_tag
{
    dipsh_token_type type;
    int line;
    const char *value;
}
dipsh_token;

int
dipsh_token_init(
    dipsh_token *token,
    dipsh_token_type type,
    int token_line,
    const char *token_value
);

void
dipsh_token_clean(
    dipsh_token *token
);

#endif /* _DIPSH_TOKEN_H_ */
