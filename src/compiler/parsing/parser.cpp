/**
 * @file compiler/shift_parser.cpp
 */
#include "compiler/parsing/parser.h"
#include "utils/lazy.h"

#include <ranges>
#include <cstring>
#include <algorithm>
#include <vector>
#include <array>
#include <bit>
#include <optional>
#include <string>
#include <string_view>
#include <stdexcept>

#include <fmt/format.h>

using namespace std::string_view_literals;
using namespace shift::compiler::lexing;

namespace shift::compiler::parsing {
    struct parser::parse_state {
        parse_state(const parser& p) : position{p.get_lexer()} {}

        parse_state(const parser&& p) = delete;

        // Current lexing::token position in the parsing process
        token_stream position;

        mods_holder mods{};
    };
}

namespace shift::compiler::parsing {
    constexpr shift_mods visibility_modifiers = shift_mods::PUBLIC | shift_mods::PROTECTED | shift_mods::PRIVATE;
    constexpr shift_mods class_modifiers = visibility_modifiers | shift_mods::STATIC;
    constexpr shift_mods function_modifiers = visibility_modifiers | shift_mods::STATIC | shift_mods::EXTERN;
    constexpr shift_mods global_function_modifiers = function_modifiers & ~(shift_mods::STATIC | visibility_modifiers);
    constexpr shift_mods type_modifiers = shift_mods::CONST_ | shift_mods::IMUT;
    constexpr shift_mods constructor_modifiers = (function_modifiers & ~shift_mods::STATIC) | shift_mods::EXPLICIT;
    constexpr shift_mods destructor_modifiers = function_modifiers & ~shift_mods::STATIC;
    constexpr shift_mods field_modifiers = visibility_modifiers | type_modifiers | shift_mods::STATIC | shift_mods::EXTERN;
    constexpr shift_mods variable_modifiers = type_modifiers;
    constexpr shift_mods global_variable_modifiers = variable_modifiers & ~(shift_mods::STATIC | visibility_modifiers);

    constexpr lexing::token this_token("this"sv, token::type::IDENTIFIER, {0, 0});
    constexpr lexing::token base_token("base"sv, token::type::IDENTIFIER, {0, 0});

    // Stores the string content of "@0", "@1", "@2", ..., which are used for identifying nameless function parameters.
    static std::unordered_set<std::string> func_null_params;

    SHIFT_API void parser::parse() {
        parse_state state{*this};
        parse_body(state, nullptr);
    }

    static void print_expr_tree(const shift_expression& expr, std::ostream& out, const std::string& prefix) {
        out << prefix;
        if (is_binary_operator(expr.type) || is_unary_operator(expr.type)) {
            out << expr.begin->get_data() << '\n';
            if (expr.has_left()) {
                print_expr_tree(*expr.get_left(), out, prefix + "|____");
            } else {
                out << prefix << "|____(null left)\n";
            }
            if (expr.has_right()) {
                print_expr_tree(*expr.get_right(), out, prefix + "|____");
            } else {
                out << prefix << "|____(null right)\n";
            }
        } else if (expr.is_dotted_expression()) {
            out << expr.to_string() << '\n';
            for (const auto& sub_expr : expr.get_dotted_expressions()) {
                print_expr_tree(sub_expr, out, prefix + "|____");
            }
        } else if (expr.is_array()) {
            out << expr.to_string() << '\n';
            if (expr.get_array_object()) {
                print_expr_tree(*expr.get_array_object(), out, prefix + "|____");
            } else {
                out << prefix << "|____(null array object)\n";
            }
            for (const auto& sub_expr : expr.get_array_dimensions()) {
                print_expr_tree(sub_expr, out, prefix + "|____");
            }
        }
            // else if (expr.type == type::NULL_TOKEN) {
            //     out << "\n";
            // }
        else if (expr.is_function_call()) {
            out << expr.to_string() << '\n';
            if (expr.get_function_call_object()) {
                print_expr_tree(*expr.get_function_call_object(), out, prefix + "|____");
            } else {
                out << prefix << "|____(null function call object)\n";
            }

            for (const auto& sub_expr : expr.get_function_call_arguments()) {
                print_expr_tree(sub_expr, out, prefix + "|____");
            }
        } else if (expr.is_comma()) {
            out << ",\n";
            for (const auto& sub_expr : expr.get_comma_expressions()) {
                print_expr_tree(sub_expr, out, prefix + "|____");
            }
        } else {
            if (expr.type != token::type::NULL_TOKEN && expr.size())
                out << expr.to_string();
            else
                out << "(null expression)";
            out << '\n';
        }
    }

    static void print_statement_tree(const shift_statement& statement, std::ostream& out, const std::string& prefix) {
        out << prefix;
        switch (statement.type) {
            case shift_statement::statement_type::variable_alloc:
                out << "variable_declaration:\n";
                out << prefix << "|____variable: " << statement.get_variable().get_fqn() << '\n';
                out << prefix << "|____value:\n";
                print_expr_tree(statement.get_variable().value, out, prefix + "|____|____");
                break;
            case shift_statement::statement_type::return_:
                out << "return_statement:\n";
                print_expr_tree(statement.get_return_statement(), out, prefix + "|____");
                break;
            case shift_statement::statement_type::expression:
                out << "expression:\n";
                print_expr_tree(statement.get_expression(), out, prefix + "|____");
                break;
            case shift_statement::statement_type::if_:
                out << "if:\n";
                out << prefix << "|____condition:\n";
                print_expr_tree(statement.get_if_condition(), out, prefix + "|____|____");
                out << prefix << "|____then:\n";
                for (const auto& sub_statement : statement.get_if_statements()) {
                    print_statement_tree(sub_statement, out, prefix + "|____|____");
                }
                if (statement.has_connected_else()) {
                    out << prefix << "|____else:\n";
                    for (const auto& sub_statement : statement.get_connected_else().get_else_statements()) {
                        print_statement_tree(sub_statement, out, prefix + "|____|____");
                    }
                }
                break;
            case shift_statement::statement_type::while_:
                out << "while:\n";
                out << prefix << "|____condition:\n";
                print_expr_tree(statement.get_while_condition(), out, prefix + "|____|____");
                out << prefix << "|____do:\n";
                for (const auto& sub_statement : statement.get_while_statements()) {
                    print_statement_tree(sub_statement, out, prefix + "|____|____");
                }
                break;
            case shift_statement::statement_type::for_:
                out << "for:\n";
                out << prefix << "|____init:\n";
                print_expr_tree(statement.get_expression(), out, prefix + "|____|____");
                out << prefix << "|____condition:\n";
                print_expr_tree(statement.get_for_condition(), out, prefix + "|____|____");
                out << prefix << "|____do:\n";
                for (const auto& sub_statement : statement.get_for_statements()) {
                    print_statement_tree(sub_statement, out, prefix + "|____|____");
                }
                break;
            case shift_statement::statement_type::continue_:
                out << "continue\n";
                break;
            case shift_statement::statement_type::break_:
                out << "break\n";
                break;
            case shift_statement::statement_type::scope_begin:
                out << "scope_begin\n";
                for (const auto& sub_statement : statement.get_block_statements()) {
                    print_statement_tree(sub_statement, out, prefix + "|____");
                }
                break;
            default:
                if (statement.data.token_[0])
                    out << statement.data.token_[0]->get_data();
                out << '\n';
                break;
        }
    }

#ifdef SHIFT_DEBUG

    SHIFT_API void parser::print_tree() {
        std::ostream& out = std::cout;

        out << "module: " << m_module->to_string() << "\n|\n";

        for (const auto& use : m_global_uses) {
            out << "use: " << use.to_string() << '\n';
        }


        for (const auto& clazz : m_classes) {
            out << "|\n";
            out << "class: " << clazz.get_fqn() << '\n';

            for (const auto& use : clazz.use_statements) {
                out << "|____use: " << use.to_string() << '\n';
            }

            for (const auto& var : clazz.variables) {
                out << "|____variable: " << var.get_fqn() << '\n';
                out << "|____|____value:\n";
                print_expr_tree(var.value, out, "|____|____|____");
            }

            for (const auto& func : clazz.functions) {
                out << "|____function: " << func.get_fqn() << '\n';
                for (const auto& statement : func.statements) {
                    print_statement_tree(statement, out, "|____|____");
                }
            }
        }

        for (const auto& func : this->m_functions) {
            out << "|____function: " << func.get_fqn() << '\n';
            for (const auto& statement : func.statements) {
                print_statement_tree(statement, out, "|____|____");
            }
        }

        for (const auto& var : m_variables) {
            out << "variable: " << var.get_fqn() << '\n';
            out << "|____value:\n";
            print_expr_tree(var.value, out, "|____|____");
        }

        out << '\n';
    }

#endif

