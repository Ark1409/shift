/**
 * @file compiler/shift_analyzer.h
 */
#ifndef SHIFT_ANALYZER_H_
#define SHIFT_ANALYZER_H_ 1
#include "compiler/shift_parser.h"

#include <unordered_map>
#include <unordered_set>

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
            void m_token_error(const parser& parser_, const token& token_, const std::string_view msg);
            void m_token_error(const parser& parser_, const token& token_, const std::string& msg);
            void m_token_error(const parser& parser_, const token& token_, const char* const msg);

            void m_token_warning(const parser& parser_, const token& token_, const std::string_view msg);
            void m_token_warning(const parser& parser_, const token& token_, const std::string& msg);
            void m_token_warning(const parser& parser_, const token& token_, const char* const msg);

            void m_name_error(const parser& parser_, const parser::shift_name& token_, const std::string_view msg);
            void m_name_error(const parser& parser_, const parser::shift_name& name, const std::string& msg);
            void m_name_error(const parser& parser_, const parser::shift_name& name, const char* const msg);

            void m_name_warning(const parser& parser_, const parser::shift_name& name, const std::string_view msg);
            void m_name_warning(const parser& parser_, const parser::shift_name& name, const std::string& msg);
            void m_name_warning(const parser& parser_, const parser::shift_name& name, const char* const msg);

            void m_error(const parser& parser_, const std::string_view msg);
            void m_error(const parser& parser_, const std::string& msg);
            void m_error(const parser& parser_, const char* const msg);

            void m_warning(const parser& parser_, const std::string_view msg);
            void m_warning(const parser& parser_, const std::string& msg);
            void m_warning(const parser& parser_, const char* const msg);

            std::string_view m_get_line(const parser&, const token&) const noexcept;

        private:
            struct scope {
                analyzer* base = nullptr;
                scope* parent = nullptr;
                parser* parser = nullptr;
                parser::shift_class* clazz = nullptr;
                parser::shift_function* func = nullptr;

                std::unordered_set<std::string> use_modules;

                inline bool using_module(const std::string& module_) const noexcept {
                    if (use_modules.find(module_) != use_modules.end()) return true;
                    if (parent) return using_module(module_); // TODO requires top of tree to have a class and a parser

                    if (clazz) {
                        for (parser::shift_module const& class_use_module : clazz->use_statements) {
                            if (class_use_module == module_) return true;
                        }
                    }

                    if (parser) {
                        size_t index = 0;
                        for (parser::shift_module const& parser_use_module : parser->m_global_uses) {
                            if (clazz && index >= clazz->implicit_use_statements) break;
                            if (parser_use_module == module_) return true;

                            index++;
                        }
                    }

                    return false;
                }

                inline bool contains_module(const std::string& module_) const noexcept {
                    return base && base->m_modules.find(module_) != base->m_modules.end();
                }

                inline bool contains_module(const parser::shift_module& _module) const noexcept {
                    return contains_module(_module.to_string());
                }

                std::list<parser::shift_class*> find_classes(const std::string& name) const noexcept {
                    if (!base) return parent ? parent->find_classes(name) : std::list<parser::shift_class*>();

                    std::list<parser::shift_class*> classes;

                    for (std::string const& module_ : use_modules) {
                        auto const find = base->m_classes.find(module_ + '.' + name);
                        if (find != base->m_classes.end()) classes.push_back(find->second);
                    }

                    if (parent) {
                        std::list<parser::shift_class*> parent_list = parent->find_classes(name);
                        classes.insert(classes.end(), parent_list.begin(), parent_list.end());
                        return classes;
                    }

                    if (clazz) {
                        for (parser::shift_module const& module_ : clazz->use_statements) {
                            auto const find = base->m_classes.find(module_.to_string() + '.' + name);
                            if (find != base->m_classes.end()) classes.push_back(find->second);
                        }
                    }

                    if (parser) {
                        size_t index = 0;
                        for (parser::shift_module const& module_ : parser->m_global_uses) {
                            if (clazz && index >= clazz->implicit_use_statements) break;

                            auto const find = base->m_classes.find(module_.to_string() + '.' + name);
                            if (find != base->m_classes.end()) classes.push_back(find->second);

                            index++;
                        }
                    }

                    return classes;
                }

                parser::shift_class* find_class(const std::string& name) const noexcept {
                    std::list<parser::shift_class*> classes = find_classes(name);
                    return classes.size() == 1 ? classes.front() : nullptr;
                }

                std::list<parser::shift_variable*> find_variables(const std::string_view name) const noexcept {
                    if (!clazz) return std::list<parser::shift_variable*>();

                    std::list<parser::shift_variable*> variables;

                    for (parser::shift_variable& _var : clazz->variables) {
                        if (_var.name->get_data() == name) variables.push_back(&_var);
                    }

                    // TODO cache result
                    return variables;
                }

                inline std::list<parser::shift_variable*> find_variables(const token* const name) const noexcept { return find_variables(name->get_data()); }
                inline std::list<parser::shift_variable*> find_variables(const std::string& name) const noexcept { return find_variables(std::string_view(name.c_str(), name.length())); }

                inline parser::shift_variable* find_variable(const std::string_view& name) const noexcept {
                    std::list<parser::shift_variable*> variables = find_variables(name);
                    return variables.size() == 1 ? variables.front() : nullptr;
                }

                inline parser::shift_variable* find_variable(const token* const name) const noexcept { return find_variable(name->get_data()); }
                inline parser::shift_variable* find_variable(const std::string& name) const noexcept { return find_variable(std::string_view(name.c_str(), name.length())); }
            };
        private:
            error_handler* m_error_handler;
            std::list<parser>* m_parsers;
            std::unordered_set<std::string> m_modules;
            std::unordered_map<std::string, parser::shift_class*> m_classes;
            std::unordered_map<std::string, parser::shift_function*> m_functions;
        };

        inline analyzer::analyzer(error_handler* const handler, std::list<parser>* const parsers) noexcept: m_error_handler(handler), m_parsers(parsers) {}
        inline analyzer::analyzer(error_handler* const handler, std::list<parser>& parsers) noexcept: analyzer(handler, &parsers) {}
    }
}
#endif