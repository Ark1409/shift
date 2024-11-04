/**
 * @file compiler/shift_analyzer.cpp
 */
#include "compiler/shift_analyzer.h"
#include "utils/utils.h"

#include <cstring>
#include <algorithm>

#define SHIFT_ANALYZER_FILE_PREFIX(__parser)               ((__parser).get_tokenizer()->get_file() == "<internal>"sv ? "<internal>"sv : (std::string_view)std::filesystem::relative((__parser).get_tokenizer()->get_file().raw_path()).string())
#define SHIFT_ANALYZER_ERROR_PREFIX(__parser)                "error: " << SHIFT_ANALYZER_FILE_PREFIX(__parser) << ": " // std::filesystem::relative call every time probably isn't that optimal
#define SHIFT_ANALYZER_WARNING_PREFIX(__parser)            "warning: " << SHIFT_ANALYZER_FILE_PREFIX(__parser) << ": " // std::filesystem::relative call every time probably isn't that optimal

#define SHIFT_ANALYZER_ERROR_PREFIX_EXT_(__parser, __line__, __col__) "error: " << SHIFT_ANALYZER_FILE_PREFIX(__parser) << ":" << __line__ << ":" << __col__ << ": " // std::filesystem::relative call every time probably isn't that optimal
#define SHIFT_ANALYZER_WARNING_PREFIX_EXT_(__parser, __line__, __col__) "warning: " << SHIFT_ANALYZER_FILE_PREFIX(__parser) << ":" << __line__ << ":" << __col__ << ": " // std::filesystem::relative call every time probably isn't that optimal

#define SHIFT_ANALYZER_ERROR_PREFIX_EXT(__parser, __token) SHIFT_ANALYZER_ERROR_PREFIX_EXT_(__parser, (__token).get_file_index().line, (__token).get_file_index().col)
#define SHIFT_ANALYZER_WARNING_PREFIX_EXT(__parser, __token) SHIFT_ANALYZER_WARNING_PREFIX_EXT_(__parser, (__token).get_file_index().line, (__token).get_file_index().col)

#define SHIFT_ANALYZER_PRINT() this->m_error_handler->print_exit_clear()

#define SHIFT_ANALYZER_WARNING(__parser, __WARN__)            this->m_error_handler->stream() << SHIFT_ANALYZER_WARNING_PREFIX(__parser) << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_ANALYZER_FATAL_WARNING(__parser, __WARN__)        SHIFT_ANALYZER_WARNING(__parser, __WARN__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_WARNING_LOG(__WARN__)        this->m_error_handler->stream() << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_ANALYZER_FATAL_WARNING_LOG(__WARN__)  SHIFT_ANALYZER_WARNING_LOG(__WARN__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_ERROR(__parser, __ERR__)            this->m_error_handler->stream() << SHIFT_ANALYZER_ERROR_PREFIX(__parser) << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_ANALYZER_FATAL_ERROR(__parser, __ERR__)        SHIFT_ANALYZER_ERROR(__parser, __ERR__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_ERROR_LOG(__ERR__)        this->m_error_handler->stream() << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_ANALYZER_FATAL_ERROR_LOG(__ERR__)  SHIFT_ANALYZER_ERROR_LOG(__ERR__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_WARNING_(__parser, __token, __WARN__)    this->m_error_handler->stream() << SHIFT_ANALYZER_WARNING_PREFIX_EXT(__parser, __token) << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_ANALYZER_FATAL_WARNING_(__parser, __token, __WARN__)        SHIFT_ANALYZER_WARNING(__parser, __token, __WARN__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_WARNING_LOG_(__WARN__)        this->m_error_handler->stream() << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_ANALYZER_FATAL_WARNING_LOG_(__WARN__)  SHIFT_ANALYZER_WARNING_LOG_( __WARN__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_ERROR_(__parser, __token, __ERR__)            this->m_error_handler->stream() << SHIFT_ANALYZER_ERROR_PREFIX_EXT(__parser, __token) << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_ANALYZER_FATAL_ERROR_(__parser, __token, __ERR__)        SHIFT_ANALYZER_ERROR_(__parser,__token,__ERR__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_ERROR_LOG_(__ERR__)        this->m_error_handler->stream() << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_ANALYZER_FATAL_ERROR_LOG_(__ERR__)  SHIFT_ANALYZER_ERROR_LOG_(__ERR__); SHIFT_ANALYZER_PRINT()

#define SHIFT_ANALYZER_SHIFT_MODULE "shift"

#define SHIFT_ANALYZER_OBJECT_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".object"

#define SHIFT_ANALYZER_INT8_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".byte"
#define SHIFT_ANALYZER_CHAR_CLASS SHIFT_ANALYZER_INT8_CLASS
#define SHIFT_ANALYZER_BYTE_CLASS SHIFT_ANALYZER_CHAR_CLASS

#define SHIFT_ANALYZER_INT16_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".short"
#define SHIFT_ANALYZER_SHORT_CLASS SHIFT_ANALYZER_INT16_CLASS

#define SHIFT_ANALYZER_INT32_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".int"
#define SHIFT_ANALYZER_INT_CLASS SHIFT_ANALYZER_INT32_CLASS

#define SHIFT_ANALYZER_INT64_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".long"
#define SHIFT_ANALYZER_LONG_CLASS SHIFT_ANALYZER_INT64_CLASS

#define SHIFT_ANALYZER_UINT16_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".ushort"
#define SHIFT_ANALYZER_USHORT_CLASS SHIFT_ANALYZER_UINT16_CLASS

#define SHIFT_ANALYZER_UINT32_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".uint"
#define SHIFT_ANALYZER_UINT_CLASS SHIFT_ANALYZER_UINT32_CLASS

#define SHIFT_ANALYZER_UINT64_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".ulong"
#define SHIFT_ANALYZER_ULONG_CLASS SHIFT_ANALYZER_UINT64_CLASS

#define SHIFT_ANALYZER_SINT8_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".sbyte"
#define SHIFT_ANALYZER_SCHAR_CLASS SHIFT_ANALYZER_SINT8_CLASS
#define SHIFT_ANALYZER_SBYTE_CLASS SHIFT_ANALYZER_SCHAR_CLASS

#define SHIFT_ANALYZER_SINT16_CLASS SHIFT_ANALYZER_INT16_CLASS
#define SHIFT_ANALYZER_SSHORT_CLASS SHIFT_ANALYZER_SINT16_CLASS

#define SHIFT_ANALYZER_SINT32_CLASS SHIFT_ANALYZER_INT32_CLASS
#define SHIFT_ANALYZER_SINT_CLASS SHIFT_ANALYZER_SINT32_CLASS

#define SHIFT_ANALYZER_SINT64_CLASS SHIFT_ANALYZER_INT64_CLASS
#define SHIFT_ANALYZER_SLONG_CLASS SHIFT_ANALYZER_SINT64_CLASS

#define SHIFT_ANALYZER_FLOAT_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".float"
#define SHIFT_ANALYZER_FLOAT32_CLASS SHIFT_ANALYZER_FLOAT_CLASS
#define SHIFT_ANALYZER_DOUBLE_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".double"
#define SHIFT_ANALYZER_FLOAT64_CLASS SHIFT_ANALYZER_DOUBLE_CLASS

#define SHIFT_ANALYZER_STRING_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".string"

#define SHIFT_ANALYZER_BOOLEAN_CLASS SHIFT_ANALYZER_SHIFT_MODULE ".bool"
#define SHIFT_ANALYZER_BOOL_CLASS SHIFT_ANALYZER_BOOLEAN_CLASS

using namespace std::string_view_literals;

namespace shift::compiler {
    using token_type = token::type;

    static constexpr bool is_overload_operator(const token_type type) noexcept {
        return token(std::string_view(), type, file_indexer()).is_overloadable_operator();
    }

    static constexpr bool is_binary_operator(const token_type type) noexcept {
        return token(std::string_view(), type, file_indexer()).is_binary_operator();
    }

    static constexpr bool is_unary_operator(const token_type type) noexcept {
        return token(std::string_view(), type, file_indexer()).is_unary_operator();
    }

    static constexpr bool is_prefix_operator(const token_type type) noexcept {
        return token(std::string_view(), type, file_indexer()).is_prefix_operator();
    }

    static constexpr bool is_suffix_operator(const token_type type) noexcept {
        return token(std::string_view(), type, file_indexer()).is_suffix_operator();
    }

    static constexpr bool is_strictly_prefix_operator(const token_type type) noexcept {
        return token(std::string_view(), type, file_indexer()).is_strictly_prefix_operator();
    }

    static constexpr bool is_strictly_suffix_operator(const token_type type) noexcept {
        return token(std::string_view(), type, file_indexer()).is_strictly_suffix_operator();
    }

    static shift_module m_shift_module;
    static shift_class m_void_class, m_null_class;

    // Storage for tokens needed for implementation
    static std::vector<token> m_token_storage;
    static std::vector<token>::iterator m_void_token{};
    static std::vector<token>::iterator m_null_token{};
    static std::vector<token>::iterator m_array_token{};
    static std::vector<token>::iterator m_length_token{};
    static std::vector<token>::iterator m_operator_token{};
    static std::vector<token>::iterator m_operator_array_function_token_begin{};
    static std::vector<token>::iterator m_operator_array_function_token_end{};
    static std::vector<token>::iterator m_operator_star_function_token_begin{};
    static std::vector<token>::iterator m_operator_star_function_token_end{};
    static std::vector<token>::iterator m_operator_arrow_function_token_begin{};
    static std::vector<token>::iterator m_operator_arrow_function_token_end{};
    static std::vector<token>::iterator m_left_square_bracket_token{};
    static std::vector<token>::iterator m_right_square_bracket_token{};
    static std::vector<token>::iterator m_true_token_begin{};
    static std::vector<token>::iterator m_true_token_end{};
    static std::vector<token>::iterator m_equals_token{};
    static std::vector<token>::iterator m_equals_equals_token{};
    static std::vector<token>::iterator m_not_equal_token{};

