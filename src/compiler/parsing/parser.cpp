/**
 * @file compiler/shift_parser.cpp
 */
#include "compiler/parsing/parser.h"
#include "utils/lazy.h"

#include <ranges>
#include <cstring>
#include <algorithm>
#include <vector>

using namespace std::string_view_literals;
using namespace shift::compiler::lexing;

namespace shift::compiler::parsing {
    struct parser::parse_state {
        explicit parse_state(error_handler& eh) : err_stream(eh) {}

        // Current lexing::token position in the parsing process
        lexing::lexer::const_iterator position{};

        // Utility variable for holding the current shift_mods specified by the user
        std::vector<std::pair<shift_mods, const lexing::token*>> current_mods;

        std::optional<std::pair<shift_mods, const lexing::token*>> find_mod(shift_mods mod) {
            auto it = std::ranges::find_if(current_mods, [mod](const auto& p) { return p.first == mod; });
            return it == std::ranges::end(current_mods) ? std::nullopt : std::optional{ *it };
        }

        bool has_mod(shift_mods mod) const {
            return std::ranges::find_if(current_mods, [mod](const auto& p) { return p.first == mod; })
                   != std::ranges::end(current_mods);
        }

        error_stream err_stream;
    };
}

namespace shift::compiler::parsing {
    static constexpr shift_mods to_access_specifier(const lexing::token& token) noexcept;

    static constexpr shift_mods visibility_modifiers = shift_mods::PUBLIC | shift_mods::PROTECTED | shift_mods::PRIVATE;
    static constexpr shift_mods class_modifiers = visibility_modifiers | shift_mods::STATIC;
    static constexpr shift_mods function_modifiers = visibility_modifiers | shift_mods::STATIC | shift_mods::EXTERN;
    static constexpr shift_mods global_function_modifiers = function_modifiers & ~(shift_mods::STATIC | visibility_modifiers);
    static constexpr shift_mods type_modifiers = shift_mods::CONST_ | shift_mods::IMUT;
    static constexpr shift_mods constructor_modifiers = (function_modifiers & ~shift_mods::STATIC) | shift_mods::EXPLICIT;
    static constexpr shift_mods destructor_modifiers = function_modifiers & ~shift_mods::STATIC;
    static constexpr shift_mods field_modifiers = visibility_modifiers | type_modifiers | shift_mods::STATIC | shift_mods::EXTERN;
    static constexpr shift_mods variable_modifiers = type_modifiers;
    static constexpr shift_mods global_variable_modifiers = variable_modifiers & ~(shift_mods::STATIC | visibility_modifiers);

    static constexpr lexing::token this_token("this"sv, token::type::IDENTIFIER, { 0, 0 });
    static constexpr lexing::token base_token("base"sv, token::type::IDENTIFIER, { 0, 0 });

    // Stores the string content of "@0", "@1", "@2", ..., which are used for identifying nameless function parameters.
    static std::unordered_set<std::string> func_null_params;

