/**
 * @file compiler/shift_analyzer.h
 */
#ifndef SHIFT_ANALYZER_H_
#define SHIFT_ANALYZER_H_ 1

#include "shift_config.h"

#include "compiler/shift_parser.h"
#include "utils/utils.h"
#include "utils/range.h"
#include "utils/ordered_set.h"

#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <algorithm>
#include <deque>
#include <variant>
#include <map>

namespace shift::compiler {
    class analyzer {
    public:
        inline analyzer(error_handler* const handler, std::deque<parser>* const parsers) noexcept;

        inline analyzer(error_handler* const handler, std::deque<parser>& parsers) noexcept;

        SHIFT_API void analyze();

        inline const error_handler* get_error_handler() const noexcept { return m_error_handler; }

        inline void set_error_handler(error_handler* const handler) noexcept { m_error_handler = handler; }

        inline std::deque<parser>* get_parsers() noexcept { return m_parsers; }

        inline const std::deque<parser>* get_parsers() const noexcept { return m_parsers; }
    private:
        struct scope;
        struct function_overload_info;
        struct type_conversion_info;
        struct function_filter_info;

        struct function_overload_info {
            shift_function* func = nullptr;
        };

        struct type_conversion_info {
            shift_type from;
            const shift_type* to = nullptr;
            std::vector<shift_function*> funcs;
        };

        struct function_filter_info {
            std::pair<const function_overload_info*, std::unordered_map<const shift_expression*, const std::list<type_conversion_info>*>> candidate;
        };

        void m_init_defaults();

        std::string m_mangle_name(const shift_function& func);

        void m_set_default_value(shift_variable& var, bool silent = false) noexcept;

        template<utils::range_of<const function_overload_info> FuncsIter, utils::range_of<const shift_expression> ParamsIter>
        std::vector<function_filter_info>
            m_filter_functions(utils::range<FuncsIter> funcs, utils::range<ParamsIter> params, bool silent = false);

        template<utils::range_of<const std::vector<function_overload_info>*> FuncsIter, utils::range_of<const shift_expression> ParamsIter>
        std::vector<function_filter_info>
            m_filter_functions(utils::range<FuncsIter> funcs, utils::range<ParamsIter> params, bool silent = false);

        inline bool contains_module(const std::string& module_) const {
            return this->m_modules.find(module_) != this->m_modules.end();
        }

        inline void m_analyze_scope(std::deque<shift_statement>& statements, scope* parent_scope = nullptr) {
            return m_analyze_scope(statements.begin(), statements.end(), parent_scope);
        }

        void m_analyze_scope(typename std::deque<shift_statement>::iterator statements_begin,
            typename std::deque<shift_statement>::iterator statements_end,
            scope* parent_scope = nullptr);

        void m_analyze_function(shift_function& func, scope& parent_scope);

        void m_analyze_function_return_type(shift_function& func, bool silent = false);

        void m_analyze_function_params(shift_function& func, bool silent = false);

        void m_analyze_function_body(shift_function& func, scope* parent_scope = nullptr);

        void m_analyze_variable(shift_variable& variable, scope& parent_scope);

        void m_analyze_variable_type(shift_variable& variable, const scope* current_scope, bool silent = false);

        inline void m_analyze_variable_type(shift_variable& variable, bool silent = false) { return m_analyze_variable_type(variable, nullptr, silent); }

        void m_analyze_variable_value(shift_variable& variable, scope& current_scope);

        void m_resolve_expression(shift_expression*, scope* const parent_scope, bool silent = false);

        void m_resolve_expression_dotted(shift_expression*, scope* const parent_scope, shift_expression* prev = nullptr, bool silent = false);

        const std::list<type_conversion_info>& m_get_implicit_conversions(const shift_type* from, const shift_type* to,
            std::unordered_set<shift_class*>& history) noexcept;

        inline const std::list<type_conversion_info>&
            m_get_implicit_conversions(const shift_type* from, const shift_type* to) noexcept {
            std::unordered_set<shift_class*> history;
            return m_get_implicit_conversions(from, to, history);
        }

