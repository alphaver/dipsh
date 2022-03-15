#include "parser.h"

static void
dipshp_nonterminal_clear(
    dipsh_nonterminal *symb
)
{
    dipsh_nonterminal_child *curr = symb->children_list;
    while (curr) {
        dipsh_nonterminal_child *temp = curr;
        curr = curr->next;
        dipsh_symbol_clear(temp->child);
        free(temp);
    }
    free(symb);
}

static void
dipshp_terminal_clear(
    dipsh_terminal *symb
)
{
    dipsh_token_clean(&symb_token);
    free(symb);
}

void
dipsh_symbol_clear(
    dipsh_symbol *symb
)
{
    if (symb->type & dipsh_symbol_nonterminal)
        dipshp_nonterminal_clear((dipsh_nonterminal *)symb);
    else if (symb->type & dipsh_symbol_terminal)
        dipshp_terminal_clear((dipsh_terminal *)symb);
}

typedef struct dipshp_grammar_rule_tag
{
    dipsh_symbol_type left_hand_symb;
    int right_hand_side;
    const dipsh_symbol_type *right_hand_symb;
}
dipshp_grammar_rule;

#define DIPSHP_DEFINE_GRAMMAR_RULE(rule_name, left_hand, ...)              \
const dipsh_symbol_type dipshp_right_hand_##rule_name[] = { __VA_ARGS__ }; \
const dipshp_grammar_rule rule_name {                                      \
    left_hand, sizeof(dipshp_right_hand_##rule_name),                      \
    dipshp_right_hand_##rule_name                                          \
};

DIPSHP_DEFINE_GRAMMAR_RULE(
    start, dipsh_symbol_script, 
    dipsh_symbol_seq_bg_start
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    seq_bg_start_1, dipsh_symbol_seq_bg_start,
    dipsh_symbol_seq_bg
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    seq_bg_start_2, dipsh_symbol_seq_bg_start,
    dipsh_symbol_seq_bg, dipsh_symbol_bg
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    seq_bg_start_3, dipsh_symbol_seq_bg_start,
    dipsh_symbol_seq_bg, dipsh_symbol_seq
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    seq_bg_1, dipsh_symbol_seq_bg,
    dipsh_symbol_and_or
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    seq_bg_2, dipsh_symbol_seq_bg,
    dipsh_symbol_seq_bg, dipsh_symbol_bg, dipsh_symbol_and_or
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    seq_bg_3, dipsh_symbol_seq_bg,
    dipsh_symbol_seq_bg, dipsh_symbol_seq, dipsh_symbol_and_or
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    and_or_1, dipsh_symbol_and_or,
    dipsh_symbol_pipe
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    and_or_2, dipsh_symbol_and_or,
    dipsh_symbol_and_or, dipsh_symbol_and, dipsh_symbol_pipe
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    and_or_3, dipsh_symbol_and_or,
    dipsh_symbol_and_or, dipsh_symbol_or, dipsh_symbol_pipe
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    pipe_1, dipsh_symbol_pipe,
    dipsh_symbol_command
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    pipe_2, dipsh_symbol_pipe,
    dipsh_symbol_pipe, dipsh_symbol_pipe_bar, dipsh_symbol_command
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    command_1, dipsh_symbol_command,
    dipsh_symbol_word
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    command_2, dipsh_symbol_command,
    dipsh_symbol_command, dipsh_symbol_word
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    command_3, dipsh_symbol_command,
    dipsh_symbol_command, dipsh_symbol_redir
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    redir_1, dipsh_symbol_redir,
    dipsh_symbol_redir_out, dipsh_symbol_word
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    redir_2, dipsh_symbol_redir,
    dipsh_symbol_redir_in, dipsh_symbol_word
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    redir_3, dipsh_symbol_redir,
    dipsh_symbol_redir_app, dipsh_symbol_word
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    redir_4, dipsh_symbol_redir,
    dipsh_symbol_redir_dig_out, dipsh_symbol_word
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    redir_5, dipsh_symbol_redir,
    dipsh_symbol_redir_dig_in, dipsh_symbol_word
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    redir_6, dipsh_symbol_redir,
    dipsh_symbol_redir_dig_app, dipsh_symbol_word
)

const dipshp_grammar_rule *dipshp_grammar_rules[] = {
    &start,
    &seq_bg_start_1, &seq_bg_start_2, &seq_bg_start_3,
    &seq_bg_1, &seq_bg_2, &seq_bg_3,
    &and_or_1, &and_or_2, &and_or_3,
    &pipe_1, &pipe_2,
    &command_1, &command_2, &command_3,
    &redir_1, &redir_2, &redir_3, &redir_4, &redir_5, &redir_6
};

typedef enum dipshp_parse_action_type_tag
{
    dipshp_parse_shift,
    dipshp_parse_reduce,
    dipshp_parse_accept,
    dipshp_parse_error
}
dipshp_parse_action_type;

typedef struct dipshp_parse_action_tag
{
    dipshp_parse_action_type action_type;
    int number;
}
dipshp_parse_action;

#define S(num) { dipshp_parse_shift, num }
#define R(num) { dipshp_parse_reduce, num }
#define A      { dipshp_parse_accept }
#define E      { dipshp_parse_error }

#define DIPSHP_TOTAL_STATES 31

static const dipshp_parse_action 
dipshp_parse_actions[DIPSHP_TOTAL_STATES][sizeof(dipsh_symbol_types)] = {
    /* 0 */
    { E,     S(1),  S(2),  S(3),  S(4),  S(5),  E,
      E,     E,     E,     E,     E,     S(6),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 1 */
    { E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     E,  
      E,     E,     E,     E,     E,     E,     A     },
    /* 2 */
    { E,     E,     E,     E,     E,     E,     E,
      S(7),  S(8),  E,     E,     E,     E,  
      E,     E,     E,     E,     E,     E,     R(1)  },
    /* 3 */
    { E,     E,     E,     E,     E,     E,     E,
      R(4),  R(4),  S(9),  S(10), E,     E,  
      E,     E,     E,     E,     E,     E,     R(4)  },
    /* 4 */
    { E,     E,     E,     E,     E,     E,     E,
      R(7),  R(7),  R(7),  R(7),  S(11), E,  
      E,     E,     E,     E,     E,     E,     R(7)  },
    /* 5 */
    { E,     E,     E,     E,     E,     E,     S(13),
      R(10), R(10), R(10), R(10), R(10), S(12),  
      S(14), S(15), S(16), S(17), S(18), S(19), R(10) },
    /* 6 */
    { E,     E,     E,     E,     E,     E,     E,
      R(12), R(12), R(12), R(12), R(12), R(12),  
      R(12), R(12), R(12), R(12), R(12), R(12), R(12) },
    /* 7 */
    { E,     E,     E,     S(20), S(4),  S(5),  E,
      E,     E,     E,     E,     E,     S(6),  
      E,     E,     E,     E,     E,     E,     R(2)  },
    /* 8 */
    { E,     E,     E,     S(21), S(4),  S(5),  E,
      E,     E,     E,     E,     E,     S(6),  
      E,     E,     E,     E,     E,     E,     R(3)  },
    /* 9 */
    { E,     E,     E,     E,     S(22), S(5),  E,
      E,     E,     E,     E,     E,     S(6),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 10 */
    { E,     E,     E,     E,     S(23), S(5),  E,
      E,     E,     E,     E,     E,     S(6),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 11 */
    { E,     E,     E,     E,     E,     S(24), E,
      E,     E,     E,     E,     E,     S(6),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 12 */
    { E,     E,     E,     E,     E,     E,     E,
      R(13), R(13), R(13), R(13), R(13), R(13),  
      R(13), R(13), R(13), R(13), R(13), R(13), R(13) },
    /* 13 */
    { E,     E,     E,     E,     E,     E,     E,
      R(14), R(14), R(14), R(14), R(14), R(14),  
      R(14), R(14), R(14), R(14), R(14), R(14), R(14) },
    /* 14 */
    { E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     S(25),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 15 */
    { E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     S(26),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 16 */
    { E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     S(27),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 17 */
    { E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     S(28),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 18 */
    { E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     S(29),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 19 */
    { E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     S(30),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 20 */
    { E,     E,     E,     E,     E,     E,     E,
      R(5),  R(5),  S(9),  S(10), E,     E,
      E,     E,     E,     E,     E,     E,     R(5)  },
    /* 21 */
    { E,     E,     E,     E,     E,     E,     E,
      R(6),  R(6),  S(9),  S(10), E,     E,
      E,     E,     E,     E,     E,     E,     R(6)  },
    /* 22 */
    { E,     E,     E,     E,     E,     E,     E,
      R(8),  R(8),  R(8),  R(8),  S(11), E,
      E,     E,     E,     E,     E,     E,     R(8)  },
    /* 23 */
    { E,     E,     E,     E,     E,     E,     E,
      R(9),  R(9),  R(9),  R(9),  S(11), E,
      E,     E,     E,     E,     E,     E,     R(9)  },
    /* 24 */
    { E,     E,     E,     E,     E,     E,     S(13),
      R(11), R(11), R(11), R(11), R(11), S(12),  
      S(14), S(15), S(16), S(17), S(18), S(19), R(11) },
    /* 25 */
    { E,     E,     E,     E,     E,     E,     E,
      R(15), R(15), R(15), R(15), R(15), R(15),  
      R(15), R(15), R(15), R(15), R(15), R(15), R(15) },
    /* 26 */
    { E,     E,     E,     E,     E,     E,     E,
      R(16), R(16), R(16), R(16), R(16), R(16),  
      R(16), R(16), R(16), R(16), R(16), R(16), R(16) },
    /* 27 */
    { E,     E,     E,     E,     E,     E,     E,
      R(17), R(17), R(17), R(17), R(17), R(17),  
      R(17), R(17), R(17), R(17), R(17), R(17), R(17) },
    /* 28 */
    { E,     E,     E,     E,     E,     E,     E,
      R(18), R(18), R(18), R(18), R(18), R(18),  
      R(18), R(18), R(18), R(18), R(18), R(18), R(18) },
    /* 29 */
    { E,     E,     E,     E,     E,     E,     E,
      R(19), R(19), R(19), R(19), R(19), R(19),  
      R(19), R(19), R(19), R(19), R(19), R(19), R(19) },
    /* 30 */
    { E,     E,     E,     E,     E,     E,     E,
      R(20), R(20), R(20), R(20), R(20), R(20),  
      R(20), R(20), R(20), R(20), R(20), R(20), R(20) },
};

#undef S
#undef R
#undef A
#undef E

struct dipsh_parser_state_tag
{
    struct dipshp_parser_stack_tag
    {
        union
        {
            dipsh_symbol *symb;
            int state_num;
        }
        struct dipshp_parser_stack_tag *next;
    } 
    *symbol_stack, *state_stack;
    int clear_symbol_stack;
    char *error_msg;
};

static int
dipshp_parser_next_symbol(
    dipshp_parser_state *state,
    dipsh_symbol *symb
)
{
}

dipsh_parser_state *
dipsh_parser_state_init()
{
    dipsh_parser_state *result = calloc(sizeof(dipsh_parser_state), 1);
    result->clear_symbol_stack = 1;
    return result;
}

static void
dipshp_parser_state_destroy_stack(
    struct dipshp_parser_stack_tag *stack
)
{
    struct dipshp_parser_stack_tag *curr = stack;
    while (curr) {
        struct dipshp_parser_stack_tag *temp = curr;
        curr = curr->next;
        free(temp);
    } 
}

void
dipsh_parser_state_destroy(
    dipsh_parser_state *state
)
{
    if (state->error_msg)
        free(state->error_msg);
    dipshp_parser_state_destroy_stack(state->state_stack);
    if (state->clear_symbol_stack)
        dipshp_parser_state_destroy_stack(state->symbol_stack);
    else 
    /* if we don't need to clear the symbol stack, that means successful 
     * parsing so we have only the starting symbol in it, but the symbol was 
     * passed outside, so the only thing we need to do here is clear the top of
     * the stack */
        free(state->symbol_stack);
    free(state);
}

const char *
dipsh_parser_get_error(
    const dipsh_parser_state *state
)
{
    return state->error_msg;
}

static const struct 
{
    dipsh_token_type token_type;
    dipsh_symbol_type symbol_type;
}
dipshp_tokens_to_symbols[] = {
    { dipsh_token_word,          dipsh_symbol_word          },
    { dipsh_token_amp,           dipsh_symbol_bg            },
    { dipsh_token_dbl_amp,       dipsh_symbol_and           },
    { dipsh_token_bar,           dipsh_symbol_pipe_bar      },
    { dipsh_token_dbl_bar,       dipsh_symbol_or            },
    { dipsh_token_semicolon,     dipsh_symbol_seq           },
    { dipsh_token_lt,            dipsh_symbol_redir_in      },
    { dipsh_token_gt,            dipsh_symbol_redir_out     },
    { dipsh_token_dbl_gt,        dipsh_symbol_redir_app     },
    { dipsh_token_digits_lt,     dipsh_symbol_redir_dig_in  },
    { dipsh_token_digits_gt,     dipsh_symbol_redir_dig_out },
    { dipsh_token_digits_dbl_gt, dipsh_symbol_redir_dig_app }
};

dipsh_symbol_type
dipshp_token_type_to_symbol_type(
    dipsh_token_type token_type
)
{
    int tokens_to_symbols_len = sizeof(dipshp_tokens_to_symbols) /
        sizeof(dipshp_tokens_to_symbols[0]);
    for (int i = 0; i < tokens_to_symbols_len; ++i) {
        if (dipshp_tokens_to_symbols[i].token_type == token_type)
            return dipshp_tokens_to_symbols[i].symbol_type;
    }
    return dipshp_symbol_error;
}

dipsh_symbol *
dipshp_token_to_symbol(
    dipsh_token *token
)
{
    dipsh_terminal *symb = calloc(sizeof(dipsh_terminal), 1);
    symb->type = dipshp_token_type_to_symbol_type(token->type);
    dipsh_token_init(&symb->token, token->type, token->line, token->value);
    return (dipsh_symbol *)symb;
}

int
dipsh_parser_next_token(
    dipsh_parser_state *state,
    dipsh_token *token
)
{
    dipsh_symbol *symb = dipshp_token_to_symbol(token);
    return dipshp_parser_next_symbol(state, symb);
}

int
dipsh_parser_finish(
    dipsh_parser_state *state,
    dipsh_symbol **parse_tree_root
)
{
    dipsh_symbol *symb = malloc(sizeof(dipsh_symbol));
    symb->type = dipsh_symbol_end_of_stream;
    int return_val = dipshp_parser_next_symbol(state, symb);
    if (dipsh_parser_accepted == return_val) {
        *parse_tree_root = state->symbol_stack->symb;
        clear_symbol_stack = 0;
    }
    return return_val;
}

int
dipsh_parse_token_list(
    dipsh_token_list *token_list,
    dipsh_symbol **parse_tree_root,
    char **parser_error
)
{
    dipsh_parser_state *state = dipsh_parser_state_init();

    int parser_ret;
    for (dipsh_token_list *curr = token_list; curr; curr = curr->next) {
        parser_ret = dipsh_parser_next_token(state, &curr->token);
        if (dipsh_parser_accepted != parser_ret) 
            break; 
    }
    if (dipsh_parser_accepted == parser_ret)
        parser_ret = dipsh_parser_finish(state, parse_tree_root);
    if (dipsh_parser_accepted != parser_ret && parser_error)
        *parser_error = strdup(dipsh_parser_state_get_error(state));

    dipsh_parser_state_destroy(state);
    return parser_ret;
}
