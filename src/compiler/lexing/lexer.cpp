#include "lexer.h"
#include "utils/utils.h"

#include <fstream>
#include <cctype>
#include <algorithm>
#include <numeric>

using namespace std::string_view_literals;
using namespace std::string_literals;

#define shift_tokenizer_can_peek(__peek_count) (((i)+(__peek_count)) < (filesize))
#define shift_tokenizer_can_peek_() shift_tokenizer_can_peek(1)
#define shift_tokenizer_peek(__peek_count) ((i + (__peek_count)) >= filesize ? char(0x0) : this->m_filedata[i+(__peek_count)])
#define shift_tokenizer_peek_() shift_tokenizer_peek(1)
#define shift_tokenizer_is_whitespace(ch) is_whitespace_ext(ch, char)
#define shift_tokenizer_advance(__count) i+=__count, col+=__count, current=this->m_filedata[i]
#define shift_tokenizer_advance_() i++, col++, current=this->m_filedata[i]
#define shift_tokenizer_pre_advance(__count) shift_tokenizer_advance(__count)
#define shift_tokenizer_pre_advance_() ++i, ++col, current=this->m_filedata[i]
#define shift_tokenizer_next_line() this->m_lines.emplace_back(&this->m_filedata[last_line], i-(last_line)), last_line = i+1, line++, col = 0

#define shift_tokenizer_char_equal(__char, __eq) ((__char) == char(__eq))
#define shift_tokenizer_current_equal(__eq) shift_tokenizer_char_equal(current, __eq)
#define shift_tokenizer_reverse(__count) i-=__count, col-=__count, current=this->m_filedata[i]
#define shift_tokenizer_reverse_() shift_tokenizer_reverse(1)

#define shift_tokenizer_reverse_peek(__count) (__count) > i ? char(0x0) : chars[i-(__count)]
#define shift_tokenizer_reverse_peek_() shift_tokenizer_reverse_peek(1)

#define shift_tokenizer_get_full_line(__out) \
{\
    size_t __my_line_size = i;\
    for(;__my_line_size < filesize && !shift_tokenizer_char_equal(this->m_filedata[__my_line_size], '\n');__my_line_size++);\
    __out = std::string_view(&this->m_filedata[last_line], __my_line_size-last_line);\
}

//#define shift_tokenizer_is_hex(__char) (is_between_in(__char, char('a'), char('f')) || is_between_in(__char, char('A'), char('F')) || is_between_in(__char, char('0'), char('9')))
#define shift_tokenizer_is_hex(__char) (std::isxdigit(__char))

#define shift_tokenizer_is_binary(__char) (((__char)==char('0')) || ((__char)==char('1')))

#define SHIFT_TOKENIZER_FILE_PREFIX     (this->m_file ? std::filesystem::relative(this->m_file.raw_path()).string() \
                                            : this->m_file.raw_path().string())

#define SHIFT_TOKENIZER_ERROR_PREFIX(_line_, _col_)     "error: " << SHIFT_TOKENIZER_FILE_PREFIX << ":" << line << ":" << col << ": "
#define SHIFT_TOKENIZER_WARNING_PREFIX(_line_, _col_)   "warning: " << SHIFT_TOKENIZER_FILE_PREFIX << ":" << line << ":" << col << ": "

#define SHIFT_TOKENIZER_ERROR_LOG(__ERR__)         if(m_error_handler) err_stream << __ERR__ << '\n', err_stream.flush(error_handler::message_type::error)
#define SHIFT_TOKENIZER_FATAL_ERROR_LOG(__ERR__)  SHIFT_TOKENIZER_ERROR_LOG(__ERR__); if(m_error_handler) this->m_error_handler->print_exit_clear()

#define SHIFT_TOKENIZER_WARNING_LOG(__ERR__)         if(m_error_handler) err_stream << __ERR__ << '\n', err_stream.flush(error_handler::message_type::warning)

