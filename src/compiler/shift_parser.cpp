#include "compiler/shift_parser.h"

#include <cstring>
#include <algorithm>

#define SHIFT_PARSER_ERROR_PREFIX 				"error: " << std::filesystem::relative(this->m_tokenizer->get_file().get_path()).native() << ": " // std::filesystem::relative call every time probably isn't that optimal
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

#define SHIFT_PARSER_WARNING_LOG_(__token, __WARN__) 		this->m_error_handler->stream() << __WARN__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::warning)
#define SHIFT_PARSER_FATAL_WARNING_LOG_(__token, __WARN__)  SHIFT_PARSER_WARNING_LOG_(__token, __WARN__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_ERROR_(__token, __ERR__) 			this->m_error_handler->stream() << SHIFT_PARSER_ERROR_PREFIX_EXT(__token) << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_PARSER_FATAL_ERROR_(__token, __ERR__) 		SHIFT_PARSER_ERROR_(__token, __ERR__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_ERROR_LOG_(__token, __ERR__) 		this->m_error_handler->stream() << __ERR__ << '\n'; this->m_error_handler->flush_stream(shift::compiler::error_handler::message_type::error)
#define SHIFT_PARSER_FATAL_ERROR_LOG_(__token, __ERR__)  SHIFT_PARSER_ERROR_LOG_(__token, __ERR__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_OBJECT_CLASS "shift.object"

#define SHIFT_PARSER_INT8_CLASS "shift.byte"
#define SHIFT_PARSER_CHAR_CLASS SHIFT_PARSER_INT8_CLASS
#define SHIFT_PARSER_BYTE_CLASS SHIFT_PARSER_CHAR_CLASS

#define SHIFT_PARSER_INT16_CLASS "shift.short"
#define SHIFT_PARSER_SHORT_CLASS SHIFT_PARSER_INT16_CLASS

#define SHIFT_PARSER_INT32_CLASS "shift.int"
#define SHIFT_PARSER_INT_CLASS SHIFT_PARSER_INT32_CLASS

#define SHIFT_PARSER_INT64_CLASS "shift.long"
#define SHIFT_PARSER_LONG_CLASS SHIFT_PARSER_INT64_CLASS

#define SHIFT_PARSER_UINT16_CLASS "shift.ushort"
#define SHIFT_PARSER_USHORT_CLASS SHIFT_PARSER_UINT16_CLASS

#define SHIFT_PARSER_UINT32_CLASS "shift.uint"
#define SHIFT_PARSER_UINT_CLASS SHIFT_PARSER_UINT32_CLASS

#define SHIFT_PARSER_UINT64_CLASS "shift.ulong"
#define SHIFT_PARSER_ULONG_CLASS SHIFT_PARSER_UINT64_CLASS

#define SHIFT_PARSER_SINT8_CLASS "shift.sbyte"
#define SHIFT_PARSER_SCHAR_CLASS SHIFT_PARSER_SINT8_CLASS
#define SHIFT_PARSER_SBYTE_CLASS SHIFT_PARSER_SCHAR_CLASS

#define SHIFT_PARSER_SINT16_CLASS SHIFT_PARSER_INT16_CLASS
#define SHIFT_PARSER_SSHORT_CLASS SHIFT_PARSER_SINT16_CLASS

#define SHIFT_PARSER_SINT32_CLASS SHIFT_PARSER_INT32_CLASS
#define SHIFT_PARSER_SINT_CLASS SHIFT_PARSER_SINT32_CLASS

#define SHIFT_PARSER_SINT64_CLASS SHIFT_PARSER_INT64_CLASS
#define SHIFT_PARSER_SLONG_CLASS SHIFT_PARSER_SINT64_CLASS

#define SHIFT_PARSER_FLOAT_CLASS "shift.float"
#define SHIFT_PARSER_DOUBLE_CLASS "shift.double"

#define SHIFT_PARSER_STRING_CLASS "shift.string"

#define SHIFT_PARSER_BOOLEAN_CLASS "shift.bool"
#define SHIFT_PARSER_BOOL_CLASS SHIFT_PARSER_BOOLEAN_CLASS

namespace shift {
    namespace compiler {
        using token_type = token::token_type;

        static constexpr uint_fast8_t operator_priority(const token_type type) noexcept;
        static constexpr parser::mods to_access_specifier(const token& token) noexcept;

        void parser::parse(void) {
            for (const token* current = &this->m_tokenizer->current_token(); !current->is_null_token(); current = &this->m_tokenizer->next_token()) {
                if (current->is_use()) {
                    // use statement
                    m_parse_use();
                    continue;
                }

                if (current->is_class()) {
                    // creating class, dont forget template args
                    //parse_class();
                    continue;
                }

                if (current->is_access_specifier()) {
                    m_parse_access_specifier();
                    continue;
                }

                if (current->is_module()) {
                    if (this->m_module.begin == this->m_module.end) {
                        // module statement; expected the least (only once)
                        m_parse_module();
                    } else {
                        this->m_token_error(*current, "module already defined");
                        this->m_skip_until(token::token_type::SEMICOLON);
                    }
                    continue;
                }
            }
        }