    void parser::parse_body(parse_state& state, parser_class* parent_class) {
        for (; !state.position.is_eof(); ++state.position) {
            const token& current = *state.position;
            if (current.is_use()) {
                // use statement
                if (parent_class) {
                    parse_use(state, parent_class->use_statements);
                } else {
                    parse_use(state);
                }

                continue;
            }

            if (current.is_class()) {
                // creating class
                parse_class(state, parent_class);
                continue;
            }

            if (current.is_modifier()) {
                parse_modifier(state);
                continue;
            }

            if (parent_class && current.is_right_scope_bracket()) { break; }

            // Module statement parsing
            if (current.is_module()) {
                if (!parent_class) {
                    if (!this->is_module_defined()) {
                        // module statement; expected the least (only once)
                        parse_module(state);
                    } else {
                        this->token_error(current, "module already defined");
                        state.position.skip_until(token::type::SEMICOLON);
                    }
                } else {
                    this->token_error(current, "unexpected module declaration inside class");
                    state.position.skip_until(token::type::SEMICOLON);
                }
                continue;
            }

            // Constructor parsing
            if (current.is_constructor()) {
                if (!parent_class) {
                    this->token_error(current, "'constructor' may only be defined inside class context");
                }
                parser_type ret_type{};
                parse_function_header(state, parent_class, ret_type);
                continue;
            }

            // Destructor parsing
            if (current.is_destructor()) {
                if (!parent_class) {
                    this->token_error(current, "'destructor' may only be defined inside class context");
                }
                parser_type ret_type{};
                parse_function_header(state, parent_class, ret_type);
                continue;
            }

            if (current.is_identifier()) {
                // It must be either a variable declaration or a function declaration
                shift_type type{};

                if (current.is_void()) {
                    type.name.name.begin = this->m_lexer->get_index();
                    type.name.name.end = type.name.name.begin + 1;
                    this->m_lexer->next_token();
                } else {
                    if (std::optional<shift_type> parsed_type = m_parse_type("variable or function type")) {
                        if (parsed_type->name.name.empty()) {
                            if (parsed_type->name.name.begin != std::vector<compiler::token>::const_iterator{}) {
                                if (!parsed_type->name.name.begin->is_eof_token()) {
                                    this->token_error(*parsed_type->name.name.begin, "expected variable or function type");
                                } else {
                                    this->token_error(this->m_lexer->reverse_peek_token(),
                                        "expected variable or function type before end of file");
                                    return;
                                }
                            } else {
                                const auto& tok = this->m_lexer->current_token();
                                if (!tok.is_eof_token()) {
                                    this->token_error(tok, "expected variable or function type");
                                } else {
                                    this->token_error(this->m_lexer->reverse_peek_token(),
                                        "expected variable or function type before end of file");
                                    return;
                                }
                            }
                        }
                        type = std::move(*parsed_type);
                    }
                }

                for (const token* tok = &this->m_lexer->current_token(); tok->is_modifier(); tok = &this->m_lexer->next_token()) {
                    this->m_parse_access_specifier();
                }

                if (this->m_lexer->current_token().is_operator()
                    || this->m_lexer->peek_token().is_left_bracket()
                    || (!type.name.name.empty() && type.name.name.begin->is_void())) {
                    m_parse_function_header(parent_class, type);
                    continue;
                }
                {
                    if (this->m_lexer->current_token().is_eof_token()) {
                        if (!type.name.name.empty()) {
                            this->token_error(this->m_lexer->reverse_peek_token(),
                                "expected variable or function name after type declaration '" + type.get_printable_fqn() +
                                "' before end of file");
                            return;
                        } else {
                            this->token_error(this->m_lexer->reverse_peek_token(),
                                "expected variable or function name before end of file");
                            return;
                        }

                    }

                    const token& after_name = this->m_lexer->peek_token();
                    if (after_name.is_eof_token()) {
                        this->token_error(this->m_lexer->reverse_peek_token(),
                            "expected either ';' or '=' for variable declaration before end of file");
                        return;
                    }

                    if (after_name.is_binary_operator() || after_name.is_unary_operator() || after_name.is_equals() ||
                        after_name.is_semicolon()) {
                        if (auto v = m_parse_variable_header(parent_class, nullptr, type)) {
                            if (parent_class) {
                                parent_class->variables.push_back(std::move(*v));
                            } else {
                                this->m_variables.push_back(std::move(*v));
                            }
                        }
                        continue;
                    }

                    if (!this->m_lexer->current_token().is_eof_token()) {
                        this->token_error(this->m_lexer->current_token(), "expected variable or function declaration");
                        this->m_skip_until(token::type::SEMICOLON);
                    } else {
                        this->token_error(this->m_lexer->reverse_peek_token(),
                            "expected variable or function declaration before end of file");
                        return;
                    }
                }
                continue;
            }

            this->token_error(*current, "unexpected lexing::token '" + std::string(current->get_data()) + "'");
        }

        if (state.mods != shift_mods::NONE) {
            const token& mod_tok = state.mods.front();
            this->token_error(mod_tok, fmt::format("unexpected '{}' specifier", mod_tok.get_data()));
        }
    }

    void parser::parse_class(parse_state& state, parser_class* parent_class) {
        const token& class_token = *state.position;

        if (!class_token.is_class()) {
            this->token_error(class_token, "expected 'class'");
            return;
        }

        if (!this->is_module_defined()) {
            this->token_error(class_token, "module must be defined before creating class");
        }

        parser_class& clazz = m_classes.emplace_back();

        clazz.implicit_use_statements = this->m_global_uses.size();
        clazz.set_module(this->m_module.get());
        clazz.set_parent(parent_class);

        this->consume_modifiers(state);

        clazz.mods = (state.mods & class_modifiers);

        if (clazz.mods != state.mods) {
            auto diff = state.mods & ~clazz.mods;
            const token& bad_mod_tok = *state.mods.find_any(diff);
            this->token_error(bad_mod_tok,
                fmt::format("unexpected '{}' specifier in class declaration", bad_mod_tok.get_data()));
        }

        state.mods.clear();

        if ((clazz.mods & visibility_modifiers) == 0x0) { clazz.mods |= shift_mods::PUBLIC; }

        clazz.name = &*state.position;

        if (!clazz.name->is_identifier()) {
            if (!clazz.name->is_eof_token()) {
                this->token_error(*clazz.name, "expected identifier for class name");
                state.position.skip_before(token::type::LEFT_SCOPE_BRACKET);
            } else {
                this->token_error(state.position.reverse_peek_token(), "expected valid class name before end of file");
                return;
            }
        } else if (clazz.name->is_keyword()) {
            this->token_error(*clazz.name, fmt::format("'{}' is not a valid class name", clazz.name->get_data()));
            state.position.skip_before(token::type::LEFT_SCOPE_BRACKET);
        }

        if (state.position.peek_token().is_colon()) {
            const token& begin_name_token = state.position.next_token(2);
            if (!begin_name_token.is_identifier()) {
                if (begin_name_token.is_eof_token()) {
                    this->token_error(state.position.reverse_peek_token(),
                        fmt::format("expected class name for base class of '{}' before end of file", clazz.get_fqn()));
                } else {
                    this->token_error(begin_name_token,
                        fmt::format("expected class name for base class of '{}'", clazz.get_fqn()));
                    state.position.skip_before(token::type::LEFT_SCOPE_BRACKET);
                }
            } else {
                clazz.base_name = expect_name(state, "class name");
                state.position.reverse_token();
            }
        }

        const token& left_bracket = *++state.position;

        if (!left_bracket.is_left_scope_bracket()) {
            if (!left_bracket.is_eof_token()) {
                this->token_error(left_bracket, "expected '{' after class declaration");
            } else {
                this->token_error(state.position.reverse_peek_token(), "expected '{' after class declaration before end of file");
            }
        }

        ++state.position; // Move onto first lexing::token inside class body

        // Parse class body
        this->parse_body(state, &clazz);

        const token& right_bracket = *state.position;

        if (!right_bracket.is_right_scope_bracket()) {
            if (!right_bracket.is_eof_token()) {
                this->token_error(right_bracket, "expected '}' after class declaration");
            } else {
                this->token_error(state.position.reverse_peek_token(), "expected '}' after class declaration before end of file");
            }
        }
    }