        inline const std::list<type_conversion_info>&
            m_get_implicit_conversions(const shift_type& from, const shift_type& to) noexcept {
            return m_get_implicit_conversions(&from, &to);
        }

        inline const std::list<type_conversion_info>&
            m_get_implicit_conversions(const shift_type& from, const shift_type&& to) noexcept = delete;

        void m_apply_implicit_conversion(shift_expression& expr, const type_conversion_info& conversion, const scope& current_scope, bool silent = false);

        void m_error_candidates(const std::list<type_conversion_info>& type_conversions);
        void m_error_candidates(const std::vector<function_filter_info>& function_overloads);

        // Checks whether clazz is accessible from parent_scope, and reports the error if it is not
        bool m_verify_class_access(scope const* const parent_scope, shift_class const* clazz, const std::variant<const token*, const shift_name*>& error);

        bool m_verify_access(const scope* parent_scope, shift_expression* expr, bool silent = false);

        bool m_verify_access(const scope* parent_scope, const shift_class* clazz, const shift_name& error_name, bool silent = false);

        bool m_verify_access(const scope* parent_scope, const shift_module* module_, const shift_name& error_name, bool silent = false);

        bool m_verify_access(const scope* parent_scope, shift_variable* variable, const shift_name& error_name, bool silent = false);

        bool m_verify_access(const scope* parent_scope, shift_function* function, const shift_name& error_name, bool silent = false);

        void m_token_error(const parser& parser_, const token& token_, const std::string_view msg);

        void m_token_error(const parser& parser_, const token& token_, const std::string& msg);

        void m_token_error(const parser& parser_, const token& token_, const char* const msg);

        void m_token_warning(const parser& parser_, const token& token_, const std::string_view msg);

        void m_token_warning(const parser& parser_, const token& token_, const std::string& msg);

        void m_token_warning(const parser& parser_, const token& token_, const char* const msg);

        void m_name_error(const parser& parser_, const shift_name& token_, const std::string_view msg);

        void m_name_error(const parser& parser_, const shift_name& name, const std::string& msg);

        void m_name_error(const parser& parser_, const shift_name& name, const char* const msg);

        void m_name_warning(const parser& parser_, const shift_name& name, const std::string_view msg);

        void m_name_warning(const parser& parser_, const shift_name& name, const std::string& msg);

        void m_name_warning(const parser& parser_, const shift_name& name, const char* const msg);

        void m_error(const parser& parser_, const std::string_view msg);

        void m_error(const parser& parser_, const std::string& msg);

        void m_error(const parser& parser_, const char* const msg);

        void m_warning(const parser& parser_, const std::string_view msg);

        void m_warning(const parser& parser_, const std::string& msg);

        void m_warning(const parser& parser_, const char* const msg);

        std::string_view m_get_line(const parser&, const token&) const noexcept;

        shift_class* m_make_array_class(shift_class* const clazz, const size_t dimensions);

        shift_class* m_make_pointer_class(shift_class* const clazz, const size_t dimensions);

        // Ensure the type has the shift.array/shift.pointer class as its name.clazz if the type has array/pointer dimensions
        shift_type& m_finalize_type(shift_type&);

    private:
        // TODO implement a caching system with scopes
        struct scope {
            scope* parent = nullptr;
            analyzer* base = nullptr;
            parser* parser_ = nullptr;
            shift_module* module_ = nullptr;
            shift_class* clazz = nullptr;
            shift_function* func = nullptr;
            shift_variable* var = nullptr;

            std::unordered_set<std::string> use_modules;
            std::unordered_map<std::string_view, shift_variable*> variables;

            parser* get_parser() const noexcept {
                if (parser_) return parser_;
                else if (var && var->clazz) return var->clazz->parser_;
                else if (func && func->clazz) return func->clazz->parser_;
                else if (clazz) return clazz->parser_;

                return parent ? parent->get_parser() : nullptr;
            }

