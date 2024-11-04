/**
 * @file compiler/shift_tokenizer.h
 */
#ifndef SHIFT_TOKENIZER_H_
#define SHIFT_TOKENIZER_H_ 1

#include "shift_config.h"
#include "utils/utils.h"

#include "filesystem/file.h"

#include "compiler/shift_error_handler.h"

#include <type_traits>

/** Namespace shift */
namespace shift::compiler {
    struct file_indexer {
        size_t line{}, col{};

        constexpr bool operator==(const file_indexer other) const noexcept {
            return this->col == other.col && this->line == other.line;
        }

        constexpr bool operator!=(const file_indexer& other) const noexcept { return !this->operator==(other); }

        constexpr bool operator>(const file_indexer other) const noexcept {
            return this->line == other.line ? this->col > other.col : this->line > other.line;
        }

        constexpr bool operator<(const file_indexer other) const noexcept {
            return this->line == other.line ? this->col < other.col : this->line < other.line;
        }

        constexpr bool operator>=(const file_indexer& other) const noexcept { return !this->operator<(other); }

        constexpr bool operator<=(const file_indexer& other) const noexcept { return !this->operator>(other); }

        constexpr std::strong_ordering operator<=>(const file_indexer& other) const noexcept = default;
    };

    struct token {
    public:
        /**
         * Structure of token types:
         *                (16 bits)
         *   [    00000000           00000000    ]
         *      for equals       for normal token
         *     combinations        types, e.g
         *     e.g. +=, &=,      +, ., [, <<, etc
         *       <<=, etc	     incremented by 1
         *      and certain
         *   other combinations
         *    e.g. ||, &&, ++,
         *         etc;
         *   act as bit-fields
         *
         */
        enum class type : uint_fast16_t {
            IDENTIFIER = 1, // [a-zA-Z_][a-zA-Z0-9_]*
            INTEGER_LITERAL, // [-]?[0-9]+
            BINARY_LITERAL, // 0(b|B)[0|1]+
            HEX_LITERAL, // 0(x|X)[0-9a-fA-F]+
            FLOAT_LITERAL,  // [0-9]*[.][0-9]+(f|F)?
            DOUBLE_LITERAL,  // [0-9]*[.][0-9]+(d|D)?
            GREATER_THAN, // >
            LESS_THAN, // <
            MODULO, // %
            OR, // |
            AND, // &
            XOR, // ^
            FLIP_BITS, // ~
            BIT_FLIP = FLIP_BITS,
            TILDE = FLIP_BITS,
            NOT, // !
            EXCLAMATION_MARK = NOT, // !
            PLUS, // +
            MINUS, // -
            MULTIPLY, // *
            STAR = MULTIPLY,
            ARROW, // ->
            DIVIDE, // /
            LEFT_BRACKET, // (
            RIGHT_BRACKET, // )
            LEFT_SQUARE_BRACKET, // [
            RIGHT_SQUARE_BRACKET, // ]
            LEFT_SCOPE_BRACKET, // {
            RIGHT_SCOPE_BRACKET, // }
            DOT, // .
            PERIOD = DOT, // .
            COMMA, // ,
            QUESTION_MARK, // ?
            COLON, // :
            SEMICOLON, // ;
            STRING_LITERAL, // ".*"
            CHAR_LITERAL, // '(.|\\.)'
            SHIFT_LEFT, // <<
            SHIFT_RIGHT, // >>
            BACKSLASH, // \  <--

