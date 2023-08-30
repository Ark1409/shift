/**
 * @file compiler/shift_analyzer.h
 */
#ifndef SHIFT_ANALYZER_H_
#define SHIFT_ANALYZER_H_ 1
#include "compiler/shift_parser.h"
#include "utils/utils.h"
#include "utils/ordered_set.h"

#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <algorithm>

namespace shift {
    namespace compiler {
        class analyzer {
        public:
            inline analyzer(error_handler* const handler, std::list<parser>* const parsers) noexcept;
            inline analyzer(error_handler* const handler, std::list<parser>& parsers) noexcept;

            void analyze();

            inline const error_handler* get_error_handler() const noexcept { return m_error_handler; }
            inline void set_error_handler(error_handler* const handler) noexcept { m_error_handler = handler; }

            inline std::list<parser>* get_parsers() noexcept { return m_parsers; }
            inline const std::list<parser>* get_parsers() const noexcept { return m_parsers; }
        private:
            struct scope;

            void m_init_defaults();
            void m_resolve_params(shift_function& func, bool silent = false);
            void m_resolve_return_type(shift_function& func, bool silent = false);
            inline void m_resolve_func(shift_function& func, bool silent = false) { m_resolve_params(func, silent), m_resolve_return_type(func, silent); }

            void m_set_null(shift_expression& value) const noexcept;

            utils::ordered_set<shift_function*>& m_filter_functions(utils::ordered_set<shift_function*>& funcs, std::list<shift_expression>& params);

            inline utils::ordered_set<shift_function*>& m_filter_functions(utils::ordered_set<shift_function*>& funcs, shift_expression& param) {
                std::list<shift_expression> list;
                list.push_back(std::move(param));
                m_filter_functions(funcs, list);
                param = std::move(list.front());
                return funcs;
            }

            inline utils::ordered_set<shift_function*>& m_filter_functions(utils::ordered_set<shift_function*>& funcs, shift_expression* const param) {
                if (param) {
                    return m_filter_functions(funcs, *param);
                }

                std::list<shift_expression> list;
                return m_filter_functions(funcs, list);
            }

            inline bool contains_module(const std::string& module_) const { return this->m_modules.find(module_) == this->m_modules.end(); }
            inline void m_analyze_scope(std::list<shift_statement>& statements, scope* parent_scope = nullptr) { return m_analyze_scope(statements.begin(), statements.end(), parent_scope); }
            void m_analyze_scope(typename std::list<shift_statement>::iterator statements_begin, typename std::list<shift_statement>::iterator statements_end, scope* parent_scope = nullptr);
            void m_analyze_function(shift_function& func, scope* parent_scope = nullptr);
            void m_resolve_expression(shift_expression*, scope* const parent_scope = nullptr);
            utils::ordered_set<shift_function*> m_get_implicit_conversions(shift_class* const from, shift_class* const to) const noexcept;
            bool m_check_access(scope const* const parent_scope, shift_expression const* expr);
            void m_token_error(const parser& parser_, const token& token_, const std::string_view msg);
            void m_token_error(const parser& parser_, const token& token_, const std::string& msg);
            void m_token_error(const parser& parser_, const token& token_, const char* const msg);
            shift_expression m_create_convert_expr(shift_expression&& value, shift_function& convert_func) const;

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
            inline shift_class* m_make_array_class(shift_type const& type) { return m_make_array_class(type.name_class, type.array_dimensions); }
        private:
            // TODO implement a cacheing system with scopes
            struct scope {
                scope* parent = nullptr;
                analyzer* base = nullptr;
                parser* parser_ = nullptr;
                shift_class* clazz = nullptr;
                shift_function* func = nullptr;
                shift_variable* var = nullptr;

                std::unordered_set<std::string> use_modules;
                std::unordered_map<std::string_view, shift_variable*> variables;

                parser* get_parser() const noexcept {
                    if (clazz) return clazz->parser_;
                    if (func && func->clazz) return func->clazz->parser_;
                    if (var && var->clazz) return var->clazz->parser_;
                    if (parent) {
                        parser* parent_parser = parent->get_parser();
                        return parent_parser ? parent_parser : parser_;
                    }
                    return parser_;
                }