    parser_function* parser::parse_function_header(parse_state& state, parser_class* parent_class, parser_type& return_type) {
        token_group name;

        this->consume_modifiers(state);

        auto name_begin = state.position.get_position();

        if (name_begin->is_operator()) {
            if (!parent_class) {
                this->token_error(*name_begin, "operator overload can only be define within class context");
            }

            const token& operator_overload_token = *++state.position;

            if (!operator_overload_token.is_overloadable_operator()) {
                if (operator_overload_token.is_left_square_bracket()) {
                    const token& right_square_bracket = *++state.position;
                    if (!right_square_bracket.is_right_square_bracket()) {
                        if (!right_square_bracket.is_eof_token()) {
                            this->token_error(right_square_bracket,
                                fmt::format("invalid operator overload '[{}'; expected ']' after '['", right_square_bracket.get_data()));
                        } else {
                            this->token_error(state.position.reverse_peek_token(),
                                "invalid operator overload '['; expected ']' after '[' before end of file");
                        }
                    }
                } else {
                    if (!operator_overload_token.is_eof_token()) {
                        this->token_error(operator_overload_token, "expected overload-able operator after keyword 'operator'");
                    } else {
                        this->token_error(state.position.reverse_peek_token(),
                            "expected overload-able operator after keyword 'operator' before end of file");
                    }
                }
            }
        } else if (name_begin->is_constructor() || name_begin->is_destructor()) {
            if (name_begin->is_constructor() && !return_type.name.empty()) {
                if (!return_type.name.front().is_eof_token()) {
                    this->token_error(return_type.name.front(), "constructor cannot have return type");
                } else {
                    this->token_error(*name_begin, "constructor cannot have return type");
                }
            } else if (name_begin->is_destructor() && !return_type.name.empty()) {
                if (!return_type.name.front().is_eof_token()) {
                    this->token_error(return_type.name.front(), "destructor cannot have return type");
                } else {
                    this->token_error(*name_begin, "destructor cannot have return type");
                }
            }
        } else if (name_begin->is_keyword()) {
            this->token_error(*name_begin, fmt::format("'{}' is not a valid variable or function name", name_begin->get_data()));
        } else if (!name_begin->is_identifier()) {
            if (!name_begin->is_eof_token()) {
                this->token_error(*name_begin,
                    fmt::format("expected identifier for variable or function name before '{}'", name_begin->get_data()));
            } else {
                this->token_error(state.position.reverse_peek_token(),
                    "expected identifier for variable or function name before end of file");
                return nullptr;
            }
        }

        const token& next_token = *++state.position;

        auto name_end = state.position.get_position();

        name.source = {name_begin, name_end};

        if (next_token.is_left_bracket()) {
            // function definition
            parser_function* func = parent_class ? &parent_class->functions.emplace_back() : &this->m_functions.emplace_back();

            func->name = name;
            func->implicit_use_statements = parent_class ? parent_class->use_statements.size() : this->m_global_uses.size();

            if (parent_class) {
                func->set_parent(parent_class);
            } else {
                if (!this->is_module_defined()) {
                    this->token_error(func->name.front(), "module must be defined before creating function");
                }
                func->set_parent(this->m_module.get());
            }

            func->return_type = std::move(return_type);

            auto func_allowed_mods = parent_class ? function_modifiers : global_function_modifiers;

            if (!func->name.empty()) {
                if (func->name.front().is_constructor()) {
                    func_allowed_mods = constructor_modifiers;
                } else if (func->name.front().is_destructor()) {
                    func_allowed_mods = destructor_modifiers;
                }
            }

            func->mods = (state.mods & func_allowed_mods);

            if (func->mods != state.mods) {
                auto diff = state.mods & ~func->mods;
                const token& bad_mod = *state.mods.find_any(diff);

            }

            for (const auto& [tok, mod] : state.mods.sorted()) {
                if ((mod & func_allowed_mods) != 0x0) {
                    if ((mod == shift_mods::STATIC) && func->name.front().is_operator()) {
                        this->token_error(*tok, "class operator overload cannot have 'static' specifier");
                    } else {
                        func->mods |= mod;
                    }
                } else {
                    if ((mod & type_modifiers) != 0x0) {
                        if (!func->return_type.name.empty() && func->return_type.name.front().is_void()) {
                            this->token_error(*tok,
                                fmt::format("'void' returning function cannot have '{}' specifier", tok->get_data()));
                        } else {
                            func->return_type.mods |= mod;
                        }
                    } else {
                        this->token_error(*tok,
                            fmt::format("unexpected '{}' specifier in function declaration", tok->get_data()));
                    }
                }
            }

            state.mods.clear();

            // parse function parameters
            ++state.position;
            if (!state.position->is_right_bracket()) {
                for (; !state.position.is_eof(); ++state.position) {
                    parser_variable param_var;
                    param_var.set_parent(func);
                    param_var.type = parse_type(state, "function parameter");
                    param_var.name = &*state.position;

                    if (param_var.type.name.empty()) {
                        if (param_var.type.name.source.begin() == state.position.end()) {
                            const token& tok = *state.position;
                            if (!tok.is_eof_token()) {
                                this->token_error(tok,
                                    fmt::format("expected parameter type in function parameter list, got '{}'", tok.get_data()));
                            } else {
                                this->token_error(state.position.reverse_peek_token(),
                                    "expected parameter type in function parameter list before end of file");
                            }
                        } else {
                            const token& tok = param_var.type.name.front();
                            if (!tok.is_eof_token()) {
                                this->token_error(tok,
                                    fmt::format("expected parameter type in function parameter list, got '{}'", tok.get_data()));
                            } else {
                                this->token_error(state.position.reverse_peek_token(),
                                    "expected parameter type in function parameter list before end of file");
                            }
                        }
                    }

                    if (param_var.name->is_comma() || param_var.name->is_right_bracket()) {
                        // nameless parameters
                        const token* const old_name = param_var.name;
                        param_var.name = &token::eof;

                        {
                            auto [it, ins] = func_null_params.emplace("@" + std::to_string(func->parameters.size()));
                            func->parameters.push_back({(std::string_view) *it, std::move(param_var)});
                        }

                        if (old_name->is_right_bracket())
                            break;

                        continue;
                    }

                    if (!param_var.name->is_identifier()) {
                        if (!param_var.name->is_eof_token()) {
                            this->token_error(*param_var.name, "expected identifier for function parameter name");
                        } else {
                            this->token_error(state.position.reverse_peek_token(),
                                "expected identifier for function parameter name before end of file");
                        }
                    } else if (param_var.name->is_keyword()) {
                        this->token_error(*param_var.name,
                            fmt::format("'{}' is not a valid function parameter name", param_var.name->get_data()));
                    } else if (func->parameters.contains(param_var.name->get_data())) {
                        this->token_error(*param_var.name,
                            fmt::format("duplicate function parameter name '{}'", param_var.name->get_data()));
                    } else {
                        func->parameters.push_back({param_var.name->get_data(), std::move(param_var)});
                    }

                    const token& after_param_name = *++state.position;
                    if (!after_param_name.is_comma()) {
                        if (!after_param_name.is_right_bracket()) {
                            if (!after_param_name.is_eof_token()) {
                                this->token_error(after_param_name, "expected ',' or ')' in function parameter list");
                            } else {
                                this->token_error(state.position.reverse_peek_token(),
                                    "expected ',' or ')' in function parameter list before end of file");
                            }
                        } else break;
                    }
                }
            }

            const token& function_right_parameter_bracket = *state.position;

            if (!function_right_parameter_bracket.is_right_bracket()) {
                if (!function_right_parameter_bracket.is_eof_token()) {
                    this->token_error(function_right_parameter_bracket, "expected ')' at end of function parameter list");
                } else {
                    this->token_error(state.position.reverse_peek_token(),
                        "expected ')' at end of function parameter list before end of file");
                }
            } else if (state.position.reverse_peek_token().is_comma()) {
                this->token_error(state.position.reverse_peek_token(), "misplaced ',' in function parameter list");
            }

            if (func->name.front().is_destructor()) {
                if (!func->parameters.empty()) {
                    this->token_error(func->name.front(), "destructor cannot have parameters");
                }
            }

            if (func->name.length() >= 2 && func->name.front().is_operator()) {
                const token& overload_token = *std::next(func->name.begin());
                if ((overload_token.is_strictly_prefix_operator() || overload_token.is_strictly_suffix_operator())
                    && !func->parameters.empty()) {
                    this->token_error(next_token,
                        fmt::format("unexpected parameter in function 'operator{}'", overload_token.get_data()));
                } else if ((overload_token.is_unary_operator() && !func->parameters.empty())
                           || ((overload_token.is_binary_operator() || overload_token.is_left_square_bracket()) &&
                               func->parameters.size() != 1)) {
                    this->token_error(next_token,
                        fmt::format("invalid number of parameters in function 'operator{}'", overload_token.get_data()));
                }
            }

            const token& function_left_bracket = *++state.position;

            if (func->mods & shift_mods::EXTERN) {
                if (function_left_bracket.is_semicolon()) return func;
                this->token_error(function_left_bracket, "external function cannot contain definition");
            }

            if (!function_left_bracket.is_left_scope_bracket()) {
                if (!function_left_bracket.is_eof_token()) {
                    this->token_error(function_left_bracket, "expected '{' after function declaration");
                } else {
                    this->token_error(state.position.reverse_peek_token(),
                        "expected '{' after function declaration before end of file");
                }
            }

            ++state.position; // Move onto first lexing::token inside function body

            // Parse function body
            parse_function(state, *func);

            const token& function_right_bracket = *state.position;

            if (!function_right_bracket.is_right_scope_bracket()) {
                if (!function_right_bracket.is_eof_token()) {
                    this->token_error(function_right_bracket, "expected '}' after function declaration");
                } else {
                    this->token_error(state.position.reverse_peek_token(),
                        "expected '}' after function declaration before end of file");
                }
            }
            return func;
        } else if (!next_token.is_eof_token()) {
            this->token_error(next_token, "expected '(' for function parameter declaration");
        } else {
            this->token_error(state.position.reverse_peek_token(),
                "expected '(' for function parameter declaration before end of file");
        }
        return nullptr;
    }

    std::optional<shift_variable>
    parser::parse_variable_header(parse_state& state, shift_class* parent_class, shift_function* parent_function, shift_type& type) {
        for (const token* tok = &this->m_lexer->current_token(); tok->is_modifier(); tok = &this->m_lexer->next_token()) {
            this->m_parse_access_specifier();
        }

        if (type.name.name.size() == 0) {
            if (type.name.name.begin != std::vector<compiler::token>::const_iterator{}) {
                if (!type.name.name.begin->is_eof_token()) {
                    this->token_error(*type.name.name.begin, "expected variable type");
                } else {
                    this->token_error(this->m_lexer->reverse_peek_token(), "expected variable type before end of file");
                }
            } else {
                const auto& tok = this->m_lexer->current_token();
                if (!tok.is_eof_token()) {
                    this->token_error(tok, "expected variable type");
                } else {
                    this->token_error(this->m_lexer->reverse_peek_token(), "expected variable type before end of file");
                }
            }
        }

        const token* name = &this->m_lexer->current_token();

        if (name->is_eof_token()) {
            this->token_error(this->m_lexer->reverse_peek_token(), "expected variable name before end of file");
            return std::nullopt;
        }

        if (name->is_keyword()) {
            this->token_error(*name, "'" + std::string(name->get_data()) + "' is not a valid variable name");
        } else if (!name->is_identifier()) {
            if (!name->is_eof_token()) {
                this->token_error(*name, "expected identifier for variable name, got '" + std::string(name->get_data()) + "'");
            } else {
                this->token_error(this->m_lexer->reverse_peek_token(), "expected identifier for variable name before end of file");
            }
        }

        if (type.name.name.size() != 0 && type.name.name.begin->is_void()) {
            this->token_error(*type.name.name.begin, "'void' cannot be used as a variable type");
        }

        shift_variable v;
        shift_variable* variable = &v;
        // variable declaration

        variable->name = name;
        variable->type = std::move(type);
        variable->implicit_use_statements = parent_class ? parent_class->use_statements.size() : this->m_global_uses.size();
        variable->function = parent_function;
        variable->clazz = parent_class;

        if (!this->m_is_module_defined()) {
            this->token_error(*variable->name, "module must be defined before creating variable");
        }

        variable->module_ = this->m_module.get();

        shift_mods var_mods;

        if (parent_function) {
            var_mods = variable_modifiers;
        } else if (parent_class) {
            var_mods = field_modifiers;
        } else {
            var_mods = global_variable_modifiers;
        }

        for (const auto& [mod, token_] : this->m_mods) {
            if ((mod & var_mods) == 0x0) {
                this->token_error(*token_, "invalid '" + std::string(token_->get_data()) + "' specifier on variable");
            } else {
                variable->shift_mods |= mod;
            }
        }
        this->m_clear_mods();

        if ((variable->type.shift_mods & visibility_modifiers) == 0x0) {
            variable->type.shift_mods |= shift_mods::PRIVATE;
        }

        const token& after_name = this->m_lexer->next_token();

        if (after_name.is_binary_operator() || after_name.is_unary_operator()) {
            if (!after_name.is_equals()) {
                this->token_error(after_name,
                    "expected ';' or '=' for variable declaration, got '" + std::string(after_name.get_data()) + "'");
            }

            // variable definition
            const token& first_expr_token = this->m_lexer->next_token(); // Move onto the expression
            variable->value = m_parse_expression();

            if (variable->value.type == token::type::NULL_TOKEN) {
                this->token_error(first_expr_token, "expected valid expression for variable assignment value");
            }
        } else if (after_name.is_semicolon()) {
            // do nothing
        } else {
            if (!after_name.is_eof_token()) {
                this->token_error(after_name, "expected either ';' or '=' for variable declaration");
            } else {
                this->token_error(this->m_lexer->reverse_peek_token(),
                    "expected either ';' or '=' for variable declaration before end of file");
            }
        }
        return v;
    }

    void parser::parse_function(parse_state& state, parser_function& func) {
        return parse_function_block(state, func, func.statements);
    }

