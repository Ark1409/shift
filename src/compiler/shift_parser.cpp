/**
 * @file compiler/shift_parser.cpp
 */
#include "compiler/shift_parser.h"

#include <cstring>
#include <algorithm>

#define SHIFT_PARSER_ERROR_PREFIX 				"error: " << std::filesystem::relative(this->m_tokenizer->get_file().get_path()).string() << ": " // std::filesystem::relative call every time probably isn't that optimal
#define SHIFT_PARSER_WARNING_PREFIX 			"warning: " << std::filesystem::relative(this->m_tokenizer->get_file().get_path()).string() << ": " // std::filesystem::relative call every time probably isn't that optimal

#define SHIFT_PARSER_ERROR_PREFIX_EXT_(__line__, __col__) "error: " << std::filesystem::relative(this->m_tokenizer->get_file().get_path()).string() << ":" << __line__ << ":" << __col__ << ": " // std::filesystem::relative call every time probably isn't that optimal
#define SHIFT_PARSER_WARNING_PREFIX_EXT_(__line__, __col__) "warning: " << std::filesystem::relative(this->m_tokenizer->get_file().get_path()).string() << ":" << __line__ << ":" << __col__ << ": " // std::filesystem::relative call every time probably isn't that optimal

#define SHIFT_PARSER_ERROR_PREFIX_EXT(__token) SHIFT_PARSER_ERROR_PREFIX_EXT_((__token).get_file_index().line, (__token).get_file_index().col)
#define SHIFT_PARSER_WARNING_PREFIX_EXT(__token) SHIFT_PARSER_WARNING_PREFIX_EXT_((__token).get_file_index().line, (__token).get_file_index().col)

#define SHIFT_PARSER_PRINT() this->m_error_handler->print_exit_clear()

#define SHIFT_PARSER_WARNING(__WARN__) 			this->m_error_handler->stream() << SHIFT_PARSER_WARNING_PREFIX << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_PARSER_FATAL_WARNING(__WARN__) 		SHIFT_PARSER_WARNING(__WARN__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_WARNING_LOG(__WARN__) 		this->m_error_handler->stream() << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_PARSER_FATAL_WARNING_LOG(__WARN__)  SHIFT_PARSER_WARNING_LOG(__WARN__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_ERROR(__ERR__) 			this->m_error_handler->stream() << SHIFT_PARSER_ERROR_PREFIX << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_PARSER_FATAL_ERROR(__ERR__) 		SHIFT_PARSER_ERROR(__ERR__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_ERROR_LOG(__ERR__) 		this->m_error_handler->stream() << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_PARSER_FATAL_ERROR_LOG(__ERR__)  SHIFT_PARSER_ERROR_LOG(__ERR__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_WARNING_(__token, __WARN__) 	this->m_error_handler->stream() << SHIFT_PARSER_WARNING_PREFIX_EXT(__token) << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_PARSER_FATAL_WARNING_(__token, __WARN__) 		SHIFT_PARSER_WARNING(__token, __WARN__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_WARNING_LOG_(__WARN__) 		this->m_error_handler->stream() << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_PARSER_FATAL_WARNING_LOG_(__WARN__)  SHIFT_PARSER_WARNING_LOG_(__WARN__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_ERROR_(__token, __ERR__) 			this->m_error_handler->stream() << SHIFT_PARSER_ERROR_PREFIX_EXT(__token) << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_PARSER_FATAL_ERROR_(__token, __ERR__) 		SHIFT_PARSER_ERROR_(__token, __ERR__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_ERROR_LOG_( __ERR__) 		this->m_error_handler->stream() << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_PARSER_FATAL_ERROR_LOG_( __ERR__)  SHIFT_PARSER_ERROR_LOG_(__ERR__); SHIFT_PARSER_PRINT()

namespace shift {
    namespace compiler {
        using token_type = token::token_type;