    SHIFT_API void parser::parse() {
        parse_state state(*m_error_handler);
        state.position = m_lexer->begin();
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
        for (; !state.position->is_eof_token(); ++state.position) {
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
                m_parse_class(parent_class);
                continue;
            }

            if (current.is_access_specifier()) {
                m_parse_access_specifier();
                continue;
            }

            if (parent_class && current.is_right_scope_bracket()) break;

            // Module statemnet parsing
            if (current.is_module()) {
                if (!parent_class) {
                    if (!this->m_is_module_defined()) {
                        // module statement; expected the least (only once)
                        m_parse_module();
                    } else {
                        this->m_token_error(*current, "module already defined");
                        this->m_skip_until(token::type::SEMICOLON);
                    }
                } else {
                    this->m_token_error(*current, "unexpected module declaration inside class");
                    this->m_skip_until(token::type::SEMICOLON);
                }
                continue;
            }

            // Constructor parsing
            if (current.is_constructor()) {
                if (!parent_class) {
                    this->m_token_error(*current, "constructor may only be defined inside of class");
                }
                shift_type ret_type{};
                m_parse_function_header(parent_class, ret_type);
                continue;
            }

            // Destructor parsing
            if (current.is_destructor()) {
                if (!parent_class) {
                    this->m_token_error(*current, "destructor may only be defined inside of class");
                }
                shift_type ret_type{};
                m_parse_function_header(parent_class, ret_type);
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
                                if (!parsed_type->name.name.begin->is_null_token()) {
                                    this->m_token_error(*parsed_type->name.name.begin, "expected variable or function type");
                                } else {
                                    this->m_token_error(this->m_lexer->reverse_peek_token(),
                                        "expected variable or function type before end of file");
                                    return;
                                }
                            } else {
                                const auto& tok = this->m_lexer->current_token();
                                if (!tok.is_null_token()) {
                                    this->m_token_error(tok, "expected variable or function type");
                                } else {
                                    this->m_token_error(this->m_lexer->reverse_peek_token(),
                                        "expected variable or function type before end of file");
                                    return;
                                }
                            }
                        }
                        type = std::move(*parsed_type);
                    }
                }

                for (const token* tok = &this->m_lexer->current_token(); tok->is_access_specifier(); tok = &this->m_lexer->next_token()) {
                    this->m_parse_access_specifier();
                }

                if (this->m_lexer->current_token().is_operator()
                    || this->m_lexer->peek_token().is_left_bracket()
                    || (!type.name.name.empty() && type.name.name.begin->is_void())) {
                    m_parse_function_header(parent_class, type);
                    continue;
                }
                {
                    if (this->m_lexer->current_token().is_null_token()) {
                        if (!type.name.name.empty()) {
                            this->m_token_error(this->m_lexer->reverse_peek_token(),
                                "expected variable or function name after type declaration '" + type.get_printable_fqn() +
                                "' before end of file");
                            return;
                        } else {
                            this->m_token_error(this->m_lexer->reverse_peek_token(),
                                "expected variable or function name before end of file");
                            return;
                        }

                    }

                    const token& after_name = this->m_lexer->peek_token();
                    if (after_name.is_null_token()) {
                        this->m_token_error(this->m_lexer->reverse_peek_token(),
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

                    if (!this->m_lexer->current_token().is_null_token()) {
                        this->m_token_error(this->m_lexer->current_token(), "expected variable or function declaration");
                        this->m_skip_until(token::type::SEMICOLON);
                    } else {
                        this->m_token_error(this->m_lexer->reverse_peek_token(),
                            "expected variable or function declaration before end of file");
                        return;
                    }
                }
                continue;
            }

            this->m_token_error(*current, "unexpected lexing::token '" + std::string(current->get_data()) + "'");
        }

        if (this->m_mods.size() > 0) {
            const auto& [mod, token_] = this->m_mods.front();
            this->m_token_error(*token_, "unexpected '" + std::string(token_->get_data()) + "' specifier");
        }
    }

    void parser::parse_class(shift_class* parent_class) {
        const token& class_token = this->m_lexer->current_token();

        if (!class_token.is_class()) {
            this->m_token_error(class_token, "expected 'class'");
            return;
        }

        if (!this->m_is_module_defined()) {
            this->m_token_error(class_token, "module must be defined before creating class");
        }

        shift_class& clazz = m_classes.emplace_back();

        clazz.implicit_use_statements = this->m_global_uses.size();
        clazz.module_ = this->m_module.get();
        clazz.this_var.name = &this_token;
        clazz.this_var.clazz = &clazz;
        clazz.this_var.type.name.clazz = &clazz;
        clazz.this_var.type.name.name_clazz = &clazz;
        clazz.this_var.type.shift_mods = shift_mods::PRIVATE;
        clazz.base_var.name = &base_token;
        clazz.base_var.clazz = &clazz; // TODO change
        clazz.base_var.type.name.clazz = clazz.base.clazz; // TODO change
        clazz.base_var.type.name.name_clazz = clazz.base.name_clazz; // TODO change
        clazz.base_var.type.shift_mods = shift_mods::PRIVATE;
        clazz.parent.clazz = parent_class;


        for (const token* access_specifier_token = &this->m_lexer->next_token(); access_specifier_token->is_access_specifier(); access_specifier_token = &this->m_lexer->next_token()) {
            this->m_parse_access_specifier();
        }

        for (const auto& [mod, token_] : this->m_mods) {
            if ((mod & class_modifiers) == 0) {
                this->m_token_error(*token_, "unexpected '" + std::string(token_->get_data()) + "' specifier in class declaration");
            } else {
                clazz.shift_mods |= mod;
            }
        }
        this->m_clear_mods();

        if ((clazz.shift_mods & visibility_modifiers) == 0x0) { clazz.shift_mods |= shift_mods::PUBLIC; }

        clazz.name = &this->m_lexer->current_token();

        if (!clazz.name->is_identifier()) {
            if (!clazz.name->is_null_token()) {
                this->m_token_error(*clazz.name, "expected identifier for class name");
                this->m_skip_before(token::type::LEFT_SCOPE_BRACKET);
            } else {
                this->m_token_error(this->m_lexer->reverse_peek_token(), "expected valid class name before end of file");
                return;
            }
        } else if (clazz.name->is_keyword()) {
            this->m_token_error(*clazz.name, "invalid class name '" + std::string(clazz.name->get_data()) + "'");
            this->m_skip_before(token::type::LEFT_SCOPE_BRACKET);
        }

        if (this->m_lexer->peek_token().is_colon()) {
            const token& begin_name_token = this->m_lexer->next_token(2);
            if (!begin_name_token.is_identifier()) {
                this->m_token_error(begin_name_token, "expected valid class name as base for class '" + clazz.get_fqn() + "'");
                this->m_skip_before(token::type::LEFT_SCOPE_BRACKET);
            } else {
                clazz.base.name = this->m_parse_name("class name");
                this->m_lexer->reverse_token();
            }
        }

        const token& left_bracket = this->m_lexer->next_token();

        if (!left_bracket.is_left_scope_bracket()) {
            if (!left_bracket.is_null_token()) {
                this->m_token_error(left_bracket, "expected '{' after class declaration");
            } else {
                this->m_token_error(this->m_lexer->reverse_peek_token(), "expected '{' after class declaration before end of file");
            }
        }

        this->m_lexer->next_token(); // Move onto first lexing::token inside class body

        // Parse class body
        this->m_parse_body(&clazz);

        const token& right_bracket = this->m_lexer->current_token();

        if (!right_bracket.is_right_scope_bracket()) {
            if (!right_bracket.is_null_token()) {
                this->m_token_error(right_bracket, "expected '}' after class declaration");
            } else {
                this->m_token_error(this->m_lexer->reverse_peek_token(), "expected '}' after class declaration before end of file");
            }
        }
    }

    shift_function* parser::parse_function_header(shift_class* parent_class, shift_type& return_type) {
        shift_name name;

        for (const token* tok = &this->m_lexer->current_token(); tok->is_access_specifier(); tok = &this->m_lexer->next_token()) {
            this->m_parse_access_specifier();
        }

        name.begin = this->m_lexer->get_index();

        if (name.begin->is_operator()) {
            if (!parent_class) {
                this->m_token_error(*name.begin, "operator overload must be within a class");
            }

            const token& operator_overload_token = this->m_lexer->next_token();

            if (!operator_overload_token.is_overloadable_operator()) {
                if (operator_overload_token.is_left_square_bracket()) {
                    const token& right_square_bracket = this->m_lexer->next_token();
                    if (!right_square_bracket.is_right_square_bracket()) {
                        if (!right_square_bracket.is_null_token()) {
                            this->m_token_error(right_square_bracket,
                                "invalid operator overload '[" + std::string(right_square_bracket.get_data()) +
                                "': expected ']' after '['");
                        } else {
                            this->m_token_error(this->m_lexer->reverse_peek_token(),
                                "invalid operator overload '[': expected ']' after '[' before end of file");
                        }
                    }
                } else {
                    if (!operator_overload_token.is_null_token()) {
                        this->m_token_error(operator_overload_token, "expected overloadable operator after keyword 'operator'");
                    } else {
                        this->m_token_error(this->m_lexer->reverse_peek_token(),
                            "expected overloadable operator after keyword 'operator' before end of file");
                    }
                }
            }
        } else if (name.begin->is_constructor() || name.begin->is_destructor()) {
            if (name.begin->is_constructor() && return_type.name.name.size() != 0) {
                if (!return_type.name.name.begin->is_null_token()) {
                    this->m_token_error(*return_type.name.name.begin, "constructor cannot have return type");
                } else {
                    this->m_token_error(*name.begin, "constructor cannot have return type");
                }
            } else if (name.begin->is_destructor() && return_type.name.name.size() != 0) {
                if (!return_type.name.name.begin->is_null_token()) {
                    this->m_token_error(*return_type.name.name.begin, "destructor cannot have return type");
                } else {
                    this->m_token_error(*name.begin, "destructor cannot have return type");
                }
            }
        } else if (name.begin->is_keyword()) {
            this->m_token_error(*name.begin, "'" + std::string(name.begin->get_data()) + "' is not a valid variable or function name");
        } else if (!name.begin->is_identifier()) {
            if (!name.begin->is_null_token()) {
                this->m_token_error(*name.begin,
                    "expected identifier for variable or function name before '" + std::string(name.begin->get_data()) +
                    "'");
            } else {
                this->m_token_error(this->m_lexer->reverse_peek_token(),
                    "expected identifier for variable or function name before end of file");
                return nullptr;
            }
        }

        const token& next_token = this->m_lexer->next_token();

        name.end = this->m_lexer->get_index();

        if (next_token.is_left_bracket()) {
            // function definition

            shift_function* func = parent_class ? &parent_class->functions.emplace_back() : &this->m_functions.emplace_back();

            func->name = name;
            func->implicit_use_statements = parent_class ? parent_class->use_statements.size() : this->m_global_uses.size();
            func->clazz = parent_class;

            if (!this->m_is_module_defined()) {
                this->m_token_error(*func->name.begin, "module must be defined before creating function");
            }

            func->module_ = this->m_module.get();

            func->return_type = std::move(return_type);

            auto func_mods = parent_class ? function_modifiers : global_function_modifiers;

            if (func->name.size() > 0) {
                if (func->name.begin->is_constructor()) {
                    func_mods = constructor_modifiers;
                } else if (func->name.begin->is_destructor()) {
                    func_mods = destructor_modifiers;
                }
            }

            for (const auto& [mod, token_] : this->m_mods) {
                if ((mod & func_mods) == 0x0) {
                    if ((mod & type_modifiers) != 0x0) {
                        if (func->return_type.name.name.size() != 0 && func->return_type.name.name.begin->is_void()) {
                            this->m_token_error(*token_,
                                "void returning function cannot have '" + std::string(token_->get_data()) + "' specifier");
                        } else {
                            func->return_type.shift_mods |= mod;
                        }
                    } else {
                        this->m_token_error(*token_,
                            "unexpected '" + std::string(token_->get_data()) + "' specifier in function declaration");
                    }
                } else if ((mod & shift_mods::STATIC) && name.begin->is_operator()) {
                    this->m_token_error(*token_, "operator overload cannot have 'static' specifier");
                } else {
                    func->shift_mods |= mod;
                }
            }
            this->m_clear_mods();

            // parse function parameters
            for (this->m_lexer->next_token(); !this->m_lexer->current_token().is_null_token() &&
                                              !this->m_lexer->current_token().is_right_bracket(); this->m_lexer->next_token()) {
                shift_variable param_var;
                param_var.function = func;
                if (auto param_type = m_parse_type("function parameter")) {
                    param_var.type = std::move(*param_type);
                }
                param_var.name = &this->m_lexer->current_token();

                if (param_var.type.name.name.size() == 0) {
                    if (param_var.type.name.name.begin != std::vector<compiler::token>::const_iterator{}) {
                        if (!param_var.type.name.name.begin->is_null_token()) {
                            this->m_token_error(*param_var.type.name.name.begin,
                                "expected parameter type in function parameter list, got '" +
                                std::string(param_var.type.name.name.begin->get_data()) + "'");
                        } else {
                            this->m_token_error(this->m_lexer->reverse_peek_token(),
                                "expected parameter type in function parameter list before end of file");
                        }
                    } else {
                        const auto& tok = this->m_lexer->current_token();
                        if (!tok.is_null_token()) {
                            this->m_token_error(tok,
                                "expected parameter type in function parameter list, got '" + std::string(tok.get_data()) +
                                "'");
                        } else {
                            this->m_token_error(this->m_lexer->reverse_peek_token(),
                                "expected parameter type in function parameter list before end of file");
                        }
                    }
                }

                if (param_var.name->is_comma() || param_var.name->is_right_bracket()) {
                    // nameless parameters
                    const token* const old_name = param_var.name;
                    param_var.name = &token::null;

                    {
                        auto [it, ins] = func_null_params.emplace("@" + std::to_string(func->parameters.size()));
                        func->parameters.push_back({ (std::string_view) *it, std::move(param_var) });
                    }

                    if (old_name->is_right_bracket())
                        break;

                    continue;
                }

                if (!param_var.name->is_identifier()) {
                    if (!param_var.name->is_null_token()) {
                        this->m_token_error(*param_var.name, "expected identifier for function parameter name");
                    } else {
                        this->m_token_error(this->m_lexer->reverse_peek_token(),
                            "expected identifier for function parameter name before end of file");
                    }
                } else if (param_var.name->is_keyword()) {
                    this->m_token_error(*param_var.name,
                        "'" + std::string(param_var.name->get_data()) + "' is not a valid function parameter name");
                }

                func->parameters.push_back({ param_var.name->get_data(), std::move(param_var) });

                const token& after_param_name = this->m_lexer->next_token();
                if (!after_param_name.is_comma()) {
                    if (!after_param_name.is_right_bracket()) {
                        if (!after_param_name.is_null_token()) {
                            this->m_token_error(after_param_name, "expected ',' or ')' in function parameter list");
                        } else {
                            this->m_token_error(this->m_lexer->reverse_peek_token(),
                                "expected ',' or ')' in function parameter list before end of file");
                        }
                    } else break;
                }
            }

            const token& function_right_parameter_bracket = this->m_lexer->current_token();

            if (!function_right_parameter_bracket.is_right_bracket()) {
                if (!function_right_parameter_bracket.is_null_token()) {
                    this->m_token_error(function_right_parameter_bracket, "expected ')' at end of function parameter list");
                } else {
                    this->m_token_error(this->m_lexer->reverse_peek_token(),
                        "expected ')' at end of function parameter list before end of file");
                }
            } else if (this->m_lexer->reverse_peek_token().is_comma()) {
                this->m_token_error(this->m_lexer->reverse_peek_token(), "misplaced ',' in function parameter list");
            }

            if (name.begin->is_destructor()) {
                if (func->parameters.size() != 0) {
                    this->m_token_error(*func->name.begin, "destructor cannot have parameters");
                }
            }

            if (name.begin->is_operator()) {
                const token& overload_token = *(name.begin + 1);
                if ((overload_token.is_strictly_prefix_operator() || overload_token.is_strictly_suffix_operator())
                    && (func->parameters.size() > 0)) {
                    this->m_token_error(next_token,
                        "unexpected parameter in function 'operator" + std::string(overload_token.get_data()) + "'");
                } else if (!(
                    ((overload_token.is_binary_operator() || overload_token.is_left_square_bracket()) && func->parameters.size() == 1) ||
                    (overload_token.is_unary_operator() && func->parameters.size() == 0))) {
                    this->m_token_error(next_token,
                        "invalid amount of parameters in function 'operator" + std::string(overload_token.get_data()) +
                        "'");
                }
            }

            const token& function_left_bracket = this->m_lexer->next_token();

            if (func->shift_mods & shift_mods::EXTERN) {
                if (function_left_bracket.is_semicolon()) return func;
                this->m_token_error(function_left_bracket, "external function cannot contain definition");
            }

            if (!function_left_bracket.is_left_scope_bracket()) {
                if (!function_left_bracket.is_null_token()) {
                    this->m_token_error(function_left_bracket, "expected '{' after function declaration");
                } else {
                    this->m_token_error(this->m_lexer->reverse_peek_token(),
                        "expected '{' after function declaration before end of file");
                }
            }

            this->m_lexer->next_token(); // Move onto first lexing::token inside function body

            // Parse function body
            m_parse_function(*func);

            const token& function_right_bracket = this->m_lexer->current_token();

            if (!function_right_bracket.is_right_scope_bracket()) {
                if (!function_right_bracket.is_null_token()) {
                    this->m_token_error(function_right_bracket, "expected '}' after function declaration");
                } else {
                    this->m_token_error(this->m_lexer->reverse_peek_token(),
                        "expected '}' after function declaration before end of file");
                }
            }
            return func;
        }
        if (!next_token.is_null_token()) {
            this->m_token_error(next_token, "expected '(' for function paramter declaration");
        } else {
            this->m_token_error(this->m_lexer->reverse_peek_token(),
                "expected '(' for function paramter declaration before end of file");
        }
        return nullptr;
    }

    std::optional<shift_variable>
    parser::parse_variable_header(shift_class* parent_class, shift_function* parent_function, shift_type& type) {
        for (const token* tok = &this->m_lexer->current_token(); tok->is_access_specifier(); tok = &this->m_lexer->next_token()) {
            this->m_parse_access_specifier();
        }

        if (type.name.name.size() == 0) {
            if (type.name.name.begin != std::vector<compiler::token>::const_iterator{}) {
                if (!type.name.name.begin->is_null_token()) {
                    this->m_token_error(*type.name.name.begin, "expected variable type");
                } else {
                    this->m_token_error(this->m_lexer->reverse_peek_token(), "expected variable type before end of file");
                }
            } else {
                const auto& tok = this->m_lexer->current_token();
                if (!tok.is_null_token()) {
                    this->m_token_error(tok, "expected variable type");
                } else {
                    this->m_token_error(this->m_lexer->reverse_peek_token(), "expected variable type before end of file");
                }
            }
        }

        const token* name = &this->m_lexer->current_token();

        if (name->is_null_token()) {
            this->m_token_error(this->m_lexer->reverse_peek_token(), "expected variable name before end of file");
            return std::nullopt;
        }

        if (name->is_keyword()) {
            this->m_token_error(*name, "'" + std::string(name->get_data()) + "' is not a valid variable name");
        } else if (!name->is_identifier()) {
            if (!name->is_null_token()) {
                this->m_token_error(*name, "expected identifier for variable name, got '" + std::string(name->get_data()) + "'");
            } else {
                this->m_token_error(this->m_lexer->reverse_peek_token(), "expected identifier for variable name before end of file");
            }
        }

        if (type.name.name.size() != 0 && type.name.name.begin->is_void()) {
            this->m_token_error(*type.name.name.begin, "'void' cannot be used as a variable type");
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
            this->m_token_error(*variable->name, "module must be defined before creating variable");
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
                this->m_token_error(*token_, "invalid '" + std::string(token_->get_data()) + "' specifier on variable");
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
                this->m_token_error(after_name,
                    "expected ';' or '=' for variable declaration, got '" + std::string(after_name.get_data()) + "'");
            }

            // variable definition
            const token& first_expr_token = this->m_lexer->next_token(); // Move onto the expression
            variable->value = m_parse_expression();

            if (variable->value.type == token::type::NULL_TOKEN) {
                this->m_token_error(first_expr_token, "expected valid expression for variable assignment value");
            }
        } else if (after_name.is_semicolon()) {
            // do nothing
        } else {
            if (!after_name.is_null_token()) {
                this->m_token_error(after_name, "expected either ';' or '=' for variable declaration");
            } else {
                this->m_token_error(this->m_lexer->reverse_peek_token(),
                    "expected either ';' or '=' for variable declaration before end of file");
            }
        }
        return v;
    }

    void parser::parse_function(shift_function& func) {
        return m_parse_function_block(func, func.statements);
    }

    void parser::parse_function_block(shift_function& func, utils::ideque<shift_statement>& statements, size_t count) {
        for (const token* _token = &this->m_lexer->current_token();
             count != 0 && !_token->is_null_token(); _token = &this->m_lexer->next_token(), count--) {
            // This function relies on parent statements being linked in a chain; addresses of statements must not change
            shift_statement& statement = statements.emplace_back();

            for (const token* access_specifier_token = _token; access_specifier_token->is_access_specifier(); access_specifier_token = &this->m_lexer->next_token()) {
                this->m_parse_access_specifier();
            }

            _token = &this->m_lexer->current_token();

            if (_token->is_use()) {
                statement.set_use(_token);
                auto const old_size = this->m_global_uses.size();
                m_parse_use(this->m_global_uses);
                if (old_size != this->m_global_uses.size()) {
                    shift_module const& module_ = this->m_global_uses.back();
                    statement.set_use_module(module_);
                    this->m_global_uses.pop_back();
                } else {
                    statements.pop_back();
                }
            } else if (_token->is_if()) {
                if (this->m_mods.size() != 0) {
                    const auto& [mod, token_] = this->m_mods.front();
                    this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                    this->m_clear_mods();
                }
                statement.set_if(_token);

                const token& left_condition_bracket = this->m_lexer->next_token();
                if (!left_condition_bracket.is_left_bracket()) {
                    if (!left_condition_bracket.is_null_token()) {
                        this->m_token_error(left_condition_bracket, "expected '(' after 'if' inside function body");
                        this->m_skip_until(token::type::LEFT_BRACKET);
                    } else {
                        this->m_token_error(this->m_lexer->reverse_peek_token(),
                            "expected '(' after 'if' inside function body before end of file");
                    }
                }

                const token& first_condition_token = this->m_lexer->next_token(); // Move onto condition token

                statement.set_if_condition(m_parse_expression(token::type::RIGHT_BRACKET));

                const token& right_condition_bracket = this->m_lexer->current_token();

                if (first_condition_token == right_condition_bracket) {
                    this->m_token_error(left_condition_bracket, "expected valid expression inside 'if' statement condition");
                }

                if (!right_condition_bracket.is_right_bracket()) {
                    this->m_token_error(this->m_lexer->reverse_peek_token(),
                        "expected ')' after 'if' condition inside function body before end of file");
                    break;
                }

                const token& left_if_bracket = this->m_lexer->next_token();
                if (left_if_bracket.is_left_scope_bracket()) {
                    this->m_lexer->next_token(); // Skip {
                    m_parse_function_block(func, statement.get_if_statements());

                    const token& right_if_bracket = this->m_lexer->current_token();
                    if (!right_if_bracket.is_right_scope_bracket()) {
                        if (!right_if_bracket.is_null_token()) {
                            this->m_token_error(right_if_bracket, "expected '}' to close 'if' declaration inside function body");
                            this->m_skip_until(token::type::RIGHT_SCOPE_BRACKET);
                        } else {
                            this->m_token_error(this->m_lexer->reverse_peek_token(),
                                "expected '}' to close 'if' declaration inside function body before end of file");
                        }
                    }
                } else {
                    m_parse_function_block(func, statement.get_if_statements(), 1);
                    if (statement.get_if_statements().size() == 0) {
                        this->m_token_error(this->m_lexer->current_token(), "expected valid statement after 'if' declaration");
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
                            if (!right_else_bracket.is_null_token()) {
                                this->m_token_error(right_else_bracket, "expected '}' to close 'else' declaration inside function body");
                                this->m_skip_until(token::type::RIGHT_SCOPE_BRACKET);
                            } else {
                                this->m_token_error(this->m_lexer->reverse_peek_token(),
                                    "expected '}' to close 'else' declaration inside function body before end of file");
                            }
                        }
                    } else {
                        m_parse_function_block(func, else_statement.get_else_statements(), 1);
                        if (else_statement.get_else_statements().size() == 0) {
                            this->m_token_error(this->m_lexer->current_token(), "expected valid statement after 'else' declaration");
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
                this->m_token_error(*_token, "unexpected 'else' statement in function body");
            } else if (_token->is_while()) {
                if (this->m_mods.size() != 0) {
                    const auto& [mod, token_] = this->m_mods.front();
                    this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                    this->m_clear_mods();
                }
                statement.set_while(_token);

                const token& left_condition_bracket = this->m_lexer->next_token();
                if (!left_condition_bracket.is_left_bracket()) {
                    if (!left_condition_bracket.is_null_token()) {
                        this->m_token_error(left_condition_bracket, "expected '(' after 'while' inside function body");
                        this->m_skip_until(token::type::LEFT_BRACKET);
                    } else {
                        this->m_token_error(this->m_lexer->reverse_peek_token(),
                            "expected '(' after 'while' inside function body before end of file");
                    }
                }

                const token& first_condition_token = this->m_lexer->next_token(); // Move onto condition token
                statement.set_while_condition(m_parse_expression(token::type::RIGHT_BRACKET));

                const token& right_condition_bracket = this->m_lexer->current_token();

                if (first_condition_token == right_condition_bracket) {
                    this->m_token_error(left_condition_bracket, "expected valid expression inside 'while' statement condition");
                }

                if (!right_condition_bracket.is_right_bracket()) {
                    this->m_token_error(this->m_lexer->reverse_peek_token(),
                        "expected ')' after 'while' condition inside function body before end of file");
                    break;
                }

                const token& left_while_bracket = this->m_lexer->next_token();
                if (left_while_bracket.is_left_scope_bracket()) {
                    this->m_lexer->next_token(); // Skip {
                    m_parse_function_block(func, statement.get_while_statements());

                    const token& right_while_bracket = this->m_lexer->current_token();
                    if (!right_while_bracket.is_right_scope_bracket()) {
                        if (!right_while_bracket.is_null_token()) {
                            this->m_token_error(right_while_bracket, "expected '}' to close 'while' declaration inside function body");
                            this->m_skip_until(token::type::RIGHT_SCOPE_BRACKET);
                        } else {
                            this->m_token_error(this->m_lexer->reverse_peek_token(),
                                "expected '}' to close 'while' declaration inside function body before end of file");
                        }
                    }
                } else {
                    m_parse_function_block(func, statement.get_while_statements(), 1);
                    if (statement.get_while_statements().size() == 0) {
                        this->m_token_error(this->m_lexer->current_token(), "expected valid statement after 'while' declaration");
                    }
                    this->m_lexer->reverse_token(); // lexing will be on lexing::token after last statement lexing::token if count causes it to end
                }

                for (auto& st : statement.get_while_statements()) {
                    st.parent = &statement;
                }
            } else if (_token->is_for()) {
                if (this->m_mods.size() != 0) {
                    const auto& [mod, token_] = this->m_mods.front();
                    this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                    this->m_clear_mods();
                }
                statement.set_for(_token);

                const token& left_condition_bracket = this->m_lexer->next_token();
                if (!left_condition_bracket.is_left_bracket()) {
                    if (!left_condition_bracket.is_null_token()) {
                        this->m_token_error(left_condition_bracket, "expected '(' after 'for' inside function body");
                        this->m_skip_until(token::type::LEFT_BRACKET);
                    } else {
                        this->m_token_error(this->m_lexer->reverse_peek_token(),
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
                                this->m_token_error(*init_statement.data.token_[0], "invalid statement inside 'for' initializer");
                            } else {
                                this->m_token_error(new_left_conditions_bracket, "invalid statement inside 'for' initializer");
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
                        this->m_token_error(this->m_lexer->reverse_peek_token(),
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
                        if (!right_for_bracket.is_null_token()) {
                            this->m_token_error(right_for_bracket, "expected '}' to close 'for' declaration inside function body");
                            this->m_skip_until(token::type::RIGHT_SCOPE_BRACKET);
                        } else {
                            this->m_token_error(this->m_lexer->reverse_peek_token(),
                                "expected '}' to close 'for' declaration inside function body before end of file");
                        }
                    }
                } else {
                    m_parse_function_block(func, statement.get_for_statements(), 1);
                    if (statement.get_for_statements().size() == 0) {
                        this->m_token_error(this->m_lexer->current_token(), "expected valid statement after 'for' declaration");
                    }
                    this->m_lexer->reverse_token(); // lexing will be on lexing::token after last statement lexing::token if count causes it to end
                }

                for (auto& st : statement.get_for_statements()) {
                    st.parent = &statement;
                }
            } else if (_token->is_return()) {
                if (this->m_mods.size() != 0) {
                    const auto& [mod, token_] = this->m_mods.front();
                    this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                    this->m_clear_mods();
                }
                statement.set_return(_token);
                this->m_lexer->next_token(); // skip 'return'
                statement.set_return_statement(m_parse_expression());
            } else if (_token->is_continue() || _token->is_break()) {
                if (this->m_mods.size() != 0) {
                    const auto& [mod, token_] = this->m_mods.front();
                    this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                    this->m_clear_mods();
                }
                if (_token->is_continue())
                    statement.set_continue(_token);
                else
                    statement.set_break(_token);

                const token& semi_colon = this->m_lexer->next_token();

                if (!semi_colon.is_semicolon()) {
                    if (!semi_colon.is_null_token()) {
                        this->m_token_error(semi_colon, "expected ';' after '" + std::string(_token->get_data()) + "' in function body");
                        this->m_skip_until(token::type::SEMICOLON);
                    } else {
                        this->m_token_error(this->m_lexer->reverse_peek_token(),
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
                    this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
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
                    this->m_token_error(this->m_lexer->reverse_peek_token(), "expected '}' in function before end of file");
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
            this->m_token_error(*token_, "unexpected '" + std::string(token_->get_data()) + "' specifier function body");
            this->m_clear_mods();
        }
    }

    void parser::parse_use(parse_state& state) {
        return parse_use(state, this->m_global_uses);
    }

    void parser::parse_use(parse_state& state, utils::ordered_set<shift_module>& modules) {
        if (!state.current_mods.empty()) {
            this->m_token_error(*state.current_mods.front().second, "unexpected access specifier in 'use' declaration");
            this->m_clear_mods();
        }

        const token& use_token = this->m_lexer->current_token();

        if (!use_token.is_use()) {
            this->m_token_error(use_token, "expected 'use'");
            this->m_skip_until(token::type::SEMICOLON);
            return;
        }

        this->m_lexer->next_token(); // skip 'use' keyword
        {
            auto module_ = shift_module{ m_parse_name("module name") };
            if (modules.contains(module_)) {
                this->m_token_warning(use_token, "redundant 'use' statement");
            } else {
                modules.push_back(std::move(module_));
            }

            const token& end_token = this->m_lexer->current_token(); // lexing::token after the module name
            if (module_.name.size() == 0) {
                this->m_token_error(end_token.is_null_token() ? use_token : end_token, "expected module name after 'use'");
                this->m_skip_until(token::type::SEMICOLON);
            } else if (end_token.is_null_token()) {
                this->m_token_error(this->m_lexer->reverse_token(), "expected ';' before end of file");
            } else if (!end_token.is_semicolon()) {
                this->m_token_error(end_token, "unexpected '" + std::string(end_token.get_data()) + "' in module name");
                this->m_skip_until(token::type::SEMICOLON);
            }
        }
    }

    void parser::parse_module() {
        if (this->m_mods.size() > 0) {
            this->m_token_error(*this->m_mods.front().second, "unexpected access specifier in 'module' declaration");
            this->m_clear_mods();
        }

        const token& module_token = this->m_lexer->current_token();

        if (!module_token.is_module()) {
            this->m_token_error(module_token, "expected 'module'");
            this->m_skip_until(token::type::SEMICOLON);
            return;
        }

        this->m_lexer->next_token(); // skip 'module' keyword
        this->m_module->name = m_parse_name("module name");

        const token& end_token = this->m_lexer->current_token(); // lexing::token after the module name

        if (this->m_module->name.size() == 0) {
            this->m_token_error(end_token.is_null_token() ? module_token : end_token, "expected module name after 'module'");
            this->m_skip_until(token::type::SEMICOLON);
        } else if (end_token.is_null_token()) {
            this->m_token_error(this->m_lexer->reverse_token(), "expected ';' before end of file");
        } else if (!end_token.is_semicolon()) {
            this->m_token_error(end_token, "unexpected '" + std::string(end_token.get_data()) + "' in module name");
            this->m_skip_until(token::type::SEMICOLON);
        }
    }

    /// @param name_type The type of name to be parsed. Will be displayed in error messages.
    ///                  e.g. "module name", "variable or function type"
    shift_name parser::parse_name(std::string_view name_type) {
        shift_name name;
        name.begin = this->m_lexer->get_index();

        token::type last_type = token::type(0x0);

        for (const token* lexing::token = &this->m_lexer->current_token(); !token->is_null_token(); lexing::token = &this->m_lexer->next_token()) {
            if (token->is_access_specifier()) {
                this->m_token_error(*token, "unexpected '" + std::string(token->get_data()) + "' specifier in " + std::string(name_type));
            } else if (token->is_keyword()) {
                // error, no keywords in (module) names
                this->m_token_error(*token, "invalid '" + std::string(token->get_data()) + "' inside " + std::string(name_type));
                last_type = token::type::IDENTIFIER;
            } else if (token->is_identifier()) {
                if (last_type == token::type::IDENTIFIER) {
                    // cannot have two identifiers in a row in a (module) name
                    last_type = token::type::IDENTIFIER;
                    break;
                }

                last_type = token::type::IDENTIFIER;
            } else if (token->get_token_type() == token::type::DOT) {
                if (last_type != token::type::IDENTIFIER) {
                    // cannot have two dots in a row in a (module) name
                    last_type = token::type::DOT;
                    this->m_lexer->next_token(); // This allows the error to be caught below
                    break;
                }

                last_type = token::type::DOT;
            } else break;
        }
        name.end = this->m_lexer->get_index();

        if (last_type == token::type::DOT) {
            this->m_token_error(this->m_lexer->reverse_peek_token(), "unexpected '.' inside " + std::string(name_type));
        }

        return name;
    }

    std::optional<shift_type> parser::parse_type(std::string_view name_type) {
        shift_type type;
        bool is_valid_type = true;

        for (const token* access_specifier_token = &this->m_lexer->current_token(); access_specifier_token->is_access_specifier(); access_specifier_token = &this->m_lexer->next_token()) {
            this->m_parse_access_specifier();
        }
        {
            const token& ref_token = this->m_lexer->current_token();

            if (ref_token.is_ref()) {
                type.ref_type = shift_type::reference_type::ref;
                this->m_lexer->next_token();
            }
        }
        for (const token* access_specifier_token = &this->m_lexer->current_token(); access_specifier_token->is_access_specifier(); access_specifier_token = &this->m_lexer->next_token()) {
            this->m_parse_access_specifier();
        }

        token::type last_type = token::type::NULL_TOKEN;
        {
            shift_name name;
            name.begin = this->m_lexer->get_index();

            for (const token* lexing::token = &this->m_lexer->current_token(); !token->is_null_token(); lexing::token = &this->m_lexer->next_token()) {
                if (token->is_access_specifier()) {
                    if (name.end == std::vector<compiler::token>::const_iterator()) {
                        name.end = this->m_lexer->get_index();
                    }

                    this->m_parse_access_specifier();
                    auto const mod = this->m_mods.back().first;

                    if ((mod & type_modifiers) == 0x0) {
                        this->m_token_error(*token,
                            "unexpected '" + std::string(token->get_data()) + "' specifier in " + std::string(name_type));
                        is_valid_type = false;
                    }
                    last_type = token::type::IDENTIFIER;
                } else if (name.end != std::vector<compiler::token>::const_iterator()) {
                    break;
                } else if (token->is_keyword()) {
                    // error, no keywords in (module) names
                    name.end = this->m_lexer->get_index();
                    break;
                    this->m_token_error(*token, "invalid '" + std::string(token->get_data()) + "' inside " + std::string(name_type));
                    is_valid_type = false;
                    last_type = token::type::IDENTIFIER;
                } else if (token->is_identifier()) {
                    if (last_type == token::type::IDENTIFIER) {
                        // cannot have two identifiers in a row in a (module) name
                        last_type = token::type::IDENTIFIER;
                        break;
                    }

                    last_type = token::type::IDENTIFIER;
                } else if (token->get_token_type() == token::type::DOT) {
                    if (last_type != token::type::IDENTIFIER) {
                        // cannot have two dots in a row in a (module) name
                        last_type = token::type::DOT;
                        this->m_lexer->next_token(); // This allows the error to be caught below
                        break;
                    }

                    last_type = token::type::DOT;
                } else break;
            }
            if (name.end == std::vector<compiler::token>::const_iterator())
                name.end = this->m_lexer->get_index();

            for (auto cur = this->m_mods.cbegin(); cur != this->m_mods.cend(); ++cur) {
                auto const mod = cur->first;
                if ((mod & type_modifiers) != 0x0) {
                    type.shift_mods |= mod;
                    cur = --this->m_mods.erase(cur);
                }
            }

            if (last_type == token::type::DOT) {
                this->m_token_error(this->m_lexer->reverse_peek_token(), "unexpected '.' inside " + std::string(name_type));
                is_valid_type = false;
            }

            type.name.name = std::move(name);
        }

        size_t dimensions = 0;
        for (const token* lexing::token = &this->m_lexer->current_token(); !token->is_null_token(); lexing::token = &this->m_lexer->next_token()) {
            if (token->is_access_specifier()) {
                this->m_token_error(*token,
                    "unexpected '" + std::string(token->get_data()) + "' specifier in " + std::string(name_type) + " type");
                is_valid_type = false;
            } else if (token->is_star()) {
                if (dimensions > 0 && last_type == token::type::RIGHT_SQUARE_BRACKET) {
                    type.add_array_dimensions(dimensions);
                    dimensions = 0;
                }
                dimensions++;
                if (last_type == token::type::LEFT_SQUARE_BRACKET) {
                    this->m_token_error(*token, "unexpected '*' after '[' in " + std::string(name_type));
                    is_valid_type = false;
                }
                last_type = token::type::STAR;
            } else if (token->is_left_square_bracket()) {
                if (dimensions > 0 && last_type == token::type::STAR) {
                    type.add_pointer_dimensions(dimensions);
                    dimensions = 0;
                }
                if (last_type != token::type::IDENTIFIER && last_type != token::type::RIGHT_SQUARE_BRACKET &&
                    last_type != token::type::STAR) {
                    this->m_token_error(*token, "unexpected '[' in " + std::string(name_type));
                    is_valid_type = false;
                } else dimensions++;

                last_type = token::type::LEFT_SQUARE_BRACKET;
            } else if (token->is_right_square_bracket()) {
                if (last_type != token::type::LEFT_SQUARE_BRACKET) {
                    this->m_token_error(*token, "unexpected ']' in " + std::string(name_type));
                    is_valid_type = false;
                }
                last_type = token::type::RIGHT_SQUARE_BRACKET;
            } else if (last_type == token::type::LEFT_SQUARE_BRACKET) {
                this->m_token_error(*token, "expected ']' in " + std::string(name_type));
                is_valid_type = false;
                break;
            } else break;
        }

        if (dimensions > 0) {
            switch (last_type) {
                case token::type::LEFT_SQUARE_BRACKET:
                case token::type::RIGHT_SQUARE_BRACKET:
                    type.add_array_dimensions(dimensions);
                    break;
                case token::type::STAR:
                    type.add_pointer_dimensions(dimensions);
                    break;
                default:
                    break;
            }
        }

        if (last_type == token::type::LEFT_SQUARE_BRACKET) {
            const token& last = this->m_lexer->reverse_peek_token();
            this->m_token_error(last, "unexpected '[' inside " + std::string(name_type));
            is_valid_type = false;
        }

        return is_valid_type ? std::optional<shift_type>(std::move(type)) : std::nullopt;
    }

    shift_expression parser::parse_expression(const utils::predicate<std::vector<token>::const_iterator>& end_func) {
        shift_expression ret_expr;
        shift_expression* expr = &ret_expr;
#ifdef SHIFT_DEBUG
        static size_t call_count = 0;

        if (call_count > 30) { /*__debugbreak();*/ }

        call_count++;
#endif
        expr->begin = this->m_lexer->get_index();
        for (const token* _token = &this->m_lexer->current_token();
             !_token->is_null_token() && !end_func(this->m_lexer->get_index()); _token = &this->m_lexer->next_token()) {
            const bool is_null_expr = expr->type == token::type::NULL_TOKEN;
            const bool is_suffix_expr = expr->parent && is_suffix_operator(expr->parent->type)
                                        && expr->parent->has_left() && expr->parent->get_left()->type != token::type::NULL_TOKEN;

            if (_token->is_string_literal()) {
                if (is_null_expr) {
                    expr->type = token::type::STRING_LITERAL;
                    expr->begin = this->m_lexer->get_index();
                    expr->end = expr->begin + 1;

                    if (is_suffix_expr) {
                        this->m_token_error(*_token, "unexpected string literal following postfix operator inside expression");
                    }
                } else {
                    this->m_token_error(*_token, "unexpected string literal in expression");
                }

                continue;
            }

            if (_token->is_char_literal()) {
                if (is_null_expr) {
                    expr->type = token::type::CHAR_LITERAL;
                    expr->begin = this->m_lexer->get_index();
                    expr->end = expr->begin + 1;

                    if (is_suffix_expr) {
                        this->m_token_error(*_token, "unexpected character literal following postfix operator inside expression");
                    }
                } else {
                    this->m_token_error(*_token, "unexpected character literal in expression");
                }

                continue;
            }

            if (_token->is_number()) {
                if (is_null_expr) {
                    expr->type = _token->get_token_type();
                    expr->begin = this->m_lexer->get_index();
                    expr->end = expr->begin + 1;
                    if (is_suffix_expr) {
                        this->m_token_error(*_token, "unexpected number literal following postfix operator inside expression");
                    }
                } else {
                    switch (_token->get_token_type()) {
                        case token::type::INTEGER_LITERAL:
                        case token::type::BINARY_LITERAL:
                        case token::type::HEX_LITERAL:
                            this->m_token_error(*_token, "unexpected integer literal in expression");
                            break;
                        case token::type::FLOAT_LITERAL:
                        case token::type::DOUBLE_LITERAL:
                            this->m_token_error(*_token, "unexpected floating point literal in expression");
                            break;
                        default:
                            this->m_token_error(*_token, "unexpected number literal in expression");
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
                        this->m_token_error(*_token, "unexpected boolean literal following postfix operator inside expression");
                    }
                } else {
                    this->m_token_error(*_token, "unexpected boolean literal in expression");
                }
                continue;
            }

            if (_token->is_null()) {
                if (is_null_expr) {
                    expr->type = token::type::IDENTIFIER;
                    expr->begin = this->m_lexer->get_index();
                    expr->end = expr->begin + 1;
                    if (is_suffix_expr) {
                        this->m_token_error(*_token, "unexpected 'null' following postfix operator inside expression");
                    }
                } else {
                    this->m_token_error(*_token, "unexpected 'null' in expression");
                }
                continue;
            }

            if (_token->is_cp() || _token->is_mv()) {
                if (is_null_expr) {
                    if (is_suffix_expr) {
                        this->m_token_error(*_token, "unexpected 'cp' or 'mv' following postfix operator inside expression");
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
                    this->m_token_error(*_token, "unexpected 'cp' or 'mv' in expression");
                }
                continue;
            }

            if (_token->is_left_bracket()) {
                // (hello.world); (hello.world)5;
                //  type=LEFT_BRACKET
                //  left=hello.world
                //  right=5
                if (!(is_null_expr) && !expr->is_bracket()) {
                    this->m_token_error(*_token, "unexpected '(' inside expression");
                } else if (is_suffix_expr && is_null_expr) {
                    this->m_token_error(*_token, "unexpected '(' following postfix operator inside expression");
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
                        this->m_token_error(this->m_lexer->reverse_peek_token(), "expected ')' inside expression before end of file");
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
                        this->m_token_error(*_token, "unexpected '[' inside expression");
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
                        this->m_token_error(*_token, "unexpected ',' inside expression");
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
                        this->m_token_error(*_token,
                            "unexpected binary operator '" + std::string(_token->get_data()) + "' inside expression");
                    }
                } else {
                    if (expr->type == token::type::NULL_TOKEN) {
                        if (_token->is_strictly_suffix_operator()) {
                            if (!is_suffix_expr) {
                                this->m_token_error(*_token, "unexpected postfix operator '" + std::string(_token->get_data()) +
                                                             "' inside expression");
                            }
                        } else if (_token->is_strictly_prefix_operator() && is_suffix_expr) {
                            this->m_token_error(*_token, "unexpected prefix operator '" + std::string(_token->get_data()) +
                                                         "' following postfix operator inside expression");
                        }
                    } else {
                        if (_token->is_strictly_prefix_operator()) {
                            this->m_token_error(*_token,
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
                        this->m_token_error(*_token, "unexpected identifier in expression");
                    }
                } else if (is_suffix_expr) {
                    this->m_token_error(*_token, "unexpected identifier following postfix operator inside expression");
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
                                    this->m_token_error(*_token, "unexpected 'new' inside expression");
                                    goto end_new_check;
                                } else if (sub_expr.is_function_call()) {
                                    if (sub_expr.get_function_call_object()->is_array()) {
                                        this->m_token_error(*_token, "unexpected 'new' inside expression");
                                        goto end_new_check;
                                    }
                                }
                                i++;
                                cur_expr = &sub_expr;
                            }
                        }
                        if (cur_expr->type != lexing::token::type::IDENTIFIER && !cur_expr->is_array()) {
                            this->m_token_error(*_token, "unexpected 'new' inside expression");
                            break;
                        } else if (cur_expr->is_function_call()) {
                            if (cur_expr->get_function_call_object()->is_array()) {
                                this->m_token_error(*_token, "unexpected 'new' inside expression");
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
                    for (const token* expr_token = &this->m_lexer->current_token(); !expr_token->is_null_token(); expr_token = &this->m_lexer->next_token()) {
                        if (expr_token->is_dot()) {
                            // We keep last_type as identifier when doing function calls and array indexing expressions (check below)
                            if (last_type != lexing::token::type::IDENTIFIER) {
                                this->m_token_error(*expr_token, "unexpected '.' inside expression");
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
                                this->m_token_error(*expr_token,
                                    "unexpected '" + std::string(expr_token->get_data()) + "' inside expression");
                            }
                            last_type = lexing::token::type::IDENTIFIER;
                            continue;
                        }

                        if (expr_token->is_identifier()) {
                            if (expr_token->is_keyword()) {
                                this->m_token_error(*expr_token,
                                    "unexpected keyword '" + std::string(expr_token->get_data()) + "' inside expression");
                            } else if (last_type != lexing::token::type::DOT && last_type != token::type::NULL_TOKEN) {
                                this->m_token_error(*expr_token, "unexpected identifier '" + std::string(expr_token->get_data()) +
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
                                                    this->m_token_error(*sub_expr.begin,
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
                                        if (!right_function_call_bracket.is_null_token()) {
                                            this->m_token_error(right_function_call_bracket, "expected ')' inside expression");
                                        } else {
                                            this->m_token_error(this->m_lexer->reverse_peek_token(),
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
                                            this->m_token_error(*loop_expr.begin, "unexpected ',' inside array indexer inside expression");
                                        }
                                        const token& right_array_bracket = this->m_lexer->current_token();
                                        if (!right_array_bracket.is_right_square_bracket()) {
                                            this->m_token_error(this->m_lexer->reverse_peek_token(),
                                                "expected ']' inside expression before end of file");
                                        } else {
                                            if (parsed_indexer_expr.type == lexing::token::type::NULL_TOKEN) {
                                                this->m_token_error(this->m_lexer->reverse_peek_token(),
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

                        //this->m_token_error(*expr_token, "unexpected lexing::token '" + std::string(expr_token->get_data()) + "' inside expression");
                        this->m_lexer->reverse_token();
                        break;
                    }

                    if (last_type == lexing::token::type::DOT) {
                        if (!this->m_lexer->current_token().is_null_token()) {
                            this->m_token_error(this->m_lexer->current_token(), "unexpected '.' inside expression");
                        } else {
                            this->m_token_error(this->m_lexer->reverse_peek_token(), "unexpected '.' inside expression");
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
                        expr->end = this->m_lexer->get_index() + size_t(!this->m_lexer->current_token().is_null_token());
                    }
                }
                continue;
            }

            this->m_token_error(*_token, "unexpected lexing::token '" + std::string(_token->get_data()) + "' in expression");
        }

        while (expr->parent) {
            if (is_unary_operator(expr->parent->type)) {
                if ((expr->parent->has_left() && expr->parent->get_left()->type != lexing::token::type::NULL_TOKEN)
                    && (expr->parent->has_right() && expr->parent->get_right()->type != lexing::token::type::NULL_TOKEN)
                    && !is_binary_operator(expr->parent->type)) {
                    // Error if we have both a left and a right meanwhile this is strictly unary
                    this->m_token_error(*expr->parent->begin, "unexpected unary operator '" + std::string(expr->parent->begin->get_data()) +
                                                              "' inside expression");
                    break;
                } else if ((!expr->parent->has_left() || expr->parent->get_left()->type == lexing::token::type::NULL_TOKEN)
                           && (!expr->parent->has_right() || expr->parent->get_right()->type == lexing::token::type::NULL_TOKEN)) {
                    // Error if we dont have a left nor a right when this is meant to be a unary operator
                    this->m_token_error(*expr->parent->begin, "unexpected unary operator '" + std::string(expr->parent->begin->get_data()) +
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
                        this->m_token_error(*expr->parent->begin,
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
            this->m_token_error(this->m_lexer->reverse_peek_token(), "misplaced ',' inside expression");
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

    void parser::parse_access_specifier() {
        const token& current_token = this->m_lexer->current_token();
        const shift_mods mod = to_access_specifier(current_token);
        const shift_mods current_mods = this->m_get_mods();
        const shift_mods current_visibility_mods = (current_mods | mod) & visibility_modifiers;

        // Check to make sure only one visibility modifier is on at a time (power of 2)
        if ((current_visibility_mods & (current_visibility_mods - 1)) != 0x0) {
            // error if the one we have (i.e. the public, protected or private that is currently specified (the one being stored in current_mods)) is NOT the one now being parsed
            this->m_token_error(current_token, "unexpected visibility specifier");
        } else if ((current_mods & mod) == mod) {
            // send a warning if we are just adding the same modifier twice
            this->m_token_warning(current_token, "redundant '" + std::string(current_token.get_data()) + "' specifier");
        } else if ((mod & (shift_mods::IMUT | shift_mods::CONST_)) != 0x0) {
            switch (mod) {
                case shift_mods::IMUT:
                    if ((current_mods & shift_mods::CONST_) != 0x0) {
                        this->m_token_warning(current_token, "redundant 'imut' specifier");
                    } else {
                        this->m_add_mod(mod, current_token);
                    }
                    break;
                case shift_mods::CONST_:
                    if ((current_mods & shift_mods::IMUT) != 0x0) {
                        auto const& [mod_, token_] = *std::find_if(this->m_mods.rbegin(), this->m_mods.rend(),
                            [](const auto& mod) { return mod.first == shift_mods::IMUT; });
                        this->m_token_warning(*token_, "redundant 'imut' specifier");
                    } else {
                        this->m_add_mod(mod, current_token);
                    }
                    break;
                default:
                    break;
            }
        } else {
            this->m_add_mod(mod, current_token);
        }
    }

    shift_mods parser::get_mods() const noexcept {
        shift_mods mods = static_cast<shift_mods>(0x0);

        for (const auto& [mod, token_] : this->m_mods) {
            mods |= mod;
        }

        return mods;
    }

    void parser::add_mod(shift_mods mod, const token& token_) noexcept {
        this->m_mods.push_back({ mod, &token_ });
    }

    void parser::clear_mods() noexcept {
        return this->m_mods.clear();
    }

    const token& parser::skip_until(const std::string_view str) noexcept {
        for (; !this->m_lexer->current_token().is_null_token() && this->m_lexer->current_token().get_data() != str;
               this->m_lexer->next_token());
        return this->m_lexer->current_token();
    }

    const token& parser::skip_until(const std::string& str) noexcept { return m_skip_until(std::string_view(str.data(), str.length())); }

    const token& parser::skip_until(const char* const str) noexcept { return m_skip_until(std::string_view(str, std::strlen(str))); }

    const token& parser::skip_until(const typename token::type type) noexcept {
        for (; !this->m_lexer->current_token().is_null_token() && this->m_lexer->current_token().get_token_type() != type;
               this->m_lexer->next_token());
        return this->m_lexer->current_token();
    }

    const token& parser::skip_after(const std::string_view str) noexcept {
        m_skip_until(str);
        return this->m_lexer->next_token();
    }

    const token& parser::skip_after(const std::string& str) noexcept { return m_skip_after(std::string_view(str.data(), str.length())); }

    const token& parser::skip_after(const char* const str) noexcept { return m_skip_after(std::string_view(str, std::strlen(str))); }

    const token& parser::skip_after(const typename token::type type) noexcept {
        m_skip_until(type);
        return this->m_lexer->next_token();
    }

    const token& parser::skip_until_closing(const typename token::type bracket_type) noexcept {
        if (bracket_type != lexing::token::type::LEFT_BRACKET && bracket_type != lexing::token::type::LEFT_SQUARE_BRACKET &&
            bracket_type != lexing::token::type::LEFT_SCOPE_BRACKET)
            return m_skip_until(bracket_type);

        const lexing::token::type look = lexing::token::type(std::underlying_type_t<token_type>(bracket_type) + 1);

        size_t count = 1;
        for (const token* _token = &this->m_lexer->current_token();
             !_token->is_null_token() && count > 0; _token = &this->m_lexer->next_token()) {
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

            this->m_token_error(this->m_lexer->reverse_peek_token(), msg);
        }

        return this->m_lexer->current_token().is_null_token() ? this->m_lexer->current_token() : this->m_lexer->reverse_token();
    }

    const token& parser::skip_before(const std::string_view str) noexcept {
        skip_until(str);
        return this->m_lexer->reverse_token();
    }

    const token& parser::skip_before(const std::string& str) noexcept {
        return skip_before(std::string_view(str.data(), str.length()));
    }

    const token& parser::skip_before(const char* const str) noexcept { return skip_before(std::string_view(str, std::strlen(str))); }

    const token& parser::skip_before(const typename token::type type) noexcept {
        skip_until(type);
        return this->m_lexer->reverse_token();
    }

    std::string parser::token_message_header(const std::string_view type, const token& tok) {
        std::string err;
        lexing::file_position file_pos = tok.get_file_position();
        err += type;
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
        // TODO Change underline algorithm if tokens are able to take up more than one line (e.g. multi-line string)
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
        if (!this->m_error_handler) return;

        std::string err = token_message_header("error", tok);

        err += msg;
        err += '\n';
        err += token_underline(tok);

        m_error_handler->add_error(std::move(err));
    }

    void parser::token_error(const token& token_, const std::string& msg) {
        return token_error(token_, std::string_view{ msg });
    }

    void parser::token_error(const token& token_, const char* const msg) {
        return token_error(token_, std::string_view{ msg });
    }

    void parser::token_warning(const token& tok, const std::string_view msg) {
        if (!this->m_error_handler) return;
        if (!this->m_error_handler->is_print_warnings()) return;

        std::string err = token_message_header("warning", tok);

        err += msg;
        err += '\n';
        err += token_underline(tok);

        m_error_handler->add_warning(std::move(err));
    }

    void parser::token_warning(const token& token_, const std::string& msg) {
        return token_warning(token_, std::string_view{ msg });
    }

    void parser::token_warning(const token& token_, const char* const msg) {
        return token_warning(token_, std::string_view{ msg });
    }

    std::string_view parser::get_line(const token& token_) const noexcept {
        return this->m_lexer->get_lines()[token_.get_file_position().line - 1];
    }

    bool parser::is_module_defined() const noexcept { return this->m_module.get() && this->m_module->depth() > 0; }

    SHIFT_API uint_fast8_t parser::operator_priority(const lexing::token::type type, const bool prefix) noexcept {
        constexpr uint_fast8_t base_priority = 0x10;
        constexpr uint_fast8_t prefix_priority = 0xf0 - base_priority;
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

    static constexpr shift_mods to_access_specifier(const token& token) noexcept {
        if (token.is_public()) {
            return shift_mods::PUBLIC;
        } else if (token.is_protected()) {
            return shift_mods::PROTECTED;
        } else if (token.is_private()) {
            return shift_mods::PRIVATE;
        } else if (token.is_static()) {
            return shift_mods::STATIC;
        } else if (token.is_const()) {
            return shift_mods::CONST_;
        } else if (token.is_extern()) {
            return shift_mods::EXTERN;
        } else if (token.is_binary()) {
            return shift_mods::BINARY;
        } else if (token.is_explicit()) {
            return shift_mods::EXPLICIT;
        } else if (token.is_imut()) {
            return shift_mods::IMUT;
        }

        return static_cast<shift_mods>(0x0);
    }

}