    void parser::parse_function_block(parse_state& state, parser_function& func, std::deque<statement_types>& statements, size_t count) {
        for (const token* _token = &*state.position; count != 0 && !_token->is_eof_token(); _token = &*++state.position, count--) {
            // This function relies on parent statements being linked in a chain; addresses of statements must not change
            // shift_statement& statement = statements.emplace_back();

            this->consume_modifiers(state);

            _token = &*state.position;

            if (_token->is_use()) {
                auto const old_size = this->m_global_uses.size();
                parse_use(state, this->m_global_uses);
                if (old_size != this->m_global_uses.size()) {
                    parser_module const& module_ = this->m_global_uses.back();
                    statements.emplace_back(use_statement{.module_ = module_});
                    this->m_global_uses.pop_back();
                }
            } else if (_token->is_if()) {
                if (this->m_mods.size() != 0) {
                    const auto& [mod, token_] = this->m_mods.front();
                    this->token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                    this->m_clear_mods();
                }
                statement.set_if(_token);

                const token& left_condition_bracket = this->m_lexer->next_token();
                if (!left_condition_bracket.is_left_bracket()) {
                    if (!left_condition_bracket.is_eof_token()) {
                        this->token_error(left_condition_bracket, "expected '(' after 'if' inside function body");
                        this->m_skip_until(token::type::LEFT_BRACKET);
                    } else {
                        this->token_error(this->m_lexer->reverse_peek_token(),
                            "expected '(' after 'if' inside function body before end of file");
                    }
                }

                const token& first_condition_token = this->m_lexer->next_token(); // Move onto condition token

                statement.set_if_condition(m_parse_expression(token::type::RIGHT_BRACKET));

                const token& right_condition_bracket = this->m_lexer->current_token();

                if (first_condition_token == right_condition_bracket) {
                    this->token_error(left_condition_bracket, "expected valid expression inside 'if' statement condition");
                }

                if (!right_condition_bracket.is_right_bracket()) {
                    this->token_error(this->m_lexer->reverse_peek_token(),
                        "expected ')' after 'if' condition inside function body before end of file");
                    break;
                }

                const token& left_if_bracket = this->m_lexer->next_token();
                if (left_if_bracket.is_left_scope_bracket()) {
                    this->m_lexer->next_token(); // Skip {
                    m_parse_function_block(func, statement.get_if_statements());

                    const token& right_if_bracket = this->m_lexer->current_token();
                    if (!right_if_bracket.is_right_scope_bracket()) {
                        if (!right_if_bracket.is_eof_token()) {
                            this->token_error(right_if_bracket, "expected '}' to close 'if' declaration inside function body");
                            this->m_skip_until(token::type::RIGHT_SCOPE_BRACKET);
                        } else {
                            this->token_error(this->m_lexer->reverse_peek_token(),
                                "expected '}' to close 'if' declaration inside function body before end of file");
                        }
                    }
                } else {
                    m_parse_function_block(func, statement.get_if_statements(), 1);
                    if (statement.get_if_statements().size() == 0) {
                        this->token_error(this->m_lexer->current_token(), "expected valid statement after 'if' declaration");
                    }
                    this->m_lexer->reverse_token(); // lexing will be on lexing::token after last statement lexing::token if count causes it to end
                }

                for (auto& st : statement.get_if_statements()) {
                    st.parent = &statement;
                }

                const token& else_token = this->m_lexer->peek_token();
                if (else_token.is_else()) {
                    shift_statement else_statement;
                    else_statement.parent = &statement;
                    else_statement.set_else(&else_token);

                    const token& left_else_bracket = this->m_lexer->next_token(2);
                    if (left_else_bracket.is_left_scope_bracket()) {
                        this->m_lexer->next_token(); // Skip {
                        m_parse_function_block(func, else_statement.get_else_statements());

                        const token& right_else_bracket = this->m_lexer->current_token();
                        if (!right_else_bracket.is_right_scope_bracket()) {
                            if (!right_else_bracket.is_eof_token()) {
                                this->token_error(right_else_bracket, "expected '}' to close 'else' declaration inside function body");
                                this->m_skip_until(token::type::RIGHT_SCOPE_BRACKET);
                            } else {
                                this->token_error(this->m_lexer->reverse_peek_token(),
                                    "expected '}' to close 'else' declaration inside function body before end of file");
                            }
                        }
                    } else {
                        m_parse_function_block(func, else_statement.get_else_statements(), 1);
                        if (else_statement.get_else_statements().size() == 0) {
                            this->token_error(this->m_lexer->current_token(), "expected valid statement after 'else' declaration");
                        }
                        this->m_lexer->reverse_token(); // lexing will be on lexing::token after last statement lexing::token if count causes it to end
                    }

                    statement.connect_else(std::move(else_statement));

                    for (auto& st : statement.get_connected_else().get_else_statements()) {
                        st.parent = &statement.get_connected_else();
                    }
                    continue;
                }
            } else if (_token->is_else()) {
                this->token_error(*_token, "unexpected 'else' statement in function body");
            } else if (_token->is_while()) {
                if (this->m_mods.size() != 0) {
                    const auto& [mod, token_] = this->m_mods.front();
                    this->token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                    this->m_clear_mods();
                }
                statement.set_while(_token);

                const token& left_condition_bracket = this->m_lexer->next_token();
                if (!left_condition_bracket.is_left_bracket()) {
                    if (!left_condition_bracket.is_eof_token()) {
                        this->token_error(left_condition_bracket, "expected '(' after 'while' inside function body");
                        this->m_skip_until(token::type::LEFT_BRACKET);
                    } else {
                        this->token_error(this->m_lexer->reverse_peek_token(),
                            "expected '(' after 'while' inside function body before end of file");
                    }
                }

                const token& first_condition_token = this->m_lexer->next_token(); // Move onto condition token
                statement.set_while_condition(m_parse_expression(token::type::RIGHT_BRACKET));

                const token& right_condition_bracket = this->m_lexer->current_token();

                if (first_condition_token == right_condition_bracket) {
                    this->token_error(left_condition_bracket, "expected valid expression inside 'while' statement condition");
                }

                if (!right_condition_bracket.is_right_bracket()) {
                    this->token_error(this->m_lexer->reverse_peek_token(),
                        "expected ')' after 'while' condition inside function body before end of file");
                    break;
                }

                const token& left_while_bracket = this->m_lexer->next_token();
                if (left_while_bracket.is_left_scope_bracket()) {
                    this->m_lexer->next_token(); // Skip {
                    m_parse_function_block(func, statement.get_while_statements());

                    const token& right_while_bracket = this->m_lexer->current_token();
                    if (!right_while_bracket.is_right_scope_bracket()) {
                        if (!right_while_bracket.is_eof_token()) {
                            this->token_error(right_while_bracket, "expected '}' to close 'while' declaration inside function body");
                            this->m_skip_until(token::type::RIGHT_SCOPE_BRACKET);
                        } else {
                            this->token_error(this->m_lexer->reverse_peek_token(),
                                "expected '}' to close 'while' declaration inside function body before end of file");
                        }
                    }
                } else {
                    m_parse_function_block(func, statement.get_while_statements(), 1);
                    if (statement.get_while_statements().size() == 0) {
                        this->token_error(this->m_lexer->current_token(), "expected valid statement after 'while' declaration");
                    }
                    this->m_lexer->reverse_token(); // lexing will be on lexing::token after last statement lexing::token if count causes it to end
                }

                for (auto& st : statement.get_while_statements()) {
                    st.parent = &statement;
                }
            } else if (_token->is_for()) {
                if (this->m_mods.size() != 0) {
                    const auto& [mod, token_] = this->m_mods.front();
                    this->token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                    this->m_clear_mods();
                }
                statement.set_for(_token);

                const token& left_condition_bracket = this->m_lexer->next_token();
                if (!left_condition_bracket.is_left_bracket()) {
                    if (!left_condition_bracket.is_eof_token()) {
                        this->token_error(left_condition_bracket, "expected '(' after 'for' inside function body");
                        this->m_skip_until(token::type::LEFT_BRACKET);
                    } else {
                        this->token_error(this->m_lexer->reverse_peek_token(),
                            "expected '(' after 'for' inside function body before end of file");
                    }
                }

                {
                    const token& new_left_conditions_bracket = this->m_lexer->current_token();
                    this->m_lexer->next_token(); // Move onto statement token

                    utils::ideque<shift_statement> temp_statement;
                    m_parse_function_block(func, temp_statement, 1);
                    if (temp_statement.size() > 0) {
                        shift_statement& init_statement = temp_statement.front();

                        if (init_statement.type != shift_statement::statement_type::expression &&
                            init_statement.type != shift_statement::statement_type::variable_alloc) {
                            // TODO make more clear what data[0].token_ is representing
                            if (init_statement.data.token_[0]) {
                                this->token_error(*init_statement.data.token_[0], "invalid statement inside 'for' initializer");
                            } else {
                                this->token_error(new_left_conditions_bracket, "invalid statement inside 'for' initializer");
                            }

                        }

                        statement.set_for_initializer(std::move(init_statement));
                    }
                }

                {
                    // lexing will be on lexing::token after last statement lexing::token if count causes it to end; no need for next_token
                    statement.set_for_condition(m_parse_expression());
                }

                {
                    this->m_lexer->next_token(); // Move onto condition token

                    statement.set_for_increment(m_parse_expression(token::type::RIGHT_BRACKET));

                    const token& right_condition_bracket = this->m_lexer->current_token();

                    if (!right_condition_bracket.is_right_bracket()) {
                        this->token_error(this->m_lexer->reverse_peek_token(),
                            "expected ')' after 'for' increment inside function body before end of file");
                        break;
                    }
                }

                const token& left_for_bracket = this->m_lexer->next_token();
                if (left_for_bracket.is_left_scope_bracket()) {
                    this->m_lexer->next_token(); // Skip {
                    m_parse_function_block(func, statement.get_for_statements());

                    const token& right_for_bracket = this->m_lexer->current_token();
                    if (!right_for_bracket.is_right_scope_bracket()) {
                        if (!right_for_bracket.is_eof_token()) {
                            this->token_error(right_for_bracket, "expected '}' to close 'for' declaration inside function body");
                            this->m_skip_until(token::type::RIGHT_SCOPE_BRACKET);
                        } else {
                            this->token_error(this->m_lexer->reverse_peek_token(),
                                "expected '}' to close 'for' declaration inside function body before end of file");
                        }
                    }
                } else {
                    m_parse_function_block(func, statement.get_for_statements(), 1);
                    if (statement.get_for_statements().size() == 0) {
                        this->token_error(this->m_lexer->current_token(), "expected valid statement after 'for' declaration");
                    }
                    this->m_lexer->reverse_token(); // lexing will be on lexing::token after last statement lexing::token if count causes it to end
                }

                for (auto& st : statement.get_for_statements()) {
                    st.parent = &statement;
                }
            } else if (_token->is_return()) {
                if (this->m_mods.size() != 0) {
                    const auto& [mod, token_] = this->m_mods.front();
                    this->token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                    this->m_clear_mods();
                }
                statement.set_return(_token);
                this->m_lexer->next_token(); // skip 'return'
                statement.set_return_statement(m_parse_expression());
            } else if (_token->is_continue() || _token->is_break()) {
                if (this->m_mods.size() != 0) {
                    const auto& [mod, token_] = this->m_mods.front();
                    this->token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                    this->m_clear_mods();
                }
                if (_token->is_continue())
                    statement.set_continue(_token);
                else
                    statement.set_break(_token);

                const token& semi_colon = this->m_lexer->next_token();

                if (!semi_colon.is_semicolon()) {
                    if (!semi_colon.is_eof_token()) {
                        this->token_error(semi_colon, "expected ';' after '" + std::string(_token->get_data()) + "' in function body");
                        this->m_skip_until(token::type::SEMICOLON);
                    } else {
                        this->token_error(this->m_lexer->reverse_peek_token(),
                            "expected ';' after '" + std::string(_token->get_data()) +
                            "' in function body before end of file");
                    }
                }
            } else if (_token->is_identifier() && !_token->is_new()) {
                // must be either variable creation or normal expression
                this->m_lexer->mark();
                if (this->m_error_handler)
                    this->m_error_handler->mark();
                auto type = m_parse_type("type declaration");

                const token& after_type = this->m_lexer->current_token();

                if (type && type->name.name.size() > 0 && after_type.is_identifier()) {
                    // variable creation
                    this->m_lexer->pop_mark();
                    if (this->m_error_handler)
                        this->m_error_handler->pop_mark();

                    statement.set_variable();

                    if (auto variable = m_parse_variable_header(func.clazz, &func, *type)) {
                        statement.set_variable(std::move(*variable));
                    }
                } else {
                    // normal expression
                    statement.set_expression();

                    this->m_lexer->rollback();
                    if (this->m_error_handler)
                        this->m_error_handler->rollback();

                    statement.set_expression(this->m_parse_expression());
                }
            } else if (_token->is_semicolon()) {
                statements.pop_back();
                continue;
            } else if (_token->is_left_scope_bracket()) {
                if (this->m_mods.size() != 0) {
                    const auto& [mod, token_] = this->m_mods.front();
                    this->token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                    this->m_clear_mods();
                }
                statement.set_block(_token);
                utils::ideque<shift_statement> _statements;
                this->m_lexer->next_token();
                m_parse_function_block(func, _statements);
                statement.set_block(std::move(_statements));

                const token& right_block_token = this->m_lexer->current_token();
                statement.set_block_end(&right_block_token);

                if (!right_block_token.is_right_scope_bracket()) {
                    this->token_error(this->m_lexer->reverse_peek_token(), "expected '}' in function before end of file");
                }
            } else if (_token->is_right_scope_bracket()) {
                statements.pop_back();
                break;
            } else {
                statement.set_expression();
                statement.set_expression(this->m_parse_expression());
            }
        }


        if (this->m_mods.size() > 0) {
            const auto& [mod, token_] = this->m_mods.front();
            this->token_error(*token_, "unexpected '" + std::string(token_->get_data()) + "' specifier function body");
            this->m_clear_mods();
        }
    }