            EQUALS = 1 << 15, // =
            EQUALS_EQUALS = (1 << 14) | EQUALS, // ==
            GREATER_THAN_OR_EQUAL = GREATER_THAN | EQUALS, // [>=| =>]
            LESS_THAN_OR_EQUAL = LESS_THAN | EQUALS, // [<= | =<]
            MODULO_EQUALS = MODULO | EQUALS, // %=
            OR_EQUALS = OR | EQUALS, // |=
            OR_OR = (1 << 13) | OR, // ||
            AND_EQUALS = AND | EQUALS, // &=
            AND_AND = (1 << 12) | AND, // &&
            XOR_EQUALS = XOR | EQUALS, // ^=
            NOT_EQUAL = NOT | EQUALS, // !=
            PLUS_EQUALS = PLUS | EQUALS, // +=
            PLUS_PLUS = (1 << 11) | PLUS, // ++
            MINUS_EQUALS = MINUS | EQUALS, // -=
            MINUS_MINUS = (1 << 10) | MINUS, // --
            MULTIPLY_EQUALS = MULTIPLY | EQUALS, // *=
            DIVIDE_EQUALS = DIVIDE | EQUALS, // /=
            SHIFT_LEFT_EQUALS = SHIFT_LEFT | EQUALS, // <<=
            SHIFT_RIGHT_EQUALS = SHIFT_RIGHT | EQUALS, // >>=

            NULL_TOKEN = 0x0 // no token
        };
    public:
        static const token null;
    public:
        constexpr token() noexcept = default;

        inline token(const std::string& str, type type, file_indexer index) noexcept;

        constexpr token(std::string_view str, type type, file_indexer index) noexcept;

        token(const std::string&& str, type type, file_indexer index) noexcept = delete;

        constexpr bool operator==(const token& other) const noexcept {
            return this->m_type == other.m_type && this->m_index == other.m_index && this->m_data == other.m_data;
        }

        constexpr bool operator==(const std::string_view other) const noexcept { return this->m_data == other; }

        inline bool operator==(const std::string& other) const noexcept { return this->m_data == other; }

        constexpr bool operator!=(const token& other) const noexcept { return !this->operator==(other); }

        constexpr bool operator!=(const std::string_view& other) const noexcept { return !this->operator==(other); }

        inline bool operator!=(const std::string& other) const noexcept {
            return !this->operator==(std::string_view(other.c_str(), other.length()));
        }

        constexpr bool operator>(const token& other) const noexcept { return this->m_index > other.m_index; }

        constexpr bool operator<(const token& other) const noexcept { return this->m_index < other.m_index; }

        constexpr std::string_view get_data() const noexcept { return this->m_data; }

        constexpr file_indexer get_file_index() const noexcept { return this->m_index; }

        constexpr type get_token_type() const noexcept { return this->m_type; }

        constexpr explicit operator std::string_view() const noexcept { return this->m_data; }

        constexpr explicit operator type() const noexcept { return this->m_type; }

        constexpr explicit operator file_indexer() const noexcept { return this->m_index; }

        constexpr bool is_cp() const noexcept { return ((this->is_identifier()) && (this->m_data == "cp")); }

        constexpr bool is_mv() const noexcept { return ((this->is_identifier()) && (this->m_data == "mv")); }

        constexpr bool is_var() const noexcept { return ((this->is_identifier()) && (this->m_data == "var")); }

        constexpr bool is_null() const noexcept { return ((this->is_identifier()) && (this->m_data == "null")); }

        constexpr bool is_module() const noexcept { return ((this->is_identifier()) && (this->m_data == "module")); }

        // unsused
        constexpr bool is_namespace() const noexcept { return ((this->is_identifier()) && (this->m_data == "namespace")); }

        constexpr bool is_const() const noexcept { return ((this->is_identifier()) && (this->m_data == "const")); }

        constexpr bool is_public() const noexcept { return ((this->is_identifier()) && (this->m_data == "public")); }

        constexpr bool is_protected() const noexcept { return ((this->is_identifier()) && (this->m_data == "protected")); }

        constexpr bool is_private() const noexcept { return ((this->is_identifier()) && (this->m_data == "private")); }

        constexpr bool is_static() const noexcept { return ((this->is_identifier()) && (this->m_data == "static")); }

        // currently unused
        constexpr bool is_binary() const noexcept { return ((this->is_identifier()) && (this->m_data == "binary")); }

        constexpr bool is_void() const noexcept { return ((this->is_identifier()) && (this->m_data == "void")); }

        // unsused
        constexpr bool is_req() const noexcept { return ((this->is_identifier()) && (this->m_data == "req")); }

        constexpr bool is_use() const noexcept { return ((this->is_identifier()) && (this->m_data == "use")); }


        constexpr bool is_unsafe() const noexcept { return ((this->is_identifier()) && (this->m_data == "unsafe")); }

