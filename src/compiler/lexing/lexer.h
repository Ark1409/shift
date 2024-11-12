#ifndef SHIFT_TOKENIZER_H_
#define SHIFT_TOKENIZER_H_ 1

#include "shift_config.h"
#include "utils/utils.h"
#include "compiler/lexing/token.h"
#include "compiler/marker.h"

#include "filesystem/file.h"

#include "compiler/shift_error_handler.h"

#include <type_traits>

namespace shift::compiler {
    namespace lexing {
        class lexer {
        public:
            typedef std::vector<token>::const_iterator iterator;
            typedef std::vector<token>::const_iterator const_iterator;
            typedef std::vector<token>::size_type size_type;
            typedef std::vector<token>::difference_type difference_type;
        public:
            inline lexer(error_handler*, const filesystem::file&);

            inline lexer(error_handler*, filesystem::file&&);

            inline lexer(error_handler*, const std::string& file_data);

            inline lexer(error_handler*, std::string&& file_data);

            SHIFT_API void tokenize();

            inline const_iterator begin() const noexcept { return this->m_tokens.begin(); }

            inline const_iterator end() const noexcept { return this->m_tokens.end(); }

            inline const token& operator[](const size_type index) const { return this->m_tokens.at(index); }

            SHIFT_API const token& token_at(file_position index) const noexcept;

            SHIFT_API const token& token_before(file_position index) const noexcept;

            SHIFT_API const token& token_after(file_position index) const noexcept;

            SHIFT_API const_iterator position_at(file_position index) const noexcept;

            SHIFT_API const_iterator position_before(file_position index) const noexcept;

            SHIFT_API const_iterator position_after(file_position index) const noexcept;

            inline const_iterator position_at(const token& tok) const noexcept { return position_at(tok.get_file_index()); }

            inline const_iterator position_before(const token& tok) const noexcept {
                return position_before(tok.get_file_index());
            }

            inline const_iterator position_after(const token& tok) const noexcept {
                return position_after(tok.get_file_index());
            }

            inline const token& token_at(const const_iterator it) const noexcept {
                return it == this->m_tokens.cend() ? token::eof : *it;
            }

            inline const token& token_before(const_iterator it) const noexcept {
                return it == this->m_tokens.cbegin() ? token::eof : token_at(--it);
            }

            inline const token& token_after(const_iterator it) const noexcept {
                return it == this->m_tokens.cend() ? token::eof : token_at(++it);
            }

            inline const filesystem::file& get_file() const noexcept { return m_file; }

            inline const std::vector<std::string_view>& get_lines() const noexcept { return this->m_lines; }

            inline const std::vector<token>& get_tokens() const noexcept { return this->m_tokens; }

            inline error_handler* get_error_handler() noexcept {
                return m_error_handler;
            }

            inline const error_handler* get_error_handler() const noexcept {
                return m_error_handler;
            }

        private:
            error_handler* m_error_handler;
            filesystem::file m_file{ std::string_view("<internal>") };
            std::string m_filedata;
            std::vector<std::string_view> m_lines;
            std::vector<token> m_tokens;
        };

        inline lexer::lexer(error_handler* const handler, const filesystem::file& file) : m_error_handler(handler),
                                                                                          m_file(file) {}

        inline lexer::lexer(error_handler* const handler, filesystem::file&& file) : m_error_handler(handler),
                                                                                     m_file(std::move(file)) {}

        inline lexer::lexer(error_handler* const handler, const std::string& file_data) : m_error_handler(handler),
                                                                                          m_filedata(file_data) {}

        inline lexer::lexer(error_handler* const handler, std::string&& file_data) : m_error_handler(handler),
                                                                                     m_filedata(std::move(file_data)) {}

        class token_stream {
        public:
            typedef lexer::iterator iterator;
            typedef lexer::const_iterator const_iterator;
            typedef lexer::size_type size_type;
            typedef lexer::difference_type difference_type;
        public:
            token_stream(const lexer& l) : m_source(&l), m_begin(l.begin()), m_end(l.end()), m_cursor(l.begin()) {}

            token_stream(const lexer&& l) = delete;

            auto begin() const noexcept { return m_cursor; }

            auto end() const noexcept { return m_end; }

            const lexer& get_source() const noexcept { return *m_source; }

            const token& current_token() const noexcept { return as_token(m_cursor); }

            inline const token& next_token(difference_type count = 1) noexcept {
                return as_token(m_cursor = peek_position(count));
            }

            inline const token& reverse_token(difference_type count = 1) noexcept { return next_token(-count); }

            [[nodiscard]] inline const token& peek_token(difference_type count = 1) const noexcept {
                return as_token(peek_position(count));
            }

            [[nodiscard]] inline const token& reverse_peek_token(difference_type count = 1) const noexcept { return peek_token(-count); }

            inline const_iterator get_position() const noexcept { return this->m_cursor; }

            inline void set_position(const_iterator pos) noexcept { this->m_cursor = pos; }

            inline const token& operator[](difference_type diff) const noexcept { return peek_token(diff); }

        private:
            [[nodiscard]] const token& as_token(const_iterator pos) const noexcept;

            [[nodiscard]] const_iterator peek_position(difference_type count) const noexcept;

        private:
            const lexer* m_source{ nullptr };
            const const_iterator m_begin, m_end;
            const_iterator m_cursor;

            friend struct marker<token_stream>;
        };
    }

    template<>
    struct marker<lexing::token_stream> : marker_helper<lexing::token_stream, lexing::token_stream::const_iterator> {
        explicit marker(lexing::token_stream& l) : marker_helper(l) {}

        inline void mark() { return this->m_marks.push(this->m_markee.m_cursor); }

        void rollback() {
            if (this->m_marks.empty()) return;

            this->m_markee.m_cursor = this->m_marks.top();
            this->m_marks.pop();
        }
    };
}


#endif /* SHIFT_TOKENIZER_H_ */