            shift_module* get_module() const noexcept {
                if (module_) return module_;
                if (var) {
                    if (var->clazz && var->clazz->module_) return var->clazz->module_;
                    if (var->module_) return var->module_;
                }
                if (func) {
                    if (func->clazz && func->clazz->module_) return func->clazz->module_;
                    if (func->module_) return func->module_;
                }
                if (clazz && clazz->module_) return clazz->module_;

                return parent ? parent->get_module() : nullptr;
            }

            bool is_using_module(const shift_module& module_) const {
                if (use_modules.find(module_) != use_modules.end()) return true;

                if ((!parent || !parent->module_) && (this->module_ && *this->module_ == module_)) return true;

                if ((!parent || !parent->clazz) && (clazz && clazz->use_statements.contains(module_))) {
                    if (var && var->clazz == clazz) {
                        size_t dis = std::distance(clazz->use_statements.begin(), clazz->use_statements.find(module_));
                        if (dis < var->implicit_use_statements) { return true; }
                    } else if (func && func->clazz == clazz) {
                        size_t dis = std::distance(clazz->use_statements.begin(), clazz->use_statements.find(module_));
                        if (dis < func->implicit_use_statements) { return true; }
                    } else {
                        // don't know where we are specifically in the class; assume all modules are used
                        return true;
                    }
                }

                if (parser* parser_ = get_parser()) {
                    if (!parent || !parent->get_parser() || parent->clazz != clazz || parent->func != func || parent->var != var) {
                        auto f = parser_->m_global_uses.find(module_);
                        if (f != parser_->m_global_uses.end()) {
                            size_t dis = std::distance(parser_->m_global_uses.begin(), f);
                            size_t implicit_use = 0;

                            // TODO implicit_use is set but not used

                            if (var) {
                                if (var->clazz) {
                                    implicit_use = clazz->implicit_use_statements;
                                } else {
                                    implicit_use = var->implicit_use_statements;
                                }
                            } else if (func) {
                                if (func->clazz) {
                                    implicit_use = clazz->implicit_use_statements;
                                } else {
                                    implicit_use = func->implicit_use_statements;
                                }
                            } else if (clazz) {
                                implicit_use = clazz->implicit_use_statements;
                            }

                            if (dis < implicit_use) return true;
                        }
                    }
                }


                return parent ? parent->is_using_module(module_) : false;
            }