        constexpr bool is_extern() const noexcept {
            return ((this->is_identifier()) && (this->m_data == "extern" || this->m_data == "ext"));
        }

        constexpr bool is_class() const noexcept { return (this->is_identifier()) && (this->m_data == "class"); }

        constexpr bool is_init() const noexcept { return (this->is_identifier()) && (this->m_data == "init"); }

        constexpr bool is_ref() const noexcept { return (this->is_identifier()) && (this->m_data == "ref"); }

        constexpr bool is_tref() const noexcept { return (this->is_identifier()) && (this->m_data == "tref"); }

        constexpr bool is_auto() const noexcept { return (this->is_identifier()) && (this->m_data == "auto"); }

        constexpr bool is_imut() const noexcept { return (this->is_identifier()) && (this->m_data == "imut"); }

        constexpr bool is_operator() const noexcept { return (this->is_identifier()) && (this->m_data == "operator"); }

        constexpr bool is_constructor() const noexcept { return (this->is_identifier()) && (this->m_data == "constructor"); }

        constexpr bool is_destructor() const noexcept { return (this->is_identifier()) && (this->m_data == "destructor"); }

        constexpr bool is_if() const noexcept { return (this->is_identifier()) && (this->m_data == "if"); }

        constexpr bool is_else() const noexcept { return (this->is_identifier()) && (this->m_data == "else"); }

        constexpr bool is_while() const noexcept { return (this->is_identifier()) && (this->m_data == "while"); }

        constexpr bool is_do() const noexcept { return (this->is_identifier()) && (this->m_data == "do"); }

        constexpr bool is_return() const noexcept { return (this->is_identifier()) && (this->m_data == "return"); }

        constexpr bool is_continue() const noexcept { return (this->is_identifier()) && (this->m_data == "continue"); }

        constexpr bool is_break() const noexcept { return (this->is_identifier()) && (this->m_data == "break"); }

        constexpr bool is_for() const noexcept { return (this->is_identifier()) && (this->m_data == "for"); }

        constexpr bool is_this() const noexcept { return (this->is_identifier()) && (this->m_data == "this"); }

        constexpr bool is_base() const noexcept { return (this->is_identifier()) && (this->m_data == "base"); }

        constexpr bool is_new() const noexcept { return (this->is_identifier()) && (this->m_data == "new"); }

        constexpr bool is_del() const noexcept { return (this->is_identifier()) && (this->m_data == "del"); }

        constexpr bool is_throw() const noexcept { return (this->is_identifier()) && (this->m_data == "throw"); }

        constexpr bool is_explicit() const noexcept { return (this->is_identifier()) && (this->m_data == "explicit"); }

        constexpr bool is_access_specifier() const noexcept {
            return this->is_public() || this->is_protected() || this->is_private() || this->is_static() || this->is_const()
                   || this->is_extern() || this->is_binary() || this->is_unsafe() || this->is_explicit() || this->is_imut();
        }

        constexpr bool is_overloadable_operator() const noexcept {
            return this->is_prefix_operator() || this->is_suffix_operator() || this->is_binary_operator();
        }

        constexpr bool is_prefix_operator() const noexcept { return this->is_unary_operator(); }

        constexpr bool is_strictly_prefix_operator() const noexcept {
            return this->is_prefix_operator() && !this->is_suffix_operator() && !this->is_binary_operator();
        }

        constexpr bool is_suffix_operator() const noexcept {
            return this->m_type == type::MINUS_MINUS || this->m_type == type::PLUS_PLUS;
        }

        constexpr bool is_strictly_suffix_operator() const noexcept {
            return !this->is_prefix_operator() && this->is_suffix_operator();
        }

        constexpr bool is_unary_operator() const noexcept {
            switch (this->m_type) {
                case type::BIT_FLIP:
                case type::PLUS_PLUS:
                case type::MINUS_MINUS:
                case type::MINUS:
                case type::NOT:
                case type::STAR:
                case type::AND:
                    return true;
                default:
                    return false;
            }
        }