                inline bool using_module(const shift_module& module_) const noexcept {
                    if (use_modules.find(module_) != use_modules.end()) return true;

                    if (clazz && (!parent || parent->clazz != clazz)) {
                        if (clazz->use_statements.contains(module_)) {
                            return true;
                        }
                    }

                    if (get_parser() && (!parent || !parent->get_parser())) {
                        parser* parser_ = get_parser();

                        auto f = parser_->m_global_uses.find(module_);
                        if (f != parser_->m_global_uses.end()) {
                            size_t dis = std::distance(parser_->m_global_uses.begin(), f);
                            size_t implicit_use = SIZE_MAX;

                            if (clazz) {
                                implicit_use = std::min(implicit_use, clazz->implicit_use_statements);

                            }

                            if (func) {
                                implicit_use = std::min(implicit_use, func->implicit_use_statements);
                            }

                            if (var) {
                                implicit_use = std::min(implicit_use, var->implicit_use_statements);
                            }

                            if (dis < clazz->implicit_use_statements) return true;
                        }
                    }

                    if (parent) return parent->using_module(module_); // TODO requires top of tree to have a class and a parser

                    return false;
                }

                utils::ordered_set<shift_class*> find_classes(const std::string& name) const noexcept {
                    if (!base) return parent ? parent->find_classes(name) : utils::ordered_set<shift_class*>();

                    utils::ordered_set<shift_class*> classes;

                    for (std::string const& module_ : use_modules) {
                        auto const find = base->m_classes.find(module_ + '.' + name);
                        if (find != base->m_classes.end()) classes.push_back(find->second);
                    }

                    if (clazz && (!parent || parent->clazz != clazz)) {
                        for (shift_module const& module_ : clazz->use_statements) {
                            auto const find = base->m_classes.find(module_.to_string() + '.' + name);
                            if (find != base->m_classes.end()) classes.push_back(find->second);
                        }
                    }

                    if (get_parser() && (!parent || !parent->get_parser())) {
                        size_t implicit_use = SIZE_MAX;

                        if (clazz) {
                            implicit_use = std::min(implicit_use, clazz->implicit_use_statements);
                        }

                        if (func) {
                            implicit_use = std::min(implicit_use, func->implicit_use_statements);
                        }

                        if (var) {
                            implicit_use = std::min(implicit_use, var->implicit_use_statements);
                        }

                        size_t index = 0;
                        for (shift_module const& module_ : get_parser()->m_global_uses) {
                            if (index >= implicit_use) break;

                            auto const find = base->m_classes.find(module_.to_string() + '.' + name);
                            if (find != base->m_classes.end()) classes.push_back(find->second);

                            index++;
                        }

                        {
                            std::string fqn = get_parser()->m_module.to_string();
                            if (fqn.length()) {
                                fqn += '.';
                                fqn += name;
                                auto f = base->m_classes.find(fqn);
                                if (f != base->m_classes.end())classes.push_back(f->second);
                            }
                        }
                    }

                    if (!parent || !parent->base) {
                        auto find = base->m_classes.find(name);
                        if (find != base->m_classes.end()) {
                            classes.push_back(find->second);
                        }
                    }

                    if (parent) {
                        utils::ordered_set<shift_class*> parent_list = parent->find_classes(name);
                        for (auto& parent_class : parent_list) {
                            classes.push_back(std::move(parent_class));
                        }
                    }

                    return classes;
                }

                shift_class* find_class(const std::string& name) const noexcept {
                    utils::ordered_set<shift_class*> classes = find_classes(name);
                    return classes.size() == 1 ? classes.front() : nullptr;
                }

