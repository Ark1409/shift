/**SHIFT_ANALYZER
 * @file compiler/shift_analyzer.cpp
 */
#include "compiler/shift_analyzer.h"
#include "utils/utils.h"

#include <cstring>

#define SHIFT_ANALYZER_ERROR_PREFIX(__parser) 				"error: " << std::filesystem::relative((__parser).get_tokenizer()->get_file().get_path()).string() << ": " // std::filesystem::relative call every time probably isn't that optimal
#define SHIFT_ANALYZER_WARNING_PREFIX(__parser) 			"warning: " << std::filesystem::relative((__parser).get_tokenizer()->get_file().get_path()).string() << ": " // std::filesystem::relative call every time probably isn't that optimal

#define SHIFT_ANALYZER_ERROR_PREFIX_EXT_(__parser, __line__, __col__) "error: " << std::filesystem::relative((__parser).get_tokenizer()->get_file().get_path()).string() << ":" << __line__ << ":" << __col__ << ": " // std::filesystem::relative call every time probably isn't that optimal
#define SHIFT_ANALYZER_WARNING_PREFIX_EXT_(__parser, __line__, __col__) "warning: " << std::filesystem::relative((__parser).get_tokenizer()->get_file().get_path()).string() << ":" << __line__ << ":" << __col__ << ": " // std::filesystem::relative call every time probably isn't that optimal

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
#define SHIFT_ANALYZER_FATAL_ERROR_LOG_( __ERR__)  SHIFT_ANALYZER_ERROR_LOG_(__ERR__); SHIFT_ANALYZER_PRINT()

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
#define SHIFT_ANALYZER_DOUBLE_CLASS "shift.double"

#define SHIFT_ANALYZER_STRING_CLASS "shift.string"

#define SHIFT_ANALYZER_BOOLEAN_CLASS "shift.bool"
#define SHIFT_ANALYZER_BOOL_CLASS SHIFT_ANALYZER_BOOLEAN_CLASS

namespace shift {
    namespace compiler {
        void analyzer::analyze() {
            for (parser& _parser : *m_parsers) {
                if (!_parser.m_is_module_defined()) {
                    this->m_error(_parser, "module not defined for file");
                } else {
                    m_modules.emplace(_parser.m_module.to_string());
                }

                for (parser::shift_class& clazz : _parser.m_classes) {
                    std::string fqn = clazz.get_fqn();

                    // TODO check to see if the class name would override a module name
                    if (m_classes.find(fqn) == m_classes.end()) {
                        m_classes[std::move(fqn)] = &clazz;

                        size_t index = 0;
                        for (parser::shift_function& func : clazz.functions) {
                            std::string function_fqn = func.get_fqn(index);
                            if (m_functions.find(function_fqn) == m_functions.end()) {
                                m_functions[std::move(function_fqn)] = &func;
                                index++;
                            } else {
                                this->m_token_error(_parser, *clazz.name, "function '" + function_fqn + "' has already been defined");
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
                _scope.parser = &_parser;

                const std::string module_name = _parser.m_module.to_string();

                for (parser::shift_module const& use : _parser.m_global_uses) {
                    if (!_scope.contains_module(use)) {
                        this->m_name_error(_parser, use, "module '" + use.to_string() + "' does not exist");
                    } else if (use == module_name) {
                        this->m_name_warning(_parser, use, "redundant 'use' statement");
                    }
                }

                for (parser::shift_class& clazz : _parser.m_classes) {
                    _scope.clazz = &clazz;

                    for (parser::shift_module const& use : clazz.use_statements) {
                        if (!_scope.contains_module(use)) {
                            this->m_name_error(_parser, use, "module '" + use.to_string() + "' does not exist");
                        } else if (this->m_error_handler && this->m_error_handler->is_print_warnings()) {
                            if (use == module_name) {
                                this->m_name_warning(_parser, use, "redundant 'use' statement");
                            } else {
                                std::string use_string = use.to_string();
                                size_t index = 0;
                                for (auto b = _parser.m_global_uses.cbegin(); index < clazz.implicit_use_statements && b != _parser.m_global_uses.cend(); ++b, index++) {
                                    if (*b == use_string) {
                                        this->m_name_warning(_parser, use, "redundant 'use' statement");
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    for (parser::shift_variable& _var : clazz.variables) {
                        auto _vars = _scope.find_variables(_var.name);
                        if (_vars.size() > 1 && (*(++_vars.begin()))->name == _var.name) {
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

                        for (parser::shift_function& func : clazz.functions) {
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

                            std::unordered_set<std::string_view> param_names;
                            for (auto& [param_type, param_name] : func.parameters) {
                                if (!param_name->is_null_token() && param_names.find(param_name->get_data()) != param_names.end()) {
                                    this->m_token_error(_parser, *param_name, "duplicate parameter name '" + std::string(param_name->get_data()) + "' in function '" + func.get_fqn() + "'");
                                }

                                std::string param_type_class_name = param_type.name.to_string();

                                auto param_type_class_candidates = _scope.find_classes(param_type_class_name);

                                if (param_type_class_candidates.size() > 1) {
                                    this->m_name_error(_parser, param_type.name, "ambiguous class reference to '" + param_type_class_name + "'");
                                } else if (param_type_class_candidates.size() == 0) {
                                    this->m_name_error(_parser, param_type.name, "unable to resolve class '" + param_type_class_name + "'");
                                } else {
                                    param_type.name_class = param_type_class_candidates.front();
                                }
                                if (!param_name->is_null_token())
                                    param_names.emplace(param_name->get_data());
                            }
                        }
                    }
                }
            }
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

        void analyzer::m_name_error(const parser& parser_, const parser::shift_name& name, const std::string_view msg) {
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

        void analyzer::m_name_error(const parser& parser_, const parser::shift_name& name, const std::string& msg) { return m_name_error(parser_, name, std::string_view(msg.c_str(), msg.length())); }
        void analyzer::m_name_error(const parser& parser_, const parser::shift_name& name, const char* const msg) { return m_name_error(parser_, name, std::string_view(msg, std::strlen(msg))); }

        void analyzer::m_name_warning(const parser& parser_, const parser::shift_name& name, const std::string_view msg) {
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

        void analyzer::m_name_warning(const parser& parser_, const parser::shift_name& name, const std::string& msg) { return m_name_warning(parser_, name, std::string_view(msg.c_str(), msg.length())); }
        void analyzer::m_name_warning(const parser& parser_, const parser::shift_name& name, const char* const msg) { return m_name_warning(parser_, name, std::string_view(msg, std::strlen(msg))); }

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