    void parser::parse_use(parse_state& state) {
        return parse_use(state, this->m_global_uses);
    }

    void parser::parse_use(parse_state& state, utils::ordered_set<parser_module>& modules) {
        if (state.mods != shift_mods::NONE) {
            this->token_error(state.mods.front(), "unexpected modifier in 'use' declaration");
            state.mods.clear();
        }

        const token& use_token = *state.position;

        if (!use_token.is_use()) {
            this->token_error(use_token, "expected 'use'");
            state.position.skip_until(token::type::SEMICOLON);
            return;
        }

        ++state.position; // skip 'use' keyword
        {
            auto module_ = parser_module{expect_name(state, "module name")};

            const token& end_token = *state.position; // lexing::token after the module name
            if (end_token.is_eof_token()) {
                this->token_error(state.position.reverse_peek_token(), "expected ';' before end of file");
            } else if (!end_token.is_semicolon()) {
                this->token_error(end_token, fmt::format("unexpected '{}' in module name", end_token.get_data()));
                state.position.skip_until(token::type::SEMICOLON);
            } else if (modules.contains(module_)) {
                this->token_warning(use_token, "redundant 'use' statement");
            } else {
                modules.push_back(std::move(module_));
            }
        }
    }

    void parser::parse_module(parse_state& state) {
        if (state.mods != shift_mods::NONE) {
            this->token_error(state.mods.front(), "unexpected modifier in 'module' declaration");
            state.mods.clear();
        }

        const token& module_token = *state.position;

        if (!module_token.is_module()) {
            this->token_error(module_token, "expected 'module'");
            state.position.skip_until(token::type::SEMICOLON);
            return;
        }

        ++state.position; // skip 'module' keyword

        auto module_ = expect_name(state, "module name");

        const token& end_token = *state.position; // lexing::token after the module name

        if (end_token.is_eof_token()) {
            this->token_error(state.position.reverse_peek_token(), "expected ';' before end of file");
        } else if (!end_token.is_semicolon()) {
            this->token_error(end_token, fmt::format("unexpected '{}' in module name", end_token.get_data()));
            state.position.skip_until(token::type::SEMICOLON);
        } else {
            if (!this->m_module) { this->m_module = std::make_unique<parser_module>(std::move(module_)); }
            else { this->m_module->name = std::move(module_); }
        }
    }

    /// @brief Parses a group of dot-separated identifiers, collectively referred to as a name.
    /// @param name_type The type of name to be parsed. Will be displayed in error messages.
    ///                   e.g. "module name", "variable or function type"
    token_group parser::parse_name(parse_state& state, std::string_view name_type) {
        token_group name;
        auto name_begin = state.position.get_position();

        lexing::token::type last_type{token::type::NULL_TOKEN};

        for (const lexing::token* tok = &*state.position; !tok->is_eof_token(); tok = &state.position.next_token()) {
            if (tok->is_modifier()) {
                this->token_error(*tok, fmt::format("unexpected '{}' specifier in {}", tok->get_data(), name_type));
            } else if (tok->is_keyword()) {
                // error, no keywords in (module) names
                this->token_error(*tok, fmt::format("invalid '{}' inside {}", tok->get_data(), name_type));
                last_type = token::type::IDENTIFIER;
            } else if (tok->is_identifier()) {
                if (last_type == token::type::IDENTIFIER) {
                    // cannot have two identifiers in a row in a (e.g. module) name
                    // We must be at the end of a name
                    break;
                }

                last_type = token::type::IDENTIFIER;
            } else if (tok->get_token_type() == token::type::DOT) {
                if (last_type != token::type::IDENTIFIER) {
                    // cannot have two dots in a row in a (module) name
                    last_type = token::type::DOT;
                    ++state.position; // This allows the error to be caught below
                    break;
                }

                last_type = token::type::DOT;
            } else break;
        }
        auto name_end = state.position.get_position();

        if (last_type == token::type::DOT) {
            this->token_error(state.position.reverse_peek_token(), fmt::format("unexpected '.' inside {}", name_type));
            name.panic = true;
        }

        name.source = utils::range{name_begin, name_end};
        return name;
    }

    /// @brief Parses a group of dot-separated identifiers, collectively referred to as a name, while also ensuring it's non-empty.
    /// @param name_type The type of name to be parsed. Will be displayed in error messages.
    ///                   e.g. "module name", "variable or function type"
    token_group parser::expect_name(parse_state& state, std::string_view name_type) {
        token_group name = parse_name(state, name_type);

        if (name.empty()) {
            const auto& tok = state.position.current_token();
            if (!tok.is_eof_token()) {
                this->token_error(tok, fmt::format("expected {} before '{}'", name_type, tok.get_data()));
            } else {
                this->token_error(state.position.reverse_peek_token(), fmt::format("expected {} before end of file", name_type));
            }
            name.panic = true;
        }

        return name;
    }

    parser_type parser::parse_type(parse_state& state, std::string_view name_type) {
        parser_type type;

        this->consume_modifiers(state);

        {
            const token& ref_token = *state.position;

            if (ref_token.is_ref()) {
                type.ref_type = shift_type::reference_type::ref;
                ++state.position;
            }
        }

        this->consume_modifiers(state);

        token::type last_type = token::type::NULL_TOKEN;
        {
            token_group name;
            auto name_begin = state.position.get_position();
            std::optional<token_group::iterator> name_end;

            for (const token* tok = &*state.position; !tok->is_eof_token(); tok = &*++state.position) {
                if (tok->is_modifier()) {
                    if (!name_end.has_value()) {
                        name_end = state.position.get_position();
                    }

                    auto const mod = this->parse_modifier(state);

                    if ((mod & type_modifiers) == 0x0) {
                        this->token_error(*tok,
                            fmt::format("unexpected '{}' specifier in {}", tok->get_data(), name_type));
                        type.panic = true;
                    }
                    last_type = token::type::IDENTIFIER;
                } else if (name_end.has_value()) {
                    break;
                } else if (tok->is_keyword()) {
                    // error, no keywords in (module) names
                    name_end = state.position.get_position();
                    break;
                } else if (tok->is_identifier()) {
                    if (last_type == token::type::IDENTIFIER) {
                        // cannot have two identifiers in a row in a (module) name
                        last_type = token::type::IDENTIFIER;
                        break;
                    }

                    last_type = token::type::IDENTIFIER;
                } else if (tok->get_token_type() == token::type::DOT) {
                    if (last_type != token::type::IDENTIFIER) {
                        // cannot have two dots in a row in a (module) name
                        last_type = token::type::DOT;
                        ++state.position; // This allows the error to be caught below
                        break;
                    }

                    last_type = token::type::DOT;
                } else break;
            }
            if (!name_end.has_value()) { name_end = state.position.get_position(); }

            for (const auto& [tok, mod] : state.mods.sorted()) {
                if ((mod & type_modifiers) != 0x0) {
                    type.mods |= mod;
                    state.mods.remove(mod);
                }
            }

            if (last_type == token::type::DOT) {
                this->token_error(state.position.reverse_peek_token(), fmt::format("unexpected '.' inside {}", name_type));
                type.panic = true;
            }

            type.name.source = {name_begin, *name_end};
        }

        size_t current_dim_count = 0;
        for (const token* tok = &*state.position; !tok->is_eof_token(); tok = &*++state.position) {
            if (tok->is_modifier()) {
                this->token_error(*tok, fmt::format("unexpected '{}' specifier in {} type", tok->get_data(), name_type));
                type.panic = true;
            } else if (tok->is_star()) {
                if (current_dim_count > 0 && last_type == token::type::RIGHT_SQUARE_BRACKET) {
                    type.add_array_dimensions(current_dim_count);
                    current_dim_count = 0;
                }
                current_dim_count++;
                if (last_type == token::type::LEFT_SQUARE_BRACKET) {
                    this->token_error(*tok, fmt::format("unexpected '*' after '[' in {}", name_type));
                    type.panic = true;
                }
                last_type = token::type::STAR;
            } else if (tok->is_left_square_bracket()) {
                if (current_dim_count > 0 && last_type == token::type::STAR) {
                    type.add_pointer_dimensions(current_dim_count);
                    current_dim_count = 0;
                }
                if (last_type != token::type::IDENTIFIER && last_type != token::type::RIGHT_SQUARE_BRACKET &&
                    last_type != token::type::STAR) {
                    this->token_error(*tok, fmt::format("unexpected '[' in {}", name_type));
                    type.panic = true;
                } else current_dim_count++;

                last_type = token::type::LEFT_SQUARE_BRACKET;
            } else if (tok->is_right_square_bracket()) {
                if (last_type != token::type::LEFT_SQUARE_BRACKET) {
                    this->token_error(*tok, fmt::format("unexpected ']' in {}", name_type));
                    type.panic = true;
                }
                last_type = token::type::RIGHT_SQUARE_BRACKET;
            } else if (last_type == token::type::LEFT_SQUARE_BRACKET) {
                break;
            } else break;
        }

        if (current_dim_count > 0) {
            switch (last_type) {
                case token::type::LEFT_SQUARE_BRACKET:
                case token::type::RIGHT_SQUARE_BRACKET:
                    type.add_array_dimensions(current_dim_count);
                    break;
                case token::type::STAR:
                    type.add_pointer_dimensions(current_dim_count);
                    break;
                default:
                    break;
            }
        }

        if (last_type == token::type::LEFT_SQUARE_BRACKET) {
            const token& last = state.position.reverse_peek_token();
            this->token_error(last, "unexpected '[' inside " + std::string(name_type));
            type.panic = true;
        }

        return type;
    }

