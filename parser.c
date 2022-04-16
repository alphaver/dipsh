#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <err.h>

typedef struct dipshp_symbol_traits_tag
{
    dipsh_symbol_type type;
    const char *name;
}
dipshp_symbol_traits;

static const dipshp_symbol_traits symbol_traits[] = {
    { dipsh_symbol_script, "script" },
    { dipsh_symbol_strings, "strings" },
    { dipsh_symbol_seq_bg_start, "seq_bg_start" },
    { dipsh_symbol_seq_bg, "seq_bg" },
    { dipsh_symbol_and_or, "and_or" },
    { dipsh_symbol_pipe, "pipe" },
    { dipsh_symbol_command, "command" },
    { dipsh_symbol_redir, "redir" },
    { dipsh_symbol_seq, "seq" },
    { dipsh_symbol_bg, "bg" },
    { dipsh_symbol_and, "and" },
    { dipsh_symbol_or, "or" },
    { dipsh_symbol_pipe_bar, "pipe_bar" },
    { dipsh_symbol_word, "word" },
    { dipsh_symbol_redir_out, "redir_out" },
    { dipsh_symbol_redir_in, "redir_in" },
    { dipsh_symbol_redir_app, "redir_app" },
    { dipsh_symbol_redir_dig_out, "redir_dig_out" },
    { dipsh_symbol_redir_dig_in, "redir_dig_in" },
    { dipsh_symbol_redir_dig_app, "redir_dig_app" },
    { dipsh_symbol_end_of_stream, "end_of_stream" },
    { dipsh_symbol_newline, "newline" },
    { dipsh_symbol_error, "error" }
}; 

const char *
dipsh_symbol_type_to_string(
    dipsh_symbol_type type
)
{
    const dipshp_symbol_traits *traits = symbol_traits;
    for (; traits->type != dipsh_symbol_error; ++traits) {
        if (type == traits->type)
            return traits->name;
    }
    return traits->name;
}

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
    dipsh_token_clean(&symb->token);
    free(symb);
}

void
dipsh_symbol_clear(
    dipsh_symbol *symb
)
{
    if (!symb)
        return;
    if (symb->type & dipsh_symbol_nonterminal)
        dipshp_nonterminal_clear((dipsh_nonterminal *)symb);
    else if (symb->type & dipsh_symbol_terminal)
        dipshp_terminal_clear((dipsh_terminal *)symb);
}

typedef struct dipshp_grammar_rule_tag
{
    dipsh_symbol_type left_hand_symb;
    int right_hand_size;
    const dipsh_symbol_type *right_hand_symb;
}
dipshp_grammar_rule;

#define DIPSHP_DEFINE_GRAMMAR_RULE(rule_name, left_hand, ...)              \
static const dipsh_symbol_type dipshp_right_hand_##rule_name[] =           \
    { __VA_ARGS__ };                                                       \