            utils::ordered_set<shift_class*> find_classes(const std::string& name) const {
                if (!base) return parent ? parent->find_classes(name) : utils::ordered_set<shift_class*>();

                {
                    auto const pointer_index = name.find('*');
                    auto const array_index = name.find('[');
                    auto const class_name_end = std::min(pointer_index, array_index);
                    if (class_name_end != std::string::npos) {
                        auto sub_classes_temp = find_classes(name.substr(0, class_name_end));
                        std::vector<shift_class*> sub_classes(std::make_move_iterator(sub_classes_temp.begin()), std::make_move_iterator(sub_classes_temp.end()));
                        for (auto it = name.begin() + class_name_end; it != name.end(); ++it) {
                            std::string::size_type dimensions;
                            switch (*it) {
                                case '*':
                                    dimensions = name.find_first_not_of('*', it - name.begin());
                                    if (dimensions == std::string::npos) { dimensions = name.end() - it; }
                                    for (shift_class*& sub_class : sub_classes) {
                                        sub_class = base->m_make_pointer_class(sub_class, dimensions);
                                    }
                                    it += dimensions - 1;
                                    break;
                                case '[':
                                    dimensions = name.find_first_not_of("[]", it - name.begin());
                                    if (dimensions == std::string::npos) { dimensions = name.end() - it; }
                                    for (shift_class*& sub_class : sub_classes) {
                                        sub_class = base->m_make_array_class(sub_class, dimensions);
                                    }
                                    it += dimensions * 2 - 1;
                                    break;
                                default: break;
                            }
                        }
                        return { std::make_move_iterator(sub_classes.begin()), std::make_move_iterator(sub_classes.end()) };
                    }
                }

                utils::ordered_set<shift_class*> classes;

                for (const std::string& module_ : use_modules) {
                    auto const find = base->m_classes.find(module_ + '.' + name);
                    if (find != base->m_classes.end()) classes.push_back(find->second);
                }

                if (clazz && (!parent || parent->clazz != clazz || parent->func != func || parent->var != var)) {
                    size_t implicit_class_use = SIZE_MAX;

                    if (var && var->clazz == clazz) {
                        implicit_class_use = var->implicit_use_statements;
                    } else if (func && func->clazz == clazz) {
                        implicit_class_use = func->implicit_use_statements;
                    }

                    for (size_t i = 0; const shift_module & module_ : clazz->use_statements) {
                        if (i >= implicit_class_use) break;

                        auto const find = base->m_classes.find(module_.to_string() + '.' + name);
                        if (find != base->m_classes.end()) classes.push_back(find->second);

                        i++;
                    }
                }

                if (parser* parser_ = get_parser()) {
                    if ((!parent || !parent->get_parser())
                        || parent->clazz != clazz || parent->func != func || parent->var != var) {
                        size_t implicit_use = 0;

                        if (var) {
                            if (var->clazz) {
                                implicit_use = clazz->implicit_use_statements;
                            } else {
                                implicit_use = var->implicit_use_statements;
                            }
                        } else if (func) {
                            if (func->clazz) {
                                implicit_use = clazz->implicit_use_statements;
                            } else {
                                implicit_use = func->implicit_use_statements;
                            }
                        } else if (clazz) {
                            implicit_use = clazz->implicit_use_statements;
                        }

                        for (size_t index = 0; const shift_module & module_ : parser_->m_global_uses) {
                            if (index >= implicit_use) break;

                            auto const find = base->m_classes.find(module_.to_string() + '.' + name);
                            if (find != base->m_classes.end()) classes.push_back(find->second);

                            index++;
                        }

                        {
                            std::string fqn = parser_->m_module->to_string();
                            if (!fqn.empty()) {
                                fqn += '.';
                                fqn += name;
                                auto f = base->m_classes.find(fqn);
                                if (f != base->m_classes.end()) classes.push_back(f->second);
                            }
                        }
                    }
                }

                if (!parent || parent->base != base) {
                    auto find = base->m_classes.find(name);
                    if (find != base->m_classes.end()) {
                        classes.push_back(find->second);
                    }
                }

                if (parent) {
                    utils::ordered_set<shift_class*> parent_list = parent->find_classes(name);
                    classes.insert(parent_list.begin(), parent_list.end());
                }

                return classes;
            }

            shift_class* find_class(const std::string& name) const {
                utils::ordered_set<shift_class*> classes = find_classes(name);
                return classes.size() == 1 ? classes.front() : nullptr;
            }