#define SHIFT_TOKENIZER_ERROR(_line_, _col_, _len_, __ERR__) \
if(this->m_error_handler) {\
    err_stream << SHIFT_TOKENIZER_ERROR_PREFIX(_line_, _col_) << __ERR__ << '\n'; err_stream.flush(error_handler::message_type::error);\
    std::string_view __temp_line; shift_tokenizer_get_full_line(__temp_line); SHIFT_TOKENIZER_ERROR_LOG(__temp_line);\
    err_stream << std::string((_col_)-1, ' ');\
    err_stream << std::string(_len_, '^');\
    err_stream << '\n';\
    err_stream.flush(error_handler::message_type::error);\
}

#define SHIFT_TOKENIZER_WARNING(_line_, _col_, _len_, __ERR__) \
if(this->m_error_handler) {\
    err_stream << SHIFT_TOKENIZER_WARNING_PREFIX(_line_, _col_) << __ERR__ << '\n'; err_stream.flush(error_handler::message_type::warning);\
    std::string_view __temp_line; shift_tokenizer_get_full_line(__temp_line); SHIFT_TOKENIZER_WARNING_LOG(__temp_line);\
    err_stream << std::string((_col_)-1, ' ');\
    err_stream << std::string(_len_, '^');\
    err_stream << '\n';\
    err_stream.flush(error_handler::message_type::warning);\
}

#define SHIFT_TOKENIZER_FATAL_ERROR(_line_, _col_, _len_, __ERR__)         SHIFT_TOKENIZER_ERROR(_line_, _col_, _len_, __ERR__); if(m_error_handler) this->m_error_handler->print_exit_clear()

/** Namespace shift */
namespace shift::compiler::lexing {
    SHIFT_API const token& lexer::token_at(const file_position index) const noexcept {
        auto pos = position_at(index);
        if (pos == this->m_tokens.cend()) return token::eof;
        return *pos;
    }

    SHIFT_API const token& lexer::token_before(const file_position index) const noexcept {
        auto pos_before = position_before(index);
        if (pos_before == this->m_tokens.cend()) return token::eof;
        return *pos_before;
    }

    SHIFT_API const token& lexer::token_after(const file_position index) const noexcept {
        auto pos_after = position_after(index);
        if (pos_after == this->m_tokens.cend()) return token::eof;
        return *pos_after;
    }

    SHIFT_API lexer::const_iterator lexer::position_at(const file_position index) const noexcept {
        // TODO may have to change if we allow tokens to take up more than one line (e.g. multi line strings)
        auto it = std::lower_bound(this->m_tokens.cbegin(), this->m_tokens.cend(), index,
            [](const token& token, const file_position index) {
                auto token_indexer = token.get_file_position();
                return token_indexer.line == index.line ? token_indexer.col + token.get_data().length() <= index.col : token_indexer.line <
                                                                                                                       index.line;
            });
        if (it == this->m_tokens.cend()) { return this->m_tokens.cend(); }

        auto it_indexer = it->get_file_position();

        if (index.col < it_indexer.col || index.col >= it_indexer.col + it->get_data().length()) { return this->m_tokens.cend(); }

        return it;
    }

    SHIFT_API lexer::const_iterator lexer::position_before(const file_position index) const noexcept {
        auto it = std::lower_bound(this->m_tokens.cbegin(), this->m_tokens.cend(), index,
            [](const token& token, const file_position index) {
                return token.get_file_position() < index;
            });

        if (it == this->m_tokens.cbegin() || it == this->m_tokens.cend()) return this->m_tokens.cend();
        return --it;
    }

    SHIFT_API lexer::const_iterator lexer::position_after(const file_position index) const noexcept {
        auto it = std::upper_bound(this->m_tokens.cbegin(), this->m_tokens.cend(), index,
            [](const file_position index, const token& token) {
                return index < token.get_file_position();
            });

        if (it == this->m_tokens.cend()) return this->m_tokens.cend();
        return it;
    }