    shift_expression parser::parse_expression(parse_state& state, const utils::predicate<std::vector<token>::const_iterator>& end_func) {
        shift_expression ret_expr;
        shift_expression* expr = &ret_expr;
#ifdef SHIFT_DEBUG
        static size_t call_count = 0;

        if (call_count > 30) { /*__debugbreak();*/ }

        call_count++;
#endif
        expr->begin = this->m_lexer->get_index();
        for (const token* _token = &this->m_lexer->current_token();
             !_token->is_eof_token() && !end_func(this->m_lexer->get_index()); _token = &this->m_lexer->next_token()) {
            const bool is_null_expr = expr->type == token::type::NULL_TOKEN;
            const bool is_suffix_expr = expr->parent && is_suffix_operator(expr->parent->type)
                                        && expr->parent->has_left() && expr->parent->get_left()->type != token::type::NULL_TOKEN;

            if (_token->is_string_literal()) {
                if (is_null_expr) {
                    expr->type = token::type::STRING_LITERAL;
                    expr->begin = this->m_lexer->get_index();
                    expr->end = expr->begin + 1;

                    if (is_suffix_expr) {
                        this->token_error(*_token, "unexpected string literal following postfix operator inside expression");
                    }
                } else {
                    this->token_error(*_token, "unexpected string literal in expression");
                }

                continue;
            }

            if (_token->is_char_literal()) {
                if (is_null_expr) {
                    expr->type = token::type::CHAR_LITERAL;
                    expr->begin = this->m_lexer->get_index();
                    expr->end = expr->begin + 1;

                    if (is_suffix_expr) {
                        this->token_error(*_token, "unexpected character literal following postfix operator inside expression");
                    }
                } else {
                    this->token_error(*_token, "unexpected character literal in expression");
                }

                continue;
            }

            if (_token->is_number()) {
                if (is_null_expr) {
                    expr->type = _token->get_token_type();
                    expr->begin = this->m_lexer->get_index();
                    expr->end = expr->begin + 1;
                    if (is_suffix_expr) {
                        this->token_error(*_token, "unexpected number literal following postfix operator inside expression");
                    }
                } else {
                    switch (_token->get_token_type()) {
                        case token::type::INTEGER_LITERAL:
                        case token::type::BINARY_LITERAL:
                        case token::type::HEX_LITERAL:
                            this->token_error(*_token, "unexpected integer literal in expression");
                            break;
                        case token::type::FLOAT_LITERAL:
                        case token::type::DOUBLE_LITERAL:
                            this->token_error(*_token, "unexpected floating point literal in expression");
                            break;
                        default:
                            this->token_error(*_token, "unexpected number literal in expression");
                            break;
                    }
                }
                continue;
            }

            if (_token->is_true() || _token->is_false()) {
                if (is_null_expr) {
                    expr->type = token::type::IDENTIFIER;
                    expr->begin = this->m_lexer->get_index();
                    expr->end = expr->begin + 1;
                    if (is_suffix_expr) {
                        this->token_error(*_token, "unexpected boolean literal following postfix operator inside expression");
                    }
                } else {
                    this->token_error(*_token, "unexpected boolean literal in expression");
                }
                continue;
            }

            if (_token->is_null()) {
                if (is_null_expr) {
                    expr->type = token::type::IDENTIFIER;
                    expr->begin = this->m_lexer->get_index();
                    expr->end = expr->begin + 1;
                    if (is_suffix_expr) {
                        this->token_error(*_token, "unexpected 'null' following postfix operator inside expression");
                    }
                } else {
                    this->token_error(*_token, "unexpected 'null' in expression");
                }
                continue;
            }

            if (_token->is_cp() || _token->is_mv()) {
                if (is_null_expr) {
                    if (is_suffix_expr) {
                        this->token_error(*_token, "unexpected 'cp' or 'mv' following postfix operator inside expression");
                    }
                    expr->type = token::type::IDENTIFIER;
                    expr->begin = this->m_lexer->get_index();

                    this->m_lexer->next_token();
                    {
                        // TODO treat mv as an operator so we dont have to do this stuff
                        auto sub_expr = m_parse_expression([&end_func](const std::vector<token>::const_iterator it) {
                            return end_func(it) || it->is_overloadable_operator() || it->is_comma();
                        });
                        expr->end = this->m_lexer->get_index();
                        this->m_lexer->reverse_token(); // We want the next iteration to handle the operator/semicolon

                        if (_token->is_cp()) {
                            expr->set_cp_expression(std::move(sub_expr));
                        } else {
                            expr->set_mv_expression(std::move(sub_expr));
                        }
                    }
                } else {
                    this->token_error(*_token, "unexpected 'cp' or 'mv' in expression");
                }
                continue;
            }

            if (_token->is_left_bracket()) {
                // (hello.world); (hello.world)5;
                //  type=LEFT_BRACKET
                //  left=hello.world
                //  right=5
                if (!(is_null_expr) && !expr->is_bracket()) {
                    this->token_error(*_token, "unexpected '(' inside expression");
                } else if (is_suffix_expr && is_null_expr) {
                    this->token_error(*_token, "unexpected '(' following postfix operator inside expression");
                }

                expr->set_bracket();
                expr->begin = this->m_lexer->get_index();
                expr->end = expr->begin + 1;
                this->m_lexer->next_token(); // skip (
                expr->set_left(m_parse_expression(token::type::RIGHT_BRACKET));
                expr->set_right();
                {
                    const token& right_bracket = this->m_lexer->current_token();
                    if (!right_bracket.is_right_bracket()) {
                        this->token_error(this->m_lexer->reverse_peek_token(), "expected ')' inside expression before end of file");
                    }
                    expr->end = this->m_lexer->get_index();
                }

                expr = expr->get_right();
                continue;
            }

            if (_token->is_left_square_bracket()) {
                if (is_null_expr) {
                    if (expr->parent && expr->parent->is_bracket()) {
                        shift_expression old_array_obj = std::move(*expr->parent->get_left());
                        shift_expression new_array_obj;
                        new_array_obj.type = expr->parent->type;
                        new_array_obj.begin = old_array_obj.begin;
                        new_array_obj.end = old_array_obj.end;
                        new_array_obj.set_left(std::move(old_array_obj));

                        expr = expr->parent;
                        expr->clear_children();
                        expr->set_array();
                        expr->set_array_object(std::move(new_array_obj));
                    } else if (is_suffix_expr) {
                        shift_expression* const parent = expr->parent;
                        shift_expression array_expr;
                        array_expr.set_array();
                        array_expr.begin = parent->begin;
                        array_expr.parent = parent->parent;

                        parent->clear_right();
                        array_expr.set_array_object(std::move(*parent));

                        *parent = std::move(array_expr);
                        expr = parent;
                    } else {
                        this->token_error(*_token, "unexpected '[' inside expression");
                        this->m_skip_until_closing(token::type::LEFT_SQUARE_BRACKET);
                        continue;
                    }
                }
                if (!expr->is_array()) {
                    shift_expression array_expr;
                    array_expr.set_array();
                    array_expr.begin = expr->begin;
                    array_expr.parent = expr->parent;

                    array_expr.set_array_object(std::move(*expr));
                    *expr = std::move(array_expr);
                }

                if (expr->is_array()) {
                    this->m_lexer->next_token();
                    expr->add_array_dimension(m_parse_expression(token::type::RIGHT_SQUARE_BRACKET));
                    expr->end = this->m_lexer->get_index() + 1;
                }
                continue;
            }

            if (_token->is_comma()) {
                if (is_null_expr) {
                    if (is_suffix_expr) {
                        expr->parent->clear_right();
                    } else {
                        this->token_error(*_token, "unexpected ',' inside expression");
                    }
                }

                if (!ret_expr.is_comma()) {
                    shift_expression temp = std::move(ret_expr);

                    ret_expr = shift_expression();
                    ret_expr.type = token::type::COMMA;
                    ret_expr.begin = this->m_lexer->get_index();
                    ret_expr.end = ret_expr.begin + 1;

                    ret_expr.add_comma_expression(std::move(temp));
                    ret_expr.get_comma_expressions().back().parent = nullptr;
                }

                ret_expr.add_comma_expression(shift_expression());
                expr = &ret_expr.get_comma_expressions().back();
                expr->parent = nullptr;

                continue;
            }

            if (_token->is_binary_operator() || _token->is_unary_operator()) {
                if (_token->is_binary_operator()) {
                    if (expr->type == token::type::NULL_TOKEN && !_token->is_prefix_operator() &&
                        (!expr->parent || !expr->parent->is_bracket()) && !is_suffix_expr) {
                        this->token_error(*_token,
                            "unexpected binary operator '" + std::string(_token->get_data()) + "' inside expression");
                    }
                } else {
                    if (expr->type == token::type::NULL_TOKEN) {
                        if (_token->is_strictly_suffix_operator()) {
                            if (!is_suffix_expr) {
                                this->token_error(*_token, "unexpected postfix operator '" + std::string(_token->get_data()) +
                                                           "' inside expression");
                            }
                        } else if (_token->is_strictly_prefix_operator() && is_suffix_expr) {
                            this->token_error(*_token, "unexpected prefix operator '" + std::string(_token->get_data()) +
                                                       "' following postfix operator inside expression");
                        }
                    } else {
                        if (_token->is_strictly_prefix_operator()) {
                            this->token_error(*_token,
                                "unexpected prefix operator '" + std::string(_token->get_data()) + "' inside expression");
                        }
                    }
                }
                /// var v = mv help + 3;
                shift_expression new_expr;
                new_expr.type = _token->get_token_type();
                new_expr.begin = this->m_lexer->get_index();
                new_expr.end = new_expr.begin + 1;
                new_expr.set_right();

                const uint_fast8_t priority = operator_priority(new_expr.type,
                    (_token->is_strictly_prefix_operator() &&
                     !_token->is_binary_operator())
                    || (_token->is_prefix_operator() &&
                        expr->type == token::type::NULL_TOKEN
                        && !is_suffix_expr));

                shift_expression* const new_ret_expr = ret_expr.is_comma() ? &ret_expr.get_comma_expressions().back() : &ret_expr;

                shift_expression* current_parent = expr->parent;

                for (; current_parent; current_parent = current_parent->parent) {
                    const bool is_prefix =
                        (is_strictly_prefix_operator(current_parent->type) && !is_binary_operator(current_parent->type)) ||
                        (is_prefix_operator(current_parent->type) && current_parent->has_left() &&
                         current_parent->get_left()->type == token::type::NULL_TOKEN);
                    const bool l_to_r = !is_prefix;
                    const uint_fast8_t parent_priority = operator_priority(current_parent->type, is_prefix);

                    if (priority > parent_priority || (!l_to_r && (priority == parent_priority))) {
                        new_expr.set_left(std::move(*current_parent->get_right()));
                        current_parent->set_right(std::move(new_expr));
                        expr = current_parent->get_right()->get_right();
                        break;
                    } else if (l_to_r && (priority <= parent_priority) && is_suffix_operator(current_parent->type)
                               && current_parent->has_left() && current_parent->get_left()->type != token::type::NULL_TOKEN
                               && _token->is_suffix_operator()) {
                        shift_expression* const parent_parent = current_parent->parent;
                        current_parent->clear_right();
                        new_expr.set_left(std::move(*current_parent));
                        *current_parent = std::move(new_expr);
                        current_parent->update_parents(parent_parent);
                        expr = current_parent->get_right();
                        break;
                    }
                }

                if (!current_parent) {
                    new_expr.set_left(std::move(*new_ret_expr));
                    *new_ret_expr = std::move(new_expr);
                    new_ret_expr->update_parents(ret_expr.is_comma() ? &ret_expr : nullptr);
                    expr = new_ret_expr->get_right();
                }
                continue;
            }

            if (_token->is_identifier()) {
                if (!is_null_expr) {
                    // we should have already errored if it's we're not null and in a suffix, no need to repeat it
                    if (!is_suffix_expr) {
                        this->token_error(*_token, "unexpected identifier in expression");
                    }
                } else if (is_suffix_expr) {
                    this->token_error(*_token, "unexpected identifier following postfix operator inside expression");
                }

                expr->type = token::type::IDENTIFIER;
                expr->begin = this->m_lexer->get_index();

                if (_token->is_new()) {
                    // new clazz.name("Hello world", 3);
                    this->m_lexer->next_token(); // skip 'new'
                    expr->set_new_expression(m_parse_expression(end_func));
                    expr->end = this->m_lexer->get_index();

                    if (const shift_expression* new_expr = expr->get_new_expression()) {
                        const shift_expression* cur_expr = new_expr;
                        if (new_expr->is_dotted_expression()) {
                            const auto& exprs = new_expr->get_dotted_expressions();
                            for (size_t i = 0; const auto& sub_expr : exprs) {
                                if (i == exprs.size() - 1) { break; }
                                if (sub_expr.is_dotted_expression() ||
                                    (sub_expr.type != lexing::token::type::IDENTIFIER && !sub_expr.is_array())) {
                                    this->token_error(*_token, "unexpected 'new' inside expression");
                                    goto end_new_check;
                                } else if (sub_expr.is_function_call()) {
                                    if (sub_expr.get_function_call_object()->is_array()) {
                                        this->token_error(*_token, "unexpected 'new' inside expression");
                                        goto end_new_check;
                                    }
                                }
                                i++;
                                cur_expr = &sub_expr;
                            }
                        }
                        if (cur_expr->type != lexing::token::type::IDENTIFIER && !cur_expr->is_array()) {
                            this->token_error(*_token, "unexpected 'new' inside expression");
                            break;
                        } else if (cur_expr->is_function_call()) {
                            if (cur_expr->get_function_call_object()->is_array()) {
                                this->token_error(*_token, "unexpected 'new' inside expression");
                                break;
                            }
                        }
                      end_new_check:
                        this->m_lexer->reverse_token(); // Move back to the last lexing::token of the new expression
                    }
                    continue;
                }

                {
                    token::type last_type = token::type::NULL_TOKEN;
                    for (const token* expr_token = &this->m_lexer->current_token(); !expr_token->is_eof_token(); expr_token = &this->m_lexer->next_token()) {
                        if (expr_token->is_dot()) {
                            // We keep last_type as identifier when doing function calls and array indexing expressions (check below)
                            if (last_type != lexing::token::type::IDENTIFIER) {
                                this->token_error(*expr_token, "unexpected '.' inside expression");
                            }
                            last_type = lexing::token::type::DOT;
                            continue;
                        }

                        if ((expr_token->is_this() || expr_token->is_base())) {
                            if (last_type == token::type::NULL_TOKEN) {
                                shift_expression var_expr;
                                var_expr.type = lexing::token::type::IDENTIFIER;
                                var_expr.begin = this->m_lexer->get_index();
                                var_expr.end = var_expr.begin + 1;
                                expr->add_dotted_expression(std::move(var_expr));
                            } else {
                                this->token_error(*expr_token,
                                    "unexpected '" + std::string(expr_token->get_data()) + "' inside expression");
                            }
                            last_type = lexing::token::type::IDENTIFIER;
                            continue;
                        }

                        if (expr_token->is_identifier()) {
                            if (expr_token->is_keyword()) {
                                this->token_error(*expr_token,
                                    "unexpected keyword '" + std::string(expr_token->get_data()) + "' inside expression");
                            } else if (last_type != lexing::token::type::DOT && last_type != token::type::NULL_TOKEN) {
                                this->token_error(*expr_token, "unexpected identifier '" + std::string(expr_token->get_data()) +
                                                               "' inside expression");
                            }

                            shift_expression loop_expr;
                            loop_expr.type = lexing::token::type::IDENTIFIER;
                            loop_expr.begin = this->m_lexer->get_index();
                            loop_expr.end = loop_expr.begin + 1;

                            while (true) {
                                const token* next_expr_token = &this->m_lexer->peek_token();

                                if (next_expr_token->is_left_bracket() && loop_expr.type == lexing::token::type::IDENTIFIER) {
                                    // No function pointers yet
                                    // function call

                                    loop_expr.set_function_call_object(shift_expression(loop_expr));

                                    loop_expr.set_function_call();

                                    this->m_lexer->next_token(2);
                                    {
                                        auto function_call_args = m_parse_expression(token_type::RIGHT_BRACKET);

                                        // if (function_call_args.type != type::COMMA) {
                                        //     expression dummy_comma;
                                        //     dummy_comma.type = type::COMMA;
                                        //     dummy_comma.begin = function_call_args.begin;
                                        //     dummy_comma.end = function_call_args.end;
                                        //     dummy_comma.add_comma_expression(std::move(function_call_args));
                                        //     loop_expr.add_function_call_arguments(std::move(dummy_comma));
                                        // } else {
                                        //     loop_expr.add_function_call_arguments(std::move(function_call_args));
                                        // }

                                        if (function_call_args.type == lexing::token::type::COMMA) {
                                            for (auto& sub_expr : function_call_args.get_comma_expressions()) {
                                                if (sub_expr.type == lexing::token::type::COMMA) {
                                                    this->token_error(*sub_expr.begin,
                                                        "unexpected ',' inside function call arguments inside expression");
                                                }
                                                loop_expr.add_function_call_arguments(std::move(sub_expr));
                                            }
                                        } else if (function_call_args.type != lexing::token::type::NULL_TOKEN) {
                                            loop_expr.add_function_call_arguments(std::move(function_call_args));
                                        }
                                    }
                                    const token& right_function_call_bracket = this->m_lexer->current_token();
                                    if (!right_function_call_bracket.is_right_bracket()) {
                                        if (!right_function_call_bracket.is_eof_token()) {
                                            this->token_error(right_function_call_bracket, "expected ')' inside expression");
                                        } else {
                                            this->token_error(this->m_lexer->reverse_peek_token(),
                                                "expected ')' inside expression before end of file");
                                        }
                                    }


                                } else if (next_expr_token->is_left_square_bracket() &&
                                           (loop_expr.type == lexing::token::type::IDENTIFIER || loop_expr.is_function_call())) {
                                    // no need to check for function calls mixed between arrays calls, since function pointers are not yet a feature
                                    // TODO add function pointers
                                    // TODO allow "test"[0] syntax
                                    // TODO allow ("hello")[0] syntax

                                    // array

                                    this->m_lexer->next_token(); // Move onto the actual '[' lexing::token (we used peek before)

                                    shift_expression array_expr;
                                    array_expr.set_array();
                                    array_expr.begin = loop_expr.begin;

                                    do {
                                        this->m_lexer->next_token();
                                        array_expr.add_array_dimension(m_parse_expression(token_type::RIGHT_SQUARE_BRACKET));
                                        const auto& array_dims = array_expr.get_array_dimensions();
                                        const auto& parsed_indexer_expr = *array_dims.back().get_array_indexer_expression();
                                        if (parsed_indexer_expr.type == token::type::COMMA) {
                                            this->token_error(*loop_expr.begin, "unexpected ',' inside array indexer inside expression");
                                        }
                                        const token& right_array_bracket = this->m_lexer->current_token();
                                        if (!right_array_bracket.is_right_square_bracket()) {
                                            this->token_error(this->m_lexer->reverse_peek_token(),
                                                "expected ']' inside expression before end of file");
                                        } else {
                                            if (parsed_indexer_expr.type == lexing::token::type::NULL_TOKEN) {
                                                this->token_error(this->m_lexer->reverse_peek_token(),
                                                    "expected expression inside array indexer");
                                            }
                                        }
                                    } while (this->m_lexer->next_token().is_left_square_bracket());

                                    array_expr.end = this->m_lexer->get_index();
                                    array_expr.set_array_object(std::move(loop_expr));
                                    loop_expr = std::move(array_expr);

                                    this->m_lexer->reverse_token();
                                } else break;
                            }

                            last_type = lexing::token::type::IDENTIFIER;

                            expr->add_dotted_expression(std::move(loop_expr));
                            continue;
                        }

                        //this->token_error(*expr_token, "unexpected lexing::token '" + std::string(expr_token->get_data()) + "' inside expression");
                        this->m_lexer->reverse_token();
                        break;
                    }

                    if (last_type == lexing::token::type::DOT) {
                        if (!this->m_lexer->current_token().is_eof_token()) {
                            this->token_error(this->m_lexer->current_token(), "unexpected '.' inside expression");
                        } else {
                            this->token_error(this->m_lexer->reverse_peek_token(), "unexpected '.' inside expression");
                        }
                    }
                    // expr->type = expr->sub.back().type;

                    if (expr->is_dotted_expression() && expr->get_dotted_expressions().size() == 1) {
                        auto* old_parent = expr->parent;
                        shift_expression e = std::move(expr->get_dotted_expressions().front());
                        expr->get_dotted_expressions().clear();
                        *expr = std::move(e);
                        expr->update_parents(old_parent);
                    } else {
                        expr->end = this->m_lexer->get_index() + size_t(!this->m_lexer->current_token().is_eof_token());
                    }
                }
                continue;
            }

            this->token_error(*_token, "unexpected lexing::token '" + std::string(_token->get_data()) + "' in expression");
        }

        while (expr->parent) {
            if (is_unary_operator(expr->parent->type)) {
                if ((expr->parent->has_left() && expr->parent->get_left()->type != lexing::token::type::NULL_TOKEN)
                    && (expr->parent->has_right() && expr->parent->get_right()->type != lexing::token::type::NULL_TOKEN)
                    && !is_binary_operator(expr->parent->type)) {
                    // Error if we have both a left and a right meanwhile this is strictly unary
                    this->token_error(*expr->parent->begin, "unexpected unary operator '" + std::string(expr->parent->begin->get_data()) +
                                                            "' inside expression");
                    break;
                } else if ((!expr->parent->has_left() || expr->parent->get_left()->type == lexing::token::type::NULL_TOKEN)
                           && (!expr->parent->has_right() || expr->parent->get_right()->type == lexing::token::type::NULL_TOKEN)) {
                    // Error if we dont have a left nor a right when this is meant to be a unary operator
                    this->token_error(*expr->parent->begin, "unexpected unary operator '" + std::string(expr->parent->begin->get_data()) +
                                                            "' inside expression");
                    break;
                } else if (expr->parent->has_left() && expr->parent->get_left()->type != lexing::token::type::NULL_TOKEN &&
                           is_suffix_operator(expr->parent->type)) {
                    // If this is a suffix expr,
                    if (expr->parent->has_right()) {
                        expr = expr->parent;
                        expr->clear_right();
                    }

                    break;
                }
            }

            if (is_binary_operator(expr->parent->type)) {
                // Error if the binary expression ended with no right side when this is not a suffix operator
                if (expr->parent->get_right()->size() == 0 && expr->parent->get_left()->size() != 0) {
                    if (!is_suffix_operator(expr->parent->type)) {
                        this->token_error(*expr->parent->begin,
                            "unexpected binary operator '" + std::string(expr->parent->begin->get_data()) +
                            "' inside expression");
                    } else {
                        expr = expr->parent;
                        expr->parent->clear_right();
                    }
                    break;
                }
            }

            if (expr->parent->is_bracket()) {
                expr = expr->parent;
                if (expr->has_right() && expr->get_right()->type == lexing::token::type::NULL_TOKEN) {
                    expr->clear_right();
                }
                continue;
            }

            break;
        }
        if (this->m_lexer->reverse_peek_token().is_comma()) {
            // Error if we ended in a comma
            this->token_error(this->m_lexer->reverse_peek_token(), "misplaced ',' inside expression");
        }
        // TODO make sure begin and end are still set appropriately even when the expression is empty
        if (ret_expr.type == token::type::NULL_TOKEN) {
            ret_expr.begin = this->m_lexer->get_index();
            ret_expr.end = ret_expr.begin + 1;
        } else if (ret_expr.is_comma()) {
            for (auto& sub_expr : ret_expr.get_comma_expressions()) {
                sub_expr.parent = &ret_expr;
            }
        }

#ifdef SHIFT_DEBUG
        call_count--;
#endif

        return ret_expr;
    }

