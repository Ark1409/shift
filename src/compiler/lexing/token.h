#ifndef SHIFT_TOKEN_H_
#define SHIFT_TOKEN_H_ 1

#include <cstdint>
#include <cstddef>
#include <compare>
#include <string_view>
#include <string>

#include "compiler/mods.h"
#include "utils/enum.h"

namespace shift::compiler::lexing {
    struct file_position {
        size_t line{}, col{};

        constexpr bool operator==(const file_position other) const noexcept {
            return this->col == other.col && this->line == other.line;
        }

        constexpr std::strong_ordering operator<=>(const file_position other) const noexcept {
            auto line_comp = line <=> other.line;
            return line_comp == std::strong_ordering::equal ? col <=> other.col : line_comp;
        }
    };

    struct token {
    public:
        /*
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

            EOF_TOKEN,

            NULL_TOKEN = 0x0 // no token
        };
    public:
        static const token eof;
    public:
        constexpr token() noexcept = default;

        inline token(const std::string& str, type type, file_position index) noexcept;

        constexpr token(std::string_view str, type type, file_position index) noexcept;

        token(const std::string&& str, type type, file_position index) noexcept = delete;

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

        constexpr file_position get_file_position() const noexcept { return this->m_index; }

        constexpr type get_token_type() const noexcept { return this->m_type; }

        constexpr explicit operator std::string_view() const noexcept { return this->m_data; }

        constexpr explicit operator type() const noexcept { return this->m_type; }

        constexpr explicit operator file_position() const noexcept { return this->m_index; }

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

        constexpr bool is_modifier() const noexcept {
            return to_mod(get_data()) != shift_mods::NONE;
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
                   || this->is_del() ||
                   this->is_modifier() || this->is_ref() ||
                   this->is_tref() || this->is_auto()
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

        constexpr bool is_eof_token() const noexcept { return (this->m_type == type::EOF_TOKEN); }

    private:
        std::string_view m_data;
        type m_type{type::NULL_TOKEN};
        file_position m_index{};
    };

    constexpr token::token(const std::string_view str, const type type, const file_position index) noexcept :
        m_data(str), m_type(type), m_index(index) {}

    inline token::token(const std::string& str, const type type, const file_position index) noexcept :
        m_data(str.c_str(), str.length()), m_type(type), m_index(index) {}

    constexpr token token::eof = token(std::string_view{}, token::type::EOF_TOKEN, file_position{});

    static constexpr bool is_operator(const token::type type) noexcept {
        return lexing::token(std::string_view{}, type, lexing::file_position{}).is_overloadable_operator();
    }

    static constexpr bool is_binary_operator(const token::type type) noexcept {
        return lexing::token(std::string_view{}, type, lexing::file_position{}).is_binary_operator();
    }

    static constexpr bool is_unary_operator(const token::type type) noexcept {
        return lexing::token(std::string_view{}, type, lexing::file_position{}).is_unary_operator();
    }

    static constexpr bool is_prefix_operator(const token::type type) noexcept {
        return lexing::token(std::string_view{}, type, lexing::file_position{}).is_prefix_operator();
    }

    static constexpr bool is_suffix_operator(const token::type type) noexcept {
        return lexing::token(std::string_view{}, type, lexing::file_position{}).is_suffix_operator();
    }

    static constexpr bool is_strictly_prefix_operator(const token::type type) noexcept {
        return lexing::token(std::string_view{}, type, lexing::file_position{}).is_strictly_prefix_operator();
    }

    static constexpr bool is_strictly_suffix_operator(const token::type type) noexcept {
        return lexing::token(std::string_view{}, type, lexing::file_position{}).is_strictly_suffix_operator();
    }

    constexpr token::type operator|(const token::type f, const token::type other) noexcept {
        return token::type(utils::to_underlying(f) | utils::to_underlying(other));
    }

    constexpr token::type& operator|=(token::type& f, const token::type other) noexcept {
        return f = operator|(f, other);
    }

    constexpr token::type operator&(const token::type f, const token::type other) noexcept {
        return token::type(utils::to_underlying(f) & utils::to_underlying(other));
    }

    constexpr token::type& operator&=(token::type& f, const token::type other) noexcept {
        return f = operator&(f, other);
    }

    constexpr token::type operator~(const token::type f) noexcept {
        return token::type(~utils::to_underlying(f));
    }
}

#endif //SHIFT_TOKEN_H_