                utils::ordered_set<shift_variable*> find_variables(const std::string_view name) const noexcept {
                    utils::ordered_set<shift_variable*> variables;

                    {
                        auto const find = this->variables.find(name);
                        if (find != this->variables.end()) {
                            variables.push_back(find->second);
                            return variables;
                        }
                    }

                    if (clazz && (!parent || !parent->clazz)) {
                        if ((func && (func->mods & shift_mods::STATIC) == 0x0 && func->clazz)
                            || (var && (var->type.mods & shift_mods::STATIC) == 0x0 && var->clazz)) {
                            if (name == "this") {
                                variables.push_back(&clazz->this_var);
                            } else if (name == "base") {
                                variables.push_back(&clazz->base_var);
                                clazz->base_var.type.name_class = clazz->base.clazz;
                            }
                        }
                        {
                            for (shift_variable& _var : clazz->variables) {
                                if (_var.name->get_data() == name) variables.push_back(&_var);
                            }
                            // TODO check super classes for variable names (should be dealt with by "parent")
                            for (shift_class* base_class = clazz->base.clazz;base_class;base_class = base_class->base.clazz) {
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

                    if (get_parser() && (!parent || !parent->get_parser())) {
                        {
                            std::string fqn = get_parser()->m_module.to_string();
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
                            for (auto& parent_var : parent_variables) {
                                variables.push_back(std::move(parent_var));
                            }
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

                inline utils::ordered_set<shift_variable*> find_variables(const token* const name) const noexcept { return find_variables(name->get_data()); }
                inline utils::ordered_set<shift_variable*> find_variables(const std::string& name) const noexcept { return find_variables(std::string_view(name.c_str(), name.length())); }

                inline shift_variable* find_variable(const std::string_view& name) const noexcept {
                    utils::ordered_set<shift_variable*> variables = find_variables(name);
                    return variables.size() == 1 ? variables.front() : nullptr;
                }

                inline shift_variable* find_variable(const token* const name) const noexcept { return find_variable(name->get_data()); }
                inline shift_variable* find_variable(const std::string& name) const noexcept { return find_variable(std::string_view(name.c_str(), name.length())); }

                utils::ordered_set<shift_function*> find_functions(const std::string_view name) const noexcept {
                    if (!base) return parent ? parent->find_functions(name) : utils::ordered_set<shift_function*>();

                    utils::ordered_set<shift_function*> funcs;

                    if (clazz && (!parent || !parent->clazz)) {
                        for (shift_class* current_class = clazz; current_class; current_class = current_class->base.clazz) {
                            for (shift_function& func : current_class->functions) {
                                if (func.name.to_string() == name) {
                                    funcs.push_back(&func);
                                }
                            }
                        }

                        for (shift_module const& module_ : clazz->use_statements) {
                            std::string func_fqn = module_.to_string() + '.' + std::string(name);
                            auto f_dupe = base->m_func_dupe_count.find(func_fqn);
                            if (f_dupe != base->m_func_dupe_count.end()) {
                                for (size_t i = 0; i < f_dupe->second; i++) {
                                    auto f_find = base->m_functions.find(func_fqn + '@' + std::to_string(i));
                                    funcs.push_back(f_find->second);
                                }
                            }
                        }

                        if (get_parser()) {
                            size_t index = 0;
                            parser* const parser_ = get_parser();
                            for (shift_module const& module_ : parser_->m_global_uses) {
                                if (index >= clazz->implicit_use_statements) break;

                                std::string func_fqn = module_.to_string() + '.' + std::string(name);
                                auto f_dupe = base->m_func_dupe_count.find(func_fqn);
                                if (f_dupe != base->m_func_dupe_count.end()) {
                                    for (size_t i = 0; i < f_dupe->second; i++) {
                                        auto f_find = base->m_functions.find(func_fqn + '@' + std::to_string(i));
                                        funcs.push_back(f_find->second);
                                    }
                                }
                                index++;
                            }
                        }
                    } else if (func && (!parent || !parent->func) && !func->clazz) {
                        if (get_parser()) {
                            size_t index = 0;
                            parser* const parser_ = get_parser();
                            for (shift_module const& module_ : parser_->m_global_uses) {
                                if (index >= func->implicit_use_statements) break;

                                std::string func_fqn = module_.to_string() + '.' + std::string(name);
                                auto f_dupe = base->m_func_dupe_count.find(func_fqn);
                                if (f_dupe != base->m_func_dupe_count.end()) {
                                    for (size_t i = 0; i < f_dupe->second; i++) {
                                        auto f_find = base->m_functions.find(func_fqn + '@' + std::to_string(i));
                                        funcs.push_back(f_find->second);
                                    }
                                }
                                index++;
                            }
                        }
                    } else if (var && (!parent || !parent->var) && !var->clazz) {
                        if (get_parser()) {
                            size_t index = 0;
                            parser* const parser_ = get_parser();
                            for (shift_module const& module_ : parser_->m_global_uses) {
                                if (index >= var->implicit_use_statements) break;

                                std::string func_fqn = module_.to_string() + '.' + std::string(name);
                                auto f_dupe = base->m_func_dupe_count.find(func_fqn);
                                if (f_dupe != base->m_func_dupe_count.end()) {
                                    for (size_t i = 0; i < f_dupe->second; i++) {
                                        auto f_find = base->m_functions.find(func_fqn + '@' + std::to_string(i));
                                        funcs.push_back(f_find->second);
                                    }
                                }
                                index++;
                            }
                        }
                    } else if (get_parser() && (!parent || !parent->get_parser())) {
                        parser* const parser_ = get_parser();
                        for (shift_module const& module_ : parser_->m_global_uses) {
                            std::string func_fqn = module_.to_string() + '.' + std::string(name);
                            auto f_dupe = base->m_func_dupe_count.find(func_fqn);
                            if (f_dupe != base->m_func_dupe_count.end()) {
                                for (size_t i = 0; i < f_dupe->second; i++) {
                                    auto f_find = base->m_functions.find(func_fqn + '@' + std::to_string(i));
                                    funcs.push_back(f_find->second);
                                }
                            }
                        }

                        {
                            std::string func_fqn = get_parser()->m_module.to_string();
                            if (func_fqn.length()) {
                                func_fqn += '.';
                                func_fqn += name;

                                auto f_dupe = base->m_func_dupe_count.find(func_fqn);
                                if (f_dupe != base->m_func_dupe_count.end()) {
                                    for (size_t i = 0; i < f_dupe->second; i++) {
                                        auto f_find = base->m_functions.find(func_fqn + '@' + std::to_string(i));
                                        funcs.push_back(f_find->second);
                                    }
                                }
                            }
                        }

                    }

                    if (parent) {
                        auto parent_funcs = parent->find_functions(name);
                        for (auto& parent_func : parent_funcs) {
                            funcs.push_back(std::move(parent_func));
                        }
                    } else {
                        std::string func_fqn = std::string(name);
                        auto f_dupe = base->m_func_dupe_count.find(func_fqn);
                        if (f_dupe != base->m_func_dupe_count.end()) {
                            for (size_t i = 0; i < f_dupe->second; i++) {
                                auto f_find = base->m_functions.find(func_fqn + '@' + std::to_string(i));
                                funcs.push_back(f_find->second);
                            }
                        }
                    }



                    // auto parts = utils::split(name, std::string_view("."));

                    // shift_variable* const _var = find_variable(parts[0]);

                    // shift_class* type = nullptr;
                    // if (_var) {
                    //     type = _var->type.name_class;
                    //     for (size_t i = 1; i < parts.size() - 1 && type; i++) {
                    //         scope _scope;
                    //         _scope.base = this->base;
                    //         _scope.parser = this->parser;
                    //         _scope.clazz = type;

                    //         shift_variable* const find_var = _scope.find_variable(parts[i]);
                    //         if (!find_var) type = nullptr;
                    //         else type = find_var->type.name_class;
                    //     }
                    // } else {
                    //     size_t const len = name.rfind('.');
                    //     type = find_class(std::string(name.data(), len));
                    // }

                    // if (type) {
                    //     size_t const len = name.rfind('.');
                    //     const std::string_view func_name = std::string_view(name.data() + len + 1, name.length() - len - 1); // may overflow
                    //     std::string func_fqn_template = type->get_fqn() + '.' + std::string(func_name) + '@';
                    //     for (size_t i = 0;;i++) {
                    //         auto _func = this->base->m_functions.find(func_fqn_template + std::to_string(i));

                    //         if (_func != this->base->m_functions.end()) {
                    //             funcs.push_back(_func->second);
                    //         } else break;
                    //     }
                    // }

                    return funcs;
                }

                shift_function* find_function(const std::string_view name) const noexcept {
                    auto funcs = find_functions(name);
                    return funcs.size() == 1 ? funcs.front() : nullptr;
                }
            };
        private:
            error_handler* m_error_handler;
            std::list<parser>* m_parsers;
            std::unordered_set<std::string> m_modules;
            std::unordered_map<std::string, shift_class*> m_classes;
            std::unordered_map<std::string, shift_function*> m_functions;
            std::unordered_map<std::string, shift_variable*> m_variables;
            std::unordered_map<std::string, size_t> m_func_dupe_count;
            std::list<shift_class> m_extra_classes;
        };

        inline analyzer::analyzer(error_handler* const handler, std::list<parser>* const parsers) noexcept : m_error_handler(handler), m_parsers(parsers) {}
        inline analyzer::analyzer(error_handler* const handler, std::list<parser>& parsers) noexcept : analyzer(handler, &parsers) {}
    }
}
#endif