    shift_mods parser::parse_modifier(parse_state& state) {
        const token& tok = *state.position;
        const shift_mods mod = to_mod(tok.get_data());
        const shift_mods current_mods = state.mods;
        const shift_mods new_visibility_mods = (current_mods | mod) & visibility_modifiers;

        // Check to make sure only one visibility modifier is on at a time (power of 2)
        if ((new_visibility_mods & (new_visibility_mods - 1)) != 0x0) {
            // error if the one we have (i.e. the public, protected or private that is currently specified (the one being stored in mods)) is NOT the one now being parsed
            const token& old_vis = *state.mods.find(current_mods & visibility_modifiers);
            this->token_error(tok,
                fmt::format("unexpected visibility modifier '{}', '{}' already specified", tok.get_data(), old_vis.get_data()));
        } else if ((current_mods & mod) == mod) {
            // send a warning if we are just adding the same modifier twice
            this->token_warning(tok, fmt::format("redundant '{}' specifier", tok.get_data()));
        } else {
            switch (mod) {
                case shift_mods::IMUT:
                    if ((current_mods & shift_mods::CONST_) != 0x0) {
                        this->token_warning(tok, "redundant 'imut' specifier");
                    } else {
                        state.mods.unsafe_add(mod, tok);
                    }
                    break;
                case shift_mods::CONST_:
                    if ((current_mods & shift_mods::IMUT) != 0x0) {
                        const token& old_imut = *state.mods.find(shift_mods::IMUT);
                        this->token_warning(old_imut, "redundant 'imut' specifier");
                    } else {
                        state.mods.unsafe_add(mod, tok);
                    }
                    break;
                default:
                    state.mods.unsafe_add(mod, tok);
                    break;
            }
        }
        return mod;
    }