        constexpr bool is_binary_operator() const noexcept {
            // Old version, manually checking token type of each binary operator
            switch (this->m_type) {
                case type::AND:
                case type::AND_AND:
                case type::LESS_THAN:
                case type::GREATER_THAN:
                case type::MODULO:
                case type::NOT_EQUAL:
                case type::OR:
                case type::OR_OR:
                case type::PLUS:
                case type::MINUS:
                case type::MULTIPLY:
                case type::DIVIDE:
                case type::SHIFT_LEFT:
                case type::SHIFT_RIGHT:
                    return true;
                default:
                    return this->has_equals();
            }
        }

        constexpr bool is_number() const noexcept {
            switch (this->m_type) {
                case type::INTEGER_LITERAL:
                case type::BINARY_LITERAL:
                case type::HEX_LITERAL:
                case type::FLOAT_LITERAL:
                case type::DOUBLE_LITERAL:
                    return true;
                default:
                    return false;
            }
        }

        constexpr bool is_negative_number() const noexcept {
            return this->is_number() && this->m_data.at(0) == char('-');
        }

        constexpr bool is_literal() const noexcept {
            return this->is_string_literal() || this->is_char_literal() || this->is_number_literal();
        }

        constexpr bool is_keyword() const noexcept {
            return this->is_binary() || this->is_const() || this->is_extern() || this->is_module() || this->is_namespace()
                   || this->is_private() || this->is_protected() || this->is_public() || this->is_req() || this->is_unsafe()
                   || this->is_use() || this->is_void() || this->is_class() || this->is_init() || this->is_operator()
                   || this->is_constructor() || this->is_destructor() || this->is_this() || this->is_base() || this->is_if()
                   || this->is_else() || this->is_while() || this->is_do() || this->is_return() || this->is_continue()
                   || this->is_break() || this->is_for() || this->is_true() || this->is_false() || this->is_new()
                   || this->is_del() || this->is_access_specifier() || this->is_ref() || this->is_tref() || this->is_auto()
                   || this->is_imut() || this->is_mv() || this->is_cp() || this->is_var() || this->is_null()
                   || this->is_throw() || this->is_explicit();
        }

        constexpr bool is_alias() const noexcept { return (this->is_identifier()) && (this->m_data == "alias"); }

        constexpr bool is_true() const noexcept { return (this->is_identifier()) && (this->m_data == "true"); }

        constexpr bool is_false() const noexcept { return (this->is_identifier()) && (this->m_data == "false"); }

        constexpr bool is_asm() const noexcept {
            return (this->is_identifier()) && (this->m_data == "asm" || this->m_data == "_asm_" || this->m_data == "__asm__");
        }

        // whether this token can be found in assembly (i.e. compatible with asm blocks)
        constexpr bool is_asm_compatible() const noexcept {
            return (this->is_identifier()) || (this->m_type == type::LEFT_SQUARE_BRACKET)
                   || (this->m_type == type::RIGHT_SQUARE_BRACKET) || (this->m_type == type::LEFT_BRACKET)
                   || (this->m_type == type::RIGHT_BRACKET) || (this->is_number()) || (this->m_type == type::DOT)
                   || (this->m_type == type::PLUS) || (this->m_type == type::MINUS)
                   || (this->m_type == type::MULTIPLY) || (this->m_type == type::COMMA);
        }

        constexpr bool is_identifier() const noexcept { return (this->m_type == type::IDENTIFIER); }

        constexpr bool is_number_literal() const noexcept { return (this->m_type == type::INTEGER_LITERAL); }

        constexpr bool is_string_literal() const noexcept { return (this->m_type == type::STRING_LITERAL); }

        constexpr bool is_char_literal() const noexcept { return (this->m_type == type::CHAR_LITERAL); }

        constexpr bool is_colon() const noexcept { return (this->m_type == type::COLON); }

        constexpr bool is_semicolon() const noexcept { return (this->m_type == type::SEMICOLON); }

        constexpr bool is_left_scope() const noexcept { return (this->m_type == type::LEFT_SCOPE_BRACKET); }

        constexpr bool is_left_scope_bracket() const noexcept { return is_left_scope(); }

