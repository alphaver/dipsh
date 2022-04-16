#ifndef _DIPSH_PARSER_H_
#define _DIPSH_PARSER_H_

#include "token.h"
#include "lexer.h"

typedef enum dipsh_symbol_type_tag
{
    /* nonterminals */
    dipsh_symbol_nonterminal   = 0x4000,
    dipsh_symbol_script        = dipsh_symbol_nonterminal + 1,
    dipsh_symbol_strings       = dipsh_symbol_nonterminal + 2,
    dipsh_symbol_seq_bg_start  = dipsh_symbol_nonterminal + 3,
    dipsh_symbol_seq_bg        = dipsh_symbol_nonterminal + 4,
    dipsh_symbol_and_or        = dipsh_symbol_nonterminal + 5,
    dipsh_symbol_pipe          = dipsh_symbol_nonterminal + 6,
    dipsh_symbol_command       = dipsh_symbol_nonterminal + 7,
    dipsh_symbol_redir         = dipsh_symbol_nonterminal + 8,
    /* terminals */
    dipsh_symbol_terminal      = 0x8000,
    dipsh_symbol_seq           = dipsh_symbol_terminal + 1,
    dipsh_symbol_bg            = dipsh_symbol_terminal + 2,
    dipsh_symbol_and           = dipsh_symbol_terminal + 3,
    dipsh_symbol_or            = dipsh_symbol_terminal + 4,
    dipsh_symbol_pipe_bar      = dipsh_symbol_terminal + 5,
    dipsh_symbol_word          = dipsh_symbol_terminal + 6,
    dipsh_symbol_redir_out     = dipsh_symbol_terminal + 7,
    dipsh_symbol_redir_in      = dipsh_symbol_terminal + 8,
    dipsh_symbol_redir_app     = dipsh_symbol_terminal + 9,
    dipsh_symbol_redir_dig_out = dipsh_symbol_terminal + 10,
    dipsh_symbol_redir_dig_in  = dipsh_symbol_terminal + 11,
    dipsh_symbol_redir_dig_app = dipsh_symbol_terminal + 12,
    dipsh_symbol_newline       = dipsh_symbol_terminal + 13,
    /* special symbols */
    dipsh_symbol_end_of_stream = 0x10000,
    dipsh_symbol_error         = 0x20000
}
dipsh_symbol_type;

const char *
dipsh_symbol_type_to_string(
    dipsh_symbol_type type
);

typedef struct dipsh_symbol_tag
{
    dipsh_symbol_type type;
}
dipsh_symbol;

typedef struct dipsh_nonterminal_child_tag
{
    dipsh_symbol *child;
    struct dipsh_nonterminal_child_tag *next;
} 
dipsh_nonterminal_child;

typedef struct dipsh_nonterminal_tag
{
    dipsh_symbol symb;
    dipsh_nonterminal_child *children_list;
}
dipsh_nonterminal;

typedef struct dipsh_terminal_tag
{
    dipsh_symbol symb;
    dipsh_token token;
}
dipsh_terminal;

void
dipsh_symbol_clear(
    dipsh_symbol *symb
);

typedef struct dipsh_parser_state_tag dipsh_parser_state;

dipsh_parser_state *
dipsh_parser_state_init();

void
dipsh_parser_state_destroy(
    dipsh_parser_state *state
);

const char *
dipsh_parser_state_get_error(
    const dipsh_parser_state *state
);

enum 
{
    dipsh_parser_accepted = 0,
    dipsh_parser_error = 1
};

int
dipsh_parser_next_token(
    dipsh_parser_state *state,
    const dipsh_token *token
);

int
dipsh_parser_finish(
    dipsh_parser_state *state,
    dipsh_symbol **parse_tree_root
);

int
dipsh_parse_token_list(
    dipsh_token_list *token_list,
    dipsh_symbol **parse_tree_root,
    char **parser_error
);

void
dipsh_make_ast(
    dipsh_symbol **parse_tree_root
);

#endif /* _DIPSH_PARSER_H_ */