        void parser::m_parse_use(void) {
            if (this->m_mods.size() > 0) {
                this->m_token_error(*this->m_mods.front().second, "unexpected access specifier");
                this->m_skip_until(token::token_type::SEMICOLON);
                return;
            }

            const token& use_token = this->m_tokenizer->current_token();

            if (!use_token.is_use()) {
                this->m_token_error(use_token, "expected 'use'");
                this->m_skip_until(token::token_type::SEMICOLON);
                return;
            }

            this->m_tokenizer->next_token(); // skip 'use' keyword
            m_global_uses.push_back(m_parse_name("module"));

            const token& end_token = this->m_tokenizer->current_token(); // token after the module name
            if (this->m_module.size() == 0) {
                this->m_token_error(end_token.is_null_token() ? use_token : end_token, "expected module name after keyword 'use'");
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
                this->m_token_error(*this->m_mods.front().second, "unexpected access specifier");
                this->m_skip_until(token::token_type::SEMICOLON);
                return;
            }

            const token& module_token = this->m_tokenizer->current_token();

            if (!module_token.is_module()) {
                this->m_token_error(module_token, "expected 'module'");
                this->m_skip_until(token::token_type::SEMICOLON);
                return;
            }

            this->m_tokenizer->next_token(); // skip 'module' keyword
            this->m_module = m_parse_name("module");

            const token& end_token = this->m_tokenizer->current_token(); // token after the module name

            if (this->m_module.size() == 0) {
                this->m_token_error(end_token.is_null_token() ? module_token : end_token, "expected module name after keyword 'module'");
                this->m_skip_until(token::token_type::SEMICOLON);
            } else if (end_token.is_null_token()) {
                this->m_token_error(this->m_tokenizer->reverse_token(), "expected ';' before end of file");
            } else if (!end_token.is_semicolon()) {
                this->m_token_error(end_token, "unexpected '" + std::string(end_token.get_data()) + "' in module name");
                this->m_skip_until(token::token_type::SEMICOLON);
            }
        }

        /// @param name_type The type of name to be parsed. Will be displayed in error messages.
        parser::shift_name parser::m_parse_name(const char* const name_type) {
            parser::shift_name name;
            name.begin = this->m_tokenizer->get_index();

            const token& orig_token = this->m_tokenizer->reverse_peek_token();

            {
                token::token_type last_type = token::token_type(0x0);

                for (const token* token = &this->m_tokenizer->current_token(); true; token = &this->m_tokenizer->next_token()) {
                    if (token->is_access_specifier() && name.size() == 0) {
                        this->m_token_error(*token, "expected valid " + std::string(name_type) + " name after '" + std::string(orig_token.get_data()) + "'");
                    }

                    else if (token->is_identifier()) {
                        if (token->is_keyword()) {
                            // error, not keywords in module names
                            this->m_token_error(*token, std::string(name_type) + " name cannot contain keyword");
                            this->m_skip_until(token::token_type::SEMICOLON);
                            return name;
                        }

                        if (last_type == token::token_type::IDENTIFIER) {
                            // cannot have two identifiers in a row in a module name
                            this->m_token_error(*token, "unexpected identifier in " + std::string(name_type) + " name");
                            this->m_skip_until(token::token_type::SEMICOLON);
                            return name;
                        }

                        last_type = token::token_type::IDENTIFIER;
                    }

                    else if (token->get_token_type() == token::token_type::DOT) {
                        if (last_type == token::token_type::DOT || last_type != token::token_type::IDENTIFIER) {
                            // cannot have two identifiers in a row in a module name
                            this->m_token_error(*token, "unexpected '.' inside " + std::string(name_type) + " declaration");
                            this->m_skip_until(token::token_type::SEMICOLON);
                            return name;
                        }

                        last_type = token::token_type::DOT;
                    } else break;
                }
                name.end = this->m_tokenizer->get_index();

                if (last_type == token::token_type::DOT) {
                    this->m_token_error(this->m_tokenizer->reverse_peek_token(), "unexpected '.' inside " + std::string(name_type) + " declaration");
                    this->m_skip_until(token::token_type::SEMICOLON);
                    return name;
                }
            }
            return name;
        }

        void parser::m_parse_access_specifier(void) {
            const token& current_token = this->m_tokenizer->current_token();
            const mods mod = to_access_specifier(current_token);
            const mods current_mods = this->m_get_mods();

            constexpr mods visibility_specifiers = (mods::PUBLIC | mods::PROTECTED | mods::PRIVATE);

            if (((current_mods & ~visibility_specifiers) & (mod & ~visibility_specifiers)) != (current_mods & ~visibility_specifiers)) {
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

        void parser::m_token_error(const token& token_, const std::string_view msg) {
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

        static constexpr uint_fast8_t operator_priority(const token_type type) noexcept {
            switch (type) {
                case token_type::AND:
                case token_type::OR:
                case token_type::XOR:
                case token_type::SHIFT_LEFT:
                case token_type::SHIFT_RIGHT:
                case token_type::FLIP_BITS:
                    return 0x1;

                case token_type::PLUS:
                case token_type::MINUS:
                    return 0x3;

                case token_type::MULTIPLY:
                case token_type::DIVIDE:
                case token_type::MODULO:
                    return 0x5;

                case token_type::MINUS_MINUS:
                case token_type::PLUS_PLUS:
                    return 0x6;

                case token_type::AND_AND:
                case token_type::OR_OR:
                case token_type::NOT:
                case token_type::NOT_EQUAL:
                case token_type::GREATER_THAN:
                case token_type::LESS_THAN:
                    return 0x7;

                case token_type::LEFT_SQUARE_BRACKET: // array operator
                    return 0x9;

                case token_type::LEFT_BRACKET: // bracket-ed expressions
                    return 0xfe;

                case token_type::EQUALS:
                    return 0x0; // i = 5 + 3; -> should be -> (i) = (5 + 3); | if = had more priority -> (i = 5) + (3)

                default: {
                    if ((type & token_type::EQUALS) == token_type::EQUALS) {
                        return operator_priority(static_cast<token_type>(type & ~token_type::EQUALS)) - 1;
                    }
                    return 0;
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