    SHIFT_API void lexer::tokenize() {
        // Clear all class data in case this function has been called more than once
        this->m_tokens.clear();
        this->m_lines.clear();

        if (this->m_file) {
            this->m_filedata = this->m_file.read_fully();
        }

        const std::uintmax_t filesize = this->m_filedata.size();

        { // tokenizing
            size_t last_line = 0; // index of character after last \n
            char current = this->m_filedata.empty() ? char(0x0) : this->m_filedata[0]; // Current character (i.e. cursor)
            size_t i, line, col; // index (starts at 0), line # (starts at 1), column # (starts at 1)
            error_stream err_stream(*m_error_handler);
            this->m_lines.reserve(
                std::reduce(this->m_filedata.begin(), this->m_filedata.end(), size_t(1),
                    [](size_t acc, char a) {
                        return acc + (a == '\n');
                    }));
            for (i = 0, line = 1, col = 1; i < filesize; shift_tokenizer_advance_()) {
                if (shift_tokenizer_is_whitespace(current)) {
                    if (shift_tokenizer_current_equal('\n')) {
                        shift_tokenizer_next_line();
                        // col++; // col will be incremented to 1 by shift_tokenizer_advance_() in the for loop
                    } else if (shift_tokenizer_current_equal('\t')) {
                        col += get_tab_size() - 1;
                    }
                    continue;
                }

                if (std::isalpha(current) || shift_tokenizer_current_equal('_')) {
                    const size_t old_col = col;
                    const size_t old_i = i;

                    for (shift_tokenizer_pre_advance_(); (i < filesize) && (std::isalnum(current) || shift_tokenizer_current_equal('_'));
                         shift_tokenizer_advance_());

                    shift_tokenizer_reverse_();
                    m_tokens.push_back(
                        token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::IDENTIFIER, { line,
                                                                                                                    old_col }));
                    continue;
                }

                if (std::isdigit(current)) {
                    const size_t old_col = col;
                    const size_t old_i = i;

                    for (shift_tokenizer_pre_advance_(); i < filesize && isdigit(current); shift_tokenizer_advance_());

                    if ((shift_tokenizer_current_equal('b') || shift_tokenizer_current_equal('B'))
                        && ((i - old_i) == 1 && this->m_filedata[old_i] == char('0'))) {
                        // binary number
                        for (shift_tokenizer_pre_advance_();
                             i < filesize && shift_tokenizer_is_binary(current); shift_tokenizer_advance_());

                        if ((i - old_i) == 2) {
                            if (this->m_error_handler) {
                                SHIFT_TOKENIZER_ERROR(line, col, 1, "Expected binary digit (bit), got '" << current << "'");
                            }
                        }

                        shift_tokenizer_reverse_();
                        m_tokens.push_back(
                            token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::BINARY_LITERAL, {
                                line, old_col }));
                    } else if ((shift_tokenizer_current_equal('x') || shift_tokenizer_current_equal('X'))
                               && ((i - old_i) == 1 && this->m_filedata[old_i] == char('0'))) {
                        // hex number
                        for (shift_tokenizer_pre_advance_(); i < filesize && shift_tokenizer_is_hex(current); shift_tokenizer_advance_());

                        if ((i - old_i) == 2) {
                            if (this->m_error_handler) {
                                SHIFT_TOKENIZER_ERROR(line, col, 1, "Expected hexadecimal digit, got '" << current << "'");
                            }
                        }

                        shift_tokenizer_reverse_();
                        m_tokens.push_back(
                            token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::HEX_LITERAL, { line,
                                                                                                                         old_col }));
                    } else if (shift_tokenizer_current_equal('.')) {
                        for (shift_tokenizer_pre_advance_(); i < filesize && isdigit(current); shift_tokenizer_advance_());

                        if (shift_tokenizer_current_equal('f') || shift_tokenizer_current_equal('F')) {
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::FLOAT_LITERAL, { line,
                                                                                                                               old_col }));
                        } else if (shift_tokenizer_current_equal('d') || shift_tokenizer_current_equal('D')) {
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::DOUBLE_LITERAL, { line,
                                                                                                                                old_col }));
                        } else {
                            shift_tokenizer_reverse_();
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::FLOAT_LITERAL, { line,
                                                                                                                               old_col }));
                        }

                    } else if (shift_tokenizer_current_equal('f') || shift_tokenizer_current_equal('F')) {
                        m_tokens.push_back(
                            token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::FLOAT_LITERAL, { line,
                                                                                                                           old_col }));
                    } else if (shift_tokenizer_current_equal('d') || shift_tokenizer_current_equal('D')) {
                        m_tokens.push_back(
                            token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::DOUBLE_LITERAL, { line,
                                                                                                                            old_col }));
                    } else {
                        shift_tokenizer_reverse_();
                        m_tokens.push_back(
                            token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::INTEGER_LITERAL, {
                                line, old_col }));
                    }
                    continue;
                }

                if (shift_tokenizer_current_equal(';')) {
                    m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::SEMICOLON, { line, col }));
                    continue;
                }

                if (shift_tokenizer_current_equal('!')) {
                    if (shift_tokenizer_char_equal(shift_tokenizer_peek_(), '=')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::NOT_EQUAL, { line, col }));
                        shift_tokenizer_advance_();
                    } else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::NOT, { line, col }));
                    }
                    continue;
                }

                if (shift_tokenizer_current_equal('{')) {
                    m_tokens.push_back(
                        token(std::string_view(&this->m_filedata[i], 1), token::type::LEFT_SCOPE_BRACKET, { line, col }));
                    continue;
                }

                if (shift_tokenizer_current_equal('}')) {
                    m_tokens.push_back(
                        token(std::string_view(&this->m_filedata[i], 1), token::type::RIGHT_SCOPE_BRACKET, { line, col }));
                    continue;
                }

                if (shift_tokenizer_current_equal('(')) {
                    m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::LEFT_BRACKET, { line, col }));
                    continue;
                }

                if (shift_tokenizer_current_equal(')')) {
                    m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::RIGHT_BRACKET, { line, col }));
                    continue;
                }

                if (shift_tokenizer_current_equal('[')) {
                    m_tokens.push_back(
                        token(std::string_view(&this->m_filedata[i], 1), token::type::LEFT_SQUARE_BRACKET, { line, col }));
                    continue;
                }

                if (shift_tokenizer_current_equal(']')) {
                    m_tokens.push_back(
                        token(std::string_view(&this->m_filedata[i], 1), token::type::RIGHT_SQUARE_BRACKET, { line, col }));
                    continue;
                }

                if (shift_tokenizer_current_equal('.')) {
                    if (isdigit(shift_tokenizer_peek_())) {
                        const size_t old_col = col;
                        const size_t old_i = i;

                        // We already know the next character is a digit
                        for (shift_tokenizer_pre_advance(2); i < filesize && isdigit(current); shift_tokenizer_advance_());

                        if (shift_tokenizer_current_equal('f') || shift_tokenizer_current_equal('F')) {
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::FLOAT_LITERAL, { line,
                                                                                                                               old_col }));
                        } else if (shift_tokenizer_current_equal('d') || shift_tokenizer_current_equal('D')) {
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::DOUBLE_LITERAL, { line,
                                                                                                                                old_col }));
                        } else {
                            shift_tokenizer_reverse_();
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::FLOAT_LITERAL, { line,
                                                                                                                               old_col }));
                        }
                    } else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::DOT, { line, col }));
                    }
                    continue;
                }

                if (shift_tokenizer_current_equal('=')) {
                    const char next = shift_tokenizer_peek_();

                    // CHECK FOR GREATHER THAN, LESS THAN, MODULO, !=, ETCCCCCC
                    //
                    // (actually, not != or -=, since it could be:  "int i =! varName;" = "int i = !varName;" or "int i =- varName;" = "int i = -varName;")

                    if (shift_tokenizer_char_equal(next, '=')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::EQUALS_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next, '%')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::MODULO_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next,
                        '*')) // If pointers are added into the language, =* might count as a dereferencing and not *=
                    {
                        m_tokens.push_back(
                            token(std::string_view(&this->m_filedata[i], 2), token::type::MULTIPLY_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next,
                        '&')) // If pointers are added into the language, =& might count as 'getting a pointer to' and not &=
                    {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::AND_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next, '|')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::OR_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next, '^')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::XOR_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next, '<')) {
                        if (shift_tokenizer_char_equal(shift_tokenizer_peek(2), '<')) {
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[i], 3), token::type::SHIFT_LEFT_EQUALS, { line, col }));
                            shift_tokenizer_advance(2);
                        } else {
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[i], 2), token::type::LESS_THAN_OR_EQUAL, { line, col }));
                            shift_tokenizer_advance_();
                        }

                    } else if (shift_tokenizer_char_equal(next, '>')) {
                        if (shift_tokenizer_char_equal(shift_tokenizer_peek(2), '>')) {
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[i], 3), token::type::SHIFT_RIGHT_EQUALS, { line, col }));
                            shift_tokenizer_advance(2);
                        } else {
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[i], 2), token::type::GREATER_THAN_OR_EQUAL, { line,
                                                                                                                       col }));
                            shift_tokenizer_advance_();
                        }

                    } else if (shift_tokenizer_char_equal(next, '/')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::DIVIDE_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next, '+') && !shift_tokenizer_char_equal(shift_tokenizer_peek(2), '+')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::PLUS_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    }
                        //
                        //                    Cannot transform =- to -=
                        //                    Reason:
                        //                    i =- 3; // -= 3 OR = -3 ? (since white spaces are ignored)
                        //
                        //                    else if (shift_tokenizer_char_equal(next, '-')) {
                        //                        m_tokens.push_back(
                        //                                token(std::string_view({next}) + current, token::type::MINUS_EQUALS, {line, col}));
                        //                        shift_tokenizer_advance_();
                        //                    }
                    else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::EQUALS, { line, col }));
                    }
                    continue;
                }

                if (shift_tokenizer_current_equal('&')) {
                    const char next = shift_tokenizer_peek_();
                    if (shift_tokenizer_char_equal(next, '&')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::AND_AND, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next, '=')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::AND_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::AND, { line, col }));
                    }
                    continue;
                }

                if (shift_tokenizer_current_equal('|')) {
                    const char next = shift_tokenizer_peek_();
                    if (shift_tokenizer_char_equal(next, '|')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::OR_OR, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next, '=')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::OR_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::OR, { line, col }));
                    }

                    continue;
                }

                if (shift_tokenizer_current_equal('^')) {
                    if (shift_tokenizer_char_equal(shift_tokenizer_peek_(), '=')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::XOR_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::XOR, { line, col }));
                    }

                    continue;
                }

                if (shift_tokenizer_current_equal('?')) {
                    m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::QUESTION_MARK, { line, col }));
                    continue;
                }

                if (shift_tokenizer_current_equal('~')) {
                    m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::FLIP_BITS, { line, col }));
                    continue;
                }

                if (shift_tokenizer_current_equal('\\')) {
                    m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::BACKSLASH, { line, col }));
                    continue;
                }

                if (shift_tokenizer_current_equal(':')) {
                    m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::COLON, { line, col }));
                    continue;
                }

                if (shift_tokenizer_current_equal(',')) {
                    m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::COMMA, { line, col }));
                    continue;
                }

                if (shift_tokenizer_current_equal('-')) {
                    const char next = shift_tokenizer_peek_();

                    if (shift_tokenizer_char_equal(next, '=')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::MINUS_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next, '-')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::MINUS_MINUS, { line, col }));
                        shift_tokenizer_advance_();
                    } else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::MINUS, { line, col }));
                    }
                    continue;
                }

                if (shift_tokenizer_current_equal('+')) {
                    const char next = shift_tokenizer_peek_();
                    if (shift_tokenizer_char_equal(next, '=')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::PLUS_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next, '+')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::PLUS_PLUS, { line, col }));
                        shift_tokenizer_advance_();
                    } else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::PLUS, { line, col }));
                    }

                    continue;
                }

                if (shift_tokenizer_current_equal('*')) {
                    if (shift_tokenizer_char_equal(shift_tokenizer_peek_(), '=')) {
                        m_tokens.push_back(
                            token(std::string_view(&this->m_filedata[i], 2), token::type::MULTIPLY_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::MULTIPLY, { line, col }));
                    }

                    continue;
                }

                if (shift_tokenizer_current_equal('>')) {
                    const char next = shift_tokenizer_peek_();
                    if (shift_tokenizer_char_equal(next, '=')) {
                        m_tokens.push_back(
                            token(std::string_view(&this->m_filedata[i], 2), token::type::GREATER_THAN_OR_EQUAL, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next, '>')) {
                        if (shift_tokenizer_char_equal(shift_tokenizer_peek(2), '=')) {
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[i], 3), token::type::SHIFT_RIGHT_EQUALS, { line, col }));
                            shift_tokenizer_advance(2);
                        } else {
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[i], 2), token::type::SHIFT_RIGHT, { line, col }));
                            shift_tokenizer_advance_();
                        }
                    } else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::GREATER_THAN, { line, col }));
                    }

                    continue;
                }

                if (shift_tokenizer_current_equal('<')) {
                    const char next = shift_tokenizer_peek_();
                    if (shift_tokenizer_char_equal(next, '=')) {
                        m_tokens.push_back(
                            token(std::string_view(&this->m_filedata[i], 2), token::type::LESS_THAN_OR_EQUAL, { line, col }));
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next, '<')) {
                        if (shift_tokenizer_char_equal(shift_tokenizer_peek(2), '=')) {
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[i], 3), token::type::SHIFT_LEFT_EQUALS, { line, col }));
                            shift_tokenizer_advance(2);
                        } else {
                            m_tokens.push_back(
                                token(std::string_view(&this->m_filedata[i], 2), token::type::SHIFT_LEFT, { line, col }));
                            shift_tokenizer_advance_();
                        }
                    } else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::LESS_THAN, { line, col }));
                    }

                    continue;
                }

                if (shift_tokenizer_current_equal('%')) {
                    if (shift_tokenizer_char_equal(shift_tokenizer_peek_(), '=')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::MODULO_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::MODULO, { line, col }));
                    }

                    continue;
                }

                if (shift_tokenizer_current_equal('/')) {
                    const char next = shift_tokenizer_peek_();
                    if (shift_tokenizer_char_equal(next, '/')) {
                        // single line comment, loop until line is finished
                        for (shift_tokenizer_pre_advance(2);
                             i < filesize && !shift_tokenizer_current_equal('\n'); shift_tokenizer_advance_());
                        shift_tokenizer_next_line();
                    } else if (shift_tokenizer_char_equal(next, '*')) {
                        // Multi line comment, loop until next "*/"
                        for (shift_tokenizer_pre_advance(2);
                             i < filesize &&
                             !(shift_tokenizer_current_equal('*') && shift_tokenizer_char_equal(shift_tokenizer_peek_(), '/'));
                             shift_tokenizer_advance_()) {
                            if (shift_tokenizer_current_equal('\n')) {
                                shift_tokenizer_next_line();
                            } else if (shift_tokenizer_current_equal('\t')) {
                                col += 3; // tabs are 4 spaces. col with be incremented the 4th time by shift_tokenizer_advance_() in the for loop
                            }
                        }
                        shift_tokenizer_advance_();
                    } else if (shift_tokenizer_char_equal(next, '=')) {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::type::DIVIDE_EQUALS, { line, col }));
                        shift_tokenizer_advance_();
                    } else {
                        m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::type::DIVIDE, { line, col }));
                    }
                    continue;
                }

                if (shift_tokenizer_current_equal('"')) {
                    bool string_end = false;

                    const size_t old_col = col;
                    const size_t old_i = i;

                    for (shift_tokenizer_pre_advance_(); i < filesize; shift_tokenizer_advance_()) {
                        if (shift_tokenizer_current_equal('\\')) {
                            if (!shift_tokenizer_can_peek_()) {
                                // error, unfinished string
                                break;
                            }

                            if (shift_tokenizer_char_equal(shift_tokenizer_peek_(), '\n')) {
                                // error, no new lines
                                break;
                            }
                            shift_tokenizer_advance_();

                            switch (std::tolower(current)) {
                                case 'a':
                                case 'b':
                                case 'f':
                                case 'n':
                                case 'r':
                                case 't':
                                case 'v':
                                case '\\':
                                case '\'':
                                case '"':
                                    break;
                                default:
                                    SHIFT_TOKENIZER_ERROR(line, col - 1, 2, "Unknown escape sequence");
                                    break;
                            }

                            continue;
                        }

                        if (shift_tokenizer_current_equal('\n')) {
                            // error, no new lines allowed inside a string
                            break;
                        }

                        if (shift_tokenizer_current_equal('"')) {
                            string_end = true;
                            break;
                        }
                    }

                    if (!string_end) {
                        // error, unfinished string
                        if (this->m_error_handler) {
                            SHIFT_TOKENIZER_ERROR(line, old_col, i - old_i + 1, "string literal must be terminated");
                        }
                    }

                    m_tokens.push_back(
                        token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::STRING_LITERAL, { line,
                                                                                                                        old_col }));
                    continue;
                }

                if (shift_tokenizer_current_equal('\'')) {
                    const size_t old_col = col;
                    const size_t old_i = i;

                    shift_tokenizer_advance_();

                    if (shift_tokenizer_current_equal('\\')) {
                        if (shift_tokenizer_can_peek_()) {
                            shift_tokenizer_advance_(); // advance only once so below it can advance and then check for \'
                            switch (std::tolower(current)) {
                                case 'a':
                                case 'b':
                                case 'f':
                                case 'n':
                                case 'r':
                                case 't':
                                case 'v':
                                case '\\':
                                case '\'':
                                case '"':
                                    break;
                                default:
                                    SHIFT_TOKENIZER_ERROR(line, col - 1, 2, "Unknown escape sequence");
                                    break;
                            }
                        }
                    } else if (shift_tokenizer_current_equal('\'')) {
                        if (this->m_error_handler) {
                            SHIFT_TOKENIZER_ERROR(line, col, 1, "Character literal cannot be empty");
                        }
                        continue;
                    }

                    shift_tokenizer_advance_();

                    if (!shift_tokenizer_current_equal('\'')) {
                        if (this->m_error_handler) {
                            SHIFT_TOKENIZER_ERROR(line, col, 1, "Expected ''', got '" << current << "'");
                        }
                    }

                    m_tokens.push_back(
                        token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::type::CHAR_LITERAL, { line,
                                                                                                                      old_col }));
                    continue;
                }

                // ALL OTHER CHARACTERS
                if (this->m_error_handler) {
                    SHIFT_TOKENIZER_ERROR(line, col, 1, "Unexpected symbol: '" << current << "'");
                }
            }
            shift_tokenizer_next_line();
        }
    }

    [[nodiscard]] token_stream::const_iterator token_stream::peek_position(difference_type count) const noexcept {
        const auto index = std::distance(m_begin, m_cursor);
        const auto len = std::distance(m_cursor, m_end);
        count = std::clamp(count, -index, len);
        return std::next(m_cursor, count);
    }


    [[nodiscard]] const token& token_stream::as_token(token_stream::const_iterator pos) const noexcept {
        return pos == m_end ? token::eof : *pos;
    }
}