    const token& parser::skip_until_closing(const typename token::type bracket_type) noexcept {
        if (bracket_type != lexing::token::type::LEFT_BRACKET && bracket_type != lexing::token::type::LEFT_SQUARE_BRACKET &&
            bracket_type != lexing::token::type::LEFT_SCOPE_BRACKET)
            return m_skip_until(bracket_type);

        const lexing::token::type look = lexing::token::type(std::underlying_type_t<token_type>(bracket_type) + 1);

        size_t count = 1;
        for (const token* _token = &this->m_lexer->current_token();
             !_token->is_eof_token() && count > 0; _token = &this->m_lexer->next_token()) {
            if (_token->get_token_type() == bracket_type) count++;
            else if (_token->get_token_type() == look) count--;
        }

        if (count != 0) {
            const char* msg;
            switch (bracket_type) {
                case token::type::LEFT_BRACKET:
                    msg = "expected ')' before end of file";
                    break;
                case token::type::LEFT_SQUARE_BRACKET:
                    msg = "expected ']' before end of file";
                    break;
                case token::type::LEFT_SCOPE_BRACKET:
                    msg = "expected '}' before end of file";
                    break;
                default:
                    msg = "expected closing bracket before end of file";
                    break;
            }

            this->token_error(this->m_lexer->reverse_peek_token(), msg);
        }

        return this->m_lexer->current_token().is_eof_token() ? this->m_lexer->current_token() : this->m_lexer->reverse_token();
    }

    std::string parser::token_message_header(error_handler::message_type type, const lexing::token& tok) {
        std::string err;
        lexing::file_position file_pos = tok.get_file_position();

        using
        enum error_handler::message_type;
        switch (type) {
            case error:
                err += "error";
                break;
            case warning:
                err += "warning";
                break;
            case info:
                err += "info";
                break;
            default:
                break;
        }

        err += ": ";
        err += m_lexer->get_file() == "<internal>"sv ? "<internal>"sv
                                                     : (std::string_view) std::filesystem::relative(
                this->m_lexer->get_file().raw_path()).string();
        err += ':';
        err += std::to_string(file_pos.line);
        err += ':';
        err += std::to_string(file_pos.col);
        err += ": ";
        return err;
    }

    std::string parser::token_underline(const lexing::token& tok) {
        // TODO Change underline algorithm if position are able to take up more than one line (e.g. multi-line string)
        auto line = std::string(this->get_line(tok));

        std::size_t use_col = tok.get_file_position().col;

        {
            const auto tab_size = m_lexer->get_tab_size();
            for (char& ch : line) {
                if (ch == '\t') {
                    ch = ' ';
                    use_col -= tab_size;
                }
            }
        }

        std::string indexer(use_col - 1, ' ');
        indexer.append(tok.get_data().length(), '^');
        return line + '\n' + indexer;
    }

    void parser::token_error(const token& tok, const std::string_view msg) {
        SHIFT_ASSERT(!tok.is_eof_token());

        if (!this->m_error_handler) return;

        std::string err = token_message_header(error_handler::message_type::error, tok);

        err += msg;
        err += '\n';
        err += token_underline(tok);

        m_error_handler->add_error(std::move(err));
    }

    void parser::token_warning(const token& tok, const std::string_view msg) {
        SHIFT_ASSERT(!tok.is_eof_token());

        if (!this->m_error_handler) return;
        if (!this->m_error_handler->is_print_warnings()) return;

        std::string err = token_message_header(error_handler::message_type::warning, tok);

        err += msg;
        err += '\n';
        err += token_underline(tok);

        m_error_handler->add_warning(std::move(err));
    }

    std::string_view parser::get_line(const token& token_) const noexcept {
        SHIFT_ASSERT(!token_.is_eof_token());
        return this->m_lexer->get_lines()[token_.get_file_position().line - 1];
    }

    bool parser::is_module_defined() const noexcept { return this->m_module && this->m_module->depth() > 0; }

    void parser::consume_modifiers(parse_state& state) {
        for (const token* mod_token = &*state.position; mod_token->is_modifier(); mod_token = &*++state.position) {
            this->parse_modifier(state);
        }
    }

    SHIFT_API std::uint8_t parser::operator_priority(const lexing::token::type type, const bool prefix) noexcept {
        constexpr std::uint8_t base_priority = 0x10;
        constexpr std::uint8_t prefix_priority = 0xf0 - base_priority;
        switch (type) {
            case lexing::token::type::AND:
                return base_priority + (prefix ? prefix_priority : 0x2);
            case lexing::token::type::OR:
            case lexing::token::type::XOR:
            case lexing::token::type::SHIFT_LEFT:
            case lexing::token::type::SHIFT_RIGHT:
                return base_priority + 0x2;

            case lexing::token::type::AND_AND:
            case lexing::token::type::OR_OR:
                return base_priority + 0x3;

            case lexing::token::type::GREATER_THAN:
            case lexing::token::type::LESS_THAN:
                return base_priority + 0x4;

            case lexing::token::type::PLUS:
            case lexing::token::type::MINUS:
                return base_priority + (prefix ? prefix_priority : 0x5);

            case lexing::token::type::MULTIPLY:
                return base_priority + (prefix ? prefix_priority : 0x6);
            case lexing::token::type::DIVIDE:
            case lexing::token::type::MODULO:
                return base_priority + 0x6;

            case lexing::token::type::MINUS_MINUS:
            case lexing::token::type::PLUS_PLUS:
                return base_priority + (prefix ? prefix_priority : prefix_priority - 1);

            case lexing::token::type::NOT:
                return base_priority + prefix_priority - 3;
            case lexing::token::type::FLIP_BITS:
                return base_priority + prefix_priority - 2;

            case lexing::token::type::LEFT_BRACKET: // bracket-ed expressions
            case lexing::token::type::LEFT_SQUARE_BRACKET: // array operator
            case lexing::token::type::LEFT_SCOPE_BRACKET:
            case lexing::token::type::IDENTIFIER: // variable names / function calls
                return base_priority + prefix_priority + 1;

            default: {
                // i = 5 + 3; -> should be -> (i) = (5 + 3); | if = had more priority -> (i = 5) + (3)
                return (type & lexing::token::type::EQUALS) == lexing::token::type::EQUALS ?
                       operator_priority(type & ~token::type::EQUALS, prefix) - base_priority : 0x0;
            }
        }
    }
}