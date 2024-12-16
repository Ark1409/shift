#ifndef SHIFT_PARSING_PARSER_TYPES_FWD_H_
#define SHIFT_PARSING_PARSER_TYPES_FWD_H_ 1

#include "types_fwd.h"
#include <functional>

namespace shift::compiler::parsing {
    struct token_group;
    struct parser_module;
    struct parser_type;

    struct shift_expression;
    struct literal_expression;
    struct unary_expression;
    struct binary_expression;
    struct cp_expression;
    struct mv_expression;
    struct bracket_expression;
    struct cast_expression;
    struct array_expression;
    struct function_call_expression;
    struct new_expression;
    struct del_expression;
    struct comma_expression;
    struct dotted_expression;

    struct parser_variable;

    struct shift_statement;
    struct block_statement;
    struct if_statement;
    struct while_statement;
    struct do_while_statement;
    struct expression_statement;
    struct variable_def_statement;
    struct for_statement;
    struct use_statement;
    struct continue_statement;
    struct break_statement;
    struct return_statement;

    struct parser_class;
    struct parser_function;
}

template<>
struct std::hash<shift::compiler::parsing::token_group>;

template<>
struct std::hash<shift::compiler::parsing::parser_module>;

template<>
struct std::hash<shift::compiler::parsing::parser_type>;

#endif //SHIFT_PARSING_PARSER_TYPES_FWD_H_