        constexpr bool is_right_scope() const noexcept { return (this->m_type == type::RIGHT_SCOPE_BRACKET); }

        constexpr bool is_right_scope_bracket() const noexcept { return is_right_scope(); }

        constexpr bool is_left_bracket() const noexcept { return (this->m_type == type::LEFT_BRACKET); }

        constexpr bool is_right_bracket() const noexcept { return (this->m_type == type::RIGHT_BRACKET); }

        constexpr bool is_left_square() const noexcept { return (this->m_type == type::LEFT_SQUARE_BRACKET); }

        constexpr bool is_left_square_bracket() const noexcept { return is_left_square(); }

        constexpr bool is_right_square() const noexcept { return (this->m_type == type::RIGHT_SQUARE_BRACKET); }

        constexpr bool is_right_square_bracket() const noexcept { return is_right_square(); }

        constexpr bool is_left_bracket_type() const noexcept {
            return is_left_bracket() || is_left_scope_bracket() || is_left_square_bracket();
        }

        constexpr bool is_right_bracket_type() const noexcept {
            return is_right_bracket() || is_right_scope_bracket() || is_right_square_bracket();
        }

        constexpr bool is_dot() const noexcept { return (this->m_type == type::DOT); }

        constexpr bool is_star() const noexcept { return (this->m_type == type::MULTIPLY); }

        constexpr bool is_arrow() const noexcept { return (this->m_type == type::ARROW); }

        constexpr bool is_period() const noexcept { return is_dot(); }

        constexpr bool is_comma() const noexcept { return (this->m_type == type::COMMA); }

        constexpr bool is_question_mark() const noexcept { return (this->m_type == type::QUESTION_MARK); }

        constexpr bool is_equals() const noexcept { return (this->m_type == type::EQUALS); }

        constexpr bool has_equals() const noexcept {
            return (static_cast<std::underlying_type_t<type>>(this->m_type) &
                    static_cast<std::underlying_type_t<type>>(type::EQUALS)) ==
                   static_cast<std::underlying_type_t<type>>(type::EQUALS);
        }

        constexpr bool is_null_token() const noexcept { return (this->m_type == type::NULL_TOKEN); }

    private:
        std::string_view m_data;
        type m_type = type::NULL_TOKEN;
        file_indexer m_index{};
    };

    constexpr token token::null = token();

    inline token::token(const std::string& str, const type type, const file_indexer index) noexcept :
        m_data(str.c_str(), str.length()), m_type(type), m_index(index) {}

    constexpr token::token(const std::string_view str, const type type, const file_indexer index) noexcept :
        m_data(str), m_type(type), m_index(index) {}

    constexpr shift::compiler::token::type operator|(const shift::compiler::token::type f,
        const shift::compiler::token::type other) noexcept {
        return shift::compiler::token::type(
            std::underlying_type_t<shift::compiler::token::type>(f) | std::underlying_type_t<shift::compiler::token::type>(other));
    }

    constexpr shift::compiler::token::type& operator|=(shift::compiler::token::type& f, const shift::compiler::token::type other) noexcept {
        return f = operator|(f, other);
    }

    constexpr shift::compiler::token::type operator&(const shift::compiler::token::type f,
        const shift::compiler::token::type other) noexcept {
        return shift::compiler::token::type(
            std::underlying_type_t<shift::compiler::token::type>(f) & std::underlying_type_t<shift::compiler::token::type>(other));
    }

    constexpr shift::compiler::token::type& operator&=(shift::compiler::token::type& f, const shift::compiler::token::type other) noexcept {
        return f = operator&(f, other);
    }

    constexpr shift::compiler::token::type operator~(const shift::compiler::token::type f) noexcept {
        return shift::compiler::token::type(~std::underlying_type_t<shift::compiler::token::type>(f));
    }

    class tokenizer {
    public:
        typedef std::vector<token>::const_iterator iterator;
        typedef std::vector<token>::const_iterator const_iterator;

        typedef std::vector<token>::size_type size_type;
    public:
        inline tokenizer(error_handler*, const filesystem::file&);

        inline tokenizer(error_handler*, filesystem::file&&);

        inline tokenizer(error_handler*, const std::string& file_data);