    SHIFT_API void analyzer::analyze() {
        // TODO add default classes (shift.int, shift.string, shift.long) before starting to analyze
        m_init_defaults();

        // Add in all modules first
        for (parser& _parser : *m_parsers) {
            if (_parser.m_is_module_defined()) {
                m_modules.emplace(_parser.m_module->to_string(), _parser.m_module.get());
            } else {
                this->m_error(_parser, "module not defined for file");
            }
        }

        for (parser& _parser : *m_parsers) {
            for (shift_variable& var : _parser.m_variables) {
                var.parser_ = &_parser;
                std::string fqn = var.get_fqn();

                if (m_modules.find(fqn) != m_modules.end()) {
                    this->m_token_error(_parser, *var.name,
                        "variable '" + fqn + "' would override module with same name");
                }

                if (m_variables.find(fqn) == m_variables.end()) {
                    m_variables[std::move(fqn)] = &var;
                } else {
                    this->m_token_error(_parser, *var.name,
                        "variable '" + fqn + "' has already been defined inside current module");
                }
            }

            for (shift_function& func : _parser.m_functions) {
                func.parser_ = &_parser;
                m_function_overloads[func.get_fqn()].push_back({ &func });
            }

            for (shift_class& clazz : _parser.m_classes) {
                clazz.parser_ = &_parser;
                std::string fqn = clazz.get_fqn();

                // error if class conflicts with module
                if (m_modules.find(fqn) != m_modules.end()) {
                    this->m_token_error(_parser, *clazz.name,
                        "class '" + fqn + "' would override module with same name");
                }

                if (m_classes.find(fqn) == m_classes.end()) {
                    m_classes[std::move(fqn)] = &clazz;

                    for (shift_function& func : clazz.functions) {
                        func.parser_ = &_parser;
                        // size_t& index = m_func_dupe_count[func.get_fqn()];
                        // std::string function_fqn = ;
                        // m_functions[std::move(function_fqn)] = &func;
                        m_function_overloads[func.get_fqn()].push_back({ &func });
                        // for (; m_functions.find(function_fqn) != m_functions.end(); function_fqn = func.get_fqn(++index));
                        // if (m_functions.find(function_fqn) == m_functions.end()) {
                        //     m_functions[std::move(function_fqn)] = &func;
                        //     index++;
                        // } else {
                        //     this->m_token_error(_parser, *clazz.name, "function '" + function_fqn + "' has already been defined");
                        // }
                    }

                    for (shift_variable& var_ : clazz.variables) {
                        var_.parser_ = &_parser;
                        std::string var_fqn = var_.get_fqn();
                        if (m_variables.find(var_fqn) == m_variables.end()) {
                            m_variables[std::move(var_fqn)] = &var_;
                        } else {
                            this->m_token_error(_parser, *clazz.name,
                                "variable '" + var_fqn + "' has already been defined inside class '" + clazz.get_fqn() +
                                "'");
                        }
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
            _scope.module_ = _parser.m_module.get();

            for (shift_module const& use : _parser.m_global_uses) {
                if (this->m_modules.find(use) == this->m_modules.end()) {
                    this->m_name_error(_parser, use.name, "module '" + use.to_string() + "' does not exist");
                } else if (this->m_error_handler && this->m_error_handler->is_print_warnings() &&
                           use == *_parser.m_module) {
                    this->m_name_warning(_parser, use.name, "redundant 'use' statement");
                }
            }

            for (shift_variable& _var : _parser.m_variables) {
                m_analyze_variable(_var, _scope);
            }

            _scope.var = nullptr;

            for (shift_function& func : _parser.m_functions) {
                m_analyze_function(func, _scope);
            }

            _scope.func = nullptr;

            for (shift_class& clazz : _parser.m_classes) {
                _scope.clazz = &clazz;

                for (auto use = clazz.use_statements.begin(); use != clazz.use_statements.end(); ++use) {
                    if (this->m_modules.find(*use) == this->m_modules.end()) {
                        this->m_name_error(_parser, use->name, "module '" + use->to_string() + "' does not exist");
                    } else if (this->m_error_handler && this->m_error_handler->is_print_warnings()) {
                        if (*use == *_parser.m_module) {
                            this->m_name_warning(_parser, use->name, "redundant 'use' statement");
                        } else {
                            size_t dis = std::distance(_parser.m_global_uses.begin(), _parser.m_global_uses.find(*use));
                            if (dis < clazz.implicit_use_statements) {
                                this->m_name_warning(_parser, use->name, "redundant 'use' statement");
                            }
                        }
                    }
                }

                if (!clazz.base.clazz && &clazz != m_classes[SHIFT_ANALYZER_OBJECT_CLASS]) {
                    if (clazz.base.name.size() > 0) {
                        auto base_class_candidates = _scope.find_classes(clazz.base.name);
                        if (base_class_candidates.size() > 1) {
                            this->m_name_error(_parser, clazz.base.name,
                                "ambiguous reference to class '" + clazz.base.name.to_string() + "'");
                        } else if (base_class_candidates.empty()) {
                            this->m_name_error(_parser, clazz.base.name,
                                "unable to resolve class '" + clazz.base.name.to_string() + "'");
                        } else {
                            clazz.base.clazz = base_class_candidates.front();
                        }
                    } else {
                        clazz.base.clazz = m_classes[SHIFT_ANALYZER_OBJECT_CLASS];
                    }
                }

                for (shift_variable& _var : clazz.variables) {
                    m_analyze_variable(_var, _scope);
                }

                _scope.var = nullptr;

                for (shift_function& func : clazz.functions) {
                    m_analyze_function(func, _scope);
                }

                _scope.func = nullptr;
            }
            _scope.clazz = nullptr;
        }
    }

    std::string analyzer::m_mangle_name(const shift_function& func) {
        using namespace std::string_view_literals;
        const std::string regular_fqn = func.get_fqn();

        std::string mangled_fqn = "_sf" + utils::replace_all(regular_fqn, "."sv, "!"sv);

        bool all_params_resolved = true;
        for (auto& [name, param] : func.parameters) {
            if (!param.type.is_resolved()) {
                all_params_resolved = false;
                break;
            }
        }

        if (all_params_resolved) {
            mangled_fqn += "$";
            for (bool past_first = false; auto& [name, param] : func.parameters) {
                if (past_first) { mangled_fqn += "$"; }

                std::string param_fqn;

                const bool is_ref = param.type.ref_type == shift_type::reference_type::ref, is_imut =
                    (param.type.mods & shift_mods::IMUT) ==
                    shift_mods::IMUT;

                if (is_ref || is_imut) {
                    if (is_imut) { param_fqn += "i"; }
                    if (is_ref) { param_fqn += "r"; }
                    param_fqn += "!!";
                }

                shift_type temp_type = param.type;
                temp_type.ref_type = shift_type::reference_type::none;
                temp_type.mods = shift_mods::NONE;

                param_fqn += temp_type.get_fqn();
                utils::replace_all(param_fqn, "."sv, "!"sv);

                mangled_fqn += param_fqn;
                past_first = true;
            }

        }

        return mangled_fqn;
    }

    void analyzer::m_analyze_variable(shift_variable& _var, scope& parent_scope) {
        scope sub_scope;
        sub_scope.parent = &parent_scope;
        sub_scope.base = parent_scope.base;
        sub_scope.parser_ = parent_scope.parser_;
        sub_scope.clazz = parent_scope.clazz;
        sub_scope.func = parent_scope.func;
        sub_scope.var = &_var;
        sub_scope.module_ = parent_scope.module_;

        parser* parser_ = sub_scope.get_parser();

        auto _vars = sub_scope.find_variables(_var.name);
        if (_vars.size() > 1) {
            if (sub_scope.clazz) {
                for (auto it = _vars.begin(); it != _vars.end(); ++it) {
                    shift_variable& found_var = **it;
                    if (!found_var.clazz) {
                        auto next = _vars.erase(it);
                        it = --next;
                    }
                }
                if (_vars.size() > 1) {
                    // TODO fix prompt to account for variable being defined in base class
                    this->m_token_error(*parser_, *_var.name,
                        "multiple definitions of variable '" + std::string(_var.name->get_data()) +
                        "' inside scope of class '" + sub_scope.clazz->get_fqn() + "'");
                }
            } else {
                this->m_token_error(*parser_, *_var.name,
                    "multiple definitions of variable '" + std::string(_var.name->get_data()) +
                    "' inside module '" +
                    parser_->get_module().to_string() + "'");
            }
        }

        m_analyze_variable_type(_var, &sub_scope);
        m_analyze_variable_value(_var, sub_scope);
    }

    void analyzer::m_analyze_variable_type(shift_variable& variable, const scope* current_scope, bool silent) {
        if (variable.type.tried_resolve) return;

        variable.type.tried_resolve = true;

        scope s;
        s.base = this;
        s.parser_ = variable.parser_;
        s.clazz = variable.clazz;
        s.func = variable.function;
        s.var = &variable;
        s.module_ = variable.module_ ? variable.module_ : variable.clazz->module_;
        if (!current_scope) current_scope = &s;

        std::string type_class_name = variable.type.name.name.to_string();

        auto type_class_candidates = current_scope->find_classes(type_class_name);

        if (type_class_candidates.size() > 1) {
            if (!silent)
                this->m_name_error(*current_scope->get_parser(), variable.type.name.name,
                    "ambiguous reference to class '" + type_class_name + "'");
        } else if (type_class_candidates.empty()) {
            if (!silent)
                this->m_name_error(*current_scope->get_parser(), variable.type.name.name,
                    "unable to resolve class '" + type_class_name + "'");
        } else {
            variable.type.name.name_clazz = type_class_candidates.front();
            variable.type.name.clazz = variable.type.name.name_clazz;
            m_finalize_type(variable.type);
            if (!silent)
                m_verify_class_access(current_scope, variable.type.name.clazz, &variable.type.name.name);
        }

    }

    void analyzer::m_analyze_variable_value(shift_variable& variable, scope& current_scope) {
        if (variable.value.resolved.type.tried_resolve) return;
        variable.value.resolved.type.tried_resolve = true;

        if (variable.value.type == token::type::NULL_TOKEN) {
            m_set_default_value(variable);
        } else {
            m_resolve_expression(&variable.value, &current_scope);
        }

        if (variable.value.is_type_resolved()) {
            m_finalize_type(variable.value.resolved.type);

            if (variable.value.resolved.type.name.clazz == &m_null_class) {
                auto& dims = variable.value.resolved.type.dimensions;
                if (dims.empty() || (dims.back().type != shift_type::dimension::dimension_type::pointer)) {
                    this->m_token_error(*current_scope.get_parser(), *variable.value.begin,
                        "cannot convert from 'null' to non-pointer type");
                }
            } else {
                if (variable.type.is_conversion_needed(variable.value.resolved.type)) {
                    // find function that will implicitly convert type
                    const auto& conversions = m_get_implicit_conversions(variable.value.resolved.type, variable.type);
                    if (conversions.size() == 1) {
                        m_apply_implicit_conversion(variable.value, conversions.front(), current_scope);
                    } else if (conversions.size() > 1) {
                        this->m_token_error(*current_scope.get_parser(), *(variable.value.begin - 1),
                            "ambiguous type conversion from '"
                            + variable.value.resolved.type.name.clazz->get_fqn() + "' to '" +
                            variable.type.name.clazz->get_fqn() + "'");
                        m_error_candidates(conversions);
                    } else {
                        this->m_token_error(*current_scope.get_parser(), *(variable.value.begin - 1), "unable to convert type '"
                                                                                                      +
                                                                                                      variable.value.resolved.type.name.clazz->get_fqn() +
                                                                                                      "' into class type '" +
                                                                                                      variable.type.name.clazz->get_fqn() +
                                                                                                      "'");
                    }
                }
            }
        }

        if (variable.value.resolved.type.name.clazz && variable.value.resolved.type.name.clazz != &m_void_class &&
            variable.value.resolved.type.name.clazz != &m_null_class
            && (variable.value.is_function_call() || variable.value.is_array() ||
                variable.value.type == token::type::IDENTIFIER)) {
            m_verify_access(&current_scope, &variable.value);
        }
    }

    void analyzer::m_analyze_function(shift_function& func, scope& _scope) {
        scope sub_scope;
        sub_scope.parent = &_scope;
        sub_scope.base = _scope.base;
        sub_scope.parser_ = _scope.parser_;
        sub_scope.clazz = _scope.clazz;
        sub_scope.func = &func;
        sub_scope.var = _scope.var;
        sub_scope.module_ = _scope.module_;

        m_analyze_function_return_type(func);
        m_analyze_function_params(func);

        m_functions[m_mangle_name(func)] = &func;

        for (auto& other_overload : m_function_overloads[func.get_fqn()]) {
            if (other_overload.func == &func) continue;
            if (other_overload.func->parameters.size() == func.parameters.size()) {
                auto func_param = func.parameters.begin(), other_func_param = other_overload.func->parameters.begin();
                for (; func_param != func.parameters.end() &&
                       other_func_param != other_overload.func->parameters.end(); ++other_func_param, ++func_param) {
                    if (func_param->second.type != other_func_param->second.type) break;
                }

                if (func_param == func.parameters.end() || other_func_param == other_overload.func->parameters.end()) {
                    this->m_name_error(*_scope.get_parser(), func.name,
                        "duplicate function declaration for function '" + func.get_fqn() + "'");
                }
            }
        }

        m_analyze_function_body(func, &_scope);
    }

    void analyzer::m_set_default_value(shift_variable& var, bool silent) noexcept {
        if (!var.type.is_resolved() && !var.type.tried_resolve) {
            m_analyze_variable_type(var, silent);
        }

        if (!var.type.is_resolved()) return;

        shift_expression value;
        value.set_function_call();
        value.begin = var.value.begin;
        value.end = var.value.end;

        if (value.begin == value.end || value.begin == std::vector<token>::const_iterator()) {
            if (var.parser_) {
                value.begin = var.parser_->get_tokenizer()->position_after(*var.name);
                value.end = value.begin + 1;
            }
        }

        shift_function* func = nullptr;
        {
            auto overload_it = m_function_overloads.find(var.type.name.clazz->get_fqn() + ".constructor");
            if (overload_it != m_function_overloads.end()) {
                for (auto& overload : overload_it->second) {
                    // TODO allow for default parameters
                    if (overload.func->parameters.size() == 0) {
                        func = overload.func;
                        break;
                    }
                }
            }
        }

        if (!func) {
            if (!silent) {
                shift_type temp_type = var.type;
                temp_type.ref_type = shift_type::reference_type::none;
                this->m_token_error(*var.parser_, *var.name,
                    "no default constructor found for class '" + temp_type.get_printable_fqn() + "'");
            }
        }

        {
            shift_expression function_call_object;
            function_call_object.type = token::type::IDENTIFIER;
            function_call_object.begin = value.begin;
            function_call_object.end = value.end;
            function_call_object.resolved.type = var.type;
            function_call_object.resolved.type.ref_type = shift_type::reference_type::tref;
            function_call_object.resolved.clazz = var.type.name.clazz;
            function_call_object.resolved.function = func;

            value.set_function_call_object(std::move(function_call_object));
        }
        value.resolved.function = func;
        value.resolved.type = var.type;
        value.resolved.type.ref_type = shift_type::reference_type::tref;
        shift_expression* const old_parent = var.value.parent;
        var.value = std::move(value);
        var.value.update_parents(old_parent);
    }

    void analyzer::m_init_defaults() {
        if (m_token_storage.empty()) {
            m_token_storage.reserve(16);
            m_token_storage.emplace_back(std::string_view("void"), token::type::IDENTIFIER, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("array"), token::type::IDENTIFIER, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("length"), token::type::IDENTIFIER, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("="), token::type::EQUALS, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("=="), token::type::EQUALS_EQUALS, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("!="), token::type::NOT_EQUAL, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("null"), token::type::IDENTIFIER, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("operator"), token::type::IDENTIFIER, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("["), token::type::LEFT_SQUARE_BRACKET, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("]"), token::type::RIGHT_SQUARE_BRACKET, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("operator"), token::type::IDENTIFIER, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("*"), token::type::STAR, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("operator"), token::type::IDENTIFIER, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("->"), token::type::ARROW, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("shift"), token::type::IDENTIFIER, file_indexer{ 0, 0 });
            m_token_storage.emplace_back(std::string_view("true"), token::type::IDENTIFIER, file_indexer{ 0, 0 });


            auto it = m_token_storage.begin();
            m_void_token = it++;
            m_array_token = it++;
            m_length_token = it++;
            m_equals_token = it++;
            m_equals_equals_token = it++;
            m_not_equal_token = it++;
            m_null_token = it++;
            m_operator_token = it;
            m_operator_array_function_token_begin = it++;
            m_left_square_bracket_token = it++;
            m_right_square_bracket_token = it++;
            m_operator_array_function_token_end = it;
            m_operator_star_function_token_begin = it++;
            m_operator_star_function_token_end = ++it;
            m_operator_arrow_function_token_begin = it++;
            m_operator_arrow_function_token_end = ++it;
            m_shift_module.name.begin = it++;
            m_shift_module.name.end = it;
            m_true_token_begin = it++;
        }

        {
            m_void_class.name = &*m_void_token;
            m_null_class.name = &*m_null_token;
        }
    }

    const std::list<analyzer::type_conversion_info>&
    analyzer::m_get_implicit_conversions(const shift_type* from, const shift_type* to,
        std::unordered_set<shift_class*>& history) noexcept {
        static const std::list<analyzer::type_conversion_info> empty_conversions;
        std::list<analyzer::type_conversion_info> funcs;

        if (!from || !to || from->name.clazz == &m_void_class || to->name.clazz == &m_void_class || from->name.clazz == &m_null_class
            || to->name.clazz == &m_null_class || from == to
            || (from->ref_type == shift_type::reference_type::tref &&
                to->ref_type == shift_type::reference_type::ref &&
                ((to->mods & shift_mods::IMUT) == 0x0))
            || (from->ref_type == shift_type::reference_type::ref && to->ref_type == shift_type::reference_type::none)
            || (from->ref_type == shift_type::reference_type::ref &&
                to->ref_type == shift_type::reference_type::ref &&
                (from->mods & shift_mods::IMUT) && ((to->mods & shift_mods::IMUT) == 0x0))
            || *from == *to || !from->is_resolved() || !to->is_resolved() ||
            !from->is_conversion_needed(*to)) {
            return empty_conversions;
        }

        static std::unordered_map<std::pair<shift_type, shift_type>, std::list<analyzer::type_conversion_info>> m_implicit_conversion_cache;

        {
            auto f = m_implicit_conversion_cache.find({ *from, *to });
            if (f != m_implicit_conversion_cache.end()) {
                return f->second;
            }
        }

        {
            auto const constructor_overloads_it = m_function_overloads.find(to->get_fqn() + ".constructor");
            if (constructor_overloads_it == m_function_overloads.end()) { goto cache_result; }

            for (auto& constructor_overload : constructor_overloads_it->second) {
                if (constructor_overload.func->mods & shift_mods::EXPLICIT) continue;
                if (constructor_overload.func->parameters.size() != 1) continue;

                auto& [param_name, param] = constructor_overload.func->parameters.front();

                if (!param.type.tried_resolve && !param.type.is_resolved()) {
                    m_analyze_function_params(*constructor_overload.func);
                }

                if (!param.type.is_resolved()) continue;

                if (((param.type.ref_type == shift_type::reference_type::ref && from->ref_type == shift_type::reference_type::ref) ||
                     (param.type.mods & shift_mods::IMUT))
                    || (param.type.ref_type != shift_type::reference_type::ref && from->ref_type == shift_type::reference_type::tref &&
                        ((param.type.mods & shift_mods::IMUT) || ((from->mods & shift_mods::IMUT) == 0x0)))) {
                    if (param.type.name.clazz == from->name.clazz) {
                        // exact match
                        funcs.clear();
                        funcs.push_back({ *from, to, { constructor_overload.func }});
                        goto cache_result;
                    } else if (from->name.clazz->has_base(param.type.name.clazz)) {
                        // base class match

                        // Remove base class matches that would be worse than this current one
                        bool should_add = true;
                        for (auto it = funcs.begin(); it != funcs.end(); ++it) {
                            auto* first_func = it->funcs.front();
                            auto& [first_func_param_name, first_func_param] = first_func->parameters.front();
                            if (param.type.name.clazz->has_base(first_func_param.type.name.clazz)) {
                                it = funcs.erase(it);
                                --it;
                            } else if (first_func_param.type.name.clazz->has_base(param.type.name.clazz)) {
                                should_add = false;
                                break;
                            }
                        }
                        // Only add if it's not a worse match than whats already in funcs (if a base class is even in funcs)
                        if (should_add) { funcs.push_back({ *from, to, { constructor_overload.func }}); }
                    } else {
                        // have to check if this type converts
                        if (history.find(from->name.clazz) == history.end()) {
                            history.insert(from->name.clazz);
                            shift_type new_from = *from;
                            new_from.ref_type = shift_type::reference_type::tref;

                            const auto& sub_conversions = m_get_implicit_conversions(&new_from, &param.type, history);

                            for (const auto& sub_conversion : sub_conversions) {
                                funcs.push_back({ *from, to, sub_conversion.funcs });
                                funcs.back().funcs.push_back(constructor_overload.func);
                            }
                        }
                    }
                }
            }
        }

      cache_result:
        auto cache_it = m_implicit_conversion_cache.insert({{ *from, *to }, std::move(funcs) });

        return cache_it.first->second;
    }

    void
    analyzer::m_apply_implicit_conversion(shift_expression& expr, const type_conversion_info& conversion, const scope& current_scope,
        bool silent) {
        if (expr.is_type_resolved()) {
            if (expr.resolved.type == *conversion.to) return;
            if (expr.resolved.type.name.clazz != conversion.from.name.clazz) {
                if (!silent) {
                    this->m_token_error(*current_scope.get_parser(), *expr.begin,
                        "cannot convert type '" + expr.resolved.type.get_printable_fqn() +
                        "' to type '" + conversion.to->get_printable_fqn() + "'");
                }
            }
        }

        shift_expression* const old_parent = expr.parent;

        for (shift_function* conversion_func : conversion.funcs) {
            shift_expression constructor_call_expr;
            constructor_call_expr.set_function_call();
            constructor_call_expr.begin = expr.begin;
            constructor_call_expr.end = expr.end;
            {
                shift_expression function_call_object;
                function_call_object.type = expr.type;
                function_call_object.begin = expr.begin;
                function_call_object.end = expr.end;
                // m_analyze_function_return_type(*conversion_func, silent);
                // TODO If we have conversion functions outside of classes, this must be fixed
                function_call_object.resolved.type.name.clazz = conversion_func->clazz;
                function_call_object.resolved.type.ref_type = shift_type::reference_type::tref;
                function_call_object.resolved.clazz = conversion_func->clazz;
                function_call_object.resolved.function = conversion_func;

                constructor_call_expr.set_function_call_object(std::move(function_call_object));
            }
            constructor_call_expr.resolved.type.name.clazz = conversion_func->clazz;
            constructor_call_expr.resolved.type.ref_type = shift_type::reference_type::tref;
            constructor_call_expr.resolved.clazz = conversion_func->clazz;
            constructor_call_expr.resolved.function = conversion_func;

            constructor_call_expr.add_function_call_arguments(std::move(expr));

            expr = std::move(constructor_call_expr);
        }
        expr.update_parents(old_parent);
    }

    bool analyzer::m_verify_class_access(const scope* const parent_scope, const shift_class* clazz,
        const std::variant<const token*, const shift_name*>& error) {

        for (parser* parser_ = parent_scope->get_parser(); clazz; clazz = clazz->parent.clazz) {
            if (clazz->mods & shift_mods::PRIVATE) {
                if (!clazz->parent.clazz && parser_->get_module() != *clazz->module_) {
                    std::visit(
                        utils::visit_overloader{
                            [&](const token* error_token) {
                                this->m_token_error(*parser_, *error_token, "private class '" + clazz->get_fqn() +
                                                                            "' can only be accessed within same module (" +
                                                                            clazz->module_->to_string() + ")");
                            },
                            [&](const shift_name* name_token) {
                                this->m_name_error(*parser_, *name_token, "private class '" + clazz->get_fqn() +
                                                                          "' can only be accessed within same module (" +
                                                                          clazz->module_->to_string() + ")");
                            }
                        }, error);

                    return false;
                } else if (clazz->parent.clazz && (!parent_scope->clazz || (clazz != parent_scope->clazz &&
                                                                            !clazz->has_parent(parent_scope->clazz) &&
                                                                            !parent_scope->clazz->has_parent(clazz)))) {
                    std::visit(
                        utils::visit_overloader{
                            [&](const token* error_token) {
                                this->m_token_error(*parser_, *error_token, "private class '" + clazz->get_fqn() +
                                                                            "' cannot be accessed within current scope");
                            },
                            [&](const shift_name* name_token) {
                                this->m_name_error(*parser_, *name_token, "private class '" + clazz->get_fqn() +
                                                                          "' cannot be accessed within current scope");
                            }
                        }, error);
                    return false;
                }
            } else if (clazz->mods & shift_mods::PROTECTED) {
                if (!clazz->parent.clazz && !utils::starts_with((std::string_view) clazz->module_->to_string(),
                    (std::string_view) parser_->get_module().to_string())) {
                    std::visit(
                        utils::visit_overloader{
                            [&](const token* error_token) {
                                this->m_token_error(*parser_, *error_token,
                                    "protected class '" + clazz->get_fqn() +
                                    "' may only be accessed from submodules of module '" + clazz->module_->to_string() +
                                    "'");
                            },
                            [&](const shift_name* name_token) {
                                this->m_name_error(*parser_, *name_token,
                                    "protected class '" + clazz->get_fqn() +
                                    "' may only be accessed from submodules of module '" + clazz->module_->to_string() +
                                    "'");
                            }
                        }, error);
                    return false;
                } else if (clazz->parent.clazz && (!parent_scope->clazz || (clazz != parent_scope->clazz &&
                                                                            !clazz->has_parent(parent_scope->clazz) &&
                                                                            !parent_scope->clazz->has_parent(clazz) &&
                                                                            !parent_scope->clazz->has_base(clazz)))) {
                    std::visit(
                        utils::visit_overloader{
                            [&](const token* error_token) {
                                this->m_token_error(*parser_, *error_token,
                                    "protected class '" + clazz->get_fqn() +
                                    "' cannot be accessed within current scope");
                            },
                            [&](const shift_name* name_token) {
                                this->m_name_error(*parser_, *name_token,
                                    "protected class '" + clazz->get_fqn() +
                                    "' cannot be accessed within current scope");
                            }
                        }, error);
                    return false;
                }
            }
        }
        return true;
    }

    bool analyzer::m_verify_access(const scope* parent_scope, const shift_class* clazz, const shift_name& error_name, bool silent) {
        if (clazz->mods & shift_mods::PRIVATE) {
            if (clazz->parent.clazz) {
                if (!parent_scope->clazz || (parent_scope->clazz != clazz && !clazz->has_parent(parent_scope->clazz))) {
                    if (!silent) {
                        this->m_name_error(*parent_scope->get_parser(), error_name,
                            "private class '" + clazz->get_fqn() + "' cannot be accessed within current scope");
                    }
                    return false;
                }
            } else {
                if (*parent_scope->get_module() != *clazz->module_) {
                    if (!silent) {
                        this->m_name_error(*parent_scope->get_parser(), error_name,
                            "private class '" + clazz->get_fqn() + "' can only be accessed within same module (" +
                            clazz->module_->to_string() + ")");
                    }
                    return false;
                }
            }
        } else if (clazz->mods & shift_mods::PROTECTED) {
            if (clazz->parent.clazz) {
                if (!parent_scope->clazz ||
                    (parent_scope->clazz != clazz && !clazz->has_parent(parent_scope->clazz) && !parent_scope->clazz->has_parent(clazz))) {
                    if (!silent) {
                        this->m_name_error(*parent_scope->get_parser(), error_name,
                            "protected class '" + clazz->get_fqn() + "' cannot be accessed within current scope");
                    }
                    return false;
                }
            } else {
                if (!utils::starts_with((std::string_view) parent_scope->get_module()->to_string(),
                    (std::string_view) clazz->module_->to_string())) {
                    if (!silent) {
                        this->m_name_error(*parent_scope->get_parser(), error_name,
                            "protected class '" + clazz->get_fqn() + "' can only be accessed from submodules of module '" +
                            clazz->module_->to_string() + "'");
                    }
                    return false;
                }
            }
        }
        return true;
    }

    bool analyzer::m_verify_access(const scope* parent_scope, const shift_module* module_, const shift_name& error_name, bool silent) {
        if (module_->mods & shift_mods::PROTECTED) {
            if (!utils::starts_with((std::string_view) parent_scope->get_module()->to_string(),
                (std::string_view) module_->to_string())) {
                if (!silent) {
                    this->m_name_error(*parent_scope->get_parser(), error_name,
                        "protected module '" + module_->to_string() + "' can only be accessed from submodules of module '" +
                        module_->to_string() + "'");
                }
                return false;
            }
        }
        return true;
    }

    bool analyzer::m_verify_access(const scope* parent_scope, shift_variable* variable, const shift_name& error_name, bool silent) {
        if (!variable->type.is_resolved()) {
            m_analyze_variable_type(*variable, nullptr, silent);
        }

        if (variable->type.is_resolved()) {
            if (!m_verify_access(parent_scope, variable->type.name.clazz, error_name, silent)) {
                return false;
            }
        }

        if (variable->clazz || variable->module_) {
            if (variable->mods & shift_mods::PRIVATE) {
                if (variable->clazz) {
                    if (!parent_scope->clazz ||
                        (parent_scope->clazz != variable->clazz && !variable->clazz->has_parent(parent_scope->clazz))) {
                        if (!silent) {
                            this->m_name_error(*parent_scope->get_parser(), error_name,
                                "private field '" + variable->get_fqn() + "' cannot be accessed within current scope");
                        }
                        return false;
                    }
                } else {
                    if (*parent_scope->get_module() != *variable->module_) {
                        if (!silent) {
                            this->m_name_error(*parent_scope->get_parser(), error_name,
                                "private field '" + variable->get_fqn() + "' can only be accessed within same module (" +
                                variable->module_->to_string() + ")");
                        }
                        return false;
                    }
                }
            } else if (variable->mods & shift_mods::PROTECTED) {
                if (variable->clazz) {
                    if (!parent_scope->clazz ||
                        (parent_scope->clazz != variable->clazz && !variable->clazz->has_parent(parent_scope->clazz) &&
                         !parent_scope->clazz->has_parent(variable->clazz))) {
                        if (!silent) {
                            this->m_name_error(*parent_scope->get_parser(), error_name,
                                "protected field '" + variable->get_fqn() + "' cannot be accessed within current scope");
                        }
                        return false;
                    }
                } else {
                    if (!utils::starts_with((std::string_view) parent_scope->get_module()->to_string(),
                        (std::string_view) variable->module_->to_string())) {
                        if (!silent) {
                            this->m_name_error(*parent_scope->get_parser(), error_name,
                                "protected field '" + variable->get_fqn() +
                                "' can only be accessed from submodules of module '" +
                                variable->module_->to_string() + "'");
                        }
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool analyzer::m_verify_access(const scope* parent_scope, shift_function* function, const shift_name& error_name, bool silent) {
        if (!function->return_type.is_resolved()) {
            m_analyze_function_return_type(*function, silent);
        }

        if (function->return_type.is_resolved()) {
            if (!m_verify_access(parent_scope, function->return_type.name.clazz, error_name, silent)) {
                return false;
            }
        }

        if (function->mods & shift_mods::PRIVATE) {
            if (!parent_scope->clazz || (parent_scope->clazz != function->clazz && !function->clazz->has_parent(parent_scope->clazz))) {
                if (!silent) {
                    m_analyze_function_params(*function, false);
                    this->m_name_error(*parent_scope->get_parser(), error_name,
                        "private function '" + function->get_signature() + "' cannot be accessed within current scope");
                }
                return false;
            }
        } else if (function->mods & shift_mods::PROTECTED) {
            if (!parent_scope->clazz || (parent_scope->clazz != function->clazz && !function->clazz->has_parent(parent_scope->clazz) &&
                                         !parent_scope->clazz->has_parent(function->clazz))) {
                if (!silent) {
                    m_analyze_function_params(*function, false);
                    this->m_name_error(*parent_scope->get_parser(), error_name,
                        "protected function '" + function->get_signature() + "' cannot be accessed within current scope");
                }
                return false;
            }
        }

        return true;
    }


    bool analyzer::m_verify_access(const scope* parent_scope, shift_expression* expr, bool silent) {
        if (expr->is_dotted_expression()) {
            for (auto& sub_expr : expr->get_dotted_expressions()) {
                if (!m_verify_access(parent_scope, &sub_expr, silent)) return false;
            }
        }
        if (expr->type == token::type::IDENTIFIER) {
            if (expr->resolved.module_) {
                return m_verify_access(parent_scope, expr->resolved.module_, shift_name{ expr->begin, expr->end }, silent);
            } else if (expr->resolved.variable) {
                return m_verify_access(parent_scope, expr->resolved.variable, shift_name{ expr->begin, expr->end }, silent);
            } else if (expr->resolved.function) {
                return m_verify_access(parent_scope, expr->resolved.function, shift_name{ expr->begin, expr->end }, silent);
            } else if (expr->resolved.clazz) {
                return m_verify_access(parent_scope, expr->resolved.clazz, shift_name{ expr->begin, expr->end }, silent);
            }
        } else if (expr->is_function_call()) {
            if (expr->resolved.function) {
                if (!m_verify_access(parent_scope, expr->resolved.function,
                    shift_name{ expr->get_function_call_object()->begin, expr->get_function_call_object()->end }, silent))
                    return false;
            } else if (expr->get_function_call_object()->resolved.function) {
                if (!m_verify_access(parent_scope, expr->get_function_call_object()->resolved.function,
                    shift_name{ expr->get_function_call_object()->begin, expr->get_function_call_object()->end }, silent))
                    return false;
            }

            for (shift_expression& param_expr : expr->get_function_call_arguments()) {
                if (!m_verify_access(parent_scope, &param_expr, silent)) return false;
            }
        } else if (expr->is_array()) {
            if (!m_verify_access(parent_scope, expr->get_array_object(), silent)) return false;
            for (shift_expression& dim_expr : expr->get_array_dimensions()) {
                if (!m_verify_access(parent_scope, dim_expr.get_array_indexer_expression(), silent)) return false;
                if (dim_expr.resolved.function && !m_verify_access(parent_scope, dim_expr.resolved.function,
                    shift_name{ dim_expr.get_array_indexer_expression()->begin,
                                dim_expr.get_array_indexer_expression()->end }, silent))
                    return false;
            }
        } else if (expr->type == token::type::COMMA) {
            for (shift_expression& sub_expr : expr->get_comma_expressions()) {
                if (!m_verify_access(parent_scope, &sub_expr, silent)) return false;
            }
        } else if (is_overload_operator(expr->type)) {
            if (expr->resolved.function) {
                if (!m_verify_access(parent_scope, expr->resolved.function, shift_name{ expr->begin, expr->end }, silent)) return false;
                shift_function* const func = expr->resolved.function;

                if ((func->mods & shift_mods::PRIVATE) && parent_scope->clazz != func->clazz) {
                    m_analyze_function_params(*func, silent);
                    this->m_token_error(*parent_scope->get_parser(), *expr->begin,
                        "function '" + func->get_signature() +
                        "' cannot be accessed within current scope");
                    return false;
                } else if ((func->mods & shift_mods::PROTECTED) && parent_scope->clazz != func->clazz &&
                           !parent_scope->clazz->has_base(func->clazz)) {
                    m_analyze_function_params(*func, silent);
                    this->m_token_error(*parent_scope->get_parser(), *expr->begin,
                        "function '" + func->get_signature() +
                        "' cannot be accessed within current scope");
                    return false;
                } else if ((func->mods & shift_mods::STATIC) == 0x0) {
                    if (parent_scope->var) {
                        if ((parent_scope->var->type.mods & shift_mods::STATIC) || !parent_scope->var) {
                            m_analyze_function_params(*func, silent);
                            this->m_token_error(*parent_scope->get_parser(), *expr->begin,
                                "non-static function '" + func->get_signature() +
                                "' cannot be accessed within current scope");
                            return false;
                        }
                    } else if (parent_scope->func) {
                        if ((parent_scope->func->mods & shift_mods::STATIC) || !parent_scope->func) {
                            m_analyze_function_params(*func, silent);
                            this->m_token_error(*parent_scope->get_parser(), *expr->begin,
                                "non-static function '" + func->get_signature() +
                                "' cannot be accessed within current scope");
                            return false;
                        }
                    }
                }
            }

            const bool has_left = expr->has_left() && expr->get_left()->type != token::type::NULL_TOKEN;
            const bool has_right = expr->has_right() && expr->get_right()->type != token::type::NULL_TOKEN;

            if (has_left) {
                if (!m_verify_access(parent_scope, expr->get_left(), silent)) return false;
            }

            if (has_right) {
                if (!m_verify_access(parent_scope, expr->get_right(), silent)) return false;
            }
        }
        if (expr->is_type_resolved()) {
            if (!m_verify_access(parent_scope, expr->resolved.type.name.clazz, shift_name{ expr->begin, expr->end }, silent))
                return false;
        }
        return true;
    }

    void analyzer::m_analyze_function_return_type(shift_function& func, bool silent) {
        if (!func.name.begin->is_constructor() && !func.name.begin->is_destructor() && !func.return_type.name.name.begin->is_void()) {
            scope current_scope;
            current_scope.base = this;
            current_scope.parser_ = func.parser_;
            current_scope.clazz = func.clazz;
            current_scope.func = &func;
            current_scope.module_ = func.module_ ? func.module_ : func.clazz->module_;

            std::string return_type_class_name = func.return_type.name.name.to_string();

            auto return_type_class_candidates = current_scope.find_classes(return_type_class_name);

            if (return_type_class_candidates.size() > 1) {
                if (!silent)
                    this->m_name_error(*current_scope.get_parser(), func.return_type.name.name,
                        "ambiguous reference to class '" + return_type_class_name + "'");
            } else if (return_type_class_candidates.empty()) {
                if (!silent)
                    this->m_name_error(*current_scope.get_parser(), func.return_type.name.name,
                        "unable to resolve class '" + return_type_class_name + "'");
            } else {
                func.return_type.name.clazz = return_type_class_candidates.front();
                func.return_type.name.name_clazz = func.return_type.name.clazz;
                m_finalize_type(func.return_type);
                m_verify_class_access(&current_scope, func.return_type.name.clazz, &*(func.return_type.name.name.end - 1));
            }
        } else if (func.name.begin->is_constructor() || func.name.begin->is_destructor() || func.return_type.name.name.begin->is_void()) {
            func.return_type.name.clazz = &m_void_class;
            func.return_type.ref_type = shift_type::reference_type::none;
            func.return_type.mods = shift_mods::NONE;
            func.return_type.dimensions.clear();
        }
    }

    void analyzer::m_analyze_function_params(shift_function& func, bool silent) {
        scope current_scope;
        current_scope.base = this;
        current_scope.parser_ = func.parser_;
        current_scope.clazz = func.clazz;
        current_scope.func = &func;
        current_scope.module_ = func.module_ ? func.module_ : func.clazz->module_;

        for (auto& [param_name, param_var] : func.parameters) {
            if (param_var.type.tried_resolve) continue;
            param_var.type.tried_resolve = true;
            if (param_var.type.name.clazz || param_var.type.name.name_clazz) continue;
            std::string param_type_class_name = param_var.type.name.name.to_string();

            auto param_type_class_candidates = current_scope.find_classes(param_type_class_name);

            if (param_type_class_candidates.size() > 1) {
                // TODO list candidates
                if (!silent)
                    this->m_name_error(*current_scope.get_parser(), param_var.type.name.name,
                        "ambiguous reference to class '" + param_type_class_name + "' in current scope");
            } else if (param_type_class_candidates.empty()) {
                if (!silent)
                    this->m_name_error(*current_scope.get_parser(), param_var.type.name.name,
                        "unable to resolve class '" + param_type_class_name + "' in current scope");
            } else {
                param_var.type.name.clazz = param_type_class_candidates.front();
                param_var.type.name.name_clazz = param_var.type.name.clazz;
                m_finalize_type(param_var.type);
                m_verify_access(&current_scope, param_var.type.name.clazz, param_var.type.name.name);
            }
        }
    }

    void analyzer::m_analyze_function_body(shift_function& func, scope* parent_scope) {
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
        func_scope.module_ = parent_scope->module_;
        m_analyze_scope(func.statements, &func_scope);
    }

    void analyzer::m_analyze_scope(utils::ideque<shift_statement>::iterator statements_begin,
        utils::ideque<shift_statement>::iterator statements_end, scope* parent_scope) {
        scope _scope;
        if (parent_scope) {
            _scope.parent = parent_scope;
            _scope.base = parent_scope->base;
            _scope.parser_ = parent_scope->parser_;
            _scope.clazz = parent_scope->clazz;
            _scope.func = parent_scope->func;
            _scope.var = parent_scope->var;
            _scope.module_ = parent_scope->module_;
        } else {
            _scope.base = this;
        }

        for (; statements_begin != statements_end; statements_begin++) {
            shift_statement& statement = *statements_begin;

            switch (statement.type) {
                case shift_statement::statement_type::variable_alloc: {
                    shift_variable& statement_var = statement.get_variable();

                    if (_scope.variables.find(statement_var.name->get_data()) != _scope.variables.end() ||
                        _scope.func->parameters.contains(statement_var.name->get_data())) {
                        this->m_token_error(*_scope.get_parser(), *statement_var.name,
                            "variable with name '" + std::string(statement_var.name->get_data()) +
                            "' has already been defined in current scope");
                    } else if (this->m_error_handler && this->m_error_handler->is_print_warnings()) {
                        auto found_vars = _scope.find_variables(statement_var.name->get_data());
                        if (!found_vars.empty()) {
                            shift_variable& found_var = *found_vars.front();
                            if (found_var.function) {
                                this->m_token_warning(*_scope.get_parser(), *statement_var.name,
                                    "variable masks variable with identical name in upper scope");
                            } else if (found_var.clazz) {
                                this->m_token_warning(*_scope.get_parser(), *statement_var.name,
                                    "variable masks variable with identical name in class '" +
                                    found_var.clazz->get_fqn() + "'");
                            }
                        }
                    }

                    _scope.variables[statement_var.name->get_data()] = &statement_var;

                    statement_var.function = _scope.func;
                    statement_var.parser_ = _scope.get_parser();
                    statement_var.module_ = _scope.module_;

                    auto statement_var_classes = _scope.find_classes(statement_var.type.name.name);
                    if (statement_var_classes.size() > 1) {
                        this->m_name_error(*_scope.get_parser(), statement_var.type.name.name,
                            "ambiguous reference to class '" + statement_var.type.name.name.to_string() +
                            "'");
                    } else if (statement_var_classes.empty()) {
                        this->m_name_error(*_scope.get_parser(), statement_var.type.name.name,
                            "unable to resolve class '" + statement_var.type.name.name.to_string() + "'");
                    } else {
                        statement_var.type.name.clazz = statement_var_classes.front();

                        m_finalize_type(statement_var.type);
                        m_verify_access(&_scope, statement_var.type.name.clazz,
                            { statement_var.type.name.name.begin, statement_var.type.name.name.end });
                    }

                    if (statement_var.value.type != token::type::NULL_TOKEN) {
                        m_resolve_expression(&statement_var.value, &_scope);
                    } else {
                        m_set_default_value(statement_var);
                    }

                    if (statement_var.value.is_type_resolved()) {
                        m_finalize_type(statement_var.value.resolved.type);

                        if (statement_var.value.resolved.type.is_conversion_needed(statement_var.type)) {
                            const auto& conversions = m_get_implicit_conversions(statement_var.value.resolved.type, statement_var.type);
                            if (conversions.size() == 1) {
                                m_apply_implicit_conversion(statement_var.value, conversions.front(), _scope);
                            } else if (conversions.size() > 1) {
                                this->m_token_error(*_scope.get_parser(), *(statement_var.value.begin - 1),
                                    "ambiguous type conversion from '" +
                                    statement_var.value.resolved.type.get_printable_fqn() +
                                    "' to '" + statement_var.type.get_printable_fqn() + "'");
                                m_error_candidates(conversions);
                            } else {
                                this->m_token_error(*_scope.get_parser(), *(statement_var.value.begin - 1),
                                    "unable to convert type '" + statement_var.value.resolved.type.get_printable_fqn() +
                                    "' into class type '" + statement_var.type.get_printable_fqn() + "'");
                            }
                        }

                        m_verify_access(&_scope, &statement_var.value);
                    }

                    break;
                }

                case shift_statement::statement_type::expression: {
                    if (statement.get_expression().type != token::type::NULL_TOKEN) {
                        m_resolve_expression(&statement.get_expression(), &_scope);
                        if (statement.get_expression().is_type_resolved()) {
                            m_finalize_type(statement.get_expression().resolved.type);
                            m_verify_access(&_scope, &statement.get_expression());
                        }
                    }

                    // TODO remove from list otherwise (if unable to resolve name.clazz)
                    break;
                }

                case shift_statement::statement_type::scope_begin: {
                    m_analyze_scope(statement.get_block_statements(), &_scope);

                    break;
                }

                case shift_statement::statement_type::if_: {
                    shift_expression& if_condition = statement.get_if_condition();
                    m_resolve_expression(&if_condition, &_scope);

                    if (if_condition.is_type_resolved()) {
                        shift_type bool_type;
                        bool_type.name.clazz = m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS];
                        bool_type.ref_type = shift_type::reference_type::ref;
                        bool_type.mods = shift_mods::IMUT;

                        if (if_condition.resolved.type.is_conversion_needed(bool_type)) {
                            const auto& conversions = m_get_implicit_conversions(if_condition.resolved.type, bool_type);
                            if (conversions.size() == 1) {
                                m_apply_implicit_conversion(if_condition, conversions.front(), _scope);
                            } else if (conversions.size() > 1) {
                                this->m_token_error(*_scope.get_parser(), *statement.get_if(),
                                    "ambiguous conversion of conditional value of type '" +
                                    if_condition.resolved.type.get_printable_fqn() +
                                    "' to type '" SHIFT_ANALYZER_BOOLEAN_CLASS "'");
                                m_error_candidates(conversions);
                            } else if (conversions.empty()) {
                                this->m_token_error(*_scope.get_parser(), *statement.get_if(),
                                    "cannot convert conditional value of type '" +
                                    if_condition.resolved.type.get_printable_fqn() +
                                    "' into type '" SHIFT_ANALYZER_BOOLEAN_CLASS "'");
                            }
                        }

                        m_verify_access(&_scope, &if_condition);
                    }

                    m_analyze_scope(statement.get_if_statements(), &_scope);

                    if (statement.has_attached_else()) {
                        m_analyze_scope(statement.get_attached_else().get_else_statements(), &_scope);
                    }

                    break;
                }

                case shift_statement::statement_type::else_: {
                    // ~~Skipping past else statement storage, located as the last statement inside an if statement~~
                    // This case should never be called
                    break;
                }

                case shift_statement::statement_type::while_: {
                    shift_expression& while_condition = statement.get_while_condition();
                    m_resolve_expression(&while_condition, &_scope);

                    if (while_condition.is_type_resolved()) {


                        shift_type bool_type;
                        bool_type.name.clazz = m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS];
                        bool_type.ref_type = shift_type::reference_type::ref;
                        bool_type.mods = shift_mods::IMUT;

                        if (while_condition.resolved.type.is_conversion_needed(bool_type)) {
                            const auto& conversions = m_get_implicit_conversions(while_condition.resolved.type, bool_type);
                            if (conversions.size() == 1) {
                                m_apply_implicit_conversion(while_condition, conversions.front(), _scope);
                            } else if (conversions.size() > 1) {
                                this->m_token_error(*_scope.get_parser(), *statement.get_if(),
                                    "ambiguous conversion of conditional value of type '" +
                                    while_condition.resolved.type.get_printable_fqn() +
                                    "' to type '" SHIFT_ANALYZER_BOOLEAN_CLASS "'");
                                m_error_candidates(conversions);
                            } else if (conversions.empty()) {
                                this->m_token_error(*_scope.get_parser(), *statement.get_if(),
                                    "cannot convert conditional value of type '" +
                                    while_condition.resolved.type.get_printable_fqn() +
                                    "' into type '" SHIFT_ANALYZER_BOOLEAN_CLASS "'");
                            }
                        }

                        m_verify_access(&_scope, &while_condition);
                    }

                    m_analyze_scope(statement.get_while_statements(), &_scope);

                    break;
                }

                case shift_statement::statement_type::for_: {
                    {
                        utils::ideque<shift_statement> temp_initalizer(std::make_move_iterator(&statement.get_for_initializer()),
                            std::make_move_iterator(&statement.get_for_initializer() + 1));
                        m_analyze_scope(temp_initalizer, &_scope);
                        statement.set_for_initializer(std::move(temp_initalizer.front()));
                        switch (statement.get_for_initializer().type) {
                            case shift_statement::statement_type::variable_alloc:
                            case shift_statement::statement_type::expression:
                                break;
                            default: {
                                this->m_token_error(*_scope.get_parser(), *statement.get_for(),
                                    "unexpected statement in for loop initializer");
                                break;
                            }
                        }
                    }
                    {
                        shift_expression& for_condition = statement.get_for_condition();
                        if (for_condition.type == token::type::NULL_TOKEN) {
                            // set to true
                            for_condition.resolved.type.name.clazz = m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS];
                            for_condition.resolved.type.ref_type = shift_type::reference_type::tref;
                            for_condition.resolved.type.dimensions.clear();
                            for_condition.resolved.type.tried_resolve = true;
                            for_condition.begin = m_true_token_begin;
                            for_condition.end = m_true_token_end;
                            for_condition.type = token::type::IDENTIFIER;
                        } else {
                            m_resolve_expression(&for_condition, &_scope);
                            if (for_condition.is_type_resolved()) {
                                shift_type bool_type;
                                bool_type.name.clazz = m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS];
                                bool_type.ref_type = shift_type::reference_type::ref;
                                bool_type.mods = shift_mods::IMUT;

                                if (for_condition.resolved.type.is_conversion_needed(bool_type)) {
                                    const auto& conversions = m_get_implicit_conversions(for_condition.resolved.type,
                                        bool_type);
                                    if (conversions.size() == 1) {
                                        m_apply_implicit_conversion(for_condition, conversions.front(), _scope);
                                    } else if (conversions.size() > 1) {
                                        this->m_token_error(*_scope.get_parser(), *statement.get_for(),
                                            "ambiguous conversion of conditional value of type '" +
                                            for_condition.resolved.type.name.clazz->get_fqn() +
                                            "' to type '" SHIFT_ANALYZER_BOOLEAN_CLASS "'");
                                        m_error_candidates(conversions);
                                    } else if (conversions.empty()) {
                                        this->m_token_error(*_scope.get_parser(), *statement.get_for(),
                                            "cannot convert conditional value of type '" +
                                            for_condition.resolved.type.name.clazz->get_fqn() +
                                            "' into type '" SHIFT_ANALYZER_BOOLEAN_CLASS "'");
                                    }
                                }

                                m_verify_access(&_scope, &for_condition);
                            }
                        }
                    }

                    {
                        m_analyze_scope(statement.get_for_statements().begin(), statement.get_for_statements().end(), &_scope);
                    }
                    break;
                }

                case shift_statement::statement_type::break_: {
                    shift_statement* temp_parent = statement.parent;
                    for (; temp_parent; temp_parent = temp_parent->parent) {
                        if (temp_parent->type == shift_statement::statement_type::while_ ||
                            temp_parent->type == shift_statement::statement_type::for_) {
                            statement.set_break_link(temp_parent);
                            break;
                        }
                    }

                    if (temp_parent == nullptr) {
                        this->m_token_error(*_scope.get_parser(), *statement.get_break(),
                            "unexpected 'break' inside current scope");
                    }
                    break;
                }

                case shift_statement::statement_type::continue_: {
                    shift_statement* temp_parent = statement.parent;
                    for (; temp_parent; temp_parent = temp_parent->parent) {
                        if (temp_parent->type == shift_statement::statement_type::while_ ||
                            temp_parent->type == shift_statement::statement_type::for_) {
                            statement.set_continue_link(temp_parent);
                            break;
                        }
                    }

                    if (temp_parent == nullptr) {
                        this->m_token_error(*_scope.get_parser(), *statement.get_continue(),
                            "unexpected 'continue' inside current scope");
                    }
                    break;
                }

                case shift_statement::statement_type::return_: {
                    shift_expression& return_expression = statement.get_return_statement();
                    if (return_expression.type != token::type::NULL_TOKEN) {
                        m_resolve_expression(&return_expression, &_scope);

                        if (return_expression.is_type_resolved()) {
                            m_finalize_type(return_expression.resolved.type);
                            m_verify_access(&_scope, &return_expression);
                        }
                    }

                    if (_scope.func->name.begin->is_constructor() || _scope.func->name.begin->is_destructor() ||
                        _scope.func->return_type.name.name.begin->is_void()) {
                        if (return_expression.type != token::type::NULL_TOKEN) {
                            if (return_expression.resolved.type.name.clazz != &m_void_class) {
                                this->m_token_error(*_scope.get_parser(), *statement.get_return(),
                                    "unexpected return value in function with return type 'void'");
                            }
                        }
                    } else {
                        if (return_expression.type == token::type::NULL_TOKEN ||
                            !return_expression.is_type_resolved()) {
                            this->m_token_error(*_scope.get_parser(), *statement.get_return(),
                                "expected return value of type '" +
                                _scope.func->return_type.get_printable_fqn() + "'");
                        } else {
                            if (_scope.func->return_type.is_resolved() && return_expression.is_type_resolved()) {
                                if (return_expression.resolved.type.is_conversion_needed(_scope.func->return_type)) {
                                    {
                                        auto const old_return_ref_type = _scope.func->return_type.ref_type;
                                        _scope.func->return_type.ref_type = return_expression.resolved.type.ref_type;
                                        if (!return_expression.resolved.type.is_conversion_needed(_scope.func->return_type)) {
                                            _scope.func->return_type.ref_type = old_return_ref_type;
                                            switch (return_expression.resolved.type.ref_type) {
                                                case shift_type::reference_type::none:
                                                case shift_type::reference_type::ref:
                                                    if (return_expression.resolved.variable) {
                                                        if (return_expression.resolved.variable->function == _scope.func) {
                                                            if (old_return_ref_type == shift_type::reference_type::none &&
                                                                return_expression.resolved.variable->type.ref_type ==
                                                                shift_type::reference_type::none) {
                                                                // TODO TODO create implicit move expression
                                                                shift_expression return_move_expr;
                                                                return_move_expr.resolved = return_expression.resolved;
                                                                return_move_expr.resolved.type.ref_type = shift_type::reference_type::tref;
                                                                return_move_expr.set_mv_expression(std::move(return_expression));
                                                                statement.set_return_expression(std::move(return_move_expr));
                                                                goto conversions_end;
                                                            } else if (return_expression.resolved.variable->type.ref_type !=
                                                                       shift_type::reference_type::ref) {
                                                                // assume old_return_ref_type equals shift_type::reference_type::ref here
                                                                this->m_token_warning(*_scope.get_parser(), *statement.get_return(),
                                                                    "returning reference to function local variable");
                                                                goto conversions_end;
                                                            }
                                                            break;
                                                        }
                                                    }

                                                    this->m_token_error(*_scope.get_parser(), *statement.get_return(),
                                                        "cannot convert return value of type '" +
                                                        return_expression.resolved.type.get_printable_fqn() +
                                                        "' into type '" + _scope.func->return_type.get_printable_fqn() +
                                                        "'");

                                                    goto conversions_end;
                                                case shift_type::reference_type::tref:
                                                    if (old_return_ref_type == shift_type::reference_type::ref) {
                                                        this->m_token_error(*_scope.get_parser(), *statement.get_return(),
                                                            "cannot convert return value of type '" +
                                                            return_expression.resolved.type.get_printable_fqn() +
                                                            "' into type '" + _scope.func->return_type.get_printable_fqn() +
                                                            "'");
                                                        goto conversions_end;
                                                    }
                                                    break;
                                                default:
                                                    break;
                                            }
                                        }
                                        _scope.func->return_type.ref_type = old_return_ref_type;
                                    }
                                    {
                                        const auto& conversions = m_get_implicit_conversions(return_expression.resolved.type,
                                            _scope.func->return_type);
                                        if (conversions.size() == 1) {
                                            m_apply_implicit_conversion(return_expression, conversions.front(), _scope);
                                        } else if (conversions.size() > 1) {
                                            this->m_token_error(*_scope.get_parser(), *statement.get_return(),
                                                "ambiguous conversion of return value of type '" +
                                                return_expression.resolved.type.get_printable_fqn() +
                                                "' to type '" + _scope.func->return_type.get_printable_fqn() + "'");
                                            m_error_candidates(conversions);
                                        } else if (conversions.empty()) {
                                            this->m_token_error(*_scope.get_parser(), *statement.get_return(),
                                                "cannot convert return value of type '" +
                                                return_expression.resolved.type.get_printable_fqn() +
                                                "' into type '" + _scope.func->return_type.get_printable_fqn() + "'");
                                        }
                                    }
                                  conversions_end:;
                                } else {
                                    if (return_expression.resolved.variable) {
                                        if (return_expression.resolved.variable->function == _scope.func) {
                                            if (return_expression.resolved.variable->type.ref_type != shift_type::reference_type::ref) {
                                                this->m_token_warning(*_scope.get_parser(), *statement.get_return(),
                                                    "returning reference to function local variable");
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    break;
                }

                case shift_statement::statement_type::use: {
                    shift_module const& use_module = statement.get_use_module();
                    if (!contains_module(use_module)) {
                        this->m_name_error(*_scope.get_parser(), use_module.name,
                            "module '" + use_module.to_string() + "' does not exist");
                    } else if (_scope.is_using_module(use_module)) {
                        this->m_name_warning(*_scope.get_parser(), use_module.name, "redundant 'use' statement");
                    } else {
                        _scope.use_modules.emplace(statement.get_use_module());
                    }
                    break;
                }

                default:
                    // error?
                    break;
            }
        }
    }

    void analyzer::m_resolve_expression(shift_expression* expr, scope* const parent_scope, bool silent) {
        if (expr->resolved.type.tried_resolve) return;
        expr->resolved.type.tried_resolve = true;

        if (expr->is_dotted_expression()) {
            expr->update_parents(expr->parent);

            for (shift_expression* prev = nullptr; shift_expression& sub_expr : expr->get_dotted_expressions()) {
                m_resolve_expression_dotted(&sub_expr, parent_scope, prev, silent);
                sub_expr.resolved.type.tried_resolve = true;
                if (!sub_expr.is_type_resolved() && !sub_expr.is_resolved()) break;
                prev = &sub_expr;
            }

            expr->resolved = expr->get_dotted_expressions().back().resolved;
        } else if (expr->type == token::type::IDENTIFIER) {
            if (expr->size() == 1) {
                expr->resolved.type.name.name.begin = expr->begin;
                expr->resolved.type.name.name.end = expr->end;

                if (expr->begin->is_true() || expr->begin->is_false()) {
                    expr->resolved.type.name.clazz = parent_scope->base->m_classes[SHIFT_ANALYZER_BOOLEAN_CLASS];
                    expr->resolved.type.name.name_clazz = expr->resolved.type.name.clazz;
                    expr->resolved.type.ref_type = shift_type::reference_type::tref;
                    return;
                } else if (expr->begin->is_null()) {
                    expr->resolved.type.name.clazz = &m_null_class;
                    expr->resolved.type.name.name_clazz = expr->resolved.type.name.clazz;
                    expr->resolved.type.ref_type = shift_type::reference_type::tref;
                    return;
                } else {
                    m_resolve_expression_dotted(expr, parent_scope, nullptr, silent);
                }
            } else if (expr->size() > 1) {
                if (expr->begin->is_new()) {
                    // Handle new call
                    m_resolve_expression(expr->get_new_expression(), parent_scope, silent);
                    if (!expr->get_new_expression()->is_type_resolved()) return;
                    m_finalize_type(expr->get_new_expression()->resolved.type);
                    expr->resolved.type.name.clazz = m_make_pointer_class(expr->get_new_expression()->resolved.type.name.clazz, 1);
                    expr->resolved.type.ref_type = shift_type::reference_type::tref;
                } else if (expr->begin->is_cp()) {
                    // Handle new call
                    m_resolve_expression(expr->get_cp_expression(), parent_scope, silent);
                    if (!expr->get_cp_expression()->is_type_resolved()) return;
                    m_finalize_type(expr->get_cp_expression()->resolved.type);
                    expr->resolved = expr->get_cp_expression()->resolved;
                    expr->resolved.type.ref_type = shift_type::reference_type::tref;
                } else if (expr->begin->is_mv()) {
                    // Handle new call
                    m_resolve_expression(expr->get_mv_expression(), parent_scope, silent);
                    if (!expr->get_mv_expression()->is_type_resolved()) return;
                    m_finalize_type(expr->get_cp_expression()->resolved.type);
                    expr->resolved = expr->get_mv_expression()->resolved;
                    expr->resolved.type.ref_type = shift_type::reference_type::tref;
                } else {
                    // error, we some how have more than one token in an identifier expression expression and its not a 'new' call
                    if (!silent)
                        this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                            "unexpected token in identifier expression");
                    return;
                }
            } else {
                // error, we have an identifier expression with no tokens
            }
        } else if (expr->is_function_call() || expr->is_array()) {
            m_resolve_expression_dotted(expr, parent_scope, nullptr, silent);
        } else if (expr->is_bracket()) {
            const bool is_cast = expr->has_right() && expr->get_right()->type != token::type::NULL_TOKEN;

            if (is_cast) {
                if (!expr->get_left()->is_dotted_expression() && expr->get_left()->type != token::type::IDENTIFIER) {
                    shift_name left_name;
                    left_name.begin = expr->get_left()->begin;
                    left_name.end = expr->get_left()->end;
                    if (!silent)
                        this->m_name_error(*parent_scope->get_parser(), left_name, "expected type name for explicit cast");
                    m_resolve_expression(expr->get_right(), parent_scope, silent);
                    return;
                }

                if (expr->get_left()->is_dotted_expression()) {
                    for (auto& sub_expr : expr->get_left()->get_dotted_expressions()) {
                        if (sub_expr.type != token::type::IDENTIFIER) {
                            shift_name left_name;
                            left_name.begin = sub_expr.begin;
                            left_name.end = sub_expr.end;
                            if (!silent)
                                this->m_name_error(*parent_scope->get_parser(), left_name, "expected type name for explicit cast");
                            m_resolve_expression(expr->get_right(), parent_scope, silent);
                            return;
                        }
                    }
                }

                shift_name cast_class_name;
                cast_class_name.begin = expr->get_left()->begin;
                cast_class_name.end = expr->get_left()->end;

                {
                    // TODO think about allowing casting to array types
                    auto cast_classes = parent_scope->find_classes(cast_class_name);
                    if (cast_classes.size() == 1) {
                        expr->resolved.type.name.name = cast_class_name;
                        expr->resolved.type.name.clazz = cast_classes.front();
                        m_finalize_type(expr->resolved.type);
                        expr->get_left()->resolved.type = expr->resolved.type;
                    } else if (cast_classes.size() > 1) {
                        if (!silent)
                            this->m_name_error(*parent_scope->get_parser(), cast_class_name,
                                "ambiguous reference to class '" + cast_class_name.to_string() +
                                "' in current scope");
                    } else {
                        if (!silent)
                            this->m_name_error(*parent_scope->get_parser(), cast_class_name,
                                "unable to resolve class '" + cast_class_name.to_string() +
                                "' in current scope");
                    }
                }

                m_resolve_expression(expr->get_right(), parent_scope, silent);

                if (expr->get_left()->is_type_resolved() && expr->get_right()->is_type_resolved()) {
                    m_finalize_type(expr->get_right()->resolved.type);

                    if (expr->get_left()->resolved.type.name.clazz != expr->get_right()->resolved.type.name.clazz
                        && !expr->get_left()->resolved.type.name.clazz->has_base(expr->get_right()->resolved.type.name.clazz)) {
                        if (expr->get_right()->resolved.type.name.clazz->has_base(expr->get_left()->resolved.type.name.clazz)) {
                            // only warn for redundancy if casting up to base class
                            this->m_name_warning(*parent_scope->get_parser(), cast_class_name,
                                "redundant explicit cast to '" + expr->resolved.type.get_printable_fqn() + "'");
                        } else {
                            this->m_name_error(*parent_scope->get_parser(), cast_class_name,
                                "cannot explicitly cast from type '" + expr->get_right()->resolved.type.get_printable_fqn() +
                                "' to type '" + expr->get_left()->resolved.type.get_printable_fqn() + "'");
                        }
                    } else if (expr->get_left()->resolved.type == expr->get_right()->resolved.type) {
                        this->m_name_warning(*parent_scope->get_parser(), cast_class_name,
                            "redundant explicit cast to '" + expr->resolved.type.get_printable_fqn() + "'");
                    } else if ((expr->get_left()->resolved.type.mods & shift_mods::IMUT) == 0 &&
                               (expr->get_right()->resolved.type.mods & shift_mods::IMUT)) {
                        this->m_name_error(*parent_scope->get_parser(), cast_class_name,
                            "cannot cast from immutable type '" + expr->get_right()->resolved.type.get_printable_fqn() +
                            "' to mutable type '" + expr->get_left()->resolved.type.get_printable_fqn() + "'");
                    } else if (expr->get_left()->resolved.type.ref_type != expr->get_right()->resolved.type.ref_type) {
                        switch (expr->get_left()->resolved.type.ref_type) {
                            case shift_type::reference_type::none:
                                expr->resolved.type.ref_type = expr->get_right()->resolved.type.ref_type;
                                break;
                            case shift_type::reference_type::ref:
                                if (expr->get_right()->resolved.type.ref_type == shift_type::reference_type::tref) {
                                    this->m_name_error(*parent_scope->get_parser(), cast_class_name,
                                        "cast from type '" + expr->get_right()->resolved.type.get_printable_fqn() +
                                        "' to type '" +
                                        expr->get_left()->resolved.type.get_printable_fqn() +
                                        "' would change reference qualifiers");
                                }
                                break;
                            default:
                                break;
                        }
                    }
                }
            } else {
                m_resolve_expression(expr->get_left(), parent_scope, silent);
                expr->resolved = expr->get_left()->resolved;
            }
        } else if (is_overload_operator(expr->type)) {
            // Look for the operator function between the two types
            // const bool is_prefix = (is_strictly_prefix_operator(expr->type) && !is_binary_operator(expr->type)) || (is_prefix_operator(expr->type) && expr->get_left()->type == token::type::NULL_TOKEN);
            // const bool is_suffix = (is_strictly_suffix_operator(expr->type) && !is_binary_operator(expr->type)) || (is_suffix_operator(expr->type) && expr->get_right()->type == token::type::NULL_TOKEN);

            const bool has_left = expr->has_left() && expr->get_left()->type != token::type::NULL_TOKEN;
            const bool has_right = expr->has_right() && expr->get_right()->type != token::type::NULL_TOKEN;

            if (has_left) {
                m_resolve_expression(expr->get_left(), parent_scope);
                if (expr->get_left()->is_type_resolved()) {
                    m_finalize_type(expr->get_left()->resolved.type);
                }
            }

            if (has_right) {
                m_resolve_expression(expr->get_right(), parent_scope);
                if (expr->get_right()->is_type_resolved()) {
                    m_finalize_type(expr->get_right()->resolved.type);
                }
            }

            shift_class* function_search_class;

            if (has_left) {
                function_search_class = expr->get_left()->resolved.type.name.clazz;
            } else if (has_right) {
                function_search_class = expr->get_right()->resolved.type.name.clazz;
            } else {
                // should never reached, ast should be built correctly
                return;
            }

            if (!function_search_class) {
                // Neither typed were resolved, error must have already been printed so we can just exit
                return;
            }

            std::string function_fqn = function_search_class->get_fqn() + ".operator" + std::string(expr->begin->get_data());
            auto overloads = m_function_overloads.find(function_fqn);
            if (overloads != m_function_overloads.end()) {
                std::vector<function_filter_info> filtered;
                if (has_right) {
                    if (has_left) {
                        // binary function
                        filtered = m_filter_functions(utils::range(overloads->second),
                            utils::range(expr->get_right(), expr->get_right() + 1),
                            silent);
                    } else {
                        // unary (prefix) function
                        filtered = m_filter_functions(utils::range(overloads->second),
                            utils::range(expr->get_right(), expr->get_right() + 0),
                            silent);
                    }
                } else if (has_left) {
                    // unary (postfix) function
                    filtered = m_filter_functions(utils::range(overloads->second), utils::range(expr->get_left(), expr->get_left() + 0),
                        silent);
                }

                if (filtered.size() == 1) {
                    expr->resolved.function = filtered.front().candidate.first->func;
                    m_analyze_function_return_type(*expr->resolved.function, silent);
                    m_finalize_type(expr->resolved.function->return_type);
                    expr->resolved.type = expr->resolved.function->return_type;
                    if (expr->resolved.function->return_type.ref_type == shift_type::reference_type::none) {
                        expr->resolved.type.ref_type = shift_type::reference_type::tref;
                    }
                    if (has_left && has_right) {
                        auto& param_conversions = filtered.front().candidate.second;
                        auto* conversion_functions = param_conversions.at(expr->get_right());
                        if (conversion_functions) {
                            if (conversion_functions->size() == 1) {
                                m_apply_implicit_conversion(*expr->get_right(), conversion_functions->front(), *parent_scope, silent);
                            } else if (conversion_functions->size() > 1) {
                                this->m_token_error(*parent_scope->get_parser(), *expr->begin,
                                    "ambiguous conversion of operator arguments for function '" + function_fqn + "'");
                                m_error_candidates(*conversion_functions);
                            } else {
                                this->m_token_error(*parent_scope->get_parser(), *expr->begin,
                                    "unable to convert operator arguments for function '" + function_fqn + "'");
                            }
                        }
                    }
                } else if (filtered.size() > 1) {
                    if (!silent) {
                        std::string error_message = "ambiguous reference to function '" + function_fqn + "(";
                        if (has_left && has_right) {
                            error_message += expr->get_right()->resolved.type.get_printable_fqn();
                        }
                        error_message += ")' in current scope";
                        this->m_token_error(*parent_scope->get_parser(), *expr->begin,
                            std::move(error_message));
                        m_error_candidates(filtered);
                    }
                } else {
                    if (!silent) {
                        std::string error_message = "unable to resolve reference to function '" + function_fqn + "(";
                        if (has_left && has_right) {
                            error_message += expr->get_right()->resolved.type.get_printable_fqn();
                        }
                        error_message += ")' in current scope";

                        this->m_token_error(*parent_scope->get_parser(), *expr->begin,
                            std::move(error_message));
                    }
                }
            } else {
                if (!silent) {
                    std::string error_message = "unable to resolve reference to function '" + function_fqn + "(";
                    if (has_left && has_right) {
                        error_message += expr->get_right()->resolved.type.get_printable_fqn();
                    }
                    error_message += ")' in current scope";

                    this->m_token_error(*parent_scope->get_parser(), *expr->begin,
                        std::move(error_message));
                }
            }
        } else if (expr->type == token::type::COMMA) {
            for (shift_expression& sub_expr : expr->get_comma_expressions()) {
                m_resolve_expression(&sub_expr, parent_scope, silent);
            }
            expr->resolved = expr->get_comma_expressions().back().resolved;
        } else {
            switch (expr->type) {
                case token_type::STRING_LITERAL:
                    expr->resolved.type.name.clazz = m_classes[SHIFT_ANALYZER_STRING_CLASS];
                    expr->resolved.type.name.name_clazz = expr->resolved.type.name.clazz;
                    expr->resolved.type.ref_type = shift_type::reference_type::tref;
                    break;
                case token_type::CHAR_LITERAL:
                    expr->resolved.type.name.clazz = m_classes[SHIFT_ANALYZER_CHAR_CLASS];
                    expr->resolved.type.name.name_clazz = expr->resolved.type.name.clazz;
                    expr->resolved.type.ref_type = shift_type::reference_type::tref;
                    break;
                case token_type::INTEGER_LITERAL:
                case token_type::BINARY_LITERAL:
                case token_type::HEX_LITERAL:
                    expr->resolved.type.name.clazz = m_classes[SHIFT_ANALYZER_INT_CLASS];
                    expr->resolved.type.name.name_clazz = expr->resolved.type.name.clazz;
                    expr->resolved.type.ref_type = shift_type::reference_type::tref;
                    break;
                case token_type::FLOAT_LITERAL:
                    expr->resolved.type.name.clazz = m_classes[SHIFT_ANALYZER_FLOAT_CLASS];
                    expr->resolved.type.name.name_clazz = expr->resolved.type.name.clazz;
                    expr->resolved.type.ref_type = shift_type::reference_type::tref;
                    break;
                case token_type::DOUBLE_LITERAL:
                    expr->resolved.type.name.clazz = m_classes[SHIFT_ANALYZER_DOUBLE_CLASS];
                    expr->resolved.type.name.name_clazz = expr->resolved.type.name.clazz;
                    expr->resolved.type.ref_type = shift_type::reference_type::tref;
                    break;
                default:
                    this->m_name_error(*parent_scope->get_parser(), expr->to_name(),
                        "unable to resolve type of expression");
                    break;
            }
        }
    }

    void analyzer::m_resolve_expression_dotted(shift_expression* expr, scope* const parent_scope, shift_expression* prev, bool silent) {
        const bool has_prev_module = prev && prev->resolved.module_, has_prev_class = prev && prev->resolved.clazz,
            has_prev_variable = prev && prev->resolved.variable, has_prev_func = prev && prev->resolved.function,
            has_prev_resolved_type = prev && prev->is_type_resolved();

        if (expr->is_function_call()) {
            bool all_param_resolved = true;
            for (auto& param_expr : expr->get_function_call_arguments()) {
                m_resolve_expression(&param_expr, parent_scope, silent);
                all_param_resolved &= param_expr.is_type_resolved();
            }

            if (has_prev_module || has_prev_class || has_prev_variable || has_prev_func || has_prev_resolved_type) {
                // TODO when adding function pointers, expr->get_function_call_object() might be a bracket expression
                const std::string iden = expr->get_function_call_object()->to_string();

                std::string function_fqn;

                if (has_prev_module)
                    function_fqn = prev->resolved.module_->to_string() + "." + iden;
                else if (has_prev_class)
                    function_fqn = prev->resolved.clazz->get_fqn() + "." + iden;
                else if (has_prev_variable)
                    function_fqn = prev->resolved.variable->type.name.clazz->get_fqn() + "." + iden;
                else if (has_prev_func)
                    function_fqn = prev->resolved.function->return_type.name.clazz->get_fqn() + "." + iden;
                else if (has_prev_resolved_type)
                    function_fqn = prev->resolved.type.name.clazz->get_fqn() + "." + iden;

                {
                    auto it = m_function_overloads.find(function_fqn);

                    if (it == m_function_overloads.end()) {
                        auto clazz_it = m_classes.find(function_fqn);
                        if (clazz_it != m_classes.end()) {
                            expr->resolved.type.name.name.begin = expr->begin;
                            expr->resolved.type.name.name.end = expr->end;
                            expr->resolved.type.name.clazz = clazz_it->second;
                            expr->resolved.type.ref_type = shift_type::reference_type::tref;

                            it = m_function_overloads.find(function_fqn = (clazz_it->second->get_fqn() + ".constructor"));
                            if (it == m_function_overloads.end()) {
                                if (!silent)
                                    this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                                        "unable to resolve constructor for class '" + clazz_it->second->get_fqn() +
                                        "' in current scope");
                                return;
                            }
                        }
                    }

                    if (it != m_function_overloads.end()) {
                        auto& overloads = it->second;
                        auto filtered = m_filter_functions(utils::range(overloads), expr->get_function_call_arguments(), silent);
                        if (filtered.size() > 1) {
                            if (!silent) {
                                this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                                    "ambiguous reference to function '" + function_fqn +
                                    "' in current scope with given arguments");
                                m_error_candidates(filtered);
                            }
                            // TODO list arguments and cadidates
                            return;
                        } else if (filtered.size() == 1) {
                            expr->resolved.function = filtered.front().candidate.first->func;
                            m_analyze_function_return_type(*expr->resolved.function, silent);
                            if (expr->resolved.function->name.begin->is_constructor()) {
                                expr->resolved.type.name.name = expr->to_name();
                                expr->resolved.type.name.clazz = expr->resolved.function->clazz;
                            } else {
                                expr->resolved.type = expr->resolved.function->return_type;
                            }
                            if (expr->resolved.function->return_type.ref_type == shift_type::reference_type::none) {
                                expr->resolved.type.ref_type = shift_type::reference_type::tref;
                            }
                            auto& param_conversions = filtered.front().candidate.second;
                            for (auto& param_expr : expr->get_function_call_arguments()) {
                                if (param_expr.is_type_resolved()) {
                                    auto* conversion_functions = param_conversions.at(&param_expr);
                                    if (conversion_functions) {
                                        if (conversion_functions->size() == 1) {
                                            if (!silent)
                                                m_apply_implicit_conversion(param_expr, conversion_functions->front(), *parent_scope,
                                                    silent);
                                        } else if (conversion_functions->size() > 1) {
                                            if (!silent) {
                                                this->m_name_error(*parent_scope->get_parser(),
                                                    shift_name{ param_expr.begin, param_expr.end },
                                                    "ambiguous conversion of function arguments for function '" +
                                                    function_fqn + "'");
                                                m_error_candidates(*conversion_functions);
                                            }
                                        } else {
                                            this->m_name_error(*parent_scope->get_parser(), shift_name{ param_expr.begin, param_expr.end },
                                                "unable to convert function arguments for function '" + function_fqn + "'");
                                        }
                                    }
                                }
                            }
                            return;
                        } else {
                            if (!silent) {
                                std::string func_signature = function_fqn;
                                func_signature += '(';
                                for (bool past_first = false; auto& p_expr : expr->get_function_call_arguments()) {
                                    if (past_first) {
                                        func_signature += ", ";
                                    }
                                    if (p_expr.is_type_resolved()) {
                                        func_signature += p_expr.resolved.type.get_printable_fqn();
                                    } else {
                                        func_signature += "<unknown>";
                                    }
                                    past_first = true;
                                }
                                func_signature += ')';
                                this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                                    "unable to resolve function '" + func_signature + "' in current scope");
                            }
                            return;
                        }
                    } else {
                        if (!silent) {
                            this->m_name_error(*parent_scope->get_parser(),
                                shift_name{ expr->get_function_call_object()->begin, expr->get_function_call_object()->end },
                                "unable to resolve function '" + function_fqn + "' in current scope");
                        }
                        return;
                    }
                }
            }

            if (!prev) {
                // TODO iden may not actually be an identifier here if we allow function pointer (but it must be one in the above if statement)
                if (expr->get_function_call_object()->type == token::type::IDENTIFIER &&
                    expr->get_function_call_object()->size() == 1) {
                    const std::string iden = expr->get_function_call_object()->to_string();
                    std::string search_fqn = iden;
                    auto funcs = parent_scope->find_functions(search_fqn);

                    if (funcs.empty()) {
                        auto classes = parent_scope->find_classes(search_fqn);
                        if (classes.size() > 1) {
                            if (!silent)
                                this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                                    "ambiguous reference to class '" + search_fqn + "' in current scope");
                            return;
                        } else if (classes.size() == 1) {
                            expr->resolved.type.name.name = expr->to_name();
                            expr->resolved.type.name.clazz = classes.front();
                            expr->resolved.type.ref_type = shift_type::reference_type::tref;

                            auto overloads_it = m_function_overloads.find(search_fqn = (classes.front()->get_fqn() + ".constructor"));
                            if (overloads_it != m_function_overloads.end()) {
                                funcs.push_back(&overloads_it->second);
                            }

                            if (funcs.empty()) {
                                if (!silent)
                                    this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                                        "unable to resolve constructor for class '" + classes.front()->get_fqn() +
                                        "' in current scope");
                                return;
                            }
                        } else {
                            if (!silent)
                                this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                                    "unable to resolve function '" + search_fqn + "' in current scope");
                            return;
                        }
                    }

                    auto filtered = m_filter_functions(utils::range(funcs), expr->get_function_call_arguments(), silent);

                    if (filtered.size() > 1 && all_param_resolved) {
                        if (!silent) {
                            this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                                "ambiguous reference to function '" + search_fqn +
                                "' in current scope with given arguments");
                            m_error_candidates(filtered);
                        }
                        return;
                    } else if (filtered.size() == 1) {
                        expr->resolved.function = filtered.front().candidate.first->func;

                        m_analyze_function_return_type(*expr->resolved.function, silent);

                        if (expr->resolved.function->name.begin->is_constructor()) {
                            expr->resolved.type.name.name = expr->to_name();
                            expr->resolved.type.name.clazz = expr->resolved.function->clazz;
                        } else {
                            expr->resolved.type = expr->resolved.function->return_type;
                        }

                        if (expr->resolved.function->return_type.ref_type == shift_type::reference_type::none) {
                            expr->resolved.type.ref_type = shift_type::reference_type::tref;
                        }

                        auto& param_conversions = filtered.front().candidate.second;
                        for (auto& param_expr : expr->get_function_call_arguments()) {
                            if (param_expr.is_type_resolved()) {
                                auto* conversion_functions = param_conversions.at(&param_expr);
                                if (conversion_functions) {
                                    if (conversion_functions->size() == 1) {
                                        m_apply_implicit_conversion(param_expr, conversion_functions->front(), *parent_scope, silent);
                                    } else if (conversion_functions->size() > 1) {
                                        if (!silent) {
                                            this->m_name_error(*parent_scope->get_parser(), shift_name{ param_expr.begin, param_expr.end },
                                                "ambiguous conversion of function arguments for function '" +
                                                expr->resolved.function->get_signature() + "'");
                                            m_error_candidates(*conversion_functions);
                                        }
                                    } else {
                                        if (!silent)
                                            this->m_name_error(*parent_scope->get_parser(), shift_name{ param_expr.begin, param_expr.end },
                                                "unable to convert function arguments for function '" +
                                                expr->resolved.function->get_signature() + "'");
                                    }
                                }
                            }
                        }
                        return;
                    } else {
                        if (!silent) {
                            std::string func_signature = search_fqn;
                            func_signature += '(';
                            for (bool past_first = false; auto& p_expr : expr->get_function_call_arguments()) {
                                if (past_first) {
                                    func_signature += ", ";
                                }
                                if (p_expr.is_type_resolved()) {
                                    func_signature += p_expr.resolved.type.get_printable_fqn();
                                } else {
                                    func_signature += "<unknown>";
                                }
                                past_first = true;
                            }
                            func_signature += ')';
                            this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                                "unable to resolve function '" + func_signature + "' in current scope");
                        }
                        return;
                    }
                } else {
                    m_resolve_expression(expr->get_function_call_object(), parent_scope, silent);
                    if (expr->get_function_call_object()->is_type_resolved()) {
                        return m_resolve_expression_dotted(expr, parent_scope, expr->get_function_call_object());
                    }
                }
            }
            if (!silent) {
                const std::string iden = expr->get_function_call_object()->to_string();
                shift_name error_name;
                error_name.begin = expr->get_function_call_object()->begin;
                error_name.end = error_name.begin;
                ++error_name.end;
                if (has_prev_resolved_type) {
                    this->m_name_error(*parent_scope->get_parser(), error_name,
                        "unable to resolve function '" + (prev->resolved.type.name.clazz->get_fqn() + "." + iden) +
                        "' in current scope");
                } else {
                    this->m_name_error(*parent_scope->get_parser(), error_name,
                        "unable to resolve function '" + iden + "' in current scope");
                }
            }
        } else if (expr->is_array()) {
            const std::string iden = "operator[]";

            m_resolve_expression(expr->get_array_object(), parent_scope, silent);
            if (!expr->get_array_object()->is_type_resolved()) return;

            std::string next_fqn_prefix = expr->get_array_object()->resolved.type.name.clazz->get_fqn();

            for (auto& dim : expr->get_array_dimensions()) {
                m_resolve_expression(dim.get_array_indexer_expression(), parent_scope, silent);

                if (!dim.get_array_indexer_expression()->is_type_resolved()) return;

                std::string function_fqn = next_fqn_prefix + '.' + iden;

                {
                    auto it = m_function_overloads.find(function_fqn);

                    if (it != m_function_overloads.end()) {
                        auto& overloads = it->second;
                        auto filtered = m_filter_functions(utils::range(overloads),
                            utils::range(dim.get_array_indexer_expression(),
                                dim.get_array_indexer_expression() + 1),
                            silent);
                        if (filtered.size() > 1) {
                            if (!silent) {
                                this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                                    "ambiguous reference to function '" + function_fqn +
                                    "' in current scope with given arguments");
                                m_error_candidates(filtered);
                            }
                            // TODO list arguments and cadidates
                            return;
                        } else if (filtered.size() == 1) {
                            dim.resolved.function = filtered.front().candidate.first->func;
                            m_analyze_function_return_type(*dim.resolved.function, silent);
                            dim.resolved.type = dim.resolved.function->return_type;
                            if (dim.resolved.function->return_type.ref_type == shift_type::reference_type::none) {
                                dim.resolved.type.ref_type = shift_type::reference_type::tref;
                            }

                            auto& param_conversions = filtered.front().candidate.second;
                            auto& param_expr = *dim.get_array_indexer_expression();
                            if (param_expr.is_type_resolved()) {
                                auto* conversion_functions = param_conversions.at(&param_expr);
                                if (conversion_functions) {
                                    if (conversion_functions->size() == 1) {
                                        m_apply_implicit_conversion(param_expr, conversion_functions->front(), *parent_scope, silent);
                                    } else if (conversion_functions->size() > 1) {
                                        if (!silent) {
                                            this->m_name_error(*parent_scope->get_parser(), shift_name{ param_expr.begin, param_expr.end },
                                                "ambiguous conversion of operator arguments for function '" +
                                                dim.resolved.function->get_signature() + "'");
                                            m_error_candidates(*conversion_functions);
                                        }
                                    } else {
                                        if (!silent)
                                            this->m_name_error(*parent_scope->get_parser(), shift_name{ param_expr.begin, param_expr.end },
                                                "unable to convert operator arguments for function '" +
                                                dim.resolved.function->get_signature() + "'");
                                    }
                                }
                            }
                            return;
                        } else {
                            if (!silent) {
                                std::string func_signature = function_fqn;
                                func_signature += '(';
                                {
                                    auto& p_expr = *dim.get_array_indexer_expression();

                                    if (p_expr.is_type_resolved()) {
                                        func_signature += p_expr.resolved.type.get_printable_fqn();
                                    } else {
                                        func_signature += "<unknown>";
                                    }
                                }
                                func_signature += ')';
                                this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                                    "unable to resolve function '" + func_signature + "' in current scope");
                            }
                            return;
                        }
                    } else {
                        if (!silent) {
                            this->m_name_error(*parent_scope->get_parser(),
                                shift_name{ expr->get_function_call_object()->begin, expr->get_function_call_object()->end },
                                "unable to resolve function '" + function_fqn + "' in current scope");
                        }
                    }
                }

                if (dim.resolved.type.is_resolved()) {
                    next_fqn_prefix = dim.resolved.type.name.clazz->get_fqn();
                } else {
                    break;
                }
            }

            expr->resolved = expr->get_array_dimensions().back().resolved;
            if (expr->get_array_dimensions().back().resolved.function->return_type.ref_type == shift_type::reference_type::none) {
                expr->resolved.type.ref_type = shift_type::reference_type::tref;
            }
        } else if (expr->type == token::type::IDENTIFIER && expr->size() == 1) {
            const std::string iden = expr->to_string();

            if (has_prev_module || !prev) {
                std::string search_fqn = iden;
                if (has_prev_module) { search_fqn = prev->resolved.module_->to_string() + "." + iden; }
                {
                    auto it = m_modules.find(search_fqn);
                    if (it != m_modules.end()) {
                        expr->resolved.module_ = it->second;
                        return;
                    }
                }
            }

            if (has_prev_module || has_prev_class) {
                std::string search_fqn = iden;
                if (has_prev_module) { search_fqn = prev->resolved.module_->to_string() + "." + iden; }
                else if (has_prev_class) {
                    search_fqn = prev->resolved.clazz->get_fqn() + "." + iden;
                }
                {
                    auto it = m_classes.find(search_fqn);
                    if (it != m_classes.end()) {
                        expr->resolved.clazz = it->second;
                        return;
                    }
                }

                {
                    auto it = m_variables.find(search_fqn);
                    if (it != m_variables.end()) {
                        expr->resolved.variable = it->second;
                        if (!expr->resolved.variable->type.is_resolved() && !expr->resolved.variable->type.tried_resolve) {
                            m_analyze_variable_type(*expr->resolved.variable, nullptr, silent);
                        }
                        expr->resolved.type = expr->resolved.variable->type;
                        expr->resolved.type.ref_type = shift_type::reference_type::ref;
                        return;
                    }
                }
            }

            if (has_prev_variable || has_prev_resolved_type) {
                if (expr->resolved.type.tried_resolve) { return; }
                expr->resolved.type.tried_resolve = true;

                shift_type* prev_type = nullptr;

                if (has_prev_variable) {
                    if (!prev->resolved.variable->type.tried_resolve) {
                        prev->resolved.variable->type.tried_resolve = true;
                        m_analyze_variable_type(*prev->resolved.variable, nullptr, silent);
                    }
                    prev_type = &prev->resolved.variable->type;
                }

                if (has_prev_resolved_type || !prev_type) {
                    prev_type = &prev->resolved.type;
                }

                if (prev_type->is_resolved()) {
                    std::string search_fqn = prev_type->name.clazz->get_fqn() + "." + iden;
                    auto it = m_variables.find(search_fqn);
                    if (it != m_variables.end()) {
                        expr->resolved.variable = it->second;
                        if (!expr->resolved.variable->type.is_resolved() && !expr->resolved.variable->type.tried_resolve) {
                            m_analyze_variable_type(*expr->resolved.variable, nullptr, silent);
                        }
                        expr->resolved.type = expr->resolved.variable->type;
                        expr->resolved.type.ref_type = shift_type::reference_type::ref;
                        return;
                    }
                } else {
                    if (!silent)
                        this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                            "unable to resolve variable '" + iden + "' in class '<unknown>'");
                    return;
                }
                if (!silent)
                    this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                        "unable to resolve variable '" + iden + "' inside class '" + prev_type->get_fqn() + "'");
                return;
            }

