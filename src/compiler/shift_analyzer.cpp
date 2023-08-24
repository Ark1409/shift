/**
 * @file compiler/shift_analyzer.cpp
 */
#include "compiler/shift_analyzer.h"
#include "utils/utils.h"

#include <cstring>
#include <algorithm>

#define SHIFT_ANALYZER_ERROR_PREFIX(__parser) 				"error: " << std::filesystem::relative((__parser).get_tokenizer()->get_file().raw_path()).string() << ": " // std::filesystem::relative call every time probably isn't that optimal
#define SHIFT_ANALYZER_WARNING_PREFIX(__parser) 			"warning: " << std::filesystem::relative((__parser).get_tokenizer()->get_file().raw_path()).string() << ": " // std::filesystem::relative call every time probably isn't that optimal

#define SHIFT_ANALYZER_ERROR_PREFIX_EXT_(__parser, __line__, __col__) "error: " << std::filesystem::relative((__parser).get_tokenizer()->get_file().raw_path()).string() << ":" << __line__ << ":" << __col__ << ": " // std::filesystem::relative call every time probably isn't that optimal
#define SHIFT_ANALYZER_WARNING_PREFIX_EXT_(__parser, __line__, __col__) "warning: " << std::filesystem::relative((__parser).get_tokenizer()->get_file().raw_path()).string() << ":" << __line__ << ":" << __col__ << ": " // std::filesystem::relative call every time probably isn't that optimal

#define SHIFT_ANALYZER_ERROR_PREFIX_EXT(__parser, __token) SHIFT_ANALYZER_ERROR_PREFIX_EXT_(__parser, (__token).get_file_index().line, (__token).get_file_index().col)
#define SHIFT_ANALYZER_WARNING_PREFIX_EXT(__parser, __token) SHIFT_ANALYZER_WARNING_PREFIX_EXT_(__parser, (__token).get_file_index().line, (__token).get_file_index().col)

#define SHIFT_ANALYZER_PRINT() this->m_error_handler->print_exit_clear()

#define SHIFT_ANALYZER_WARNING(__parser, __WARN__) 			this->m_error_handler->stream() << SHIFT_ANALYZER_WARNING_PREFIX(__parser) << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_ANALYZER_FATAL_WARNING(__parser, __WARN__) 		SHIFT_ANALYZER_WARNING(__parser, __WARN__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_WARNING_LOG(__WARN__) 		this->m_error_handler->stream() << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_ANALYZER_FATAL_WARNING_LOG(__WARN__)  SHIFT_ANALYZER_WARNING_LOG(__WARN__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_ERROR(__parser, __ERR__) 			this->m_error_handler->stream() << SHIFT_ANALYZER_ERROR_PREFIX(__parser) << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_ANALYZER_FATAL_ERROR(__parser, __ERR__) 		SHIFT_ANALYZER_ERROR(__parser, __ERR__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_ERROR_LOG(__ERR__) 		this->m_error_handler->stream() << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_ANALYZER_FATAL_ERROR_LOG(__ERR__)  SHIFT_ANALYZER_ERROR_LOG(__ERR__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_WARNING_(__parser, __token, __WARN__) 	this->m_error_handler->stream() << SHIFT_ANALYZER_WARNING_PREFIX_EXT(__parser, __token) << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_ANALYZER_FATAL_WARNING_(__parser, __token, __WARN__) 		SHIFT_ANALYZER_WARNING(__parser, __token, __WARN__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_WARNING_LOG_(__WARN__) 		this->m_error_handler->stream() << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_ANALYZER_FATAL_WARNING_LOG_(__WARN__)  SHIFT_ANALYZER_WARNING_LOG_( __WARN__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_ERROR_(__parser, __token, __ERR__) 			this->m_error_handler->stream() << SHIFT_ANALYZER_ERROR_PREFIX_EXT(__parser, __token) << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_ANALYZER_FATAL_ERROR_(__parser, __token, __ERR__) 		SHIFT_ANALYZER_ERROR_(__parser,__token,__ERR__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_ERROR_LOG_(__ERR__) 		this->m_error_handler->stream() << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_ANALYZER_FATAL_ERROR_LOG_(__ERR__)  SHIFT_ANALYZER_ERROR_LOG_(__ERR__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_OBJECT_CLASS "shift.object"

#define SHIFT_ANALYZER_INT8_CLASS "shift.byte"
#define SHIFT_ANALYZER_CHAR_CLASS SHIFT_ANALYZER_INT8_CLASS
#define SHIFT_ANALYZER_BYTE_CLASS SHIFT_ANALYZER_CHAR_CLASS

#define SHIFT_ANALYZER_INT16_CLASS "shift.short"
#define SHIFT_ANALYZER_SHORT_CLASS SHIFT_ANALYZER_INT16_CLASS

#define SHIFT_ANALYZER_INT32_CLASS "shift.int"
#define SHIFT_ANALYZER_INT_CLASS SHIFT_ANALYZER_INT32_CLASS

#define SHIFT_ANALYZER_INT64_CLASS "shift.long"
#define SHIFT_ANALYZER_LONG_CLASS SHIFT_ANALYZER_INT64_CLASS

#define SHIFT_ANALYZER_UINT16_CLASS "shift.ushort"
#define SHIFT_ANALYZER_USHORT_CLASS SHIFT_ANALYZER_UINT16_CLASS

#define SHIFT_ANALYZER_UINT32_CLASS "shift.uint"
#define SHIFT_ANALYZER_UINT_CLASS SHIFT_ANALYZER_UINT32_CLASS

#define SHIFT_ANALYZER_UINT64_CLASS "shift.ulong"
#define SHIFT_ANALYZER_ULONG_CLASS SHIFT_ANALYZER_UINT64_CLASS

#define SHIFT_ANALYZER_SINT8_CLASS "shift.sbyte"
#define SHIFT_ANALYZER_SCHAR_CLASS SHIFT_ANALYZER_SINT8_CLASS
#define SHIFT_ANALYZER_SBYTE_CLASS SHIFT_ANALYZER_SCHAR_CLASS

#define SHIFT_ANALYZER_SINT16_CLASS SHIFT_ANALYZER_INT16_CLASS
#define SHIFT_ANALYZER_SSHORT_CLASS SHIFT_ANALYZER_SINT16_CLASS

#define SHIFT_ANALYZER_SINT32_CLASS SHIFT_ANALYZER_INT32_CLASS
#define SHIFT_ANALYZER_SINT_CLASS SHIFT_ANALYZER_SINT32_CLASS

#define SHIFT_ANALYZER_SINT64_CLASS SHIFT_ANALYZER_INT64_CLASS
#define SHIFT_ANALYZER_SLONG_CLASS SHIFT_ANALYZER_SINT64_CLASS

#define SHIFT_ANALYZER_FLOAT_CLASS "shift.float"
#define SHIFT_ANALYZER_FLOAT32_CLASS SHIFT_ANALYZER_FLOAT_CLASS
#define SHIFT_ANALYZER_DOUBLE_CLASS "shift.double"
#define SHIFT_ANALYZER_FLOAT64_CLASS SHIFT_ANALYZER_DOUBLE_CLASS

#define SHIFT_ANALYZER_STRING_CLASS "shift.string"

#define SHIFT_ANALYZER_BOOLEAN_CLASS "shift.bool"
#define SHIFT_ANALYZER_BOOL_CLASS SHIFT_ANALYZER_BOOLEAN_CLASS

namespace shift {
    namespace compiler {
        using token_type = token::token_type;