static const dipshp_grammar_rule rule_name = {                             \
    left_hand,                                                             \
    sizeof(dipshp_right_hand_##rule_name) / sizeof(dipsh_symbol_type),     \
    dipshp_right_hand_##rule_name                                          \
};

DIPSHP_DEFINE_GRAMMAR_RULE(
    start, dipsh_symbol_script, 
    dipsh_symbol_strings
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    strings_1, dipsh_symbol_strings,
    dipsh_symbol_seq_bg_start
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    strings_2, dipsh_symbol_strings,
    dipsh_symbol_strings, dipsh_symbol_newline
)
DIPSHP_DEFINE_GRAMMAR_RULE(
    strings_3, dipsh_symbol_strings,
    dipsh_symbol_strings, dipsh_symbol_newline, dipsh_symbol_seq_bg_start
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

static const dipshp_grammar_rule *dipshp_grammar_rules[] = {
    &start,
    &strings_1, &strings_2, &strings_3,
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
    dipshp_parse_action_type type;
    int number;
}
dipshp_parse_action;

#define S(num) { dipshp_parse_shift, num }
#define R(num) { dipshp_parse_reduce, num }
#define A      { dipshp_parse_accept }
#define E      { dipshp_parse_error }

#define DIPSHP_TOTAL_STATES 34

static const dipsh_symbol_type dipshp_symbol_types[] = {
    dipsh_symbol_script,
    dipsh_symbol_strings,
    dipsh_symbol_seq_bg_start,
    dipsh_symbol_seq_bg,
    dipsh_symbol_and_or,
    dipsh_symbol_pipe,
    dipsh_symbol_command,
    dipsh_symbol_redir,

    dipsh_symbol_newline,
    dipsh_symbol_seq,
    dipsh_symbol_bg,
    dipsh_symbol_and,
    dipsh_symbol_or,
    dipsh_symbol_pipe_bar,
    dipsh_symbol_word,
    dipsh_symbol_redir_out,
    dipsh_symbol_redir_in,
    dipsh_symbol_redir_app,
    dipsh_symbol_redir_dig_out,
    dipsh_symbol_redir_dig_in,
    dipsh_symbol_redir_dig_app,

    dipsh_symbol_end_of_stream
};

#define DIPSHP_SYMBOL_TYPES_NUM \
    sizeof(dipshp_symbol_types) / sizeof(dipsh_symbol_type)

static const dipshp_parse_action 
dipshp_parse_actions[DIPSHP_TOTAL_STATES][DIPSHP_SYMBOL_TYPES_NUM] = {
    /* 0 */
    { E,     S(1),  S(2),  S(3),  S(4),  S(5),  S(6),  E,
      E,     E,     E,     E,     E,     E,     S(7),  
      E,     E,     E,     E,     E,     E,     E      },
    /* 1 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      S(8),  E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     E,     A     },
    /* 2 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(1),  E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     E,     R(1)  },
    /* 3 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(4),  S(9),  S(10), E,     E,     E,     E,  
      E,     E,     E,     E,     E,     E,     R(1)  },
    /* 4 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(7),  R(7),  R(7),  S(11), S(12), E,     E,  
      E,     E,     E,     E,     E,     E,     R(7)  },
    /* 5 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(10), R(10), R(10), R(10), R(10), S(13), E,  
      E,     E,     E,     E,     E,     E,     R(10) },
    /* 6 */
    { E,     E,     E,     E,     E,     E,     E,     S(15),
      R(13), R(13), R(13), R(13), R(13), R(13), S(14),  
      S(16), S(17), S(18), S(19), S(20), S(21), R(13) },
    /* 7 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(15), R(15), R(15), R(15), R(15), R(15), R(15),  
      R(15), R(15), R(15), R(15), R(15), R(15), R(15) },
    /* 8 */
    { E,     E,     S(22), S(3),  S(4),  S(5),  S(6),  E,
      R(2),  E,     E,     E,     E,     E,     S(7),  
      E,     E,     E,     E,     E,     E,     R(2)  },
    /* 9 */
    { E,     E,     E,     E,     S(23), S(5),  S(6),  E,
      R(5),  E,     E,     E,     E,     E,     S(7),  
      E,     E,     E,     E,     E,     E,     R(5)  },
    /* 10 */
    { E,     E,     E,     E,     S(24), S(5),  S(6),  E,
      R(6),  E,     E,     E,     E,     E,     S(7),  
      E,     E,     E,     E,     E,     E,     R(6)  },
    /* 11 */
    { E,     E,     E,     E,     E,     S(25), S(6),  E,
      E,     E,     E,     E,     E,     E,     S(7),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 12 */
    { E,     E,     E,     E,     E,     S(26), S(6),  E,
      E,     E,     E,     E,     E,     E,     S(7),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 13 */
    { E,     E,     E,     E,     E,     E,     S(27), E,
      E,     E,     E,     E,     E,     E,     S(7),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 14 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(16), R(16), R(16), R(16), R(16), R(16), R(16),  
      R(16), R(16), R(16), R(16), R(16), R(16), R(16) },
    /* 15 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(17), R(17), R(17), R(17), R(17), R(17), R(17),  
      R(17), R(17), R(17), R(17), R(17), R(17), R(17) },
    /* 16 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     E,     S(28),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 17 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     E,     S(29),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 18 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     E,     S(30),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 19 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     E,     S(31),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 20 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     E,     S(32),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 21 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     E,     S(33),  
      E,     E,     E,     E,     E,     E,     E     },
    /* 22 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(3),  E,     E,     E,     E,     E,     E,
      E,     E,     E,     E,     E,     E,     R(3)  },
    /* 23 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(8),  R(8),  R(8),  S(11), S(12), E,     E,
      E,     E,     E,     E,     E,     E,     R(8)  },
    /* 24 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(9),  R(9),  R(9),  S(11), S(12), E,     E,
      E,     E,     E,     E,     E,     E,     R(9)  },
    /* 25 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(11), R(11), R(11), R(11), R(11), S(13), E,
      E,     E,     E,     E,     E,     E,     R(11) },
    /* 26 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(12), R(12), R(12), R(12), R(12), S(13), E,
      E,     E,     E,     E,     E,     E,     R(12) },
    /* 27 */
    { E,     E,     E,     E,     E,     E,     E,     S(15),
      R(14), R(14), R(14), R(14), R(14), R(14), S(14),  
      S(16), S(17), S(18), S(19), S(20), S(21), R(14) },
    /* 28 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(18), R(18), R(18), R(18), R(18), R(18), R(18),  
      R(18), R(18), R(18), R(18), R(18), R(18), R(18) },
    /* 29 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(19), R(19), R(19), R(19), R(19), R(19), R(19),  
      R(19), R(19), R(19), R(19), R(19), R(19), R(19) },
    /* 30 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(20), R(20), R(20), R(20), R(20), R(20), R(20),  
      R(20), R(20), R(20), R(20), R(20), R(20), R(20) },
    /* 31 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(21), R(21), R(21), R(21), R(21), R(21), R(21),  
      R(21), R(21), R(21), R(21), R(21), R(21), R(21) },
    /* 32 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(22), R(22), R(22), R(22), R(22), R(22), R(22),  
      R(22), R(22), R(22), R(22), R(22), R(22), R(22) },
    /* 33 */
    { E,     E,     E,     E,     E,     E,     E,     E,
      R(23), R(23), R(23), R(23), R(23), R(23), R(23),  
      R(23), R(23), R(23), R(23), R(23), R(23), R(23) },
};

#undef S
#undef R
#undef A
#undef E

typedef struct dipshp_parser_stack_tag
{
    union
    {
        dipsh_symbol *symb;
        int state_num;
    };
    struct dipshp_parser_stack_tag *next;
} 
dipshp_parser_stack;

struct dipsh_parser_state_tag
{
    dipshp_parser_stack *symbol_stack, *state_stack;
    int clear_symbol_stack;
    int last_line;
    char *error;
};

static void
dipshp_push_onto_stack(
    dipshp_parser_stack **stack,
    dipshp_parser_stack *new_top
)
{
    new_top->next = *stack;
    *stack = new_top;
}

static dipshp_parser_stack *
dipshp_pop_stack(
    dipshp_parser_stack **stack
)
{
    dipshp_parser_stack *top = *stack;
    *stack = (*stack)->next;
    return top;
}

static void
dipshp_push_slr_state(
    dipsh_parser_state *state,
    int state_num
)
{
    dipshp_parser_stack *new_top = calloc(sizeof(dipshp_parser_stack), 1);
    new_top->state_num = state_num;
    dipshp_push_onto_stack(&state->state_stack, new_top);
}

static void
dipshp_push_slr_symbol(
    dipsh_parser_state *state,
    dipsh_symbol *symb
)
{
    dipshp_parser_stack *new_top = calloc(sizeof(dipshp_parser_stack), 1);
    new_top->symb = symb;
    dipshp_push_onto_stack(&state->symbol_stack, new_top);
}

static dipshp_parser_stack *
dipshp_pop_slr_state(
    dipsh_parser_state *state
)
{
    return dipshp_pop_stack(&state->state_stack);
}

static dipshp_parser_stack *
dipshp_pop_slr_symbol(
    dipsh_parser_state *state
)
{
    return dipshp_pop_stack(&state->symbol_stack);
}

static int
dipshp_type_index(
    dipsh_symbol_type type
)
{
    for (const dipsh_symbol_type *curr = dipshp_symbol_types; 
         curr - dipshp_symbol_types < DIPSHP_SYMBOL_TYPES_NUM;
         ++curr) {
        if (*curr == type)
            return curr - dipshp_symbol_types;
    }
    return -1;
}

static void
dipshp_parser_state_destroy_stack(
    dipshp_parser_stack *stack,
    int clear_symbol_stack
)
{
    dipshp_parser_stack *curr = stack;
    while (curr) {
        dipshp_parser_stack *temp = curr;
        curr = curr->next;
        if (clear_symbol_stack)
            dipsh_symbol_clear(temp->symb);
        free(temp);
    } 
}

static dipsh_symbol *
dipshp_build_nonterminal(
    dipsh_symbol_type type,
    dipshp_parser_stack *stack 
)
{
    dipsh_nonterminal *result = calloc(sizeof(dipsh_nonterminal), 1);
    dipsh_nonterminal_child *last_child = NULL;
    result->symb.type = type;
    for (dipshp_parser_stack *pos = stack; pos; pos = pos->next) {
        dipsh_nonterminal_child *ch = 
            calloc(sizeof(dipsh_nonterminal_child), 1);
        ch->child = pos->symb;
        if (!last_child) {
            result->children_list = ch;
        } else {
            last_child->next = ch;
        }
        last_child = ch;
    }
    return (dipsh_symbol *)result;
}

static void
dipshp_handle_reduce(
    dipsh_parser_state *state,
    const dipshp_parse_action *action
)
{
    const dipshp_grammar_rule *rule = dipshp_grammar_rules[action->number];
    const dipsh_symbol_type *rule_symb = 
        rule->right_hand_symb + rule->right_hand_size - 1;
    dipshp_parser_stack *st = NULL;
    for (; rule_symb >= rule->right_hand_symb; --rule_symb) {
        dipshp_parser_stack *symb_top = dipshp_pop_slr_symbol(state);
        dipshp_parser_stack *state_top = dipshp_pop_slr_state(state);
        free(state_top);
        dipshp_push_onto_stack(&st, symb_top);
    }
    int left_hand_index = dipshp_type_index(rule->left_hand_symb);
    int top_state = state->state_stack->state_num; 
    const dipshp_parse_action *lh_action = 
        &dipshp_parse_actions[top_state][left_hand_index];
    dipsh_symbol *nonterm = 
        dipshp_build_nonterminal(rule->left_hand_symb, st);
    dipshp_parser_state_destroy_stack(st, 0);
    dipshp_push_slr_state(state, lh_action->number);
    dipshp_push_slr_symbol(state, nonterm);
}

#define DIPSHP_SET_STATE_ERROR(st, str) st->error = strdup(str)
#define DIPSHP_SET_STATE_ERROR_VA(st, fmt, ...) \
    asprintf(&st->error, fmt, __VA_ARGS__)
#define DIPSHP_UNKNOWN_TOKEN          "line %d: unknown token: '%s'"
#define DIPSHP_UNKNOWN_SYMBOL         "unknown symbol type encountered"
#define DIPSHP_UNEXPECTED_NONTERMINAL "parser didn't expect a nonterminal here"
#define DIPSHP_TOKEN_UNEXPECTED_HERE  "line %d: token '%s' unexpected here"

static int
dipshp_parser_next_symbol(
    dipsh_parser_state *state,
    dipsh_symbol *symb
)
{
    int symb_index = dipshp_type_index(symb->type);
    dipsh_terminal *term_symb = (dipsh_terminal *)symb;
    if (-1 == symb_index || 
        (!(symb->type & dipsh_symbol_terminal) &&
         (symb->type != dipsh_symbol_end_of_stream))) {
        if (symb->type & dipsh_symbol_terminal) {
            DIPSHP_SET_STATE_ERROR_VA(
                state, DIPSHP_UNKNOWN_TOKEN, 
                term_symb->token.line, term_symb->token.value
            );
        } else {
            DIPSHP_SET_STATE_ERROR(state, DIPSHP_UNKNOWN_SYMBOL);
        }
        return dipsh_parser_error;
    }
    if (symb->type & dipsh_symbol_terminal)
        state->last_line = term_symb->token.line;
    for (;;) {
        int top_state = state->state_stack->state_num;

        const dipshp_parse_action *action = 
            &dipshp_parse_actions[top_state][symb_index];
        switch (action->type) {
        case dipshp_parse_shift: 
            dipshp_push_slr_state(state, action->number);
            dipshp_push_slr_symbol(state, symb);
            return dipsh_parser_accepted;
        case dipshp_parse_reduce: 
            dipshp_handle_reduce(state, action);
            break;
        case dipshp_parse_accept: {
            dipshp_parse_action acc_act = { dipshp_parse_reduce, 0 };
            dipshp_handle_reduce(state, &acc_act);
            return dipsh_parser_accepted;
        }
        default:
            if (symb->type & dipsh_symbol_terminal) {
                DIPSHP_SET_STATE_ERROR_VA(
                    state, DIPSHP_TOKEN_UNEXPECTED_HERE, 
                    term_symb->token.line, term_symb->token.value
                );
            } else {
                DIPSHP_SET_STATE_ERROR_VA(
                    state, DIPSHP_TOKEN_UNEXPECTED_HERE,
                    state->last_line, "end of stream"
                );
            }
            return dipsh_parser_error;
        }
    }
}

dipsh_parser_state *
dipsh_parser_state_init()
{
    dipsh_parser_state *result = calloc(sizeof(dipsh_parser_state), 1);
    result->clear_symbol_stack = 1;
    dipshp_push_slr_state(result, 0);
    return result;
}

void
dipsh_parser_state_destroy(
    dipsh_parser_state *state
)
{
    if (state->error)
        free(state->error);
    dipshp_parser_state_destroy_stack(state->state_stack, 0);
    dipshp_parser_state_destroy_stack(
        state->symbol_stack, state->clear_symbol_stack
    );
    free(state);
}

const char *
dipsh_parser_state_get_error(
    const dipsh_parser_state *state
)
{
    return state->error;
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
    { dipsh_token_digits_dbl_gt, dipsh_symbol_redir_dig_app },
    { dipsh_token_newline,       dipsh_symbol_newline       }
};

static dipsh_symbol_type
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
    return dipsh_symbol_error;
}

static dipsh_symbol *
dipshp_token_to_symbol(
    const dipsh_token *token
)
{
    dipsh_terminal *result = calloc(sizeof(dipsh_terminal), 1);
    result->symb.type = dipshp_token_type_to_symbol_type(token->type);
    dipsh_token_init(&result->token, token->type, token->line, token->value);
    return (dipsh_symbol *)result;
}

int
dipsh_parser_next_token(
    dipsh_parser_state *state,
    const dipsh_token *token
)
{
    dipsh_symbol *symb = dipshp_token_to_symbol(token);
    int ret = dipshp_parser_next_symbol(state, symb);
    if (dipsh_parser_error == ret)
        dipsh_symbol_clear(symb);
    return ret;
}

int
dipsh_parser_finish(
    dipsh_parser_state *state,
    dipsh_symbol **parse_tree_root
)
{
    dipsh_symbol symb = { dipsh_symbol_end_of_stream };
    int return_val = dipshp_parser_next_symbol(state, &symb);
    if (dipsh_parser_accepted == return_val) {
        state->clear_symbol_stack = 0;
        *parse_tree_root = state->symbol_stack->symb;
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

static void
dipshp_prune_terminals(
    dipsh_nonterminal_child **list
)
{
    dipsh_nonterminal_child **curr = list;
    while (*curr) {
        if ((*curr)->child->type & dipsh_symbol_terminal) {
            dipsh_nonterminal_child *temp = *curr;
            *curr = (*curr)->next;
            dipsh_symbol_clear(temp->child);
            free(temp);
        } else {
            curr = &((*curr)->next);
        }
    }
}

static void
dipshp_flatten_left_recursion(
    dipsh_symbol **parse_tree_root,
    dipsh_symbol_type symb_to_flatten,
    int preserve_terminals
)
{
    dipsh_nonterminal_child *flat_children = NULL;
    dipsh_nonterminal_child *root_children =
        (*(dipsh_nonterminal **)parse_tree_root)->children_list;
    if (symb_to_flatten != root_children->child->type)
        return;
    while (root_children) {
        dipsh_nonterminal_child *temp = NULL;
        if (symb_to_flatten == root_children->child->type) {
            temp = root_children;
            root_children = root_children->next;
        }
        dipsh_nonterminal_child **root_children_end = &root_children;
        while (*root_children_end)
            root_children_end = &((*root_children_end)->next);
        *root_children_end = flat_children;
        flat_children = root_children;
        if (temp) {
            root_children = ((dipsh_nonterminal *)temp->child)->children_list;
            free(temp->child);
            free(temp);
        } else {
            root_children = NULL; 
        }
    }
    if (!preserve_terminals)
        dipshp_prune_terminals(&flat_children);
    (*(dipsh_nonterminal **)parse_tree_root)->children_list = flat_children;
}

#define DIPSHP_DEFINE_FLATTEN_FUNCTION(sn, fsn, isn, pt)                       \
static void                                                                    \
dipshp_flatten_##sn(                                                           \
    dipsh_symbol **sn##_root                                                   \
)                                                                              \
{                                                                              \
    dipshp_flatten_left_recursion(sn##_root, dipsh_symbol_##fsn, pt);          \
    dipsh_nonterminal_child *children =                                        \
        (*(dipsh_nonterminal **)sn##_root)->children_list;                     \
    while (children) {                                                         \
        if (dipsh_symbol_##isn == children->child->type)                       \
            dipshp_flatten_##isn(&children->child);                            \
        children = children->next;                                             \
    }                                                                          \
}

static void
dipshp_flatten_command(
    dipsh_symbol **command_root
)
{
    dipshp_flatten_left_recursion(command_root, dipsh_symbol_command, 1);
}

DIPSHP_DEFINE_FLATTEN_FUNCTION(pipe, pipe, command, 0)
DIPSHP_DEFINE_FLATTEN_FUNCTION(and_or, and_or, pipe, 1)
DIPSHP_DEFINE_FLATTEN_FUNCTION(seq_bg_start, seq_bg, and_or, 1)
DIPSHP_DEFINE_FLATTEN_FUNCTION(script, strings, seq_bg_start, 0)

static void
dipshp_clean_chains_int(
    dipsh_symbol **subtree_root
)
{
    if (!((*subtree_root)->type & dipsh_symbol_nonterminal))
        return;
    dipsh_nonterminal_child *children;
    do {
        children = (*(dipsh_nonterminal **)subtree_root)->children_list;
        int in_chain = !children->next && 
            (children->child->type & dipsh_symbol_nonterminal);
        if (in_chain) {
            dipsh_nonterminal *temp = *(dipsh_nonterminal **)subtree_root;
            *subtree_root = children->child;
            free(children);
            free(temp);
        } else {
            break;
        }
    } while (1);
    children = (*(dipsh_nonterminal **)subtree_root)->children_list;
    while (children) {
        if (children->child->type & dipsh_symbol_nonterminal)
            dipshp_clean_chains_int(&children->child);
        children = children->next;
    }
}

static void
dipshp_clean_chains(
    dipsh_symbol **parse_tree_root
)
{
    dipsh_nonterminal_child *children =
        (*(dipsh_nonterminal **)parse_tree_root)->children_list;
    while (children) {
        dipshp_clean_chains_int(&children->child);
        children = children->next;
    } 
}

void
dipsh_make_ast(
    dipsh_symbol **parse_tree_root
)
{
    if (dipsh_symbol_script != (*parse_tree_root)->type)
        return;
    dipshp_flatten_script(parse_tree_root);
    dipshp_clean_chains(parse_tree_root);
}