            utils::ordered_set<shift_variable*> find_variables(const std::string_view name) const {
                utils::ordered_set<shift_variable*> variables;

                {
                    auto const find = this->variables.find(name);
                    if (find != this->variables.end()) {
                        variables.push_back(find->second);
                        return variables;
                    }
                }

                if ((func && (!parent || parent->func != func)) || (var && (!parent || parent->var != var))) {
                    if ((var && (var->type.mods & shift_mods::STATIC) == 0x0 && var->clazz)) {
                        if (name == "this") {
                            variables.push_back(&var->clazz->this_var);
                        } else if (name == "base") {
                            variables.push_back(&var->clazz->base_var);
                            var->clazz->base_var.type.name.clazz = var->clazz->base.clazz;
                            var->clazz->base_var.type.name.name_clazz = var->clazz->base.name_clazz;
                        }
                    } else if ((func && (func->mods & shift_mods::STATIC) == 0x0 && func->clazz)) {
                        if (name == "this") {
                            variables.push_back(&func->clazz->this_var);
                        } else if (name == "base") {
                            variables.push_back(&func->clazz->base_var);
                            func->clazz->base_var.type.name.clazz = func->clazz->base.clazz;
                            func->clazz->base_var.type.name.name_clazz = func->clazz->base.name_clazz;
                        }
                    }
                }

                if (clazz && (!parent || !parent->clazz)) {
                    {
                        for (shift_variable& _var : clazz->variables) {
                            if (_var.name->get_data() == name) variables.push_back(&_var);
                        }
                        // TODO check super classes for variable names (should be dealt with by "parent")
                        for (shift_class* base_class = clazz->base.clazz; base_class; base_class = base_class->base.clazz) {
                            for (shift_variable& _var : base_class->variables) {
                                if (_var.name->get_data() == name) variables.push_back(&_var);
                            }
                        }
                    }
                }

                if (func && (!parent || !parent->func)) {
                    auto param = func->parameters.find(name);
                    if (param != func->parameters.end()) {
                        variables.push_back(&param->second);
                    }
                }
                if (parser* parser_ = get_parser()) {
                    if (!parent || !parent->get_parser()) {
                        std::string fqn = parser_->m_module->to_string();
                        if (fqn.length()) {
                            fqn += '.';
                            fqn += name;
                            auto f = base->m_variables.find(fqn);
                            if (f != base->m_variables.end()) variables.push_back(f->second);
                        }
                    }
                }

                if (parent) {
                    if (variables.size() == 0) {
                        auto parent_variables = parent->find_variables(name);
                        variables.insert(parent_variables.begin(), parent_variables.end());
                    }
                }

                if (base && (!parent || !parent->base)) {
                    auto var_find = base->m_variables.find(std::string(name));
                    if (var_find != base->m_variables.end()) {
                        variables.push_back(var_find->second);
                    }
                }

                return variables;
            }

            inline utils::ordered_set<shift_variable*>
                find_variables(const token* const name) const { return find_variables(name->get_data()); }

            inline shift_variable* find_variable(const std::string_view& name) const {
                utils::ordered_set<shift_variable*> variables = find_variables(name);
                return variables.size() == 1 ? variables.front() : nullptr;
            }

            inline shift_variable* find_variable(const token* const name) const {
                return find_variable(name->get_data());
            }