        inline tokenizer(error_handler*, std::string&& file_data);

        SHIFT_API void tokenize();

        inline void mark() noexcept { return this->m_token_marks.push(this->m_token_index); }

        SHIFT_API void rollback() noexcept;

        inline void pop_mark() noexcept { return pop_marks(1); }

        inline void pop_marks(
            typename std::stack<const_iterator, std::stack<const_iterator, std::vector<const_iterator>>>::size_type count = -1) noexcept {
            utils::pop_stack(this->m_token_marks, count);
        }

        inline const_iterator begin() const noexcept { return this->m_tokens.begin(); }

        inline const_iterator end() const noexcept { return this->m_tokens.end(); }

        inline const token& operator[](const size_type index) const { return this->m_tokens[index]; }

        inline const token& current_token() const noexcept { return this->token_at(this->m_token_index); }

        SHIFT_API const token& next_token(size_type count = 1) noexcept;

        SHIFT_API const token& reverse_token(size_type count = 1) noexcept;

        [[nodiscard]] SHIFT_API const token& peek_token(size_type count = 1) const noexcept;

        [[nodiscard]] SHIFT_API const token& reverse_peek_token(size_type count = 1) const noexcept;

        SHIFT_API const token& token_at(file_indexer index) const noexcept;

        SHIFT_API const token& token_before(file_indexer index) const noexcept;

        SHIFT_API const token& token_after(file_indexer index) const noexcept;

        SHIFT_API const_iterator position_at(file_indexer index) const noexcept;

        SHIFT_API const_iterator position_before(file_indexer index) const noexcept;

        SHIFT_API const_iterator position_after(file_indexer index) const noexcept;

        inline const_iterator position_at(const token& tok) const noexcept { return position_at(tok.get_file_index()); }

        inline const_iterator position_before(const token& tok) const noexcept {
            return position_before(tok.get_file_index());
        }

        inline const_iterator position_after(const token& tok) const noexcept {
            return position_after(tok.get_file_index());
        }

        inline void set_index(const_iterator index) noexcept {
            this->m_token_index = index;
        }

        inline void set_index(const size_type index) noexcept {
            return set_index(std::next(begin(), index));
        }

        inline void set_position(const const_iterator index) noexcept {
            return set_index(index);
        }

        inline void set_position(const size_type index) noexcept {
            return set_index(index);
        }

        inline const const_iterator& get_index() const noexcept { return this->m_token_index; }

        inline const_iterator& get_index() noexcept { return this->m_token_index; }

        inline const const_iterator& get_position() const noexcept { return this->m_token_index; }

        inline const_iterator& get_position() noexcept { return this->m_token_index; }

        inline const token& token_at(const const_iterator it) const noexcept {
            return it == this->m_tokens.cend() ? token::null : *it;
        }

        inline const token& token_before(const_iterator it) const noexcept {
            return it == this->m_tokens.cbegin() ? token::null : token_at(--it);
        }

        inline const token& token_after(const_iterator it) const noexcept {
            return it == this->m_tokens.cend() ? token::null : token_at(++it);
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
        error_handler* const m_error_handler;
        filesystem::file m_file{ std::string_view("<internal>") };
        std::string m_filedata;
        std::vector<std::string_view> m_lines;
        std::vector<token> m_tokens;
        std::stack<const_iterator, std::vector<const_iterator>> m_token_marks;
        const_iterator m_token_index;
    };

    inline tokenizer::tokenizer(error_handler* const handler, const filesystem::file& file) : m_error_handler(handler),
                                                                                              m_file(file) {}

    inline tokenizer::tokenizer(error_handler* const handler, filesystem::file&& file) : m_error_handler(handler),
                                                                                         m_file(std::move(file)) {}

    inline tokenizer::tokenizer(error_handler* const handler, const std::string& file_data) : m_error_handler(handler),
                                                                                              m_filedata(file_data) {}

    inline tokenizer::tokenizer(error_handler* const handler, std::string&& file_data) : m_error_handler(handler),
                                                                                         m_filedata(std::move(file_data)) {}

}

#endif /* SHIFT_TOKENIZER_H_ */