        static constexpr inline bool is_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_overload_operator(); }
        static constexpr inline bool is_binary_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_binary_operator(); }
        static constexpr inline bool is_unary_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_unary_operator(); }
        static constexpr inline bool is_prefix_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_prefix_overload_operator(); }
        static constexpr inline bool is_suffix_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_suffix_overload_operator(); }
        static constexpr inline bool is_strictly_prefix_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_strictly_prefix_overload_operator(); }
        static constexpr inline bool is_strictly_suffix_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_strictly_suffix_overload_operator(); }
        static constexpr parser::mods to_access_specifier(const token& token) noexcept;

        static constexpr parser::mods visibility_modifiers = parser::mods::PUBLIC | parser::mods::PROTECTED | parser::mods::PRIVATE;
        static constexpr parser::mods class_modifiers = visibility_modifiers | parser::mods::STATIC;
        static constexpr parser::mods function_modifiers = visibility_modifiers | parser::mods::STATIC | parser::mods::EXTERN;
        static constexpr parser::mods type_modifiers = parser::mods::CONST_;
        static constexpr parser::mods constructor_modifiers = visibility_modifiers;
        static constexpr parser::mods variable_modifiers = visibility_modifiers | parser::mods::STATIC | parser::mods::CONST_ | parser::mods::EXTERN;

        parser& parser::operator=(const parser& p) {
            this->m_tokenizer = p.m_tokenizer;
            this->m_error_handler = p.m_error_handler;
            this->m_mods = p.m_mods;
            this->m_module = p.m_module;
            this->m_global_uses = p.m_global_uses;
            this->m_classes = p.m_classes;
            for (auto& clazz : m_classes) {
                clazz.module_ = &m_module;
            }
            return *this;
        }

        parser& parser::operator=(parser&& p) noexcept {
            this->m_tokenizer = p.m_tokenizer;
            this->m_error_handler = p.m_error_handler;
            this->m_mods = std::move(p.m_mods);
            this->m_module = std::move(p.m_module);
            this->m_global_uses = std::move(p.m_global_uses);
            this->m_classes = std::move(p.m_classes);
            for (auto& clazz : m_classes) {
                clazz.module_ = &m_module;
            }
            return *this;
        }

        void parser::parse() {
            for (const token* current = &this->m_tokenizer->current_token(); !current->is_null_token(); current = &this->m_tokenizer->next_token()) {
                if (current->is_use()) {
                    // use statement
                    m_parse_use();
                    continue;
                }

                if (current->is_class()) {
                    // creating class
                    m_parse_class();
                    continue;
                }

                if (current->is_access_specifier()) {
                    m_parse_access_specifier();
                    continue;
                }

                if (current->is_module()) {
                    if (!this->m_is_module_defined()) {
                        // module statement; expected the least (only once)
                        m_parse_module();
                    } else {
                        this->m_token_error(*current, "module already defined");
                        this->m_skip_until(token::token_type::SEMICOLON);
                    }
                    continue;
                }

                this->m_token_error(*current, "unexpected token '" + std::string(current->get_data()) + "'");
            }

            if (this->m_mods.size() > 0) {
                const auto& [mod, token_] = this->m_mods.front();
                this->m_token_error(*token_, "unexpected '" + std::string(token_->get_data()) + "' specifier");
            }
        }

        void parser::m_parse_class(void) {
            const token& class_token = this->m_tokenizer->current_token();
            if (!class_token.is_class()) {
                this->m_token_error(class_token, "expected 'class'");
                return;
            }

            if (!this->m_is_module_defined()) {
                this->m_token_error(class_token, "module must first be defined");
            }

            m_classes.emplace_back();
            shift_class& clazz = m_classes.back();

            clazz.implicit_use_statements = this->m_global_uses.size();
            clazz.module_ = &this->m_module;

            for (const token* access_specifier_token = &this->m_tokenizer->next_token(); access_specifier_token->is_access_specifier(); access_specifier_token = &this->m_tokenizer->next_token()) {
                this->m_parse_access_specifier();
            }

            for (const auto& [mod, token_] : this->m_mods) {
                if ((mod & class_modifiers) == 0) {
                    this->m_token_error(*token_, "unexpected '" + std::string(token_->get_data()) + "' specifier in class declaration");
                } else {
                    clazz.mods |= mod;
                }
            }
            this->m_clear_mods();

            clazz.name = &this->m_tokenizer->current_token();

            if (!clazz.name->is_identifier()) {
                if (!clazz.name->is_null_token()) {
                    this->m_token_error(*clazz.name, "expected identifier for class name");
                    this->m_skip_before(token::token_type::LEFT_SCOPE_BRACKET);
                } else {
                    this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected valid class name before end of file");
                    return;
                }
            } else if (clazz.name->is_keyword()) {
                this->m_token_error(*clazz.name, "invalid class name '" + std::string(clazz.name->get_data()) + "'");
                this->m_skip_before(token::token_type::LEFT_SCOPE_BRACKET);
            }

            const token& left_bracket = this->m_tokenizer->next_token();

            if (!left_bracket.is_left_scope_bracket()) {
                if (!left_bracket.is_null_token()) {
                    this->m_token_error(left_bracket, "expected '{' after class declaration");
                } else {
                    this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '{' after class declaration before end of file");
                }
            }

            this->m_tokenizer->next_token(); // Move onto first token inside class body

            // Parse class body
            this->m_parse_class(clazz);

            const token& right_bracket = this->m_tokenizer->current_token();

            if (!right_bracket.is_right_scope_bracket()) {
                if (!right_bracket.is_null_token()) {
                    this->m_token_error(right_bracket, "expected '}' after class declaration");
                } else {
                    this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '}' after class declaration before end of file");
                }
            }

            if (this->m_mods.size() > 0) {
                const auto& [mod, token_] = this->m_mods.front();
                this->m_token_error(*token_, "unexpected '" + std::string(token_->get_data()) + "' specifier inside class");
                this->m_clear_mods();
            }
        }

        void parser::m_parse_class(shift_class& clazz) {
            for (const token* token_ = &this->m_tokenizer->current_token(); !token_->is_null_token(); token_ = &this->m_tokenizer->next_token()) {
                if (token_->is_access_specifier()) {
                    this->m_parse_access_specifier();
                    continue;
                }

                if (token_->is_use()) {
                    m_parse_use(clazz.use_statements);
                    continue;
                }

                if (token_->is_constructor()) {
                    // parse constructor
                    shift_function func;
                    func.name = token_;
                    func.clazz = &clazz;

                    for (const auto& [mod, token_] : this->m_mods) {
                        if ((mod & constructor_modifiers) == 0x0) {
                            this->m_token_error(*token_, "unexpected '" + std::string(token_->get_data()) + "' specifier in constructor declaration");
                        } else {
                            func.mods |= mod;
                        }
                    }
                    this->m_clear_mods();

                    const token& next_token = this->m_tokenizer->next_token();

                    if (!next_token.is_left_bracket()) {
                        if (!next_token.is_null_token()) {
                            this->m_token_error(next_token, "expected '(' after class constructor declaration");
                            this->m_skip_until(token::token_type::LEFT_BRACKET);
                        } else {
                            this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '(' after class constructor declaration before end of file");
                        }
                    }

                    // parse constructor parameters
                    for (this->m_tokenizer->next_token(); !this->m_tokenizer->current_token().is_null_token() && !this->m_tokenizer->current_token().is_right_bracket(); this->m_tokenizer->next_token()) {
                        shift_type param_type = m_parse_type("constructor parameter");
                        const token& param_name = this->m_tokenizer->current_token();

                        if (param_type.name.size() == 0) {
                            this->m_token_error(*param_type.name.begin, "expected parameter type in constructor parameter list, got '" + std::string(param_type.name.begin->get_data()) + "'");
                        }

                        if (param_name.is_comma() || param_name.is_right_bracket()) {
                            // nameless parameters
                            func.parameters.push_back({ std::move(param_type), &token::null });

                            if (param_name.is_right_bracket())
                                break;

                            continue;
                        }

                        if (!param_name.is_identifier()) {
                            if (!param_name.is_null_token()) {
                                this->m_token_error(param_name, "expected identifier for constructor parameter name");
                            } else {
                                this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected identifier for constructor parameter name before end of file");
                            }
                        } else if (param_name.is_keyword()) {
                            this->m_token_error(param_name, "'" + std::string(param_name.get_data()) + "' is not a valid constructor parameter name");
                        }

                        func.parameters.push_back({ std::move(param_type), &param_name });

                        const token& after_param_name = this->m_tokenizer->next_token();
                        if (!after_param_name.is_comma()) {
                            if (!after_param_name.is_right_bracket()) {
                                if (!after_param_name.is_null_token()) {
                                    this->m_token_error(after_param_name, "expected ',' or ')' in constructor parameter list");
                                } else {
                                    this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected ',' or ')' in constructor parameter list before end of file");
                                }
                            } else break;
                        }
                    }

                    const token& function_right_parameter_bracket = this->m_tokenizer->current_token();

                    if (!function_right_parameter_bracket.is_right_bracket()) {
                        if (!function_right_parameter_bracket.is_null_token()) {
                            this->m_token_error(function_right_parameter_bracket, "expected ')' at end of constructor parameter list");
                            this->m_skip_until(token::token_type::RIGHT_BRACKET);
                        } else {
                            this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected ')' at end of constructor parameter list before end of file");
                        }
                    } else if (this->m_tokenizer->reverse_peek_token().is_comma()) {
                        this->m_token_error(this->m_tokenizer->reverse_peek_token(), "misplaced ',' in constructor parameter list");
                    }

                    const token& function_left_bracket = this->m_tokenizer->next_token();

                    if (!function_left_bracket.is_left_scope_bracket()) {
                        if (!function_left_bracket.is_null_token()) {
                            this->m_token_error(function_left_bracket, "expected '{' after class constructor declaration");
                            this->m_skip_until(token::token_type::LEFT_SCOPE_BRACKET);
                        } else {
                            this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '{' after class constructor declaration before end of file");
                        }
                    }

                    this->m_tokenizer->next_token(); // Move onto first token inside constructor body

                    // Parse constructor body
                    m_parse_function(func);

                    const token& function_right_bracket = this->m_tokenizer->current_token();

                    if (!function_right_bracket.is_right_scope_bracket()) {
                        if (!function_right_bracket.is_null_token()) {
                            this->m_token_error(function_right_bracket, "expected '}' after class constructor declaration");
                            this->m_skip_until(token::token_type::RIGHT_SCOPE_BRACKET);
                        } else {
                            this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '}' after class constructor declaration before end of file");
                        }
                    }

                    clazz.functions.push_back(std::move(func));
                    continue;
                }

                if (token_->is_destructor()) {
                    // parse destructor
                    continue;
                }

                if (token_->is_class()) {
                    // TODO support for classes inside of classes
                    continue;
                }

                if (token_->is_identifier()) {
                    // It must be either a variable declaration or a function declaration
                    shift_type type;

                    if (token_->is_void()) {
                        type.name.begin = this->m_tokenizer->get_index();
                        type.name.end = type.name.begin + 1;
                        this->m_tokenizer->next_token();
                    } else {
                        type = m_parse_type("variable or function type");
                        if (type.name.size() == 0) {
                            this->m_token_error(*type.name.begin, "expected variable or function type");
                        }
                    }

                    const token* name = &this->m_tokenizer->current_token();

                    for (; name->is_access_specifier(); name = &this->m_tokenizer->next_token()) {
                        this->m_parse_access_specifier();
                    }

                    if (!name->is_identifier()) {
                        if (!name->is_null_token()) {
                            this->m_token_error(*name, "expected identifier for variable or function name");
                            name = &this->m_tokenizer->current_token();
                        } else {
                            this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected identifier for variable or function name before end of file");
                        }
                    } else if (name->is_keyword()) {
                        if (name->is_constructor() && type.name.size() != 0) {
                            this->m_token_error(*type.name.begin, "class constructor cannot have return type");
                        } else {
                            this->m_token_error(*name, "'" + std::string(name->get_data()) + "' is not a valid variable or function name");
                        }
                        name = &this->m_tokenizer->current_token();
                    }

                    const token& next_token = this->m_tokenizer->next_token();
                    if (next_token.is_left_bracket()) {
                        // function definition
                        shift_function func;
                        func.name = name;
                        func.clazz = &clazz;
                        func.return_type = std::move(type);

                        for (const auto& [mod, token_] : this->m_mods) {
                            if ((mod & function_modifiers) == 0x0) {
                                if ((mod & type_modifiers) != 0) {
                                    if (func.return_type.name.size() != 0 && func.return_type.name.begin->is_void()) {
                                        this->m_token_error(*token_, "void returning function cannot have '" + std::string(token_->get_data()) + "' specifier");
                                    } else {
                                        func.return_type.mods |= mod;
                                    }

                                } else {
                                    this->m_token_error(*token_, "unexpected '" + std::string(token_->get_data()) + "' specifier in function declaration");
                                }

                            } else {
                                func.mods |= mod;
                            }
                        }
                        this->m_clear_mods();

                        // parse function parameters
                        for (this->m_tokenizer->next_token(); !this->m_tokenizer->current_token().is_null_token() && !this->m_tokenizer->current_token().is_right_bracket(); this->m_tokenizer->next_token()) {
                            shift_type param_type = m_parse_type("function parameter");
                            const token& param_name = this->m_tokenizer->current_token();

                            if (param_type.name.size() == 0) {
                                this->m_token_error(*param_type.name.begin, "expected parameter type in function parameter list, got '" + std::string(param_type.name.begin->get_data()) + "'");
                            }

                            if (param_name.is_comma() || param_name.is_right_bracket()) {
                                // nameless parameters
                                func.parameters.push_back({ std::move(param_type), &token::null });

                                if (param_name.is_right_bracket())
                                    break;

                                continue;
                            }

                            if (!param_name.is_identifier()) {
                                if (!param_name.is_null_token()) {
                                    this->m_token_error(param_name, "expected identifier for function parameter name");
                                } else {
                                    this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected identifier for function parameter name before end of file");
                                }
                            } else if (param_name.is_keyword()) {
                                this->m_token_error(param_name, "'" + std::string(param_name.get_data()) + "' is not a valid function parameter name");
                            }

                            func.parameters.push_back({ std::move(param_type), &param_name });

                            const token& after_param_name = this->m_tokenizer->next_token();
                            if (!after_param_name.is_comma()) {
                                if (!after_param_name.is_right_bracket()) {
                                    if (!after_param_name.is_null_token()) {
                                        this->m_token_error(after_param_name, "expected ',' or ')' in function parameter list");
                                    } else {
                                        this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected ',' or ')' in function parameter list before end of file");
                                    }
                                } else break;
                            }
                        }

                        const token& function_right_parameter_bracket = this->m_tokenizer->current_token();

                        if (!function_right_parameter_bracket.is_right_bracket()) {
                            if (!function_right_parameter_bracket.is_null_token()) {
                                this->m_token_error(function_right_parameter_bracket, "expected ')' at end of function parameter list");
                            } else {
                                this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected ')' at end of function parameter list before end of file");
                            }
                        } else if (this->m_tokenizer->reverse_peek_token().is_comma()) {
                            this->m_token_error(this->m_tokenizer->reverse_peek_token(), "misplaced ',' in function parameter list");
                        }


                        const token& function_left_bracket = this->m_tokenizer->next_token();

                        if (!function_left_bracket.is_left_scope_bracket()) {
                            if (!function_left_bracket.is_null_token()) {
                                this->m_token_error(function_left_bracket, "expected '{' after function declaration");
                            } else {
                                this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '{' after function declaration before end of file");
                            }
                        }

                        this->m_tokenizer->next_token(); // Move onto first token inside function body

                        // Parse function body
                        m_parse_function(func);

                        const token& function_right_bracket = this->m_tokenizer->current_token();

                        if (!function_right_bracket.is_right_scope_bracket()) {
                            if (!function_right_bracket.is_null_token()) {
                                this->m_token_error(function_right_bracket, "expected '}' after function declaration");
                            } else {
                                this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '}' after function declaration before end of file");
                            }
                        }

                        clazz.functions.push_back(std::move(func));
                    } else if (next_token.is_semicolon() || next_token.is_binary_operator() || next_token.is_unary_operator()) {
                        if (token_->is_void()) {
                            this->m_token_error(*token_, "'void' is not a valid variable type");
                        }

                        // variable declaration
                        shift_variable variable;
                        variable.name = name;
                        variable.type = std::move(type);
                        variable.clazz = &clazz;

                        for (const auto& [mod, token_] : this->m_mods) {
                            if ((mod & variable_modifiers) == 0x0) {
                                this->m_token_error(*token_, "invalid '" + std::string(token_->get_data()) + "' specifier on variable");
                            } else {
                                variable.type.mods |= mod;
                            }
                        }
                        this->m_clear_mods();

                        if (next_token.is_binary_operator() || next_token.is_unary_operator()) {
                            if (!next_token.is_equals()) {
                                this->m_token_error(next_token, "expected ';' or '=' for variable declaration, got '" + std::string(next_token.get_data()) + "'");
                            }

                            // variable definition
                            const token& first_expr_token = this->m_tokenizer->next_token(); // Move onto the expression
                            variable.value = m_parse_expression();

                            if (variable.value.type == token::token_type::NULL_TOKEN) {
                                this->m_token_error(first_expr_token, "expected valid expression for variable assignment value");
                            }
                        }

                        clazz.variables.push_back(std::move(variable));
                    } else {
                        this->m_token_error(next_token, "expected either variable or function declaration");
                    }
                    continue;
                }

                if (token_->is_right_scope_bracket()) break;
            }
        }

        void parser::m_parse_function(shift_function& func) {
            return m_parse_function_block(func, func.statements);
        }

        void parser::m_parse_function_block(shift_function& func, std::list<shift_statement>& statements, size_t count) {
            for (const token* _token = &this->m_tokenizer->current_token(); count != 0 && !_token->is_null_token(); _token = &this->m_tokenizer->next_token(), count--) {
                shift_statement statement;

                if (_token->is_access_specifier()) {
                    this->m_parse_access_specifier();
                    continue;
                }

                else if (_token->is_use()) {
                    statement.set_use(_token);
                    m_parse_use(this->m_global_uses);
                    shift_module& module_ = this->m_global_uses.back();
                    statement.set_use_module(std::move(module_));
                    this->m_global_uses.pop_back();
                }

                else if (_token->is_if()) {
                    if (this->m_mods.size() != 0) {
                        const auto& [mod, token_] = this->m_mods.front();
                        this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                        this->m_clear_mods();
                    }
                    statement.set_if(_token);

                    const token& left_condition_bracket = this->m_tokenizer->next_token();
                    if (!left_condition_bracket.is_left_bracket()) {
                        if (!left_condition_bracket.is_null_token()) {
                            this->m_token_error(left_condition_bracket, "expected '(' after 'if' inside function body");
                            this->m_skip_until(token::token_type::LEFT_BRACKET);
                        } else {
                            this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '(' after 'if' inside function body before end of file");
                        }
                    }

                    this->m_tokenizer->next_token(); // Move onto condition token
                    statement.set_if_condition(m_parse_expression(token::token_type::RIGHT_BRACKET));

                    const token& right_condition_bracket = this->m_tokenizer->current_token();

                    if (!right_condition_bracket.is_right_bracket()) {
                        this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected ')' after 'if' condition inside function body before end of file");
                        break;
                    }

                    const token& left_if_bracket = this->m_tokenizer->next_token();
                    if (left_if_bracket.is_left_scope_bracket()) {
                        this->m_tokenizer->next_token(); // Skip {
                        m_parse_function_block(func, statement.get_if_statements());

                        const token& right_if_bracket = this->m_tokenizer->current_token();
                        if (!right_if_bracket.is_right_scope_bracket()) {
                            if (!right_if_bracket.is_null_token()) {
                                this->m_token_error(right_if_bracket, "expected '}' to close 'if' declaration inside function body");
                                this->m_skip_until(token::token_type::RIGHT_SCOPE_BRACKET);
                            } else {
                                this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '}' to close 'if' declaration inside function body before end of file");
                            }
                        }
                    } else {
                        m_parse_function_block(func, statement.get_if_statements(), 1);
                        if (statement.get_if_statements().size() == 0) {
                            this->m_token_error(this->m_tokenizer->current_token(), "expected valid statement after 'if' declaration");
                        }
                        this->m_tokenizer->reverse_token(); // tokenizer will be on token after last statement token if count causes it to end
                    }
                }

                else if (_token->is_else()) {
                    if (this->m_mods.size() != 0) {
                        const auto& [mod, token_] = this->m_mods.front();
                        this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                        this->m_clear_mods();
                    }
                    if (statements.size() == 0 || statements.back().type != shift_statement::statement_type::if_) {
                        this->m_token_error(*_token, "unexpected 'else' statement in function body");
                    }

                    statement.set_else(_token);

                    const token& left_else_bracket = this->m_tokenizer->next_token();
                    if (left_else_bracket.is_left_scope_bracket()) {
                        this->m_tokenizer->next_token(); // Skip {
                        m_parse_function_block(func, statement.get_else_statements());

                        const token& right_else_bracket = this->m_tokenizer->current_token();
                        if (!right_else_bracket.is_right_scope_bracket()) {
                            if (!right_else_bracket.is_null_token()) {
                                this->m_token_error(right_else_bracket, "expected '}' to close 'else' declaration inside function body");
                                this->m_skip_until(token::token_type::RIGHT_SCOPE_BRACKET);
                            } else {
                                this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '}' to close 'else' declaration inside function body before end of file");
                            }
                        }
                    } else {
                        m_parse_function_block(func, statement.get_else_statements(), 1);
                        if (statement.get_else_statements().size() == 0) {
                            this->m_token_error(this->m_tokenizer->current_token(), "expected valid statement after 'else' declaration");
                        }
                        this->m_tokenizer->reverse_token(); // tokenizer will be on token after last statement token if count causes it to end
                    }
                }

                else if (_token->is_while()) {
                    if (this->m_mods.size() != 0) {
                        const auto& [mod, token_] = this->m_mods.front();
                        this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                        this->m_clear_mods();
                    }
                    statement.set_while(_token);

                    const token& left_condition_bracket = this->m_tokenizer->next_token();
                    if (!left_condition_bracket.is_left_bracket()) {
                        if (!left_condition_bracket.is_null_token()) {
                            this->m_token_error(left_condition_bracket, "expected '(' after 'while' inside function body");
                            this->m_skip_until(token::token_type::LEFT_BRACKET);
                        } else {
                            this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '(' after 'while' inside function body before end of file");
                        }
                    }

                    this->m_tokenizer->next_token(); // Move onto condition token
                    statement.set_while_condition(m_parse_expression(token::token_type::RIGHT_BRACKET));

                    const token& right_condition_bracket = this->m_tokenizer->current_token();

                    if (!right_condition_bracket.is_right_bracket()) {
                        this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected ')' after 'while' condition inside function body before end of file");
                        break;
                    }

                    const token& left_while_bracket = this->m_tokenizer->next_token();
                    if (left_while_bracket.is_left_scope_bracket()) {
                        this->m_tokenizer->next_token(); // Skip {
                        m_parse_function_block(func, statement.get_while_statements());

                        const token& right_while_bracket = this->m_tokenizer->current_token();
                        if (!right_while_bracket.is_right_scope_bracket()) {
                            if (!right_while_bracket.is_null_token()) {
                                this->m_token_error(right_while_bracket, "expected '}' to close 'while' declaration inside function body");
                                this->m_skip_until(token::token_type::RIGHT_SCOPE_BRACKET);
                            } else {
                                this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '}' to close 'while' declaration inside function body before end of file");
                            }
                        }
                    } else {
                        m_parse_function_block(func, statement.get_while_statements(), 1);
                        if (statement.get_while_statements().size() == 0) {
                            this->m_token_error(this->m_tokenizer->current_token(), "expected valid statement after 'while' declaration");
                        }
                        this->m_tokenizer->reverse_token(); // tokenizer will be on token after last statement token if count causes it to end
                    }
                }

                else if (_token->is_for()) {
                    if (this->m_mods.size() != 0) {
                        const auto& [mod, token_] = this->m_mods.front();
                        this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                        this->m_clear_mods();
                    }
                    statement.set_for(_token);

                    const token& left_conditions_bracket = this->m_tokenizer->next_token();
                    if (!left_conditions_bracket.is_left_bracket()) {
                        if (!left_conditions_bracket.is_null_token()) {
                            this->m_token_error(left_conditions_bracket, "expected '(' after 'for' inside function body");
                            this->m_skip_until(token::token_type::LEFT_BRACKET);
                        } else {
                            this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '(' after 'for' inside function body before end of file");
                        }
                    }

                    {
                        this->m_tokenizer->next_token(); // Move onto statement token

                        std::list<shift_statement> temp_statement;
                        m_parse_function_block(func, temp_statement, 1);
                        if (temp_statement.size() > 0)
                            // TODO add statement type checking
                            statement.set_for_initializer(std::move(temp_statement.front()));
                    }

                    {
                        // tokenizer will be on token after last statement token if count causes it to end; no need for next_token
                        statement.set_for_condition(m_parse_expression());
                    }

                    {
                        this->m_tokenizer->next_token(); // Move onto condition token

                        statement.set_for_increment(m_parse_expression(token::token_type::RIGHT_BRACKET));

                        const token& right_condition_bracket = this->m_tokenizer->current_token();

                        if (!right_condition_bracket.is_right_bracket()) {
                            this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected ')' after 'for' increment inside function body before end of file");
                            break;
                        }
                    }

                    const token& left_for_bracket = this->m_tokenizer->next_token();
                    if (left_for_bracket.is_left_scope_bracket()) {
                        this->m_tokenizer->next_token(); // Skip {
                        m_parse_function_block(func, statement.get_for_statements());

                        const token& right_for_bracket = this->m_tokenizer->current_token();
                        if (!right_for_bracket.is_right_scope_bracket()) {
                            if (!right_for_bracket.is_null_token()) {
                                this->m_token_error(right_for_bracket, "expected '}' to close 'for' declaration inside function body");
                                this->m_skip_until(token::token_type::RIGHT_SCOPE_BRACKET);
                            } else {
                                this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '}' to close 'for' declaration inside function body before end of file");
                            }
                        }
                    } else {
                        m_parse_function_block(func, statement.get_for_statements(), 1);
                        if (statement.get_else_statements().size() == 0) {
                            this->m_token_error(this->m_tokenizer->current_token(), "expected valid statement after 'for' declaration");
                        }
                        this->m_tokenizer->reverse_token(); // tokenizer will be on token after last statement token if count causes it to end
                    }
                }

                else if (_token->is_return()) {
                    if (this->m_mods.size() != 0) {
                        const auto& [mod, token_] = this->m_mods.front();
                        this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                        this->m_clear_mods();
                    }
                    statement.set_return(_token);
                    this->m_tokenizer->next_token(); // skip 'return'
                    statement.set_return_statement(m_parse_expression());
                }

                else if (_token->is_continue() || _token->is_break()) {
                    if (this->m_mods.size() != 0) {
                        const auto& [mod, token_] = this->m_mods.front();
                        this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                        this->m_clear_mods();
                    }
                    if (_token->is_continue())
                        statement.set_continue(_token);
                    else
                        statement.set_break(_token);

                    const token& semi_colon = this->m_tokenizer->next_token();

                    if (!semi_colon.is_semicolon()) {
                        if (!semi_colon.is_null_token()) {
                            this->m_token_error(semi_colon, "expected ';' after '" + std::string(_token->get_data()) + "' in function body");
                            this->m_skip_until(token::token_type::SEMICOLON);
                        } else {
                            this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected ';' after '" + std::string(_token->get_data()) + "' in function body before end of file");
                        }
                    }
                }

                else if (_token->is_identifier() && !_token->is_new()) {
                    // must be either variable creation or normal expression
                    this->m_tokenizer->mark();
                    if (this->m_error_handler)
                        this->m_error_handler->mark();
                    shift_type type = m_parse_type("expression type");

                    const token& after_type = this->m_tokenizer->current_token();

                    if (after_type.is_identifier()) {
                        // variable creation
                        this->m_tokenizer->pop_mark();
                        if (this->m_error_handler)
                            this->m_error_handler->pop_mark();

                        statement.set_variable();

                        if (after_type.is_keyword()) {
                            this->m_token_error(after_type, "'" + std::string(after_type.get_data()) + "' is not a valid variable name");
                        }

                        shift_variable variable;
                        variable.name = &after_type;
                        variable.type = std::move(type);
                        variable.function = &func;

                        for (const auto& [mod, token_] : this->m_mods) {
                            if ((mod & variable_modifiers) == 0x0) {
                                this->m_token_error(*token_, "invalid '" + std::string(token_->get_data()) + "' specifier on variable");
                            } else {
                                variable.type.mods |= mod;
                            }
                        }
                        this->m_clear_mods();

                        const token& next_token = this->m_tokenizer->next_token();

                        if (next_token.is_binary_operator() || next_token.is_unary_operator()) {
                            if (!next_token.is_equals()) {
                                this->m_token_error(next_token, "expected ';' or '=' for variable declaration, got '" + std::string(next_token.get_data()) + "'");
                            }
                            // variable definition
                            const token& first_expr_token = this->m_tokenizer->next_token(); // Move onto the expression
                            variable.value = m_parse_expression();

                            if (variable.value.type == token::token_type::NULL_TOKEN) {
                                this->m_token_error(first_expr_token, "expected valid expression for variable assignment value");
                            }
                        } else if (!next_token.is_semicolon()) {
                            if (!next_token.is_null_token()) {
                                this->m_token_error(next_token, "expected ';' or '=' for variable declaration, got '" + std::string(next_token.get_data()) + "'");
                            } else {
                                this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected ';' or '=' for variable declaration before end of file, got '" + std::string(next_token.get_data()) + "'");
                            }
                            this->m_skip_until(token::token_type::SEMICOLON);
                        }

                        statement.set_variable(std::move(variable));
                    } else {
                        // normal expression
                        statement.set_expression();

                        this->m_tokenizer->rollback();
                        if (this->m_error_handler)
                            this->m_error_handler->rollback();

                        statement.set_expression(this->m_parse_expression());
                    }
                }

                else if (_token->is_semicolon()) {
                    statement.set_expression();
                    shift_expression expr;
                    expr.begin = this->m_tokenizer->get_index();
                    expr.end = expr.begin;
                    statement.set_expression(std::move(expr));
                }

                else if (_token->is_left_scope_bracket()) {
                    if (this->m_mods.size() != 0) {
                        const auto& [mod, token_] = this->m_mods.front();
                        this->m_token_error(*token_, "unexpected specifier '" + std::string(token_->get_data()) + "' in function body");
                        this->m_clear_mods();
                    }
                    statement.set_block(_token);
                    std::list<shift_statement> _statements;
                    this->m_tokenizer->next_token();
                    m_parse_function_block(func, _statements);
                    statement.set_block(std::move(_statements));

                    const token& right_block_token = this->m_tokenizer->current_token();
                    statement.set_block_end(&right_block_token);

                    if (!right_block_token.is_right_scope_bracket()) {
                        this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected '}' in function before end of file");
                    }
                }

                else if (_token->is_right_scope_bracket()) break;

                else {
                    statement.set_expression();
                    statement.set_expression(this->m_parse_expression());
                }


                statements.push_back(std::move(statement));
            }


            if (this->m_mods.size() > 0) {
                const auto& [mod, token_] = this->m_mods.front();
                this->m_token_error(*token_, "unexpected '" + std::string(token_->get_data()) + "' specifier function body");
                this->m_clear_mods();
            }
        }

        void parser::m_parse_use() {
            return m_parse_use(this->m_global_uses);
        }

        void parser::m_parse_use(std::list<shift_module>& modules) {
            // TODO warn if the module being used has already been included
            if (this->m_mods.size() > 0) {
                this->m_token_error(*this->m_mods.front().second, "unexpected access specifier in 'use' declaration");
                this->m_clear_mods();
            }

            const token& use_token = this->m_tokenizer->current_token();

            if (!use_token.is_use()) {
                this->m_token_error(use_token, "expected 'use'");
                this->m_skip_until(token::token_type::SEMICOLON);
                return;
            }

            this->m_tokenizer->next_token(); // skip 'use' keyword
            modules.push_back(m_parse_name("module name"));

            const token& end_token = this->m_tokenizer->current_token(); // token after the module name
            if (m_global_uses.back().size() == 0) {
                this->m_token_error(end_token.is_null_token() ? use_token : end_token, "expected module name after 'use'");
                this->m_skip_until(token::token_type::SEMICOLON);
            } else if (end_token.is_null_token()) {
                this->m_token_error(this->m_tokenizer->reverse_token(), "expected ';' before end of file");
            } else if (!end_token.is_semicolon()) {
                this->m_token_error(end_token, "unexpected '" + std::string(end_token.get_data()) + "' in module name");
                this->m_skip_until(token::token_type::SEMICOLON);
            }
        }

        void parser::m_parse_module(void) {
            if (this->m_mods.size() > 0) {
                this->m_token_error(*this->m_mods.front().second, "unexpected access specifier in 'module' declaration");
                this->m_clear_mods();
            }

            const token& module_token = this->m_tokenizer->current_token();

            if (!module_token.is_module()) {
                this->m_token_error(module_token, "expected 'module'");
                this->m_skip_until(token::token_type::SEMICOLON);
                return;
            }

            this->m_tokenizer->next_token(); // skip 'module' keyword
            this->m_module = m_parse_name("module name");

            const token& end_token = this->m_tokenizer->current_token(); // token after the module name

            if (this->m_module.size() == 0) {
                this->m_token_error(end_token.is_null_token() ? module_token : end_token, "expected module name after 'module'");
                this->m_skip_until(token::token_type::SEMICOLON);
            } else if (end_token.is_null_token()) {
                this->m_token_error(this->m_tokenizer->reverse_token(), "expected ';' before end of file");
            } else if (!end_token.is_semicolon()) {
                this->m_token_error(end_token, "unexpected '" + std::string(end_token.get_data()) + "' in module name");
                this->m_skip_until(token::token_type::SEMICOLON);
            }
        }

        /// @param name_type The type of name to be parsed. Will be displayed in error messages.
        ///                  e.g. "module name", "variable or function type"
        parser::shift_name parser::m_parse_name(const char* const name_type) {
            parser::shift_name name;
            name.begin = this->m_tokenizer->get_index();

            token::token_type last_type = token::token_type(0x0);

            for (const token* token = &this->m_tokenizer->current_token(); !token->is_null_token(); token = &this->m_tokenizer->next_token()) {
                if (token->is_access_specifier()) {
                    this->m_token_error(*token, "unexpected '" + std::string(token->get_data()) + "' specifier in " + std::string(name_type));
                }

                else if (token->is_keyword()) {
                    // error, no keywords in (module) names
                    this->m_token_error(*token, "invalid '" + std::string(token->get_data()) + "' inside " + std::string(name_type));
                    last_type = token::token_type::IDENTIFIER;
                }

                else if (token->is_identifier()) {
                    if (last_type == token::token_type::IDENTIFIER) {
                        // cannot have two identifiers in a row in a (module) name
                        last_type = token::token_type::IDENTIFIER;
                        break;
                    }

                    last_type = token::token_type::IDENTIFIER;
                }

                else if (token->get_token_type() == token::token_type::DOT) {
                    if (last_type != token::token_type::IDENTIFIER) {
                        // cannot have two dots in a row in a (module) name
                        last_type = token::token_type::DOT;
                        this->m_tokenizer->next_token(); // This allows the error to be caught below
                        break;
                    }

                    last_type = token::token_type::DOT;
                } else break;
            }
            name.end = this->m_tokenizer->get_index();

            if (last_type == token::token_type::DOT) {
                this->m_token_error(this->m_tokenizer->reverse_peek_token(), "unexpected '.' inside " + std::string(name_type));
            }

            return name;
        }

        parser::shift_type parser::m_parse_type(const char* const name_type) {
            parser::shift_type type;

            for (const token* access_specifier_token = &this->m_tokenizer->current_token(); access_specifier_token->is_access_specifier(); access_specifier_token = &this->m_tokenizer->next_token()) {
                this->m_parse_access_specifier();
            }
            token::token_type last_type = token::token_type::NULL_TOKEN;
            {
                parser::shift_name name;
                name.begin = this->m_tokenizer->get_index();

                for (const token* token = &this->m_tokenizer->current_token(); !token->is_null_token(); token = &this->m_tokenizer->next_token()) {
                    if (token->is_access_specifier()) {
                        if (name.end == std::vector<compiler::token>::const_iterator()) {
                            name.end = this->m_tokenizer->get_index();
                        }

                        this->m_parse_access_specifier();
                        auto const mod = this->m_mods.back().first;

                        if ((mod & type_modifiers) == 0x0) {
                            this->m_token_error(*token, "unexpected '" + std::string(token->get_data()) + "' specifier in " + std::string(name_type));
                        }
                        last_type = token::token_type::IDENTIFIER;
                    }

                    else if (name.end != std::vector<compiler::token>::const_iterator()) {
                        break;
                    }

                    else if (token->is_keyword()) {
                        // error, no keywords in (module) names
                        this->m_token_error(*token, "invalid '" + std::string(token->get_data()) + "' inside " + std::string(name_type));
                        last_type = token::token_type::IDENTIFIER;
                    }

                    else if (token->is_identifier()) {
                        if (last_type == token::token_type::IDENTIFIER) {
                            // cannot have two identifiers in a row in a (module) name
                            last_type = token::token_type::IDENTIFIER;
                            break;
                        }

                        last_type = token::token_type::IDENTIFIER;
                    }

                    else if (token->get_token_type() == token::token_type::DOT) {
                        if (last_type != token::token_type::IDENTIFIER) {
                            // cannot have two dots in a row in a (module) name
                            last_type = token::token_type::DOT;
                            this->m_tokenizer->next_token(); // This allows the error to be caught below
                            break;
                        }

                        last_type = token::token_type::DOT;
                    } else break;
                }
                if (name.end == std::vector<compiler::token>::const_iterator())
                    name.end = this->m_tokenizer->get_index();


                for (auto cur = this->m_mods.cbegin(); cur != this->m_mods.cend(); ++cur) {
                    auto const mod = cur->first;
                    if ((mod & type_modifiers) != 0x0) {
                        type.mods |= mod;
                        cur = --this->m_mods.erase(cur);
                    }
                }

                if (last_type == token::token_type::DOT) {
                    this->m_token_error(this->m_tokenizer->reverse_peek_token(), "unexpected '.' inside " + std::string(name_type));
                }

                type.name = std::move(name);
            }

            for (const token* token = &this->m_tokenizer->current_token(); !token->is_null_token(); token = &this->m_tokenizer->next_token()) {
                if (token->is_access_specifier()) {
                    this->m_token_error(*token, "unexpected '" + std::string(token->get_data()) + "' specifier in " + std::string(name_type) + " type");
                }

                else if (token->is_left_square_bracket()) {
                    if (last_type != token::token_type::IDENTIFIER && last_type != token::token_type::RIGHT_SQUARE_BRACKET) {
                        this->m_token_error(*token, "unexpected '[' in " + std::string(name_type));
                    } else type.array_dimensions++;
                    
                    last_type = token::token_type::LEFT_SQUARE_BRACKET;
                }

                else if (token->is_right_square_bracket()) {
                    if (last_type != token::token_type::LEFT_SQUARE_BRACKET) {
                        this->m_token_error(*token, "unexpected ']' in " + std::string(name_type));
                    }
                    last_type = token::token_type::RIGHT_SQUARE_BRACKET;
                }

                else if (last_type == token::token_type::LEFT_SQUARE_BRACKET) {
                    this->m_token_error(*token, "expected ']' in " + std::string(name_type));
                    break;
                } else break;
            }

            if (last_type == token::token_type::LEFT_SQUARE_BRACKET) {
                const token& last = this->m_tokenizer->reverse_peek_token();
                this->m_token_error(last, "unexpected '[' inside " + std::string(name_type));
            }

            return type;
        }

        void parser::m_parse_access_specifier(void) {
            const token& current_token = this->m_tokenizer->current_token();
            const mods mod = to_access_specifier(current_token);
            const mods current_mods = this->m_get_mods();

            if ((current_mods & visibility_modifiers) != 0x0 && ((current_mods | mod) & visibility_modifiers) != (current_mods & visibility_modifiers)) {
                // error if the one we have (i.e. the public, protected or private that is currently specified (the one being stored in current_mods)) is NOT the one now being parsed
                this->m_token_error(current_token, "unexpected visibility specifier");
            } else if ((current_mods & mod) == mod) {
                // send a warning if we are just adding the same modifier twice
                this->m_token_warning(current_token, "redundant '" + std::string(current_token.get_data()) + "' specifier");
            } else {
                this->m_add_mod(mod, current_token);
            }
        }

        parser::mods parser::m_get_mods(void) const noexcept {
            mods mods = static_cast<parser::mods>(0x0);

            for (const auto& [mod, token_] : this->m_mods) {
                mods |= mod;
            }

            return mods;
        }

        void parser::m_add_mod(mods mod, const token& token_) noexcept {
            this->m_mods.push_back({ mod, &token_ });
        }

        void parser::m_clear_mods(void) noexcept {
            return this->m_mods.clear();
        }

        const token& parser::m_skip_until(const std::string_view str) noexcept {
            for (; !this->m_tokenizer->current_token().is_null_token() && this->m_tokenizer->current_token().get_data() != str;
                this->m_tokenizer->next_token());
            return this->m_tokenizer->current_token();
        }

        const token& parser::m_skip_until(const std::string& str) noexcept { return m_skip_until(std::string_view(str.data(), str.length())); }
        const token& parser::m_skip_until(const char* const str) noexcept { return m_skip_until(std::string_view(str, std::strlen(str))); }

        const token& parser::m_skip_until(const typename token::token_type type) noexcept {
            for (; !this->m_tokenizer->current_token().is_null_token() && this->m_tokenizer->current_token().get_token_type() != type;
                this->m_tokenizer->next_token());
            return this->m_tokenizer->current_token();
        }

        const token& parser::m_skip_after(const std::string_view str) noexcept {
            m_skip_until(str);
            return this->m_tokenizer->next_token();
        }

        const token& parser::m_skip_after(const std::string& str) noexcept { return m_skip_after(std::string_view(str.data(), str.length())); }
        const token& parser::m_skip_after(const char* const str) noexcept { return m_skip_after(std::string_view(str, std::strlen(str))); }

        const token& parser::m_skip_after(const typename token::token_type type) noexcept {
            m_skip_until(type);
            return this->m_tokenizer->next_token();
        }

        const token& parser::m_skip_until_closing(const typename token::token_type bracket_type) noexcept {
            if (bracket_type != token_type::LEFT_BRACKET && bracket_type != token_type::LEFT_SQUARE_BRACKET && bracket_type != token_type::LEFT_SCOPE_BRACKET)
                return m_skip_until(bracket_type);

            const token_type look = token_type(std::underlying_type_t<token_type>(bracket_type) + 1);

            size_t count = 1;
            for (const token* _token = &this->m_tokenizer->current_token(); !_token->is_null_token() && count > 0; _token = &this->m_tokenizer->next_token()) {
                if (_token->get_token_type() == bracket_type) count++;
                else if (_token->get_token_type() == look) count--;
            }

            if (count != 0) {
                const char* msg;
                switch (bracket_type) {
                    case token::token_type::LEFT_BRACKET:
                        msg = "expected ')' before end of file";
                        break;
                    case token::token_type::LEFT_SQUARE_BRACKET:
                        msg = "expected ']' before end of file";
                        break;
                    case token::token_type::LEFT_SCOPE_BRACKET:
                        msg = "expected '}' before end of file";
                        break;
                    default:
                        msg = "expected closing bracket before end of file";
                        break;
                }

                this->m_token_error(this->m_tokenizer->reverse_peek_token(), msg);
            }

            return this->m_tokenizer->current_token().is_null_token() ? this->m_tokenizer->current_token() : this->m_tokenizer->reverse_token();
        }


        parser::shift_expression parser::m_parse_expression(const token::token_type end_type) {
            shift_expression ret_expr;
            shift_expression* expr = &ret_expr;


            for (const token* _token = &this->m_tokenizer->current_token(); !_token->is_null_token() && _token->get_token_type() != end_type; _token = &this->m_tokenizer->next_token()) {
                if (_token->is_string_literal()) {
                    if (expr->type == token::token_type::NULL_TOKEN) {
                        expr->type = token::token_type::STRING_LITERAL;
                        expr->begin = this->m_tokenizer->get_index();
                        expr->end = expr->begin + 1;
                    } else {
                        this->m_token_error(*_token, "unexpected string literal in expression");
                    }
                    continue;
                }

                if (_token->is_char_literal()) {
                    if (expr->type == token::token_type::NULL_TOKEN) {
                        expr->type = token::token_type::CHAR_LITERAL;
                        expr->begin = this->m_tokenizer->get_index();
                        expr->end = expr->begin + 1;
                    } else {
                        this->m_token_error(*_token, "unexpected character literal in expression");
                    }
                    continue;
                }

                if (_token->is_number()) {
                    if (expr->type == token::token_type::NULL_TOKEN) {
                        expr->type = _token->get_token_type();
                        expr->begin = this->m_tokenizer->get_index();
                        expr->end = expr->begin + 1;
                    } else {
                        switch (_token->get_token_type()) {
                            case token::token_type::INTEGER_LITERAL:
                            case token::token_type::BINARY_NUMBER:
                            case token::token_type::HEX_NUMBER:
                                this->m_token_error(*_token, "unexpected integer literal in expression");
                                break;
                            case token::token_type::FLOAT:
                            case token::token_type::DOUBLE:
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
                    if (expr->type == token::token_type::NULL_TOKEN) {
                        expr->type = token::token_type::IDENTIFIER;
                        expr->begin = this->m_tokenizer->get_index();
                        expr->end = expr->begin + 1;
                    } else {
                        this->m_token_error(*_token, "unexpected boolean literal in expression");
                    }
                    continue;
                }

                if (_token->is_null()) {
                    if (expr->type == token::token_type::NULL_TOKEN) {
                        expr->type = token::token_type::IDENTIFIER;
                        expr->begin = this->m_tokenizer->get_index();
                        expr->end = expr->begin + 1;
                    } else {
                        this->m_token_error(*_token, "unexpected 'null' in expression");
                    }
                    continue;
                }

                if (_token->is_left_bracket()) {
                    // (hello.world); (hello.world)5;
                    //  type=LEFT_BRACKET
                    //  left=hello.world
                    //  right=5
                    if (expr->type != token::token_type::NULL_TOKEN && !expr->is_bracket()) {
                        this->m_token_error(*_token, "unexpected '(' inside expression");
                    }

                    expr->set_bracket();
                    expr->begin = this->m_tokenizer->get_index();
                    expr->end = expr->begin + 1;
                    this->m_tokenizer->next_token(); // skip (
                    expr->set_left(m_parse_expression(token::token_type::RIGHT_BRACKET));
                    expr->set_right();
                    expr = expr->get_right();
                    const token& right_bracket = this->m_tokenizer->current_token();
                    if (!right_bracket.is_right_bracket()) {
                        this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected ')' inside expression before end of file");
                    }
                    continue;
                }

                if (_token->is_comma()) {
                    if (expr->type == token::token_type::NULL_TOKEN) {
                        this->m_token_error(*_token, "unexpected ',' inside expression");
                    }

                    if (ret_expr.type != token::token_type::COMMA) {
                        shift_expression temp = std::move(ret_expr);

                        ret_expr = shift_expression();
                        ret_expr.type = token::token_type::COMMA;
                        ret_expr.begin = this->m_tokenizer->get_index();
                        ret_expr.end = ret_expr.begin + 1;

                        ret_expr.sub.push_back(std::move(temp));
                    }

                    ret_expr.sub.back().parent = &ret_expr;
                    ret_expr.sub.emplace_back();
                    expr = &ret_expr.sub.back();

                    continue;
                }

                if (_token->is_binary_operator() || _token->is_unary_operator()) {
                    if (_token->is_binary_operator()) {
                        if (expr->type == token::token_type::NULL_TOKEN && !_token->is_prefix_overload_operator()) {
                            this->m_token_error(*_token, "unexpected operator '" + std::string(_token->get_data()) + "' inside expression");
                        }
                    } else {
                        if (expr->type == token::token_type::NULL_TOKEN) {
                            if (_token->is_strictly_suffix_overload_operator()) {
                                this->m_token_error(*_token, "unexpected operator '" + std::string(_token->get_data()) + "' inside expression");
                            }
                        } else {
                            if (_token->is_strictly_prefix_overload_operator()) {
                                this->m_token_error(*_token, "unexpected operator '" + std::string(_token->get_data()) + "' inside expression");
                            }
                        }
                    }

                    shift_expression new_expr;
                    new_expr.type = _token->get_token_type();
                    new_expr.begin = this->m_tokenizer->get_index();
                    new_expr.end = new_expr.begin + 1;
                    new_expr.set_right();

                    const uint_fast8_t priority = operator_priority(new_expr.type, (_token->is_strictly_prefix_overload_operator() && !_token->is_binary_operator()) || (_token->is_prefix_overload_operator() && expr->type == token::token_type::NULL_TOKEN));

                    shift_expression* const new_ret_expr = ret_expr.type == token_type::COMMA ? &ret_expr.sub.back() : &ret_expr;

                    shift_expression* current_parent = expr->parent;

                    for (; current_parent != nullptr; current_parent = current_parent->parent) {
                        const bool is_prefix = (is_strictly_prefix_operator(current_parent->type) && !is_binary_operator(current_parent->type)) || (is_prefix_operator(current_parent->type) && current_parent->get_left()->type == token::token_type::NULL_TOKEN);
                        const bool l_to_r = !is_prefix;
                        const uint_fast8_t parent_priority = operator_priority(current_parent->type, is_prefix);

                        if (priority > parent_priority || (!l_to_r && (priority == parent_priority))) {
                            new_expr.set_left(std::move(*current_parent->get_right()));
                            current_parent->set_right(std::move(new_expr));
                            current_parent->get_right()->parent = current_parent;
                            current_parent->get_right()->get_left()->parent = current_parent->get_right();
                            current_parent->get_right()->get_right()->parent = current_parent->get_right();
                            expr = current_parent->get_right()->get_right();
                            break;
                        }
                    }

                    if (current_parent == nullptr) {
                        new_expr.set_left(std::move(*new_ret_expr));
                        *new_ret_expr = std::move(new_expr);
                        new_ret_expr->get_left()->parent = new_ret_expr;
                        new_ret_expr->get_right()->parent = new_ret_expr;
                        expr = new_ret_expr->get_right();
                    }
                    continue;
                }

                if (_token->is_identifier()) {
                    if (expr->type != token::token_type::NULL_TOKEN) {
                        this->m_token_error(*_token, "unexpected identifier in expression");
                    }
                    expr->type = token::token_type::IDENTIFIER;

                    // hello.world;
                    //  type=IDENTIFIER
                    //  begin/end=hello.world

                    // hello.world();
                    //  type=LEFT_SCOPE_BRACKET
                    //  begin/end=hello.world
                    //  sub[0]={params}

                    // hello.world[3][value][1];
                    //  type=LEFT_SQUARE_BRACKET
                    //  begin/end=hello.world
                    //  sub={indexers}

                    // new hello.world();
                    //  type=IDENTIFIER
                    //  begin/end=new
                    //  sub[0]=--- type=LEFT_SCOPE_BRACKET
                    //             begin/end=hello.world
                    //             sub[0]={params}

                    // new hello.world[3][value][1];
                    //  type=IDENTIFIER
                    //  begin/end=new
                    //  sub[0]=--- type=LEFT_SQUARE_BRACKET
                    //             begin/end=hello.world
                    //             sub={indexers}
                    if (_token->is_new()) {
                        // new clazz.name("Hello world", 3);
                        expr->begin = this->m_tokenizer->get_index();
                        expr->end = expr->begin + 1;
                        this->m_tokenizer->next_token(); // Skip 'new'
                        expr->sub.push_back(m_parse_expression(end_type));
                        const shift_expression& new_left = expr->sub.back();

                        if (!new_left.is_bracket() && !new_left.is_function_call() && !new_left.is_array()) {
                            if (new_left.type != token::token_type::IDENTIFIER) {
                                this->m_token_error(*expr->begin, "unexpected 'new' inside expression");
                            } else if (new_left.type == token::token_type::IDENTIFIER && new_left.begin->is_new()) {
                                this->m_token_error(*new_left.begin, "unexpected 'new' inside expression");
                            }
                        }
                        this->m_tokenizer->reverse_token();
                    } else {
                        expr->begin = this->m_tokenizer->get_index();

                        if (this->m_tokenizer->current_token().is_this() || this->m_tokenizer->current_token().is_base()) {
                            // Allow (this|base)[.]? at the beginning of the expression
                            this->m_tokenizer->next_token();
                            if (this->m_tokenizer->current_token().is_dot()) {
                                this->m_tokenizer->next_token();
                                this->m_parse_name("expression");
                            }
                        } else {
                            this->m_parse_name("expression");
                        }

                        expr->end = this->m_tokenizer->get_index();

                        const token& after_name = this->m_tokenizer->current_token();

                        if (after_name.is_left_bracket()) {
                            expr->set_function_call();
                            this->m_tokenizer->next_token();
                            expr->sub.push_back(m_parse_expression(token_type::RIGHT_BRACKET));

                            const token& right_function_call_bracket = this->m_tokenizer->current_token();
                            if (!right_function_call_bracket.is_right_bracket()) {
                                if (!right_function_call_bracket.is_null_token()) {
                                    this->m_token_error(right_function_call_bracket, "expected ')' inside expression");
                                } else {
                                    this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected ')' inside expression before end of file");
                                }
                            }
                        } else if (after_name.is_left_square_bracket()) {
                            expr->set_array();
                            do {
                                this->m_tokenizer->next_token();
                                expr->sub.push_back(std::move(m_parse_expression(token_type::RIGHT_SQUARE_BRACKET)));
                                if (expr->sub.back().type == token::token_type::COMMA) {
                                    this->m_token_error(*expr->sub.back().begin, "unexpected ',' inside array indexer inside expression");
                                }
                                const token& right_array_bracket = this->m_tokenizer->current_token();
                                if (!right_array_bracket.is_right_square_bracket()) {
                                    this->m_token_error(this->m_tokenizer->reverse_peek_token(), "expected ']' inside expression before end of file");
                                }
                            } while (this->m_tokenizer->next_token().is_left_square_bracket());
                            this->m_tokenizer->reverse_token();
                        } else {
                            this->m_tokenizer->reverse_token();
                        }
                    }
                    continue;
                }

                this->m_token_error(*_token, "unexpected token '" + std::string(_token->get_data()) + "' in expression");
            }

            if (expr->parent != nullptr) {
                if (is_unary_operator(expr->parent->type)) {
                    if (expr->parent->get_left()->type != token_type::NULL_TOKEN && expr->parent->get_right()->type != token_type::NULL_TOKEN && !is_binary_operator(expr->parent->type)) {
                        this->m_token_error(*expr->parent->begin, "unexpected operator '" + std::string(expr->parent->begin->get_data()) + "' inside expression");
                    } else if (expr->parent->get_left()->type == token_type::NULL_TOKEN && expr->parent->get_right()->type == token_type::NULL_TOKEN) {
                        this->m_token_error(*expr->parent->begin, "unexpected operator '" + std::string(expr->parent->begin->get_data()) + "' inside expression");
                    }
                }

                if (is_binary_operator(expr->parent->type)) {
                    if (expr->parent->get_right()->size() == 0 && expr->parent->get_left()->size() != 0 && !is_suffix_operator(expr->parent->type)) {
                        this->m_token_error(*expr->parent->begin, "unexpected operator '" + std::string(expr->parent->begin->get_data()) + "' inside expression");
                    }
                }
            } else if (this->m_tokenizer->reverse_peek_token().is_comma()) {
                this->m_token_error(this->m_tokenizer->reverse_peek_token(), "misplaced ',' inside expression");
            }

            return ret_expr;
        }

        const token& parser::m_skip_before(const std::string_view str) noexcept {
            m_skip_until(str);
            return this->m_tokenizer->reverse_token();
        }

        const token& parser::m_skip_before(const std::string& str) noexcept { return m_skip_before(std::string_view(str.data(), str.length())); }
        const token& parser::m_skip_before(const char* const str) noexcept { return m_skip_before(std::string_view(str, std::strlen(str))); }

        const token& parser::m_skip_before(const typename token::token_type type) noexcept {
            m_skip_until(type);
            return this->m_tokenizer->reverse_token();
        }

        void parser::m_token_error(const token& token_, const std::string_view msg) {
            if (!this->m_error_handler) return;
            SHIFT_PARSER_ERROR_(token_, msg);
            std::string line = std::string(this->m_get_line(token_));
            size_t use_col = token_.get_file_index().col;
            std::for_each(line.begin(), line.end(), [&use_col](char& ch) {
                if (ch == '\t') {
                    ch = ' ';
                    use_col -= 3;
                }
                });

            std::string indexer(use_col - 1, ' ');
            indexer.append(token_.get_data().size(), '^');
            SHIFT_PARSER_ERROR_LOG(line);
            SHIFT_PARSER_ERROR_LOG(indexer);
        }

        void parser::m_token_error(const token& token_, const std::string& msg) { return m_token_error(token_, std::string_view(msg.c_str(), msg.length())); }
        void parser::m_token_error(const token& token_, const char* const msg) { return m_token_error(token_, std::string_view(msg, std::strlen(msg))); }

        void parser::m_token_warning(const token& token_, const std::string_view msg) {
            if (!this->m_error_handler) return;
            if (!this->m_error_handler->is_print_warnings()) return;
            SHIFT_PARSER_WARNING_(token_, msg);
            std::string line = std::string(this->m_get_line(token_));
            size_t use_col = token_.get_file_index().col;
            std::for_each(line.begin(), line.end(), [&use_col](char& ch) {
                if (ch == '\t') {
                    ch = ' ';
                    use_col -= 3;
                }
                });

            std::string indexer(use_col - 1, ' ');
            indexer.append(token_.get_data().size(), '^');

            SHIFT_PARSER_WARNING_LOG(line);
            SHIFT_PARSER_WARNING_LOG(indexer);
        }

        void parser::m_token_warning(const token& token_, const std::string& msg) { return m_token_warning(token_, std::string_view(msg.c_str(), msg.length())); }
        void parser::m_token_warning(const token& token_, const char* const msg) { return m_token_warning(token_, std::string_view(msg, std::strlen(msg))); }

        std::string_view parser::m_get_line(const token& token_) const noexcept { return this->m_tokenizer->get_lines()[token_.get_file_index().line - 1]; }

        bool parser::m_is_module_defined(void) const noexcept { return this->m_module.size() != 0; }

        uint_fast8_t parser::operator_priority(const token_type type, const bool prefix) noexcept {
            constexpr uint_fast8_t prefix_priority = 0xfc;

            switch (type) {
                case token_type::AND:
                case token_type::OR:
                case token_type::XOR:
                case token_type::SHIFT_LEFT:
                case token_type::SHIFT_RIGHT:
                    return 0x2;

                case token_type::AND_AND:
                case token_type::OR_OR:
                    return 0x3;

                case token_type::GREATER_THAN:
                case token_type::LESS_THAN:
                    return 0x4;

                case token_type::PLUS:
                    return 0x5;

                case token_type::MINUS:
                    return prefix ? prefix_priority : 0x5;

                case token_type::MULTIPLY:
                case token_type::DIVIDE:
                case token_type::MODULO:
                    return 0x6;

                case token_type::MINUS_MINUS:
                case token_type::PLUS_PLUS:
                    return prefix ? prefix_priority : 0x7;

                case token_type::NOT:
                case token_type::FLIP_BITS:
                    return prefix_priority;

                case token_type::LEFT_BRACKET: // bracket-ed expressions
                case token_type::LEFT_SQUARE_BRACKET: // array operator
                case token_type::IDENTIFIER: // variable names / function calls
                    return 0xfe;

                default: {
                    // i = 5 + 3; -> should be -> (i) = (5 + 3); | if = had more priority -> (i = 5) + (3)
                    return (type & token_type::EQUALS) == token_type::EQUALS ? 0x1 : 0x0;
                }
            }
        }

        static constexpr parser::mods to_access_specifier(const token& token) noexcept {
            using mods = parser::mods;
            if (token.is_public()) {
                return mods::PUBLIC;
            } else if (token.is_protected()) {
                return mods::PROTECTED;
            } else if (token.is_private()) {
                return mods::PRIVATE;
            } else if (token.is_static()) {
                return mods::STATIC;
            } else if (token.is_const()) {
                return mods::CONST_;
            } else if (token.is_extern()) {
                return mods::EXTERN;
            } else if (token.is_binary()) {
                return mods::BINARY;
            }

            return static_cast<mods>(0x0);
        }
    }
}