            utils::ordered_set<std::vector<function_overload_info>*> find_functions(const std::string_view name) const {
                if (!base) return parent ? parent->find_functions(name) : utils::ordered_set<std::vector<function_overload_info>*>{};

                utils::ordered_set<std::vector<function_overload_info>*> funcs;

                if (clazz && (!parent || parent->clazz != clazz)) {
                    for (shift_class* current_class = clazz; current_class; current_class = current_class->base.clazz) {
                        for (shift_function& func : current_class->functions) {
                            if (func.name == name) {
                                funcs.emplace(&base->m_function_overloads.find(func.get_fqn())->second);
                            }
                        }
                        if (!funcs.empty()) break;
                    }

                    if (!funcs.empty()) return funcs;

                    {
                        const std::string name_str(name);
                        for (shift_class* current_class = clazz; current_class; current_class = current_class->parent.clazz) {
                            for (auto& use_statement : current_class->use_statements) {
                                auto f = base->m_modules.find(use_statement.to_string());
                                if (f != base->m_modules.end()) {
                                    std::string func_fqn = f->second->to_string() + '.' + name_str;
                                    auto overloaded_func = base->m_function_overloads.find(func_fqn);
                                    if (overloaded_func != base->m_function_overloads.end()) {
                                        funcs.emplace(&overloaded_func->second);
                                    }
                                }
                            }
                            auto& use_statements = current_class->parent.clazz ? current_class->parent.clazz->use_statements : clazz->parser_->m_global_uses;
                            for (std::size_t count = 0; auto & use_statement : use_statements) {
                                if (count >= current_class->implicit_use_statements) break;
                                auto f = base->m_modules.find(use_statement.to_string());
                                if (f != base->m_modules.end()) {
                                    std::string func_fqn = f->second->to_string() + '.' + name_str;
                                    auto overloaded_func = base->m_function_overloads.find(func_fqn);
                                    if (overloaded_func != base->m_function_overloads.end()) {
                                        funcs.emplace(&overloaded_func->second);
                                    }
                                }
                                count++;
                            }
                        }
                    }
                }

                if (var && var->module_ && var->parser_ && (!parent || parent->var != var)) {
                    const std::string name_str(name);
                    for (std::size_t count = 0; auto & use_statement : var->parser_->m_global_uses) {
                        if (count >= var->implicit_use_statements) break;
                        auto f = base->m_modules.find(use_statement.to_string());
                        if (f != base->m_modules.end()) {
                            std::string func_fqn = f->second->to_string() + '.' + name_str;
                            auto overloaded_func = base->m_function_overloads.find(func_fqn);
                            if (overloaded_func != base->m_function_overloads.end()) {
                                funcs.emplace(&overloaded_func->second);
                            }
                        }
                        count++;
                    }
                } else if (func && func->module_ && func->parser_ && (!parent || parent->func != func)) {
                    const std::string name_str(name);
                    for (std::size_t count = 0; auto & use_statement : func->parser_->m_global_uses) {
                        if (count >= func->implicit_use_statements) break;
                        auto f = base->m_modules.find(use_statement.to_string());
                        if (f != base->m_modules.end()) {
                            std::string func_fqn = f->second->to_string() + '.' + name_str;
                            auto overloaded_func = base->m_function_overloads.find(func_fqn);
                            if (overloaded_func != base->m_function_overloads.end()) {
                                funcs.emplace(&overloaded_func->second);
                            }
                        }
                        count++;
                    }
                }

                if (shift_module* const scope_module = get_module()) {
                    if (!parent || parent->get_module() != scope_module) {
                        std::string func_fqn = scope_module->to_string() + '.' + std::string(name);
                        auto overloaded_func = base->m_function_overloads.find(func_fqn);
                        if (overloaded_func != base->m_function_overloads.end()) {
                            funcs.emplace(&overloaded_func->second);
                        }
                    }
                }

                if (parent) {
                    utils::ordered_set<std::vector<function_overload_info>*> parent_funcs = parent->find_functions(name);
                    funcs.insert(std::make_move_iterator(parent_funcs.begin()), std::make_move_iterator(parent_funcs.end()));
                }

                return funcs;
            }

            std::vector<function_overload_info>* find_function(const std::string_view name) const {
                auto funcs = find_functions(name);
                return funcs.size() == 1 ? funcs.front() : nullptr;
            }
        };
    private:
        friend struct scope;
    private:
        error_handler* m_error_handler{ nullptr };
        std::deque<parser>* m_parsers{ nullptr };
        std::unordered_map<std::string, shift_module*> m_modules;
        std::unordered_map<std::string, shift_class*> m_classes;
        std::unordered_map<std::string, shift_function*> m_functions;
        std::unordered_map<std::string, std::vector<function_overload_info>> m_function_overloads;
        std::unordered_map<std::string, shift_variable*> m_variables;
        // std::unordered_map<std::string, size_t> m_func_dupe_count;
        std::deque<shift_class> m_extra_classes;
        std::deque<token> m_extra_tokens;
    };

    inline analyzer::analyzer(error_handler* const handler, std::deque<parser>* const parsers) noexcept
        : m_error_handler(handler), m_parsers(parsers) {}

    inline analyzer::analyzer(error_handler* const handler, std::deque<parser>& parsers) noexcept : analyzer(handler,
        &parsers) {}

