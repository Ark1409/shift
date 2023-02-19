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
                    } else {
                        this->m_token_error(_parser, *clazz.name, "class '" + fqn + "' has already been defined");
                    }
                }
            }

            for (parser& _parser : *m_parsers) {
                const std::string module_name = _parser.m_module.to_string();

                for (parser::shift_module const& use : _parser.m_global_uses) {
                    const std::string use_string = use.to_string();
                    if (!m_contains_module(use_string)) {
                        this->m_token_error(_parser, *use.begin, "module '" + use.to_string() + "' does not exist");
                    } else if (_parser.m_is_module_defined() && use_string == module_name) {
                        this->m_token_warning(_parser, *use.begin, "redundant 'use' statement");
                    }
                }

                for (parser::shift_class& clazz : _parser.m_classes) {
                    for (parser::shift_module const& use : clazz.use_statements) {
                        const std::string use_string = use.to_string();
                        if (!m_contains_module(use_string)) {
                            this->m_token_error(_parser, *use.begin, "module '" + use.to_string() + "' does not exist");
                        } else if (this->m_error_handler && this->m_error_handler->is_print_warnings()) {
                            if (_parser.m_is_module_defined() && use.to_string() == module_name) {
                                this->m_token_warning(_parser, *use.begin, "redundant 'use' statement");
                            } else {
                                auto end = clazz.implicit_use_statements;
                                ++end;
                                for (auto b = _parser.m_global_uses.cbegin(); b != end; ++b) {
                                    if (b->to_string() == use_string) {
                                        this->m_token_warning(_parser, *use.begin, "redundant 'use' statement");
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        bool analyzer::m_contains_module(const parser::shift_module& m) const noexcept {
            return m_contains_module(m.to_string());
        }

        bool analyzer::m_contains_module(const std::string& m) const noexcept {
            return m_modules.find(m) != m_modules.end();
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