            if (!prev) {
                std::string search_fqn = iden;

                {
                    auto module_it = m_modules.find(search_fqn);
                    if (module_it != m_modules.end()) {
                        expr->resolved.module_ = module_it->second;
                        return;
                    }
                }


                auto variables = parent_scope->find_variables(search_fqn);
                if (variables.size() == 1) {
                    expr->resolved.variable = variables.front();
                    if (!expr->resolved.variable->type.is_resolved() && !expr->resolved.variable->type.tried_resolve) {
                        m_analyze_variable_type(*expr->resolved.variable, nullptr, silent);
                    }
                    expr->resolved.type = expr->resolved.variable->type;
                    expr->resolved.type.ref_type = shift_type::reference_type::ref;
                    return;
                } else if (variables.size() > 1) {
                    if (!silent)
                        this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                            "ambiguous reference to variable '" + iden + "' in current scope");
                    return;
                }
            }
            if (!silent)
                this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                    "unable to find module, class, or variable '" + iden + "' in current scope");
        } else {
            // error, something unexpected in dotted expression
            if (!silent)
                this->m_name_error(*parent_scope->get_parser(), shift_name{ expr->begin, expr->end },
                    "unexpected expression in dotted expression");
        }
    }

    shift_class* analyzer::m_make_array_class(shift_class* const clazz, const size_t dimensions) {
        // TODO make array return type
        if (dimensions == 0) { return clazz; }

        std::string array_class_fqn = "shift.array@" + clazz->get_fqn() + "@" + std::to_string(dimensions);

        {
            auto f = m_classes.find(array_class_fqn);
            if (f != m_classes.end()) return f->second;
        }

        const auto& [it, inserted_] = m_classes.insert_or_assign(std::move(array_class_fqn), &m_extra_classes.emplace_back());
        const auto& [fqn, clazz_] = *it;

        {
            shift_type temp_type;
            temp_type.name.name_clazz = clazz;
            temp_type.add_array_dimensions(dimensions);
            m_classes[temp_type.get_printable_fqn()] = clazz_;
        }

        shift_class& array_class = *clazz_;

        {
            const size_t offset = std::strlen("shift.");
            array_class.name = &this->m_extra_tokens.emplace_back(
                std::string_view(fqn.data() + offset,
                    fqn.length() - offset),
                token::type::IDENTIFIER, file_indexer{ 0, 0 }
            );
        }

        array_class.module_ = &m_shift_module;
        array_class.mods = shift_mods::PUBLIC;

        {
            shift_variable& length_var = array_class.variables.emplace_back();
            length_var.name = &*m_length_token;
            length_var.clazz = &array_class;
            length_var.type.name.clazz = m_classes[SHIFT_ANALYZER_ULONG_CLASS];
            length_var.type.name.name_clazz = length_var.type.name.clazz;
            length_var.type.mods = shift_mods::PUBLIC | shift_mods::IMUT;
        }

        {
            shift_function& bracket_function = array_class.functions.emplace_back();
            bracket_function.name.begin = m_operator_array_function_token_begin;
            bracket_function.name.end = m_operator_array_function_token_end;
            bracket_function.mods = shift_mods::PUBLIC;
            bracket_function.return_type.ref_type = shift_type::reference_type::ref;
            bracket_function.return_type.name.clazz = m_make_array_class(clazz, dimensions - 1);
            bracket_function.return_type.name.name_clazz = m_classes[clazz->get_fqn()];
            bracket_function.clazz = &array_class;

            {
                shift_variable bracket_function_param;
                bracket_function_param.type.name.clazz = m_classes[SHIFT_ANALYZER_ULONG_CLASS];
                bracket_function_param.type.name.name_clazz = bracket_function_param.type.name.clazz;
                bracket_function_param.function = &bracket_function;

                bracket_function.parameters.push_back({ "@0", std::move(bracket_function_param) });
            }
        }

        return &array_class;
    }

    shift_class* analyzer::m_make_pointer_class(shift_class* const clazz, const size_t dimensions) {
        // TODO make array return type
        if (dimensions == 0) { return clazz; }

        std::string pointer_class_fqn = "shift.pointer@" + clazz->get_fqn() + "@" + std::to_string(dimensions);

        {
            auto f = m_classes.find(pointer_class_fqn);
            if (f != m_classes.end()) return f->second;
        }

        const auto& [it, inserted_] = m_classes.insert_or_assign(std::move(pointer_class_fqn), &m_extra_classes.emplace_back());
        const auto& [fqn, clazz_] = *it;

        {
            shift_type temp_type;
            temp_type.name.name_clazz = clazz;
            temp_type.add_pointer_dimensions(dimensions);
            m_classes[temp_type.get_printable_fqn()] = clazz_;
        }

        shift_class& pointer_class = *clazz_;
        { // This might want to be changed eventually: we want to consider that unordered_map may copy when resizing bucket count
            const size_t offset = std::strlen("shift.");
            pointer_class.name = &this->m_extra_tokens.emplace_back(
                std::string_view(fqn.data() + offset,
                    fqn.length() - offset),
                token::type::IDENTIFIER, file_indexer{ 0, 0 }
            );
        }

        pointer_class.module_ = &m_shift_module;
        pointer_class.mods = shift_mods::PUBLIC;
        {
            shift_type return_type;
            return_type.name.clazz = m_make_pointer_class(clazz, dimensions - 1);
            return_type.name.name_clazz = m_classes[clazz->get_fqn()];

            {
                shift_function& star_function = pointer_class.functions.emplace_back();
                star_function.name.begin = m_operator_star_function_token_begin;
                star_function.name.end = m_operator_star_function_token_end;
                star_function.mods = shift_mods::PUBLIC;
                star_function.return_type = return_type;
                star_function.return_type.ref_type = shift_type::reference_type::ref;
                star_function.clazz = &pointer_class;
            }

            {
                shift_function& arrow_function = pointer_class.functions.emplace_back();
                arrow_function.name.begin = m_operator_arrow_function_token_begin;
                arrow_function.name.end = m_operator_arrow_function_token_end;
                arrow_function.mods = shift_mods::PUBLIC;
                arrow_function.return_type = return_type;
                arrow_function.clazz = &pointer_class;
            }
        }

        return &pointer_class;
    }

    shift_type& analyzer::m_finalize_type(shift_type& type) {
        if (type.name.clazz && type.name.name_clazz && type.name.clazz != type.name.name_clazz) { return type; }

        shift_class* final_class = type.name.clazz ? type.name.clazz : type.name.name_clazz;

        {
            using namespace std::string_view_literals;
            const std::string pre_fqn = final_class->get_fqn();
            if (utils::starts_with((std::string_view) pre_fqn, "shift.pointer"sv)
                || utils::starts_with((std::string_view) pre_fqn, "shift.array"sv)) {
                return type;
            }
        }

        if (!type.name.name_clazz) {
            type.name.name_clazz = final_class;
        }

        for (const auto& dim : type.dimensions) {
            switch (dim.type) {
                case shift_type::dimension::dimension_type::array:
                    final_class = m_make_array_class(final_class, dim.count);
                    break;
                case shift_type::dimension::dimension_type::pointer:
                    final_class = m_make_pointer_class(final_class, dim.count);
                    break;
                default:
                    break;
            }
        }

        type.name.clazz = final_class;
        return type;

    }

    void analyzer::m_token_error(const parser& parser_, const token& token_, const std::string_view msg) {
        if (!this->m_error_handler) return;
        SHIFT_ANALYZER_ERROR_(parser_, token_, msg);
        std::string line(this->m_get_line(parser_, token_));
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

    void analyzer::m_token_error(const parser& parser_, const token& token_, const std::string& msg) {
        return m_token_error(parser_, token_, std::string_view(msg.c_str(), msg.length()));
    }

    void analyzer::m_token_error(const parser& parser_, const token& token_, const char* const msg) {
        return m_token_error(parser_, token_, std::string_view(msg, std::strlen(msg)));
    }

    void analyzer::m_token_warning(const parser& parser_, const token& token_, const std::string_view msg) {
        if (!this->m_error_handler) return;
        if (!this->m_error_handler->is_print_warnings()) return;
        SHIFT_ANALYZER_WARNING_(parser_, token_, msg);
        std::string line(this->m_get_line(parser_, token_));
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

    void analyzer::m_token_warning(const parser& parser_, const token& token_, const std::string& msg) {
        return m_token_warning(parser_, token_, std::string_view(msg.c_str(), msg.length()));
    }

    void analyzer::m_token_warning(const parser& parser_, const token& token_, const char* const msg) {
        return m_token_warning(parser_, token_, std::string_view(msg, std::strlen(msg)));
    }

    void analyzer::m_name_error(const parser& parser_, const shift_name& name, const std::string_view msg) {
        if (!this->m_error_handler) return;
        SHIFT_ANALYZER_ERROR_(parser_, *name.begin, msg);
        std::string line(this->m_get_line(parser_, *name.begin));
        size_t use_col = name.begin->get_file_index().col;

        for (auto cur = line.begin(); cur != line.begin() + use_col - 1; ++cur) {
            char& ch = *cur;
            if (ch == '\t') {
                ch = ' ';
                use_col -= 3;
            }
        } // TODO deal with tabs

        std::string indexer(use_col - 1, ' ');
        indexer.append((name.end - 1)->get_file_index().col + (name.end - 1)->get_data().size() -
                       name.begin->get_file_index().col, '^');
        SHIFT_ANALYZER_ERROR_LOG(line);
        SHIFT_ANALYZER_ERROR_LOG(indexer);
    }

    void analyzer::m_name_error(const parser& parser_, const shift_name& name, const std::string& msg) {
        return m_name_error(parser_, name, std::string_view(msg.c_str(), msg.length()));
    }

    void analyzer::m_name_error(const parser& parser_, const shift_name& name, const char* const msg) {
        return m_name_error(parser_, name, std::string_view(msg, std::strlen(msg)));
    }

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
        indexer.append((name.end - 1)->get_file_index().col + (name.end - 1)->get_data().size() -
                       name.begin->get_file_index().col, '^');
        SHIFT_ANALYZER_WARNING_LOG(line);
        SHIFT_ANALYZER_WARNING_LOG(indexer);
    }

    void analyzer::m_error_candidates(const std::list<type_conversion_info>& type_conversions) {
        if (!this->m_error_handler) return;
        const std::string indent = "\t";

        for (const auto& conversion : type_conversions) {
            this->m_error_handler->stream() << indent << "candidate: " << conversion.from.get_printable_fqn() << " -> "
                                            << conversion.to->get_printable_fqn() << '\n';
            this->m_error_handler->stream() << indent << indent << conversion.funcs.front()->get_signature() << '\n';
        }
        this->m_error_handler->flush_stream(error_handler::error);
    }

    void analyzer::m_error_candidates(const std::vector<function_filter_info>& function_overloads) {
        if (!this->m_error_handler) return;
        const std::string indent = "\t";

        for (const auto& filter_info : function_overloads) {
            this->m_error_handler->stream() << indent << "candidate: " << filter_info.candidate.first->func->get_signature() << '\n';
        }
        this->m_error_handler->flush_stream(error_handler::error);
    }

    void analyzer::m_name_warning(const parser& parser_, const shift_name& name, const std::string& msg) {
        return m_name_warning(parser_, name, std::string_view(msg.c_str(), msg.length()));
    }

    void analyzer::m_name_warning(const parser& parser_, const shift_name& name, const char* const msg) {
        return m_name_warning(parser_, name, std::string_view(msg, std::strlen(msg)));
    }

    void analyzer::m_error(const parser& parser_, const std::string_view msg) {
        if (!this->m_error_handler) return;
        SHIFT_ANALYZER_ERROR(parser_, msg);
    }

    void analyzer::m_error(const parser& parser_, const std::string& msg) {
        return m_error(parser_, std::string_view(msg.c_str(), msg.length()));
    }

    void analyzer::m_error(const parser& parser_, const char* const msg) {
        return m_error(parser_, std::string_view(msg, std::strlen(msg)));
    }

    void analyzer::m_warning(const parser& parser_, const std::string_view msg) {
        if (!this->m_error_handler) return;
        SHIFT_ANALYZER_WARNING(parser_, msg);
    }

    void analyzer::m_warning(const parser& parser_, const std::string& msg) {
        return m_warning(parser_, std::string_view(msg.c_str(), msg.length()));
    }

    void analyzer::m_warning(const parser& parser_, const char* const msg) {
        return m_warning(parser_, std::string_view(msg, std::strlen(msg)));
    }

    std::string_view analyzer::m_get_line(const parser& p, const token& t) const noexcept {
        return p.get_tokenizer()->get_lines()[t.get_file_index().line - 1];
    }
}