    template<utils::range_of<const analyzer::function_overload_info> FuncsIter, utils::range_of<const shift_expression> ParamsIter>
    std::vector<analyzer::function_filter_info>
        analyzer::m_filter_functions(utils::range<FuncsIter> funcs, utils::range<ParamsIter> params, bool silent) {
        std::vector<function_filter_info> filtered;

        struct match_quality {
            // number of function paramters that match the class type of an expression parameter exactly
            size_t exact_count = 0;

            // Base class resolution levels for each parameterin the form [level, count]
            std::map<size_t, size_t> base_levels;

            shift_function* func = nullptr;

            inline std::strong_ordering operator<=>(const match_quality& other) const noexcept {
                if (exact_count != other.exact_count) return exact_count <=> other.exact_count;

                for (auto it = base_levels.begin(), other_it = other.base_levels.begin(); it != base_levels.end() && other_it != other.base_levels.end(); ++it, ++other_it) {
                    if (it->first != other_it->first) return other_it->first <=> it->first;
                    if (it->second != other_it->second) return it->second <=> other_it->second;
                }

                if (func && other.func) {
                    // TODO potential edits if default parameters are added
                    for (auto params_it = func->parameters.begin(), other_params_it = other.func->parameters.begin();
                        params_it != func->parameters.end() && other_params_it != other.func->parameters.end();
                        ++params_it, ++other_params_it) {
                        auto current_imut = (params_it->second.type.mods & shift_mods::IMUT);
                        auto other_imut = (other_params_it->second.type.mods & shift_mods::IMUT);
                        if (current_imut != other_imut) {
                            return other_imut <=> current_imut;
                        }
                    }
                }

                return base_levels.size() <=> other.base_levels.size();
            }
        } best_quality;

        for (function_overload_info const& overload_info : funcs) {
            function_filter_info filter_info;
            auto& [filter_func, filter_conversions] = filter_info.candidate;

            filter_func = &overload_info;

            shift_function* const func = overload_info.func;

            match_quality current_quality{};
            current_quality.func = func;

            // TODO add default parameters
            if (func->parameters.size() != size_t(params.size())) continue;

            auto param_it = params.begin();
            size_t param_index = 0;
            for (auto const& [func_param_name, func_param_var] : func->parameters) {
                if (!func_param_var.type.tried_resolve && !func_param_var.type.is_resolved()) {
                    m_analyze_function_params(*func, silent);
                }
                if (!func_param_var.type.is_resolved()) break;
                const shift_expression& param_expr = *param_it;
                if (param_expr.resolved.type.is_conversion_needed(func_param_var.type)) {
                    const auto& conversions = m_get_implicit_conversions(&param_expr.resolved.type, &func_param_var.type);
                    if (conversions.empty()) { break; }

                    filter_conversions[&param_expr] = &conversions;
                    ++current_quality.base_levels[std::numeric_limits<size_t>::max()];

                } else {
                    if (!param_expr.is_type_resolved()) { break; }
                    filter_conversions[&param_expr] = nullptr;
                    if (func_param_var.type.name.clazz == param_expr.resolved.type.name.clazz) {
                        current_quality.exact_count++;
                    } else if (size_t base_level = param_expr.resolved.type.name.clazz->has_base(func_param_var.type.name.clazz)) {
                        ++current_quality.base_levels[base_level];
                    }
                }
                ++param_it;
                ++param_index;
            }

            if (param_it == params.end()) {
                std::strong_ordering quality_comp = current_quality <=> best_quality;
                if (quality_comp == std::strong_ordering::greater) {
                    best_quality = std::move(current_quality);
                    filtered.clear();
                    filtered.push_back(std::move(filter_info));
                } else if (quality_comp == std::strong_ordering::equal) {
                    filtered.push_back(std::move(filter_info));
                }
            }
        }

        return filtered;
    }

    template<utils::range_of<const std::vector<analyzer::function_overload_info>*> FuncsIter, utils::range_of<const shift_expression> ParamsIter>
    std::vector<analyzer::function_filter_info>
        analyzer::m_filter_functions(utils::range<FuncsIter> funcs, utils::range<ParamsIter> params, bool silent) {
        std::vector<function_filter_info> filtered;
        for (const std::vector<analyzer::function_overload_info>* overloads : funcs) {
            debug_log("Filtering functions with " << overloads->size() << " overloads");
            auto sub_filter = m_filter_functions(utils::range(*overloads), params, silent);
            filtered.insert(filtered.end(), std::make_move_iterator(sub_filter.begin()), std::make_move_iterator(sub_filter.end()));
        }
        return filtered;
    }
}

#endif