        static constexpr inline bool is_overload_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_overload_operator(); }
        static constexpr inline bool is_binary_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_binary_operator(); }
        static constexpr inline bool is_unary_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_unary_operator(); }
        static constexpr inline bool is_prefix_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_prefix_overload_operator(); }
        static constexpr inline bool is_suffix_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_suffix_overload_operator(); }
        static constexpr inline bool is_strictly_prefix_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_strictly_prefix_overload_operator(); }
        static constexpr inline bool is_strictly_suffix_operator(const token_type type) noexcept { return token(std::string_view(), type, file_indexer()).is_strictly_suffix_overload_operator(); }

        static shift_module m_shift_module;
        static shift_class m_void_class, m_null_class;

        // Storage for tokens needed for implementation
        static std::vector<token> m_token_storage;
        static std::vector<token>::iterator m_void_token;
        static std::vector<token>::iterator m_null_token;
        static std::vector<token>::iterator m_new_token;
        static std::vector<token>::iterator m_array_token;
        static std::vector<token>::iterator m_length_token;
        static std::vector<token>::iterator m_operator_token;
        static std::vector<token>::iterator m_left_square_bracket_token;
        static std::vector<token>::iterator m_right_square_bracket_token;
        static std::vector<token>::iterator m_true_token;
        static std::vector<token>::iterator m_equals_token;
        static std::vector<token>::iterator m_equals_equals_token;
        static std::vector<token>::iterator m_not_equal_token;

        void analyzer::analyze() {
            // TODO add default classes (shift.int, shift.string, shift.long) before starting to analyze
            m_init_defaults();

            // Add in all modules first
            for (parser& _parser : *m_parsers) {
                if (!_parser.m_is_module_defined()) {
                    this->m_error(_parser, "module not defined for file");
                } else {
                    m_modules.emplace(_parser.m_module.to_string());
                }
            }

            for (parser& _parser : *m_parsers) {
                for (shift_variable& var : _parser.m_variables) {
                    var.parser_ = &_parser;
                    std::string fqn = var.get_fqn();
                    if (m_variables.find(fqn) == m_variables.end()) {
                        m_variables[std::move(fqn)] = &var;
                    } else {
                        this->m_token_error(_parser, *var.name, "variable '" + fqn + "' has already been defined inside current module");
                    }
                }

                for (shift_function& func : _parser.m_functions) {
                    func.parser_ = &_parser;
                    m_functions[func.get_fqn(m_func_dupe_count[func.get_fqn()]++)] = &func;
                }

                for (shift_class& clazz : _parser.m_classes) {
                    clazz.parser_ = &_parser;
                    std::string fqn = clazz.get_fqn();

                    // error if class conflicts with module
                    if (m_modules.find(fqn) != m_modules.end()) {
                        this->m_token_error(_parser, *clazz.name, "class '" + fqn + "' would override module with same name");
                    }

                    if (m_classes.find(fqn) == m_classes.end()) {
                        m_classes[std::move(fqn)] = &clazz;

                        for (shift_function& func : clazz.functions) {
                            func.parser_ = &_parser;
                            // size_t& index = m_func_dupe_count[func.get_fqn()];
                            // std::string function_fqn = ;
                            // m_functions[std::move(function_fqn)] = &func;
                            m_functions[func.get_fqn(m_func_dupe_count[func.get_fqn()]++)] = &func;
                            // for (; m_functions.find(function_fqn) != m_functions.end(); function_fqn = func.get_fqn(++index));
                            // if (m_functions.find(function_fqn) == m_functions.end()) {
                            //     m_functions[std::move(function_fqn)] = &func;
                            //     index++;
                            // } else {
                            //     this->m_token_error(_parser, *clazz.name, "function '" + function_fqn + "' has already been defined");
                            // }
                        }
                    } else {
                        this->m_token_error(_parser, *clazz.name, "class '" + fqn + "' has already been defined");
                    }
                }
            }

            scope _scope;
            _scope.base = this;

            for (parser& _parser : *m_parsers) {
                _scope.parser_ = &_parser;

                for (shift_module const& use : _parser.m_global_uses) {
                    if (this->m_modules.find(use) == this->m_modules.end()) {
                        this->m_name_error(_parser, use, "module '" + use.to_string() + "' does not exist");
                    } else if (this->m_error_handler && this->m_error_handler->is_print_warnings() && use == _parser.m_module) {
                        this->m_name_warning(_parser, use, "redundant 'use' statement");
                    }
                }

                for (shift_variable& _var : _parser.m_variables) {
                    _scope.var = &_var;
                    _var.parser_ = &_parser;

                    std::string type_class_name = _var.type.name.to_string();

                    auto type_class_candidates = _scope.find_classes(type_class_name);

                    if (type_class_candidates.size() > 1) {
                        this->m_name_error(_parser, _var.type.name, "ambiguous class reference to '" + type_class_name + "'");
                    } else if (type_class_candidates.size() == 0) {
                        this->m_name_error(_parser, _var.type.name, "unable to resolve class '" + type_class_name + "'");
                    } else {
                        _var.type.name_class = type_class_candidates.front();
                    }

                    if (_var.value.type == token::token_type::NULL_TOKEN) {
                        m_set_null(_var.value);
                    } else {
                        m_resolve_types(&_var.value, &_scope);
                    }

                    if (_var.value.expr_type.name_class != _var.type.name_class) {
                        if (_var.value.expr_type.name_class && _var.type.name_class && _var.value.expr_type.name_class != &m_null_class) {
                            // find function that will implicitly convert type
                            auto conversions = m_get_implicit_conversions(_var.value.expr_type.name_class, _var.type.name_class);
                            if (conversions.size() == 1) {
                                _var.value = m_create_convert_expr(std::move(_var.value), *conversions.front());
                            } else if (conversions.size() > 1) {
                                this->m_token_error(_parser, *(_var.value.begin - 1), "ambiguous type conversion from '"
                                    + _var.value.expr_type.name_class->get_fqn() + "' to '" + _var.type.name_class->get_fqn() + "'");
                            } else {
                                this->m_token_error(_parser, *(_var.value.begin - 1), "unable convert type '"
                                    + _var.value.expr_type.name_class->get_fqn() + "' into class type '" + _var.type.name_class->get_fqn() + "'");
                            }
                        }
                    }

                    if (_var.value.expr_type.name_class && _var.value.expr_type.name_class != &m_void_class && _var.value.expr_type.name_class != &m_null_class
                        && (_var.value.is_function_call() || _var.value.is_array() || _var.value.type == token::token_type::IDENTIFIER)) {
                        m_check_access(&_scope, &_var.value);
                    }
                }

                _scope.var = nullptr;

                for (shift_function& func : _parser.m_functions) {
                    if (!func.return_type.name.begin->is_void()) {
                        std::string return_type_class_name = func.return_type.name.to_string();

                        auto return_type_class_candidates = _scope.find_classes(return_type_class_name);

                        if (return_type_class_candidates.size() > 1) {
                            this->m_name_error(_parser, func.return_type.name, "ambiguous class reference to '" + return_type_class_name + "'");
                        } else if (return_type_class_candidates.size() == 0) {
                            this->m_name_error(_parser, func.return_type.name, "unable to resolve class '" + return_type_class_name + "'");
                        } else {
                            func.return_type.name_class = return_type_class_candidates.front();
                        }
                    }

                    for (auto& [param_name, param_var] : func.parameters) {
                        std::string param_type_class_name = param_var.type.name.to_string();

                        auto param_type_class_candidates = _scope.find_classes(param_type_class_name);

                        if (param_type_class_candidates.size() > 1) {
                            // TODO list candidates
                            this->m_name_error(_parser, param_var.type.name, "ambiguous class reference to '" + param_type_class_name + "' in current scope");
                        } else if (param_type_class_candidates.size() == 0) {
                            this->m_name_error(_parser, param_var.type.name, "unable to resolve class '" + param_type_class_name + "' in current scope");
                        } else {
                            param_var.type.name_class = param_type_class_candidates.front();
                        }
                    }

                    for (size_t i = 0; i < this->m_func_dupe_count[func.get_fqn()]; i++) {
                        shift_function* func_ = this->m_functions[func.get_fqn(i)];
                        if (func_ == &func) continue;
                        if (func_->parameters.size() == func.parameters.size()) {
                            auto func_param = func.parameters.begin();
                            auto other_func_param = func_->parameters.begin();
                            for (;func_param != func.parameters.end() && other_func_param != func_->parameters.end(); other_func_param++, func_param++) {
                                if (func_param->second.type.name_class != other_func_param->second.type.name_class)break;
                            }

                            if (func_param == func.parameters.end() || other_func_param == func_->parameters.end()) {
                                this->m_name_error(_parser, func.name, "duplicate function declaration");
                            }
                        }
                    }

                    m_analyze_function(func, &_scope);
                }

                _scope.func = nullptr;

                for (shift_class& clazz : _parser.m_classes) {
                    _scope.clazz = &clazz;

                    for (auto use = clazz.use_statements.begin(); use != clazz.use_statements.end(); use++) {
                        if (this->m_modules.find(*use) == this->m_modules.end()) {
                            this->m_name_error(_parser, *use, "module '" + use->to_string() + "' does not exist");
                            auto next = clazz.use_statements.erase(use);
                            use = --next;
                        } else if (this->m_error_handler && this->m_error_handler->is_print_warnings()) {
                            if (*use == _parser.m_module) {
                                this->m_name_warning(_parser, *use, "redundant 'use' statement");
                                auto next = clazz.use_statements.erase(use);
                                use = --next;
                            } else {
                                size_t dis = std::distance(_parser.m_global_uses.begin(), _parser.m_global_uses.find(*use));
                                if (dis < clazz.implicit_use_statements) {
                                    this->m_name_warning(_parser, *use, "redundant 'use' statement");
                                    auto next = clazz.use_statements.erase(use);
                                    use = --next;
                                }
                            }
                        }
                    }

                    for (shift_variable& _var : clazz.variables) {
                        _scope.var = &_var;
                        _var.parser_ = &_parser;
                        auto _vars = _scope.find_variables(_var.name);
                        if (_vars.size() > 1) {
                            // TODO fix prompt to account for variable being defined in base class
                            this->m_token_error(_parser, *_var.name, "variable with name '" + std::string(_var.name->get_data()) + "' has already been defined inside class '" + clazz.get_fqn() + "'");
                        }

                        std::string type_class_name = _var.type.name.to_string();

                        auto type_class_candidates = _scope.find_classes(type_class_name);

                        if (type_class_candidates.size() > 1) {
                            this->m_name_error(_parser, _var.type.name, "ambiguous class reference to '" + type_class_name + "'");
                        } else if (type_class_candidates.size() == 0) {
                            this->m_name_error(_parser, _var.type.name, "unable to resolve class '" + type_class_name + "'");
                        } else {
                            _var.type.name_class = type_class_candidates.front();
                        }
                        // TODO set to null automatically if no value was specified
                        if (_var.value.type == token::token_type::NULL_TOKEN) {
                            m_set_null(_var.value);
                        } else {
                            m_resolve_types(&_var.value, &_scope);
                        }

                        if (_var.value.expr_type.name_class != _var.type.name_class) {
                            if (_var.value.expr_type.name_class && _var.type.name_class && _var.value.expr_type.name_class != &m_null_class) {
                                // find function that will implicitly convert type
                                auto conversions = m_get_implicit_conversions(_var.value.expr_type.name_class, _var.type.name_class);
                                if (conversions.size() == 1) {
                                    _var.value = m_create_convert_expr(std::move(_var.value), *conversions.front());
                                } else if (conversions.size() > 1) {
                                    this->m_token_error(_parser, *(_var.value.begin - 1), "ambiguous type conversion from '"
                                        + _var.value.expr_type.name_class->get_fqn() + "' to '" + _var.type.name_class->get_fqn() + "'");
                                } else {
                                    this->m_token_error(_parser, *(_var.value.begin - 1), "unable convert type '"
                                        + _var.value.expr_type.name_class->get_fqn() + "' into class type '" + _var.type.name_class->get_fqn() + "'");
                                }
                            }
                        }

                        if (_var.value.expr_type.name_class && _var.value.expr_type.name_class != &m_void_class && _var.value.expr_type.name_class != &m_null_class
                            && (_var.value.is_function_call() || _var.value.is_array() || _var.value.type == token::token_type::IDENTIFIER)) {
                            m_check_access(&_scope, &_var.value);
                        }
                    }

                    _scope.var = nullptr;

                    for (shift_function& func : clazz.functions) {
                        if (!func.name.begin->is_constructor()) {
                            if (!func.return_type.name.begin->is_void()) {
                                std::string return_type_class_name = func.return_type.name.to_string();

                                auto return_type_class_candidates = _scope.find_classes(return_type_class_name);

                                if (return_type_class_candidates.size() > 1) {
                                    this->m_name_error(_parser, func.return_type.name, "ambiguous class reference to '" + return_type_class_name + "'");
                                } else if (return_type_class_candidates.size() == 0) {
                                    this->m_name_error(_parser, func.return_type.name, "unable to resolve class '" + return_type_class_name + "'");
                                } else {
                                    func.return_type.name_class = return_type_class_candidates.front();
                                }
                            }
                        }

                        for (auto& [param_name, param_var] : func.parameters) {
                            std::string param_type_class_name = param_var.type.name.to_string();

                            auto param_type_class_candidates = _scope.find_classes(param_type_class_name);

                            if (param_type_class_candidates.size() > 1) {
                                // TODO list candidates
                                this->m_name_error(_parser, param_var.type.name, "ambiguous class reference to '" + param_type_class_name + "' in current scope");
                            } else if (param_type_class_candidates.size() == 0) {
                                this->m_name_error(_parser, param_var.type.name, "unable to resolve class '" + param_type_class_name + "' in current scope");
                            } else {
                                param_var.type.name_class = param_type_class_candidates.front();
                            }
                        }

                        for (size_t i = 0; i < this->m_func_dupe_count[func.get_fqn()]; i++) {
                            shift_function* func_ = this->m_functions[func.get_fqn(i)];
                            if (func_ == &func) continue;
                            if (func_->parameters.size() == func.parameters.size()) {
                                auto func_param = func.parameters.begin();
                                auto other_func_param = func_->parameters.begin();
                                for (;func_param != func.parameters.end() && other_func_param != func_->parameters.end(); other_func_param++, func_param++) {
                                    if (func_param->second.type.name_class != other_func_param->second.type.name_class)break;
                                }

                                if (func_param == func.parameters.end() || other_func_param == func_->parameters.end()) {
                                    this->m_name_error(_parser, func.name, "duplicate function declaration");
                                }
                            }
                        }

                        m_analyze_function(func, &_scope);
                    }
                }
            }
            // for (parser& _parser : *m_parsers) {
            //     _scope.parser = &_parser;
            //     for (shift_class& clazz : _parser.m_classes) {
            //         _scope.clazz = &clazz;
            //         for (shift_variable& _var : clazz.variables) {
            //             m_resolve_types(&_var.value, &_scope);

            //             // if (_var.value.class_type != _var.type.name_class) {
            //             //     auto conversions = m_get_conversions(_var.value.class_type, _var.type.name_class);
            //             //     if (conversions.size() == 0) {
            //             //         shift_name expr_name;
            //             //         expr_name.begin = _var.value.begin;
            //             //         expr_name.end = _var.value.end;
            //             //         this->m_name_error(*_scope.parser, expr_name, "cannot implicitly convert type '" + _var.value.class_type->get_fqn() + "' to '" + _var.type.name_class->get_fqn() + "'");
            //             //     } else if (conversions.size() > 1) {
            //             //         shift_name expr_name;
            //             //         expr_name.begin = _var.value.begin;
            //             //         expr_name.end = _var.value.end;
            //             //         this->m_name_error(*_scope.parser, expr_name, "ambiguous implicit type conversion '" + _var.value.class_type->get_fqn() + "' to '" + _var.type.name_class->get_fqn() + "'");
            //             //     } else {
            //             //         shift_expression expr;
            //             //         expr.class_type = _var.type.name_class;
            //             //         expr.function = conversions.front();
            //             //         expr.set_function_call();
            //             //         expr.sub.push_back(std::move(_var.value));
            //             //         _var.value = std::move(expr);
            //             //     }
            //             // }
            //         }

            //         // for (shift_function& func : clazz.functions) {

            //         // }
            //     }
            // }
        }

        void analyzer::m_set_null(shift_expression& value) const noexcept {
            value.type = token::token_type::IDENTIFIER;
            value.begin = m_null_token;
            value.end = value.begin + 1;
            value.expr_type.array_dimensions = 0;
            value.expr_type.name.begin = value.begin;
            value.expr_type.name.end = value.end;
            value.expr_type.name_class = &m_null_class;
            value.clazz = &m_null_class;
            value.function = nullptr;
            value.variable = nullptr;
            value.sub.clear();
        }

        void analyzer::m_resolve_params(shift_function& func, bool silent) {
            scope _scope;
            _scope.base = this;
            _scope.clazz = func.clazz;
            _scope.func = &func;
            for (auto& [param_name, param_var] : func.parameters) {
                if (param_var.type.name_class) continue;
                std::string param_type_class_name = param_var.type.name.to_string();

                auto param_type_class_candidates = _scope.find_classes(param_type_class_name);

                if (param_type_class_candidates.size() > 1) {
                    // TODO list candidates
                    if (!silent)
                        this->m_name_error(*_scope.get_parser(), param_var.type.name, "ambiguous class reference to '" + param_type_class_name + "' in current scope");
                } else if (param_type_class_candidates.size() == 0) {
                    if (!silent)
                        this->m_name_error(*_scope.get_parser(), param_var.type.name, "unable to resolve class '" + param_type_class_name + "' in current scope");
                } else {
                    param_var.type.name_class = param_type_class_candidates.front();
                }
            }
        }

        void analyzer::m_resolve_return_type(shift_function& func, bool silent) {
            scope _scope;
            _scope.base = this;
            _scope.clazz = func.clazz;
            _scope.func = &func;
            
            if (!func.return_type.name_class && func.return_type.name.size() > 0) {
                auto class_candidates = _scope.find_classes(func.return_type.name);
                if (class_candidates.size() == 1) {
                    func.return_type.name_class = class_candidates.front();
                } else if (class_candidates.size() > 1) {
                    if (!silent)
                        this->m_name_error(*_scope.get_parser(), func.return_type.name, "ambiguous class reference to '" + func.return_type.name.to_string() + "' in current scope");
                } else {
                    if (!silent)
                        this->m_name_error(*_scope.get_parser(), func.return_type.name, "unable to resolve class '" + func.return_type.name.to_string() + "' in current scope");
                }
            }
        }

        shift_expression analyzer::m_create_convert_expr(shift_expression&& value, shift_function& convert_func) const {
            if (convert_func.name.size() == 1 && convert_func.name.begin->is_constructor()) {
                shift_expression convert_new_expr;
                convert_new_expr.type = token_type::IDENTIFIER;
                convert_new_expr.begin = m_new_token;
                convert_new_expr.end = convert_new_expr.begin + 1;
                convert_new_expr.function = &convert_func;
                convert_new_expr.expr_type.name_class = convert_func.clazz;

                // TODO remake convert_new_expr to support new 'new' model; maybe make a single function that does this too
                shift_expression convert_func_expr;
                convert_func_expr.set_function_call();
                convert_func_expr.function = &convert_func;

                // TODO fix function call begin and end
                convert_func_expr.begin = is_overload_operator(value.type) ? value.begin : value.begin - 1;
                convert_func_expr.end = convert_func_expr.begin + 1;
                convert_func_expr.sub.push_back(std::move(value));

                convert_new_expr.sub.push_back(std::move(convert_func_expr));

                return convert_new_expr;
            } else {
                return shift_expression();
            }
        }

        void analyzer::m_init_defaults() {
            if (m_token_storage.size() == 0) {
                m_token_storage.reserve(10); // Change each time new token is added

                m_token_storage.emplace_back(std::string_view("void"), token::token_type::IDENTIFIER, file_indexer{ 0,0 });
                m_void_token = --m_token_storage.end();

                m_token_storage.emplace_back(std::string_view("null"), token::token_type::IDENTIFIER, file_indexer{ 0,0 });
                m_null_token = --m_token_storage.end();

                m_token_storage.emplace_back(std::string_view("new"), token::token_type::IDENTIFIER, file_indexer{ 0,0 });
                m_new_token = --m_token_storage.end();

                m_token_storage.emplace_back(std::string_view("array"), token::token_type::IDENTIFIER, file_indexer{ 0,0 });
                m_array_token = --m_token_storage.end();

                m_token_storage.emplace_back(std::string_view("length"), token::token_type::IDENTIFIER, file_indexer{ 0,0 });
                m_length_token = --m_token_storage.end();

                m_token_storage.emplace_back(std::string_view("operator"), token::token_type::IDENTIFIER, file_indexer{ 0,0 });
                m_operator_token = --m_token_storage.end();

                m_token_storage.emplace_back(std::string_view("["), token::token_type::LEFT_SQUARE_BRACKET, file_indexer{ 0,0 });
                m_left_square_bracket_token = --m_token_storage.end();

                m_token_storage.emplace_back(std::string_view("]"), token::token_type::RIGHT_SQUARE_BRACKET, file_indexer{ 0,0 });
                m_right_square_bracket_token = --m_token_storage.end();

                m_token_storage.emplace_back(std::string_view("true"), token::token_type::RIGHT_SQUARE_BRACKET, file_indexer{ 0,0 });
                m_true_token = --m_token_storage.end();

                m_token_storage.emplace_back(std::string_view("="), token::token_type::EQUALS, file_indexer{ 0,0 });
                m_equals_token = --m_token_storage.end();

                m_token_storage.emplace_back(std::string_view("=="), token::token_type::EQUALS_EQUALS, file_indexer{ 0,0 });
                m_equals_equals_token = --m_token_storage.end();

                m_token_storage.emplace_back(std::string_view("!="), token::token_type::NOT_EQUAL, file_indexer{ 0,0 });
                m_not_equal_token = --m_token_storage.end();

                {
                    m_token_storage.emplace_back(std::string_view("shift"), token::token_type::IDENTIFIER, file_indexer{ 0,0 });
                    m_token_storage.emplace_back(std::string_view(";"), token::token_type::IDENTIFIER, file_indexer{ 0,0 });
                    m_shift_module.begin = m_token_storage.end() - 2;
                    m_shift_module.end = m_shift_module.begin + 1;
                }

                {
                    m_void_class.name = m_void_token.operator->();
                    m_null_class.name = m_null_token.operator->();
                }
            }
        }

        utils::ordered_set<shift_function*> analyzer::m_get_implicit_conversions(shift_class* const from, shift_class* const to) const noexcept {
            if (!from || !to || from == &m_void_class || to == &m_void_class || from == &m_null_class
                || from == &m_void_class || to == &m_null_class || to == &m_void_class)
                return utils::ordered_set<shift_function*>();

            utils::ordered_set<shift_function*> funcs;
            for (shift_function& func : to->functions) {
                if (func.name.begin->is_constructor()) {
                    // TODO check if function has explicit keyword
                    // TODO allow default parameters, and then the length will not necessarily have to be 1
                    if (func.parameters.size() == 1) {
                        auto& [param_name, param] = func.parameters.front();
                        if (!param.type.name_class) {
                            scope find_scope;
                            find_scope.base = const_cast<analyzer*>(this);
                            find_scope.clazz = to;
                            find_scope.parser_ = to->parser_;

                            // error handling for ambigiuous class or unresolved class will be taken care of by other functions (e.g. loop in ::analyze())
                            param.type.name_class = find_scope.find_class(param.type.name);
                        }

                        if (param.type.name_class) {
                            if (param.type.name_class == from) {
                                // exact match
                                funcs.clear();
                                funcs.push(&func);
                                return funcs;
                            } else if (from->has_base(param.type.name_class)) {
                                // base class match
                                funcs.push(&func);
                            } else if (param.type.name_class != to) {
                                auto sub_conversions = m_get_implicit_conversions(from, param.type.name_class);
                                if (sub_conversions.size() == 1) {
                                    funcs.push(&func);
                                } else if (sub_conversions.size() > 1) {
                                    for (auto& sub_conversion : sub_conversions) {
                                        funcs.push(std::move(sub_conversion));
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return funcs;
        }

        bool analyzer::m_check_access(scope const* const parent_scope, shift_expression const* expr) {
            if (expr->type == token::token_type::IDENTIFIER || expr->is_function_call() || expr->is_array()) {
                if (expr->size() > 0 && expr->begin->is_new()) {
                    // shift_function const* const constructor = expr->function;
                    // if (!constructor) return true;
                    // if ((constructor->mods & shift_mods::PRIVATE) && parent_scope->clazz != constructor->clazz) {
                    //     // error

                    // } else if ((constructor->mods & shift_mods::PROTECTED) && parent_scope->clazz != constructor->clazz && !parent_scope->clazz->has_base(constructor->clazz)) return &expr->sub.front();
                    // // must be public
                    // expr = &expr->sub.front().sub.front();
                }

                for (shift_expression const& sub_expr : expr->sub) {
                    if (sub_expr.type == token_type::IDENTIFIER) {
                        if (sub_expr.variable) {
                            shift_variable const* const var_ = sub_expr.variable;
                            if (var_->function == parent_scope->func) continue;
                            if ((var_->type.mods & shift_mods::PRIVATE) && parent_scope->clazz != var_->clazz) {
                                this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "field '" + var_->get_fqn() + "' cannot be accessed within current scope");
                                return false;
                            } else if ((var_->type.mods & shift_mods::PROTECTED) && parent_scope->clazz != var_->clazz && !parent_scope->clazz->has_base(var_->clazz)) {
                                this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "field '" + var_->get_fqn() + "' cannot be accessed within current scope");
                                return false;
                            } else if ((var_->type.mods & shift_mods::STATIC) == 0x0) {
                                if (parent_scope->var) {
                                    if ((parent_scope->var->type.mods & shift_mods::STATIC) || !parent_scope->var->clazz) {
                                        this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "non-static field '" + var_->get_fqn() + "' cannot be accessed within current scope");
                                        return false;
                                    }
                                } else if (parent_scope->func) {
                                    if ((parent_scope->func->mods & shift_mods::STATIC) || !parent_scope->func->clazz) {
                                        this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "non-static field '" + var_->get_fqn() + "' cannot be accessed within current scope");
                                        return false;
                                    }
                                }
                            }
                        } else if (sub_expr.clazz) {
                            // TODO do class modifiers do anything currently?
                        } else {
                            if (expr->size() && expr->begin->is_new()) {
                                if (!m_check_access(parent_scope, &sub_expr)) return false;
                            }
                        }
                    } else if (sub_expr.is_function_call()) {
                        // TODO don't forget to check for static functions
                        shift_function* const func = sub_expr.function;
                        if (!func) continue;
                        if ((func->mods & shift_mods::PRIVATE) && parent_scope->clazz != func->clazz) {
                            m_resolve_params(*func, true);
                            this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "function '" + func->get_signature() + "' cannot be accessed within current scope");
                            return false;
                        } else if ((func->mods & shift_mods::PROTECTED) && parent_scope->clazz != func->clazz && !parent_scope->clazz->has_base(func->clazz)) {
                            m_resolve_params(*func, true);
                            this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "function '" + func->get_signature() + "' cannot be accessed within current scope");
                            return false;
                        } else if ((func->mods & shift_mods::STATIC) == 0x0) {
                            if (parent_scope->var) {
                                if ((parent_scope->var->type.mods & shift_mods::STATIC) || !parent_scope->var) {
                                    m_resolve_params(*func, true);
                                    this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "non-static function '" + func->get_signature() + "' cannot be accessed within current scope");
                                    return false;
                                }
                            } else if (parent_scope->func) {
                                if ((parent_scope->func->mods & shift_mods::STATIC) || !parent_scope->func) {
                                    m_resolve_params(*func, true);
                                    this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "non-static function '" + func->get_signature() + "' cannot be accessed within current scope");
                                    return false;
                                }
                            }
                        }

                        auto const& func_params = sub_expr.sub.front();

                        if (!m_check_access(parent_scope, &func_params)) return false;
                    } else if (sub_expr.is_array()) {
                        if (!m_check_access(parent_scope, &sub_expr)) return false;
                        // {
                        //     shift_expression const& index_expr = sub_expr.sub.front();
                        //     if (index_expr.is_function_call()) {
                        //         shift_function const* const func = index_expr.function;
                        //         if ((func->mods & shift_mods::PRIVATE) && parent_scope->clazz != func->clazz) {
                        //             this->m_token_error(*parent_scope->get_parser(), *index_expr.begin, "function cannot be accessed within current scope");
                        //             return false;
                        //         } else if ((func->mods & shift_mods::PROTECTED) && parent_scope->clazz != func->clazz && !parent_scope->clazz->has_base(func->clazz)) {
                        //             this->m_token_error(*parent_scope->get_parser(), *index_expr.begin, "function cannot be accessed within current scope");
                        //             return false;
                        //         } else if ((func->mods & shift_mods::STATIC) == 0x0) {
                        //             if (parent_scope->var) {
                        //                 if ((parent_scope->var->type.mods & shift_mods::STATIC) || !parent_scope->var) {
                        //                     this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "static function '" + func->get_signature() + "' cannot be accessed within current scope");
                        //                     return false;
                        //                 }
                        //             } else if (parent_scope->func) {
                        //                 if ((parent_scope->func->mods & shift_mods::STATIC) || !parent_scope->func) {
                        //                     this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "static function '" + func->get_signature() + "' cannot be accessed within current scope");
                        //                     return false;
                        //                 }
                        //             }
                        //         }

                        //         auto const& func_params = sub_expr.sub.front();

                        //         if (!m_check_access(parent_scope, &func_params)) return false;
                        //     } else if (index_expr.type == token_type::IDENTIFIER) {
                        //         shift_variable const* const var_ = sub_expr.variable;
                        //         if ((var_->type.mods & shift_mods::PRIVATE) && parent_scope->clazz != var_->clazz) {
                        //             this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "field '" + var_->get_fqn() + "' cannot be accessed within current scope");
                        //             return false;
                        //         } else if ((var_->type.mods & shift_mods::PROTECTED) && parent_scope->clazz != var_->clazz && !parent_scope->clazz->has_base(var_->clazz)) {
                        //             this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "field '" + var_->get_fqn() + "' cannot be accessed within current scope");
                        //             return false;
                        //         } else if ((var_->type.mods & shift_mods::STATIC) == 0x0) {
                        //             if (parent_scope->var) {
                        //                 if ((parent_scope->var->type.mods & shift_mods::STATIC) || !parent_scope->var->clazz) {
                        //                     this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "static field '" + var_->get_fqn() + "' cannot be accessed within current scope");
                        //                     return false;
                        //                 }
                        //             } else if (parent_scope->func) {
                        //                 if ((parent_scope->func->mods & shift_mods::STATIC) || !parent_scope->func->clazz) {
                        //                     this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "static field '" + var_->get_fqn() + "' cannot be accessed within current scope");
                        //                     return false;
                        //                 }
                        //             }
                        //         }
                        //     }
                        // }

                        // if (sub_expr.function) {
                        //     // Accessing operator overload []
                        //     shift_function const* const func = sub_expr.function;
                        //     if ((func->mods & shift_mods::PRIVATE) && parent_scope->clazz != func->clazz) {
                        //         this->m_token_error(*parent_scope->parser, *sub_expr.begin, "function cannot be accessed within current scope");
                        //         return false;
                        //     } else if ((func->mods & shift_mods::PROTECTED) && parent_scope->clazz != func->clazz && !parent_scope->clazz->has_base(func->clazz)) {
                        //         this->m_token_error(*parent_scope->parser, *sub_expr.begin, "function cannot be accessed within current scope");
                        //         return false;
                        //     }
                        //     //find_scope.clazz = func->return_type.name_class;
                        // } else {
                        //     // returns a basic array
                        //     // TODO make array return type
                        // }
                    }
                }
            } else if (expr->type == token::token_type::COMMA) {
                for (shift_expression const& sub_expr : expr->sub) {
                    if (!m_check_access(parent_scope, &sub_expr)) return false;
                }
            } else if (is_overload_operator(expr->type)) {
                if (expr->function) {
                    shift_function* const func = expr->function;

                    if ((func->mods & shift_mods::PRIVATE) && parent_scope->clazz != func->clazz) {
                        m_resolve_params(*func, true);
                        this->m_token_error(*parent_scope->get_parser(), *expr->begin, "function '" + func->get_signature() + "' cannot be accessed within current scope");
                        return false;
                    } else if ((func->mods & shift_mods::PROTECTED) && parent_scope->clazz != func->clazz && !parent_scope->clazz->has_base(func->clazz)) {
                        m_resolve_params(*func, true);
                        this->m_token_error(*parent_scope->get_parser(), *expr->begin, "function '" + func->get_signature() + "' cannot be accessed within current scope");
                        return false;
                    } else if ((func->mods & shift_mods::STATIC) == 0x0) {
                        if (parent_scope->var) {
                            if ((parent_scope->var->type.mods & shift_mods::STATIC) || !parent_scope->var) {
                                m_resolve_params(*func, true);
                                this->m_token_error(*parent_scope->get_parser(), *expr->begin, "non-static function '" + func->get_signature() + "' cannot be accessed within current scope");
                                return false;
                            }
                        } else if (parent_scope->func) {
                            if ((parent_scope->func->mods & shift_mods::STATIC) || !parent_scope->func) {
                                m_resolve_params(*func, true);
                                this->m_token_error(*parent_scope->get_parser(), *expr->begin, "non-static function '" + func->get_signature() + "' cannot be accessed within current scope");
                                return false;
                            }
                        }
                    }
                }

                const bool has_left = expr->has_left() && expr->get_left()->type != token::token_type::NULL_TOKEN;
                const bool has_right = expr->has_right() && expr->get_right()->type != token::token_type::NULL_TOKEN;

                if (has_left && has_right) {
                    if (!m_check_access(parent_scope, expr->get_left()) || !m_check_access(parent_scope, expr->get_right())) return false;
                } else if (has_left && !has_right) {
                    if (!m_check_access(parent_scope, expr->get_left())) return false;
                } else if (!has_left && has_right) {
                    if (!m_check_access(parent_scope, expr->get_right())) return false;
                }
            }
            return true;
        }

        void analyzer::m_analyze_function(shift_function& func, scope* parent_scope) {
            // shift_function* old_func = nullptr;

            // if (parent_scope) {
            //     old_func = parent_scope->func;
            //     parent_scope->func = &func;
            // }

            // m_analyze_scope(func.statements, parent_scope);

            // if (parent_scope) {
            //     parent_scope->func = old_func;
            // }
            scope func_scope;
            func_scope.base = this;
            func_scope.parser_ = parent_scope->parser_;
            func_scope.clazz = parent_scope->clazz;
            func_scope.func = &func;
            func_scope.parent = parent_scope;
            m_analyze_scope(func.statements, &func_scope);
        }

        void analyzer::m_analyze_scope(typename std::list<shift_statement>::iterator statements_begin, typename std::list<shift_statement>::iterator statements_end,
            scope* parent_scope) {
            scope _scope;
            if (parent_scope) {
                _scope.parent = parent_scope;
                _scope.base = parent_scope->base;
                _scope.parser_ = parent_scope->parser_;
                _scope.clazz = parent_scope->clazz;
                _scope.func = parent_scope->func;
                _scope.var = parent_scope->var;
            } else {
                _scope.base = this;
            }

            for (;statements_begin != statements_end;statements_begin++) {
                shift_statement& statement = *statements_begin;

                switch (statement.type) {
                    case shift_statement::statement_type::variable_alloc:
                    {
                        shift_variable& statement_var = statement.get_variable();

                        if (_scope.variables.find(statement_var.name->get_data()) != _scope.variables.end() || _scope.func->parameters.contains(statement_var.name->get_data())) {
                            this->m_token_error(*_scope.get_parser(), *statement_var.name, "variable with name '" + std::string(statement_var.name->get_data()) + "' has already been defined in current scope");
                        } else if (this->m_error_handler && this->m_error_handler->is_print_warnings()) {
                            auto found_vars = _scope.find_variables(statement_var.name->get_data());
                            if (found_vars.size() >= 1) {
                                shift_variable& found_var = *found_vars.front();
                                if (found_var.function) {
                                    this->m_token_warning(*_scope.get_parser(), *statement_var.name, "variable masks variable with identical name in upper scope");
                                } else if (found_var.clazz) {
                                    this->m_token_warning(*_scope.get_parser(), *statement_var.name, "variable masks variable with identical name in class '" + found_var.clazz->get_fqn() + "'");
                                }
                            }
                        }

                        _scope.variables[statement_var.name->get_data()] = &statement_var;

                        auto statement_var_classes = _scope.find_classes(statement_var.type.name);
                        if (statement_var_classes.size() > 1) {
                            this->m_name_error(*_scope.get_parser(), statement_var.type.name, "ambiguous class reference to '" + statement_var.type.name.to_string() + "'");
                        } else if (statement_var_classes.size() == 0) {
                            this->m_name_error(*_scope.get_parser(), statement_var.type.name, "unable to resolve class '" + statement_var.type.name.to_string() + "'");
                        } else {
                            statement_var.type.name_class = statement_var_classes.front();
                        }

                        if (statement_var.value.type != token::token_type::NULL_TOKEN) {
                            m_resolve_types(&statement_var.value, &_scope);
                        } else {
                            m_set_null(statement_var.value);
                        }

                        if (statement_var.value.expr_type.name_class != statement_var.type.name_class) {
                            if (statement_var.value.expr_type.name_class && statement_var.type.name_class && statement_var.value.expr_type.name_class != &m_null_class) {
                                // find function that will implicitly convert type
                                auto conversions = m_get_implicit_conversions(statement_var.value.expr_type.name_class, statement_var.type.name_class);
                                if (conversions.size() == 1) {
                                    statement_var.value = m_create_convert_expr(std::move(statement_var.value), *conversions.front());
                                } else if (conversions.size() > 1) {
                                    this->m_token_error(*_scope.get_parser(), *(statement_var.value.begin - 1), "ambiguous type conversion from '"
                                        + statement_var.value.expr_type.name_class->get_fqn() + "' to '" + statement_var.type.name_class->get_fqn() + "'");
                                } else {
                                    this->m_token_error(*_scope.get_parser(), *(statement_var.value.begin - 1), "unable convert type '"
                                        + statement_var.value.expr_type.name_class->get_fqn() + "' into class type '" + statement_var.type.name_class->get_fqn() + "'");
                                }
                            }
                        }
                    }
                    break;
                    case shift_statement::statement_type::expression:
                    {
                        if (statement.get_expression().type != token::token_type::NULL_TOKEN) {
                            m_resolve_types(&statement.get_expression(), &_scope);
                            if (statement.get_expression().expr_type.name_class) {
                                m_check_access(&_scope, &statement.get_expression());
                            }
                        }

                        // TODO remove from list otherwise
                    }
                    break;
                    case shift_statement::statement_type::scope_begin:
                    {
                        m_analyze_scope(statement.get_block_statements(), &_scope);
                    }
                    break;
                    case shift_statement::statement_type::if_:
                    {
                        shift_expression& if_condition = statement.get_if_condition();
                        m_resolve_types(&if_condition, &_scope);

                        if (if_condition.expr_type.name_class) {
                            m_check_access(&_scope, &if_condition);
                            if (if_condition.expr_type.name_class != m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS]) {
                                auto conversions = m_get_implicit_conversions(if_condition.expr_type.name_class, m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS]);
                                if (conversions.size() == 1) {
                                    if_condition = m_create_convert_expr(std::move(if_condition), *conversions.front());
                                } else if (conversions.size() > 1) {
                                    this->m_token_error(*_scope.get_parser(), *statement.get_if(), "ambiguous conversion of conditional value of type '" + if_condition.expr_type.name_class->get_fqn() + "' to type '" SHIFT_ANALYZER_BOOLEAN_CLASS "'");
                                } else if (conversions.size() == 0) {
                                    this->m_token_error(*_scope.get_parser(), *statement.get_if(), "cannot convert conditional value of type '" + if_condition.expr_type.name_class->get_fqn() + "' into type '" SHIFT_ANALYZER_BOOLEAN_CLASS "'");
                                }
                            }
                        }

                        m_analyze_scope(statement.get_if_statements(), &_scope);

                        if (statement.get_attached_else()) {
                            m_analyze_scope(statement.get_attached_else()->get_else_statements(), &_scope);
                        }
                    }
                    break;
                    case shift_statement::statement_type::else_:
                    {
                        // Skipping past else statement storage, located as the last statement inside an if statement
                        break;
                    }
                    break;
                    case shift_statement::statement_type::while_:
                    {
                        shift_expression& while_condition = statement.get_while_condition();
                        m_resolve_types(&while_condition, &_scope);

                        if (while_condition.expr_type.name_class) {
                            m_check_access(&_scope, &while_condition);
                            if (while_condition.expr_type.name_class != m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS]) {
                                auto conversions = m_get_implicit_conversions(while_condition.expr_type.name_class, m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS]);
                                if (conversions.size() == 1) {
                                    while_condition = m_create_convert_expr(std::move(while_condition), *conversions.front());
                                } else if (conversions.size() > 1) {
                                    this->m_token_error(*_scope.get_parser(), *statement.get_while(), "ambiguous conversion of conditional value of type '" + while_condition.expr_type.name_class->get_fqn() + "' to type '" SHIFT_ANALYZER_BOOLEAN_CLASS "'");
                                } else if (conversions.size() == 0) {
                                    this->m_token_error(*_scope.get_parser(), *statement.get_while(), "cannot convert conditional value of type '" + while_condition.expr_type.name_class->get_fqn() + "' into type '" SHIFT_ANALYZER_BOOLEAN_CLASS "'");
                                }
                            }
                        }

                        m_analyze_scope(statement.get_while_statements(), &_scope);
                    }
                    break;
                    case shift_statement::statement_type::for_:
                    {
                        {
                            m_analyze_scope(statement.sub.begin(), ++statement.sub.begin(), &_scope);
                        }
                        {
                            shift_expression& for_condition = statement.get_for_condition();
                            if (for_condition.type == token::token_type::NULL_TOKEN) {
                                // set to true
                                for_condition.expr_type.name_class = m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS];
                                for_condition.begin = m_true_token;
                                for_condition.end = for_condition.begin + 1;
                                for_condition.type = token::token_type::IDENTIFIER;
                            } else {
                                m_resolve_types(&for_condition, &_scope);
                                if (for_condition.expr_type.name_class) {
                                    m_check_access(&_scope, &for_condition);

                                    if (for_condition.expr_type.name_class != m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS]) {
                                        auto conversions = m_get_implicit_conversions(for_condition.expr_type.name_class, m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS]);
                                        if (conversions.size() == 1) {
                                            for_condition = m_create_convert_expr(std::move(for_condition), *conversions.front());
                                        } else if (conversions.size() > 1) {
                                            this->m_token_error(*_scope.get_parser(), *statement.get_for(), "ambiguous conversion of conditional value of type '" + for_condition.expr_type.name_class->get_fqn() + "' to type '" SHIFT_ANALYZER_BOOLEAN_CLASS "'");
                                        } else if (conversions.size() == 0) {
                                            this->m_token_error(*_scope.get_parser(), *statement.get_for(), "cannot convert conditional value of type '" + for_condition.expr_type.name_class->get_fqn() + "' into type '" SHIFT_ANALYZER_BOOLEAN_CLASS "'");
                                        }
                                    }
                                }
                            }
                        }

                        {
                            m_analyze_scope(++statement.sub.begin(), statement.sub.end(), &_scope);
                        }

                    }
                    break;
                    case shift_statement::statement_type::break_:
                    {
                        shift_statement* temp_parent = statement.parent;
                        for (;temp_parent;temp_parent = temp_parent->parent) {
                            if (temp_parent->type == shift_statement::statement_type::while_ || temp_parent->type == shift_statement::statement_type::for_) {
                                statement.set_break_link(temp_parent);
                                break;
                            }
                        }

                        if (temp_parent == nullptr) {
                            this->m_token_error(*_scope.get_parser(), *statement.get_break(), "unexpected 'break' inside current scope");
                        }
                    }
                    break;
                    case shift_statement::statement_type::continue_:
                    {
                        shift_statement* temp_parent = statement.parent;
                        for (;temp_parent;temp_parent = temp_parent->parent) {
                            if (temp_parent->type == shift_statement::statement_type::while_ || temp_parent->type == shift_statement::statement_type::for_) {
                                statement.set_continue_link(temp_parent);
                                break;
                            }
                        }

                        if (temp_parent == nullptr) {
                            this->m_token_error(*_scope.get_parser(), *statement.get_continue(), "unexpected 'continue' inside current scope");
                        }
                    }
                    break;
                    case shift_statement::statement_type::return_:
                    {
                        shift_expression& return_statement = statement.get_return_statement();
                        if (return_statement.type != token::token_type::NULL_TOKEN) {
                            m_resolve_types(&return_statement, &_scope);

                            if (return_statement.expr_type.name_class) {
                                m_check_access(&_scope, &return_statement);
                            }
                        }

                        if (_scope.func->name.begin->is_constructor() || _scope.func->return_type.name.begin->is_void()) {
                            if (return_statement.type != token::token_type::NULL_TOKEN) {
                                m_resolve_types(&return_statement, &_scope);
                                if (return_statement.expr_type.name_class != &m_void_class) {
                                    this->m_token_error(*_scope.get_parser(), *statement.get_return(), "unexpected return value in function with return type 'void'");
                                }
                            }
                        } else {
                            if (return_statement.type == token::token_type::NULL_TOKEN || !return_statement.expr_type.name_class) {
                                if (_scope.func->return_type.name_class) {
                                    this->m_token_error(*_scope.get_parser(), *statement.get_return(), "expected return value of type '" + _scope.func->return_type.name_class->get_fqn() + "'");
                                } else {
                                    this->m_token_error(*_scope.get_parser(), *statement.get_return(), "expected return value of type '" + _scope.func->return_type.name.to_string() + "'");
                                }
                            } else {
                                if (_scope.func->return_type.name_class && return_statement.expr_type.name_class && return_statement.expr_type != _scope.func->return_type && !return_statement.expr_type.name_class->has_base(_scope.func->return_type.name_class) && return_statement.expr_type.name_class != &m_null_class) {
                                    auto conversions = m_get_implicit_conversions(return_statement.expr_type.name_class, _scope.func->return_type.name_class);
                                    if (conversions.size() == 1) {
                                        return_statement = m_create_convert_expr(std::move(return_statement), *conversions.front());
                                    } else if (conversions.size() > 1) {
                                        this->m_token_error(*_scope.get_parser(), *statement.get_return(), "ambiguous conversion of return value of type '" + return_statement.expr_type.name_class->get_fqn() + "' to type '" + _scope.func->return_type.name_class->get_fqn() + "'");
                                    } else if (conversions.size() == 0) {
                                        this->m_token_error(*_scope.get_parser(), *statement.get_return(), "cannot convert return value of type '" + return_statement.expr_type.name_class->get_fqn() + "' into type '" + _scope.func->return_type.name_class->get_fqn() + "'");
                                    }
                                }

                            }
                        }
                    }
                    break;
                    case shift_statement::statement_type::use:
                    {
                        shift_module const& use_module = statement.get_use_module();
                        if (!contains_module(use_module)) {
                            this->m_name_error(*_scope.get_parser(), use_module, "module '" + use_module.to_string() + "' does not exist");
                        } else if (_scope.using_module(use_module)) {
                            this->m_name_warning(*_scope.get_parser(), use_module, "redundant 'use' statement");
                        } else {
                            _scope.use_modules.emplace(statement.get_use_module());
                        }
                    }
                    break;

                    default:
                        // error?
                        break;
                }
            }
        }

        void analyzer::m_resolve_types(shift_expression* expr, scope* const parent_scope) {
            if (expr->type == token::token_type::IDENTIFIER) {
                if (expr->size() == 1) {
                    if (expr->begin->is_true() || expr->begin->is_false()) {
                        expr->expr_type.name_class = parent_scope->base->m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS];
                        return;
                    } else if (expr->begin->is_null()) {
                        expr->expr_type.name_class = &m_null_class;
                        return;
                    } else if (expr->begin->is_new()) {
                        shift_name new_class_name;
                        if (expr->sub.front().is_function_call()) {
                            // expr->sub.front() -> express the entire expression ends in a func call
                            // expr->sub.front().sub.back() -> the actual func call expression
                            // (see parser for more information)

                            new_class_name.begin = expr->sub.front().sub.front().begin;
                            new_class_name.end = expr->sub.front().sub.back().end;
                        } else if (expr->sub.front().is_array()) {
                            // expr->sub.front() -> express the entire expression ends in an array indexing expression
                            // expr->sub.front().sub.back() -> the actual array indexing expression
                            // (see parser for more information)

                            new_class_name.begin = expr->sub.front().sub.front().begin;
                            new_class_name.end = expr->sub.front().sub.back().end;

                            // This should also be valid for new_class_name.end
                            // new_class_name.end = expr->sub.front().sub.back().sub.front().end;
                        }

                        auto classes = parent_scope->find_classes(new_class_name);

                        if (classes.size() == 1) {
                            expr->expr_type.name = new_class_name;
                            expr->expr_type.name_class = classes.front();
                            if (expr->sub.front().is_array()) {
                                // look at shift_parser.cpp for reference on how array indexing is stored
                                // - 1 is because the first item in sub holds what we are actaually indexing
                                expr->expr_type.array_dimensions = expr->sub.front().sub.back().sub.size() - 1;

                                // TODO make sure array indexers are valid integers (or longs?)
                                for (auto b = ++expr->sub.front().sub.back().sub.begin(); b != expr->sub.front().sub.back().sub.end(); b++) {
                                    m_resolve_types(&*b, parent_scope);
                                    if (b->expr_type.name_class && ((b->expr_type.name_class != m_classes[SHIFT_ANALYZER_UINT64_CLASS] && b->expr_type.name_class != m_classes[SHIFT_ANALYZER_UINT32_CLASS]) || b->expr_type.array_dimensions > 0)) {
                                        // auto conversions = m_get_implicit_conversions(b->expr_type.name_class, b->expr_type.name_class != m_classes[SHIFT_ANALYZER_UINT64_CLASS]);
                                        // if (conversions.size() == 1) {

                                        // }
                                        this->m_token_error(*parent_scope->get_parser(), *(b->begin - 1), "cannot convert integral value of type '" + b->expr_type.get_fqn() + "' into type '" SHIFT_ANALYZER_UINT64_CLASS "'");
                                    }
                                }
                            } else {
                                shift_expression& param_exprs = expr->sub.front().sub.back().sub.front();
                                size_t const param_count = param_exprs.sub.front().type == token::token_type::NULL_TOKEN ? 0 : param_exprs.sub.size();

                                if (param_count > 0) {
                                    for (shift_expression& param_expr : param_exprs.sub) {
                                        m_resolve_types(&param_expr, parent_scope);
                                    }
                                }

                                scope find_scope;
                                find_scope.base = parent_scope->base;
                                find_scope.clazz = classes.front();

                                // TODO maybe either make a separate list for constructors and the single destructor or make an unordered_map which maps function names to shift_function*'s
                                auto constructors = find_scope.find_functions("constructor");

                                for (auto b = constructors.begin(); b != constructors.end(); ++b) {
                                    shift_function& func = **b;
                                    // TODO allow default parameters so that the sizes dont have to be to the same (change to >=)
                                    if (func.parameters.size() == param_count) {
                                        auto func_param_it = func.parameters.begin();
                                        auto expr_param_it = param_exprs.sub.begin();

                                        for (; func_param_it != func.parameters.end() && expr_param_it != param_exprs.sub.end(); ++expr_param_it, ++func_param_it) {
                                            auto& [_name, param] = *func_param_it;
                                            if (!param.type.name_class) {
                                                auto param_classes = find_scope.find_classes(param.type.name);
                                                if (param_classes.size() == 1) {
                                                    param.type.name_class = param_classes.front();
                                                } else if (param_classes.size() > 1) {
                                                    this->m_name_error(*find_scope.get_parser(), param.type.name, "ambiguous class reference to '" + param.type.name.to_string() + "' in scope");
                                                } else {
                                                    this->m_name_error(*find_scope.get_parser(), param.type.name, "unable to resolve class '" + param.type.name.to_string() + "' in scope");
                                                }
                                            }

                                            // TODO change simple parameter type deduction implementation
                                            if (param.type.name_class && param.type.name_class != expr_param_it->expr_type.name_class) {
                                                auto next = constructors.erase(b);
                                                b = --next;
                                                break;
                                            }
                                        }
                                    } else {
                                        // TODO allow default parameters
                                        auto next = constructors.erase(b);
                                        b = --next;
                                    }
                                }

                                if (constructors.size() == 1) {
                                    expr->function = constructors.front();
                                    expr->sub.front().function = constructors.front();
                                } else {
                                    std::string function_signature = find_scope.clazz->get_fqn() + ".constructor";
                                    function_signature += '(';
                                    {
                                        size_t param_count = param_exprs.sub.size() == 1 && param_exprs.sub.front().type == token::token_type::NULL_TOKEN ? 0 : param_exprs.sub.size();
                                        size_t param_index = 1;
                                        if (param_count > 0) {
                                            for (shift_expression& expr_param : param_exprs.sub) {
                                                if (expr_param.expr_type.name_class) {
                                                    function_signature += expr_param.expr_type.name_class->get_fqn();
                                                } else {
                                                    function_signature += "<unknown>";
                                                }
                                                if (param_index < param_count)
                                                    function_signature += ", ";
                                                param_index++;
                                            }
                                        }
                                    }
                                    function_signature += ')';

                                    if (constructors.size() > 1) {
                                        // this->m_name_error(*find_scope.parser, sub_expr.function->return_type.name, "ambiguous function reference to '" + function_signature + "' in scope");
                                        this->m_name_error(*parent_scope->get_parser(), new_class_name, "ambiguous function reference to '" + function_signature + "' in scope");
                                    } else {
                                        this->m_name_error(*parent_scope->get_parser(), new_class_name, "unable to resolve function '" + function_signature + "' in class '" + find_scope.clazz->get_fqn() + "' in current scope");
                                    }
                                }
                            }
                        } else if (classes.size() > 1) {
                            this->m_name_error(*parent_scope->get_parser(), new_class_name, "ambiguous class reference to '" + new_class_name.to_string() + "' in scope");
                        } else {
                            this->m_name_error(*parent_scope->get_parser(), new_class_name, "unable to resolve class '" + new_class_name.to_string() + "' in scope");
                        }
                        return;
                    }
                }
            }

            if (expr->type == token::token_type::IDENTIFIER || expr->is_function_call() || expr->is_array()) {
                scope find_scope;
                find_scope.parent = parent_scope;
                find_scope.base = parent_scope->base;
                //find_scope.parser_ = parent_scope->parser_;
                find_scope.clazz = parent_scope->clazz;
                find_scope.func = parent_scope->func;
                find_scope.var = parent_scope->var;

                // TODO fix copy of variables
                //find_scope.variables = parent_scope->variables;

                bool can_fqn = true;
                for (auto expr_it = expr->sub.begin(); expr_it != expr->sub.end(); expr_it++) {
                    shift_expression& sub_expr = *expr_it;
                    if (sub_expr.type == token::token_type::IDENTIFIER) {
                        // variable
                        if (find_scope.clazz == &m_void_class) {
                            this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "cannot access field on void type");
                            return;
                        }

                        shift_name var_name;

                        if (can_fqn) {
                            var_name.begin = expr->sub.front().begin;
                            var_name.end = sub_expr.end;
                        } else {
                            var_name.begin = sub_expr.begin;
                            var_name.end = sub_expr.end;
                        }

                        auto vars = find_scope.find_variables(var_name);
                        if (vars.size() == 1) {
                            shift_variable* found_var = vars.front();
                            find_scope.clazz = found_var->clazz;
                            find_scope.func = nullptr;
                            find_scope.var = found_var;
                            find_scope.parser_ = found_var->parser_;

                            if (!found_var->type.name_class) {
                                auto found_var_classes = find_scope.find_classes(found_var->type.name);

                                if (found_var_classes.size() == 1) {
                                    found_var->type.name_class = found_var_classes.front();
                                } else if (found_var_classes.size() > 1) {
                                    this->m_name_error(*find_scope.get_parser(), found_var->type.name, "ambiguous class reference to '" + found_var->type.name.to_string() + "' in scope");
                                    return;
                                } else {
                                    this->m_name_error(*find_scope.get_parser(), found_var->type.name, "unable to resolve class '" + found_var->type.name.to_string() + "' in scope");
                                    return;
                                }
                            }

                            find_scope.clazz = found_var->type.name_class;
                            find_scope.parser_ = find_scope.clazz ? find_scope.clazz->parser_ : nullptr;
                            find_scope.func = nullptr;
                            find_scope.var = nullptr;
                            sub_expr.expr_type.name_class = find_scope.clazz;
                            sub_expr.expr_type.array_dimensions = found_var->type.array_dimensions;
                            sub_expr.variable = found_var;
                            can_fqn = false;
                        } else if (vars.size() > 1) {
                            // TODO list candidates
                            this->m_name_error(*parent_scope->get_parser(), var_name, "ambiguous variable reference to '" + var_name.to_string() + "' in current scope");
                            return;
                        } else {
                            // TODO check for class
                            if (can_fqn) {
                                shift_name class_name;
                                class_name.begin = expr->sub.front().begin;
                                class_name.end = expr_it->end;
                                auto classes = parent_scope->find_classes(class_name);

                                if (classes.size() == 1) {
                                    find_scope.clazz = classes.front();
                                    find_scope.parser_ = classes.front()->parser_;
                                    find_scope.func = nullptr;
                                    find_scope.var = nullptr;
                                } else if (classes.size() > 1) {
                                    this->m_token_error(*parent_scope->get_parser(), *expr_it->begin, "ambiguous class reference to '" + class_name.to_string() + "'");
                                    return;
                                } else {
                                    auto cur = expr_it;
                                    auto next = ++expr_it;
                                    if (next == expr->sub.end() || (next->type != token::token_type::IDENTIFIER && !next->is_function_call())) {
                                        this->m_token_error(*parent_scope->get_parser(), *cur->begin, "unable to resolve class or variable reference to '" + class_name.to_string() + "'");
                                        return;
                                    }
                                    expr_it = cur;
                                }
                            } else {
                                this->m_token_error(*parent_scope->get_parser(), *sub_expr.begin, "unable to resolve variable '" + std::string(sub_expr.begin->get_data()) + "' in class '" + find_scope.clazz->get_fqn() + "' in current scope");
                                return;
                            }
                        }
                    } else if (sub_expr.is_function_call()) {
                        // function call

                        shift_name func_name;

                        if (can_fqn) {
                            func_name.begin = expr->sub.front().begin;
                            func_name.end = sub_expr.end;
                        } else {
                            func_name.begin = sub_expr.begin;
                            func_name.end = sub_expr.end;
                        }

                        auto functions = find_scope.find_functions(func_name.to_string());
                        shift_expression& param_expr = sub_expr.sub.front();
                        size_t const param_count = param_expr.type == token::token_type::COMMA ? param_expr.sub.size() : bool(param_expr.size());

                        if (param_count > 0) {
                            for (shift_expression& expr_param : param_expr.sub) {
                                m_resolve_types(&expr_param, parent_scope);
                                if (expr_param.expr_type.name_class == nullptr) {
                                    return;
                                }
                            }
                        }

                        for (auto b = functions.begin(); b != functions.end(); ++b) {
                            shift_function& func = **b;
                            find_scope.func = &func;
                            if (func.parameters.size() == param_count) {
                                // TODO parameter type deduction
                                auto func_param_it = func.parameters.begin();
                                auto expr_param_it = param_expr.sub.begin();
                                for (; func_param_it != func.parameters.end() && expr_param_it != param_expr.sub.end(); ++func_param_it, ++expr_param_it) {
                                    auto& [_name, param] = *func_param_it;
                                    if (!param.type.name_class) {
                                        auto param_classes = find_scope.find_classes(param.type.name);

                                        if (param_classes.size() == 1) {
                                            param.type.name_class = param_classes.front();
                                        } else if (param_classes.size() > 1) {
                                            this->m_name_error(*find_scope.get_parser(), param.type.name, "ambiguous class reference to '" + param.type.name.to_string() + "' in scope");
                                        } else {
                                            this->m_name_error(*find_scope.get_parser(), param.type.name, "unable to resolve class '" + param.type.name.to_string() + "' in scope");
                                        }
                                    }

                                    // simple parameter type deduction implementation, TODO change
                                    if (param.type.name_class && param.type.name_class != expr_param_it->expr_type.name_class) {
                                        auto conversions = m_get_implicit_conversions(expr_param_it->expr_type.name_class, param.type.name_class);
                                        if (conversions.size() != 1) {
                                            auto next = functions.erase(b);
                                            b = --next;
                                            break;
                                        }
                                    }
                                }
                            } else {
                                auto next = functions.erase(b);
                                b = --next;
                            }
                        }

                        if (functions.size() == 1) {
                            sub_expr.function = functions.front();

                            find_scope.func = functions.front();
                            find_scope.clazz = find_scope.func->clazz;
                            find_scope.parser_ = find_scope.func->parser_;


                            if (sub_expr.function->name.begin->is_constructor()) {
                                // do nothing, keep same class as constructor itself
                            } else if (sub_expr.function->name.begin->is_destructor() || sub_expr.function->return_type.name.begin->is_void()) {
                                find_scope.clazz = &m_void_class;
                                find_scope.parser_ = nullptr;
                            } else if (!sub_expr.function->return_type.name_class) {
                                auto found_func_classes = find_scope.find_classes(sub_expr.function->return_type.name);

                                if (found_func_classes.size() == 1) {
                                    sub_expr.function->return_type.name_class = found_func_classes.front();
                                } else if (found_func_classes.size() > 1) {
                                    this->m_name_error(*find_scope.get_parser(), sub_expr.function->return_type.name, "ambiguous class reference to '" + sub_expr.function->return_type.name.to_string() + "' in current scope");
                                    return;
                                } else {
                                    this->m_name_error(*find_scope.get_parser(), sub_expr.function->return_type.name, "unable to resolve class '" + sub_expr.function->return_type.name.to_string() + "' in current scope");
                                    return;
                                }
                                find_scope.clazz = sub_expr.function->return_type.name_class;
                                find_scope.parser_ = sub_expr.function->return_type.name_class->parser_;
                            }

                            sub_expr.expr_type.name_class = find_scope.clazz;
                            sub_expr.expr_type.array_dimensions = sub_expr.function->return_type.array_dimensions;
                            find_scope.func = nullptr;
                            find_scope.var = nullptr;
                            can_fqn = false;
                        } else {
                            if (find_scope.clazz != &m_void_class) {
                                std::string function_signature = find_scope.clazz ? find_scope.clazz->get_fqn() : "";
                                if (function_signature.size() > 0)function_signature += '.';
                                function_signature += func_name.to_string();
                                function_signature += '(';
                                {
                                    size_t param_index = 1;
                                    for (shift_expression& expr_param : param_expr.sub) {
                                        if (expr_param.expr_type.name_class) {
                                            function_signature += expr_param.expr_type.name_class->get_fqn();
                                        } else {
                                            if (!(param_count == 0 && expr_param.type == token::token_type::NULL_TOKEN))
                                                function_signature += "<unknown>";
                                        }
                                        if (param_index < param_count)
                                            function_signature += ", ";
                                        param_index++;
                                    }
                                }
                                function_signature += ')';

                                if (functions.size() > 1) {
                                    // this->m_name_error(*find_scope.parser, sub_expr.function->return_type.name, "ambiguous function reference to '" + function_signature + "' in scope");
                                    this->m_token_error(*parent_scope->get_parser(), !sub_expr.begin->is_operator() ? *sub_expr.begin : *(sub_expr.begin + 1), "ambiguous function reference to '" + function_signature + "' in scope");
                                    return;
                                } else {
                                    this->m_token_error(*parent_scope->get_parser(), !sub_expr.begin->is_operator() ? *sub_expr.begin : *(sub_expr.begin + 1), "unable to resolve function '" + function_signature + "' in current scope");
                                    return;
                                }
                            } else {
                                this->m_token_error(*parent_scope->get_parser(), !sub_expr.begin->is_operator() ? *sub_expr.begin : *(sub_expr.begin + 1), "cannot call function on 'void' type");
                                return;
                            }
                        }
                    } else if (sub_expr.is_array()) {
                        // array indexing
                        if (find_scope.clazz == &m_void_class) {
                            if (sub_expr.sub.front().is_function_call()) {
                                this->m_token_error(*parent_scope->get_parser(), !sub_expr.sub.front().begin->is_operator() ? *sub_expr.sub.front().begin : *(sub_expr.sub.front().begin + 1), "cannot call function on 'void' type");
                            } else if (sub_expr.sub.front().type == token::token_type::IDENTIFIER) {
                                this->m_token_error(*parent_scope->get_parser(), *sub_expr.sub.front().begin, "cannot access field on void type");
                            } else {
                                this->m_token_error(*parent_scope->get_parser(), *((++sub_expr.sub.begin())->begin - 1), "cannot access void type as an array");
                            }

                            return;
                        }

                        {
                            {
                                shift_expression index_expr;
                                index_expr.type = sub_expr.sub.front().type;
                                index_expr.begin = sub_expr.sub.front().begin;
                                index_expr.end = sub_expr.sub.front().end;
                                index_expr.sub.push_back(std::move(sub_expr.sub.front()));
                                m_resolve_types(&index_expr, &find_scope);
                                sub_expr.sub.front() = std::move(index_expr.sub.front());
                            }


                            for (auto b = ++sub_expr.sub.begin(); b != sub_expr.sub.end();b++) {
                                m_resolve_types(&*b, &find_scope);
                                if (b->expr_type.name_class && ((b->expr_type.name_class != m_classes[SHIFT_ANALYZER_UINT64_CLASS] && b->expr_type.name_class != m_classes[SHIFT_ANALYZER_UINT32_CLASS]) || b->expr_type.array_dimensions > 0)) {
                                    // auto conversions = m_get_implicit_conversions(b->expr_type.name_class, b->expr_type.name_class != m_classes[SHIFT_ANALYZER_UINT64_CLASS]);
                                    // if (conversions.size() == 1) {

                                    // }
                                    this->m_token_error(*parent_scope->get_parser(), *(b->begin - 1), "cannot convert integral value of type '" + b->expr_type.get_fqn() + "' into type '" SHIFT_ANALYZER_UINT64_CLASS "'");
                                }
                            }
                        }

                        // sub_expr.sub.front().sub.push_back(sub_expr.sub.front()); // Should this be removed after the call?
                        // m_resolve_types(&sub_expr.sub.front(), &find_scope);

                        if (sub_expr.sub.front().expr_type.name_class) {
                            {
                                sub_expr.expr_type = sub_expr.sub.front().expr_type;
                                sub_expr.expr_type.array_dimensions = sub_expr.sub.front().expr_type.array_dimensions - std::min(sub_expr.sub.size() - 1, sub_expr.sub.front().expr_type.array_dimensions);
                                find_scope.func = nullptr;
                                if (sub_expr.expr_type.array_dimensions) {
                                    // "shift.array<int>"
                                    // TODO make array class
                                    // find_scope.clazz = nullptr;
                                    // find_scope.parser = nullptr;
                                    // sub_expr.expr_type.name_class = nullptr;
                                    find_scope.clazz = m_make_array_class(sub_expr.expr_type);
                                    find_scope.parser_ = nullptr;
                                    sub_expr.expr_type.name_class = find_scope.clazz;
                                } else if (sub_expr.sub.size() - 1 == sub_expr.sub.front().expr_type.array_dimensions) {
                                    find_scope.clazz = sub_expr.expr_type.name_class;
                                    find_scope.parser_ = find_scope.clazz->parser_;
                                } else {
                                    find_scope.clazz = sub_expr.expr_type.name_class;
                                    //find_scope.parser = find_scope.clazz->parser;
                                    find_scope.parser_ = parent_scope->parser_;

                                    auto b = ++sub_expr.sub.begin();
                                    for (size_t i = 0; i < sub_expr.sub.front().expr_type.array_dimensions; i++, ++b);

                                    for (size_t i = 0; i < sub_expr.sub.size() - 1 - sub_expr.sub.front().expr_type.array_dimensions; i++, ++b) {
                                        shift_expression func_expr;
                                        func_expr.set_function_call();

                                        std::vector<token> temp_tokens;
                                        temp_tokens.push_back(token(std::string_view("operator"), token_type::IDENTIFIER, (b->begin - 1)->get_file_index()));
                                        temp_tokens.push_back(token(std::string_view("["), token_type::LEFT_SQUARE_BRACKET, (b->begin - 1)->get_file_index()));
                                        temp_tokens.push_back(token(std::string_view("]"), token_type::RIGHT_SQUARE_BRACKET, (b->begin - 1)->get_file_index()));
                                        func_expr.begin = temp_tokens.begin();

                                        func_expr.end = temp_tokens.end();
                                        // func_expr.sub.push_back(func_expr);
                                        func_expr.sub.push_back(*b);
                                        func_expr.sub.back().sub.push_back(*b);
                                        shift_expression parent_func_expr;
                                        parent_func_expr.type = func_expr.type;
                                        parent_func_expr.sub.push_back(std::move(func_expr));
                                        parent_func_expr.begin = temp_tokens.begin();
                                        parent_func_expr.end = temp_tokens.end();
                                        m_resolve_types(&parent_func_expr, &find_scope);

                                        if (!parent_func_expr.expr_type.name_class) {
                                            // m_resolve_types call will do error reporting for me?
                                            return;
                                        }
                                        find_scope.clazz = parent_func_expr.expr_type.name_class;
                                        // find_scope.parser = parent_scope->parser;
                                        b->function = parent_func_expr.function;
                                        b->expr_type = parent_func_expr.expr_type;
                                    }

                                    find_scope.parser_ = find_scope.clazz->parser_;
                                    sub_expr.expr_type = sub_expr.sub.back().expr_type;
                                }
                            }
                            find_scope.func = nullptr;
                            find_scope.var = nullptr;
                            can_fqn = false;
                        } else {
                            // TODO better error message; is `sub_expr.to_string()` safe?
                            this->m_name_error(*parent_scope->get_parser(), sub_expr.sub.front().to_name(), "unable to resolve type of '" + sub_expr.to_string() + "' in current scope");
                            return;
                        }
                    }
                }
                expr->expr_type = expr->sub.back().expr_type;
                expr->function = expr->sub.back().function;
                expr->variable = expr->sub.back().variable;
            } else if (is_overload_operator(expr->type)) {
                // Look for the operator function between the two types
                // const bool is_prefix = (is_strictly_prefix_operator(expr->type) && !is_binary_operator(expr->type)) || (is_prefix_operator(expr->type) && expr->get_left()->type == token::token_type::NULL_TOKEN);
                // const bool is_suffix = (is_strictly_suffix_operator(expr->type) && !is_binary_operator(expr->type)) || (is_suffix_operator(expr->type) && expr->get_right()->type == token::token_type::NULL_TOKEN);

                const bool has_left = expr->has_left() && expr->get_left()->type != token::token_type::NULL_TOKEN;
                const bool has_right = expr->has_right() && expr->get_right()->type != token::token_type::NULL_TOKEN;

                if (has_left) {
                    m_resolve_types(expr->get_left(), parent_scope);
                }

                if (has_right) {
                    m_resolve_types(expr->get_right(), parent_scope);
                }

                // if (!is_prefix)
                //     m_resolve_types(expr->get_left(), parent_scope);

                // if (!is_suffix)
                //     m_resolve_types(expr->get_right(), parent_scope);

                shift_expression func_expr;
                func_expr.set_function_call();

                std::vector<token> temp_tokens;
                temp_tokens.emplace_back(std::string_view("operator"), token_type::IDENTIFIER, expr->begin->get_file_index());
                temp_tokens.push_back(*expr->begin);
                func_expr.begin = temp_tokens.begin();
                func_expr.end = temp_tokens.end();

                shift_expression parent_func_expr;
                parent_func_expr.type = func_expr.type;
                parent_func_expr.begin = temp_tokens.begin();
                parent_func_expr.end = temp_tokens.end();

                scope find_scope;
                find_scope.base = parent_scope->base;
                find_scope.clazz = parent_scope->clazz;
                find_scope.func = parent_scope->func;
                find_scope.var = parent_scope->var;
                find_scope.parser_ = parent_scope->parser_;
                if (has_left && !has_right) {
                    // stricly prefix operator
                    if (expr->get_left()->expr_type.name_class) {
                        find_scope.func = nullptr;
                        find_scope.var = nullptr;
                        find_scope.parser_ = nullptr;
                        find_scope.clazz = expr->get_left()->expr_type.name_class;

                        {
                            shift_name func_name;
                            func_name.begin = parent_func_expr.begin;
                            func_name.end = parent_func_expr.end;
                            auto funcs = find_scope.find_functions(func_name.to_string());

                            m_match_types(funcs, nullptr);

                            if (funcs.size() == 1) {
                                expr->function = funcs.front();
                                if (!funcs.front()->return_type.name_class) {
                                    scope _scope;
                                    _scope.base = this;
                                    _scope.clazz = funcs.front()->clazz;
                                    _scope.func = funcs.front();
                                    funcs.front()->return_type.name_class = _scope.find_class(funcs.front()->return_type.name);
                                }
                                expr->expr_type = funcs.front()->return_type;
                            } else {
                                std::string func_fqn = expr->get_left()->expr_type.name_class->get_fqn() + ".operator" + std::string(expr->begin->get_data()) + "()";
                                if (funcs.size() > 1) {
                                    this->m_token_error(*parent_scope->get_parser(), *expr->begin, "ambiguous function reference to '" + func_fqn + "' in current scope");
                                } else {
                                    this->m_token_error(*parent_scope->get_parser(), *expr->begin, "unable to resolve function reference to '" + func_fqn + "' in current scope");
                                }
                            }
                        }
                    }
                } else if (!has_left && has_right) {
                    // strictly suffix operator
                    if (expr->get_right()->expr_type.name_class) {
                        find_scope.func = nullptr;
                        find_scope.var = nullptr;
                        find_scope.parser_ = nullptr;
                        find_scope.clazz = expr->get_right()->expr_type.name_class;

                        {
                            shift_name func_name;
                            func_name.begin = parent_func_expr.begin;
                            func_name.end = parent_func_expr.end;
                            auto funcs = find_scope.find_functions(func_name.to_string());

                            m_match_types(funcs, nullptr);

                            if (funcs.size() == 1) {
                                expr->function = funcs.front();
                                if (!funcs.front()->return_type.name_class) {
                                    scope _scope;
                                    _scope.base = this;
                                    _scope.clazz = funcs.front()->clazz;
                                    _scope.func = funcs.front();
                                    funcs.front()->return_type.name_class = _scope.find_class(funcs.front()->return_type.name);
                                }
                                expr->expr_type = funcs.front()->return_type;
                            } else {
                                std::string func_fqn = expr->get_right()->expr_type.name_class->get_fqn() + ".operator" + std::string(expr->begin->get_data()) + "()";
                                if (funcs.size() > 1) {
                                    this->m_token_error(*parent_scope->get_parser(), *expr->begin, "ambiguous function reference to '" + func_fqn + "' in current scope");
                                } else {
                                    this->m_token_error(*parent_scope->get_parser(), *expr->begin, "unable to resolve function reference to '" + func_fqn + "' in current scope");
                                }
                            }
                        }
                    }
                } else if (has_left && has_right) {
                    // binary operator
                    if (expr->get_left()->expr_type.name_class) {
                        find_scope.func = nullptr;
                        find_scope.var = nullptr;
                        find_scope.parser_ = nullptr;
                        find_scope.clazz = expr->get_left()->expr_type.name_class;

                        {
                            shift_name func_name;
                            func_name.begin = parent_func_expr.begin;
                            func_name.end = parent_func_expr.end;
                            auto funcs = find_scope.find_functions(func_name.to_string());

                            if (expr->get_right()->expr_type.name_class) {
                                m_match_types(funcs, *expr->get_right());
                            }

                            if (funcs.size() == 0) {
                                if (expr->get_right()->expr_type.name_class) {
                                    if (expr->type == token::token_type::EQUALS) {
                                        auto conversions = m_get_implicit_conversions(expr->get_right()->expr_type.name_class, expr->get_left()->expr_type.name_class);
                                        if (conversions.size() == 1) {
                                            // if (!conversions.front()->return_type.name_class) {
                                            //     scope _scope;
                                            //     _scope.base = this;
                                            //     _scope.clazz = conversions.front()->clazz;
                                            //     _scope.func = conversions.front();
                                            //     conversions.front()->return_type.name_class = _scope.find_class(funcs.front()->return_type.name);
                                            // }
                                            *expr->get_right() = m_create_convert_expr(std::move(*expr->get_right()), *conversions.front());
                                            //expr->expr_type = conversions.front()->return_type;
                                            expr->expr_type.name_class = expr->get_left()->expr_type.name_class;
                                            expr->function = nullptr;
                                            return;
                                        } else if (conversions.size() > 1) {
                                            this->m_token_error(*parent_scope->get_parser(), *expr->get_right()->begin, "ambiguous implicit type conversion from '" + expr->get_right()->expr_type.name_class->get_fqn() + "' to '" + expr->get_left()->expr_type.name_class->get_fqn() + "'");
                                            return;
                                        } else {
                                            this->m_token_error(*parent_scope->get_parser(), *expr->get_right()->begin, "unable to resolve implicit type conversion from '" + expr->get_right()->expr_type.name_class->get_fqn() + "' to '" + expr->get_left()->expr_type.name_class->get_fqn() + "'");
                                            return;
                                        }
                                    } else if (expr->type == token::token_type::EQUALS_EQUALS) {
                                        std::string_view new_data = "!=";
                                        temp_tokens.back() = token(new_data, token::token_type::NOT_EQUAL, expr->begin->get_file_index());
                                        auto new_funcs = find_scope.find_functions(func_name.to_string());
                                        if (expr->get_right()->expr_type.name_class) {
                                            m_match_types(new_funcs, *expr->get_right());
                                        }

                                        if (new_funcs.size() == 1) {
                                            if (!new_funcs.front()->return_type.name_class) {
                                                scope _scope;
                                                _scope.base = this;
                                                _scope.clazz = new_funcs.front()->clazz;
                                                _scope.func = new_funcs.front();
                                                new_funcs.front()->return_type.name_class = _scope.find_class(new_funcs.front()->return_type.name);
                                                //expr->expr_type = new_funcs.front()->return_type;
                                            }

                                            if (new_funcs.front()->return_type.name_class) {
                                                std::string_view new_data_2 = "!";
                                                temp_tokens.back() = token(new_data_2, token::token_type::NOT, expr->begin->get_file_index());
                                                auto negate_funcs = find_scope.find_functions(func_name.to_string());
                                                m_match_types(negate_funcs, nullptr);
                                                if (negate_funcs.size() == 1) {
                                                    if (!negate_funcs.front()->return_type.name_class) {
                                                        scope _scope;
                                                        _scope.base = this;
                                                        _scope.clazz = negate_funcs.front()->clazz;
                                                        _scope.func = negate_funcs.front();
                                                        negate_funcs.front()->return_type.name_class = _scope.find_class(negate_funcs.front()->return_type.name);
                                                        expr->expr_type = new_funcs.front()->return_type;
                                                    }
                                                    if (negate_funcs.front()->return_type.name_class) {
                                                        shift_expression negate_expr;
                                                        negate_expr.type = token::token_type::NOT;
                                                        negate_expr.function = negate_funcs.front();
                                                        negate_expr.expr_type = negate_funcs.front()->return_type;

                                                        expr->type = token::token_type::NOT_EQUAL;
                                                        expr->begin = m_not_equal_token;
                                                        expr->end = expr->begin + 1;
                                                        expr->function = new_funcs.front();
                                                        expr->expr_type = new_funcs.front()->return_type;
                                                        negate_expr.set_right(std::move(*expr));
                                                        *expr = std::move(negate_expr);
                                                        return;
                                                    } else {
                                                        this->m_token_error(*parent_scope->get_parser(), *expr->begin, "unable to resolve return type of '" + negate_funcs.front()->get_signature() + "'");
                                                    }
                                                    return;
                                                } else if (negate_funcs.size() > 1) {
                                                    this->m_token_error(*parent_scope->get_parser(), *expr->begin, "ambiguous reference to function '" + new_funcs.front()->return_type.name_class->get_fqn() + ".operator!()'");
                                                    return;
                                                } else {
                                                    this->m_token_error(*parent_scope->get_parser(), *expr->begin, "unable to resolve function '" + new_funcs.front()->return_type.name_class->get_fqn() + ".operator!()'");
                                                    return;
                                                }
                                            } else {
                                                this->m_token_error(*parent_scope->get_parser(), *expr->begin, "unable to resolve return type of '" + new_funcs.front()->get_signature() + "'");
                                            }
                                            return;
                                        }
                                    } else if (expr->type == token::token_type::NOT_EQUAL) {
                                        temp_tokens.back() = token(m_equals_equals_token->get_data(), token::token_type::EQUALS_EQUALS, expr->begin->get_file_index());
                                        auto new_funcs = find_scope.find_functions(func_name.to_string());
                                        if (expr->get_right()->expr_type.name_class) {
                                            m_match_types(new_funcs, *expr->get_right());
                                        }
                                        if (new_funcs.size() == 1) {
                                            if (!new_funcs.front()->return_type.name_class) {
                                                scope _scope;
                                                _scope.base = this;
                                                _scope.clazz = new_funcs.front()->clazz;
                                                _scope.func = new_funcs.front();
                                                new_funcs.front()->return_type.name_class = _scope.find_class(new_funcs.front()->return_type.name);
                                                //expr->expr_type = new_funcs.front()->return_type;
                                            }

                                            if (new_funcs.front()->return_type.name_class) {
                                                std::string_view new_data_2 = "!";
                                                temp_tokens.back() = token(new_data_2, token::token_type::NOT, expr->begin->get_file_index());
                                                auto negate_funcs = find_scope.find_functions(func_name.to_string());
                                                m_match_types(negate_funcs, nullptr);
                                                if (negate_funcs.size() == 1) {
                                                    if (!negate_funcs.front()->return_type.name_class) {
                                                        scope _scope;
                                                        _scope.base = this;
                                                        _scope.clazz = negate_funcs.front()->clazz;
                                                        _scope.func = negate_funcs.front();
                                                        negate_funcs.front()->return_type.name_class = _scope.find_class(negate_funcs.front()->return_type.name);
                                                        expr->expr_type = new_funcs.front()->return_type;
                                                    }
                                                    if (negate_funcs.front()->return_type.name_class) {
                                                        shift_expression negate_expr;
                                                        negate_expr.type = token::token_type::NOT;
                                                        negate_expr.function = negate_funcs.front();
                                                        negate_expr.expr_type = negate_funcs.front()->return_type;

                                                        expr->type = token::token_type::EQUALS_EQUALS;
                                                        expr->begin = m_equals_equals_token;
                                                        expr->end = expr->begin + 1;
                                                        expr->function = new_funcs.front();
                                                        expr->expr_type = new_funcs.front()->return_type;
                                                        negate_expr.set_right(std::move(*expr));
                                                        *expr = std::move(negate_expr);
                                                        return;
                                                    } else {
                                                        this->m_token_error(*parent_scope->get_parser(), *expr->begin, "unable to resolve return type of '" + negate_funcs.front()->get_signature() + "'");
                                                    }
                                                    return;
                                                } else if (negate_funcs.size() > 1) {
                                                    this->m_token_error(*parent_scope->get_parser(), *expr->begin, "ambiguous reference to function '" + new_funcs.front()->return_type.name_class->get_fqn() + ".operator!()'");
                                                    return;
                                                } else {
                                                    this->m_token_error(*parent_scope->get_parser(), *expr->begin, "unable to resolve function '" + new_funcs.front()->return_type.name_class->get_fqn() + ".operator!()'");
                                                    return;
                                                }
                                            } else {
                                                this->m_token_error(*parent_scope->get_parser(), *expr->begin, "unable to resolve return type of '" + new_funcs.front()->get_signature() + "'");
                                            }
                                            return;
                                        }
                                    } else if (expr->type & token::token_type::EQUALS) {
                                        std::string_view new_data = expr->begin->get_data();
                                        auto equals_index = new_data.find('=');
                                        new_data = equals_index == 0 ? new_data.substr(1) : new_data.substr(0, equals_index);

                                        if (new_data != expr->begin->get_data()) {
                                            temp_tokens.back() = token(new_data, expr->type & ~token::token_type::EQUALS, expr->begin->get_file_index());
                                            auto new_funcs = find_scope.find_functions(func_name.to_string());
                                            if (expr->get_right()->expr_type.name_class) {
                                                m_match_types(new_funcs, *expr->get_right());
                                            }
                                            if (new_funcs.size() == 1) {
                                                expr->function = new_funcs.front();
                                                expr->type = expr->type & ~token::token_type::EQUALS;

                                                if (!new_funcs.front()->return_type.name_class) {
                                                    scope _scope;
                                                    _scope.base = this;
                                                    _scope.clazz = new_funcs.front()->clazz;
                                                    _scope.func = new_funcs.front();
                                                    new_funcs.front()->return_type.name_class = _scope.find_class(new_funcs.front()->return_type.name);
                                                    expr->expr_type = new_funcs.front()->return_type;
                                                }
                                                if (new_funcs.front()->return_type.name_class) {
                                                    shift_expression temp_current_expr = std::move(*expr);
                                                    *expr = shift_expression();
                                                    expr->type = token::token_type::EQUALS;
                                                    expr->begin = m_equals_token;
                                                    expr->end = expr->begin + 1;
                                                    expr->set_left(*temp_current_expr.get_left());
                                                    expr->set_right(std::move(temp_current_expr));

                                                    if (expr->get_left()->expr_type.name_class && expr->get_right()->expr_type.name_class) {
                                                        if (expr->get_left()->expr_type.name_class != expr->get_right()->expr_type.name_class && expr->get_right()->expr_type.name_class != &m_null_class && expr->get_right()->expr_type.name_class != &m_void_class) {
                                                            auto conversions = m_get_implicit_conversions(expr->get_right()->expr_type.name_class, expr->get_left()->expr_type.name_class);
                                                            if (conversions.size() == 1) {
                                                                if (!conversions.front()->return_type.name_class) {
                                                                    scope _scope;
                                                                    _scope.base = this;
                                                                    _scope.clazz = conversions.front()->clazz;
                                                                    _scope.func = conversions.front();
                                                                    conversions.front()->return_type.name_class = _scope.find_class(conversions.front()->return_type.name);
                                                                }
                                                                *expr->get_right() = m_create_convert_expr(std::move(*expr->get_right()), *conversions.front());
                                                                expr->expr_type = expr->get_left()->expr_type;
                                                                //expr->function = conversions.front();
                                                                return;
                                                            } else if (conversions.size() > 1) {
                                                                this->m_token_error(*parent_scope->get_parser(), *expr->get_right()->begin, "ambiguous implicit type conversion from '" + expr->get_right()->expr_type.name_class->get_fqn() + "' to '" + expr->get_left()->expr_type.name_class->get_fqn() + "'");
                                                                return;
                                                            } else {
                                                                this->m_token_error(*parent_scope->get_parser(), *expr->get_right()->begin, "unable to resolve implicit type conversion from '" + expr->get_right()->expr_type.name_class->get_fqn() + "' to '" + expr->get_left()->expr_type.name_class->get_fqn() + "'");
                                                                return;
                                                            }
                                                        } else {
                                                            expr->expr_type = expr->get_left()->expr_type;
                                                        }
                                                    }
                                                } else {
                                                    this->m_token_error(*parent_scope->get_parser(), *expr->begin, "unable to resolve return type of '" + expr->function->get_signature() + "'");
                                                }
                                                return;
                                            }
                                        }
                                    }
                                }
                            }

                            if (funcs.size() == 1) {
                                expr->function = funcs.front();
                                if (!funcs.front()->return_type.name_class) {
                                    scope _scope;
                                    _scope.base = this;
                                    _scope.clazz = funcs.front()->clazz;
                                    _scope.func = funcs.front();
                                    funcs.front()->return_type.name_class = _scope.find_class(funcs.front()->return_type.name);
                                }
                                expr->expr_type = funcs.front()->return_type;
                            } else {
                                if (!expr->expr_type.name_class) {
                                    std::string func_fqn = expr->get_left()->expr_type.name_class->get_fqn() + ".operator" + std::string(expr->begin->get_data()) + "(";
                                    if (expr->get_right()->expr_type.name_class) {
                                        func_fqn += expr->get_right()->expr_type.get_fqn();
                                    } else {
                                        func_fqn += "<unknown>";
                                    }
                                    func_fqn += ")";
                                    if (funcs.size() > 1) {
                                        this->m_token_error(*parent_scope->get_parser(), *expr->begin, "ambiguous function reference to '" + func_fqn + "' in current scope");
                                    } else {
                                        this->m_token_error(*parent_scope->get_parser(), *expr->begin, "unable to resolve function reference to '" + func_fqn + "' in current scope");
                                    }
                                }
                            }
                        }
                    }
                }

                // if (is_prefix) {
                //     if (!expr->get_right()->expr_type.name_class) {
                //         // error
                //         return;
                //     } else {
                //         find_scope.clazz = expr->get_right()->expr_type.name_class;
                //         find_scope.parser_ = parent_scope->parser_;

                //         func_expr.sub.push_back(shift_expression());
                //         // func_expr.sub.back().sub.push_back(shift_expression());
                //         parent_func_expr.sub.push_back(std::move(func_expr));
                //         m_resolve_types(&parent_func_expr, &find_scope);
                //     }
                // } else if (is_suffix) {

                // } else {
                //     if (!expr->get_left()->expr_type.name_class) {
                //         // error
                //         return;
                //     } else if (!expr->get_right()->expr_type.name_class) {
                //         // error
                //         return;
                //     } else {
                //         find_scope.clazz = expr->get_left()->expr_type.name_class;
                //         //find_scope.parser = find_scope.clazz->parser;
                //         find_scope.parser_ = parent_scope->parser_;
                //         func_expr.sub.push_back(*expr->get_right());
                //         func_expr.sub.back().sub.push_back(*expr->get_right());

                //         parent_func_expr.sub.push_back(std::move(func_expr));
                //         m_resolve_types(&parent_func_expr, &find_scope);
                //     }
                // }

                // if (!parent_func_expr.expr_type.name_class) {
                //     // m_resolve_types call will do error reporting for us?
                //     return;
                // }
                // expr->expr_type = parent_func_expr.expr_type;
                // expr->function = parent_func_expr.function;
                // expr->variable = parent_func_expr.variable;
            } else if (expr->type == token::token_type::COMMA) {
                for (shift_expression& sub_expr : expr->sub) {
                    m_resolve_types(&sub_expr, parent_scope);
                }
            } else {
                switch (expr->type) {
                    case token_type::STRING_LITERAL:
                        expr->expr_type.name_class = parent_scope->base->m_classes[SHIFT_ANALYZER_STRING_CLASS];
                        break;
                    case token_type::CHAR_LITERAL:
                        expr->expr_type.name_class = parent_scope->base->m_classes[SHIFT_ANALYZER_CHAR_CLASS];
                        break;
                    case token_type::INTEGER_LITERAL:
                    case token_type::BINARY_NUMBER:
                    case token_type::HEX_NUMBER:
                        expr->expr_type.name_class = parent_scope->base->m_classes[SHIFT_ANALYZER_INT_CLASS];
                        break;
                    case token_type::FLOAT:
                        expr->expr_type.name_class = parent_scope->base->m_classes[SHIFT_ANALYZER_FLOAT_CLASS];
                        break;
                    case token_type::DOUBLE:
                        expr->expr_type.name_class = parent_scope->base->m_classes[SHIFT_ANALYZER_DOUBLE_CLASS];
                        break;
                    default:
                        this->m_name_error(*parent_scope->get_parser(), expr->to_name(), "unable to resolve type of expression");
                        break;
                }
            }
        }

        utils::ordered_set<shift_function*>& analyzer::m_match_types(utils::ordered_set<shift_function*>& funcs, std::list<shift_expression>& params) {
            for (auto it = funcs.begin(); it != funcs.end(); ++it) {
                shift_function& func = **it;

                // TODO allow default parameters
                if (func.parameters.size() != params.size()) {
                    auto next = funcs.erase(it);
                    it = --next;
                    continue;
                }

                auto func_param_it = func.parameters.begin();
                auto expr_param_it = params.begin();
                bool exact_match = true;
                for (;expr_param_it != params.end(); ++expr_param_it, ++func_param_it) {
                    if (!expr_param_it->expr_type.name_class) {
                        // no chance at name resolution if one of the expression hasn't even been resolved yet
                        funcs.clear();
                        return funcs;
                    }

                    if (!func_param_it->second.type.name_class) {
                        scope _scope;
                        _scope.base = this;
                        _scope.clazz = func.clazz;
                        _scope.func = &func;
                        func_param_it->second.type.name_class = _scope.find_class(func_param_it->second.type.name);
                    }

                    if (func_param_it->second.type.name_class) {
                        if (func_param_it->second.type != expr_param_it->expr_type) {
                            exact_match = false;
                            if (func_param_it->second.type.array_dimensions == 0 && expr_param_it->expr_type.array_dimensions == 0) {
                                auto conversions = m_get_implicit_conversions(expr_param_it->expr_type.name_class, func_param_it->second.type.name_class);
                                if (conversions.size() == 1) {
                                    // TODO complete conversion
                                    *expr_param_it = m_create_convert_expr(std::move(*expr_param_it), *conversions.front());
                                    continue;
                                }
                            }
                            break;
                        }
                    } else {
                        exact_match = false;
                        break;
                    }
                }

                if (expr_param_it != params.end()) {
                    auto next = funcs.erase(it);
                    it = --next;
                    continue;
                } else if (exact_match) {
                    shift_function* const ret_func = &func;
                    funcs.clear();
                    funcs.push_back(ret_func);
                    return funcs;
                }
            }
            return funcs;
        }

        shift_class* analyzer::m_make_array_class(shift_class* const clazz, const size_t dimensions) {
            if (dimensions == 0) return clazz;
            std::string array_class_name = "shift.array@" + clazz->get_fqn() + "@" + std::to_string(dimensions);
            {
                auto f = m_classes.find(array_class_name);
                if (f != m_classes.end()) return f->second;
            }
            m_extra_classes.emplace_back();
            m_classes[array_class_name] = &m_extra_classes.back();

            shift_class& array_class = *m_classes[array_class_name];
            array_class.name = &*m_array_token;
            array_class.module_ = &m_shift_module;
            array_class.mods = shift_mods::PUBLIC;

            {
                shift_variable length_var;
                length_var.name = &*m_length_token;
                length_var.clazz = &array_class;
                length_var.type.name_class = m_classes[SHIFT_ANALYZER_INT_CLASS];
                length_var.type.mods = shift_mods::PUBLIC;
                array_class.variables.push_back(std::move(length_var));
            }

            {
                array_class.functions.emplace_back();
                shift_function& bracket_function = array_class.functions.back();
                bracket_function.name.begin = m_left_square_bracket_token;
                bracket_function.name.end = bracket_function.name.begin + 1;
                bracket_function.mods = shift_mods::PUBLIC;
                bracket_function.return_type.name_class = m_make_array_class(clazz, dimensions - 1);
                bracket_function.clazz = &array_class;

                {
                    shift_variable bracket_function_param;
                    bracket_function_param.type.name_class = m_classes[SHIFT_ANALYZER_INT_CLASS];
                    bracket_function_param.function = &bracket_function;

                    bracket_function.parameters.push_back({ "@0", std::move(bracket_function_param) });
                }
            }

            return &array_class;
        }

        void analyzer::m_token_error(const parser& parser_, const token& token_, const std::string_view msg) {
            if (!this->m_error_handler) return;
            SHIFT_ANALYZER_ERROR_(parser_, token_, msg);
            std::string line = std::string(this->m_get_line(parser_, token_));
            size_t use_col = token_.get_file_index().col;
            std::for_each(line.begin(), line.end(), [&use_col](char& ch) {
                if (ch == '\t') {
                    ch = ' ';
                    use_col -= 3;
                }
                });

            std::string indexer(use_col - 1, ' ');
            indexer.append(token_.get_data().size(), '^');
            SHIFT_ANALYZER_ERROR_LOG(line);
            SHIFT_ANALYZER_ERROR_LOG(indexer);
        }

        void analyzer::m_token_error(const parser& parser_, const token& token_, const std::string& msg) { return m_token_error(parser_, token_, std::string_view(msg.c_str(), msg.length())); }
        void analyzer::m_token_error(const parser& parser_, const token& token_, const char* const msg) { return m_token_error(parser_, token_, std::string_view(msg, std::strlen(msg))); }

        void analyzer::m_token_warning(const parser& parser_, const token& token_, const std::string_view msg) {
            if (!this->m_error_handler) return;
            if (!this->m_error_handler->is_print_warnings()) return;
            SHIFT_ANALYZER_WARNING_(parser_, token_, msg);
            std::string line = std::string(this->m_get_line(parser_, token_));
            size_t use_col = token_.get_file_index().col;
            std::for_each(line.begin(), line.end(), [&use_col](char& ch) {
                if (ch == '\t') {
                    ch = ' ';
                    use_col -= 3;
                }
                });

            std::string indexer(use_col - 1, ' ');
            indexer.append(token_.get_data().size(), '^');

            SHIFT_ANALYZER_WARNING_LOG(line);
            SHIFT_ANALYZER_WARNING_LOG(indexer);
        }

        void analyzer::m_token_warning(const parser& parser_, const token& token_, const std::string& msg) { return m_token_warning(parser_, token_, std::string_view(msg.c_str(), msg.length())); }
        void analyzer::m_token_warning(const parser& parser_, const token& token_, const char* const msg) { return m_token_warning(parser_, token_, std::string_view(msg, std::strlen(msg))); }

        void analyzer::m_name_error(const parser& parser_, const shift_name& name, const std::string_view msg) {
            if (!this->m_error_handler) return;
            SHIFT_ANALYZER_ERROR_(parser_, *name.begin, msg);
            std::string line = std::string(this->m_get_line(parser_, *name.begin));
            size_t use_col = name.begin->get_file_index().col;

            for (auto cur = line.begin(); cur != line.begin() + use_col - 1; ++cur) {
                char& ch = *cur;
                if (ch == '\t') {
                    ch = ' ';
                    use_col -= 3;
                }
            } // TODO deal with tabs

            std::string indexer(use_col - 1, ' ');
            indexer.append((name.end - 1)->get_file_index().col + (name.end - 1)->get_data().size() - name.begin->get_file_index().col, '^');
            SHIFT_ANALYZER_ERROR_LOG(line);
            SHIFT_ANALYZER_ERROR_LOG(indexer);
        }

        void analyzer::m_name_error(const parser& parser_, const shift_name& name, const std::string& msg) { return m_name_error(parser_, name, std::string_view(msg.c_str(), msg.length())); }
        void analyzer::m_name_error(const parser& parser_, const shift_name& name, const char* const msg) { return m_name_error(parser_, name, std::string_view(msg, std::strlen(msg))); }

        void analyzer::m_name_warning(const parser& parser_, const shift_name& name, const std::string_view msg) {
            if (!this->m_error_handler) return;
            if (!this->m_error_handler->is_print_warnings()) return;
            SHIFT_ANALYZER_WARNING_(parser_, *name.begin, msg);
            std::string line = std::string(this->m_get_line(parser_, *name.begin));
            size_t use_col = name.begin->get_file_index().col;
            std::for_each(line.begin(), line.end(), [&use_col](char& ch) {
                if (ch == '\t') {
                    ch = ' ';
                    use_col -= 3;
                }
                });

            std::string indexer(use_col - 1, ' ');
            indexer.append((name.end - 1)->get_file_index().col + (name.end - 1)->get_data().size() - name.begin->get_file_index().col, '^');
            SHIFT_ANALYZER_WARNING_LOG(line);
            SHIFT_ANALYZER_WARNING_LOG(indexer);
        }

        void analyzer::m_name_warning(const parser& parser_, const shift_name& name, const std::string& msg) { return m_name_warning(parser_, name, std::string_view(msg.c_str(), msg.length())); }
        void analyzer::m_name_warning(const parser& parser_, const shift_name& name, const char* const msg) { return m_name_warning(parser_, name, std::string_view(msg, std::strlen(msg))); }

        void analyzer::m_error(const parser& parser_, const std::string_view msg) {
            if (!this->m_error_handler) return;
            SHIFT_ANALYZER_ERROR(parser_, msg);
        }

        void analyzer::m_error(const parser& parser_, const std::string& msg) { return m_error(parser_, std::string_view(msg.c_str(), msg.length())); }
        void analyzer::m_error(const parser& parser_, const char* const msg) { return m_error(parser_, std::string_view(msg, std::strlen(msg))); }

        void analyzer::m_warning(const parser& parser_, const std::string_view msg) {
            if (!this->m_error_handler) return;
            SHIFT_ANALYZER_WARNING(parser_, msg);
        }

        void analyzer::m_warning(const parser& parser_, const std::string& msg) { return m_warning(parser_, std::string_view(msg.c_str(), msg.length())); }
        void analyzer::m_warning(const parser& parser_, const char* const msg) { return m_warning(parser_, std::string_view(msg, std::strlen(msg))); }

        std::string_view analyzer::m_get_line(const parser& p, const token& t) const noexcept {
            return p.get_tokenizer()->get_lines()[t.get_file_index().line - 1];
        }
    }
}