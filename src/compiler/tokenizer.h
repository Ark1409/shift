/**
 * @file include/tokenizer.h
 */

#ifndef SHIFT_TOKENIZER_H_
#define SHIFT_TOKENIZER_H_ 1

#include "../shift_config.h"
#include "stdutils.h"
#include "io/file.h"
#include "shift_error_handler.h"

#include <string>
#include <string_view>
#include <memory>

/** Namespace shift */
namespace shift {
	/** Namespace compiler */
	namespace compiler {

		struct SHIFT_COMPILER_API file_indexer {
				size_t line = 0, col = 0;

//				constexpr file_indexer(void) noexcept = default;
//				constexpr file_indexer(const size_t _line, const size_t _col) noexcept : line(_line), col(_col) {
//				}
//
//				file_indexer(const std::initializer_list<size_t> list);

				constexpr inline bool operator==(const file_indexer other) const noexcept {
					return this->col == other.col && this->line == other.line;
				}

				constexpr inline bool operator!=(const file_indexer& other) const noexcept {
					return !this->operator==(other);
				}

				constexpr inline bool operator>(const file_indexer& other) const noexcept {
					return this->line == other.line ? this->col > other.col : this->line > other.line;
				}

				constexpr inline bool operator<(const file_indexer& other) const noexcept {
					return this->line == other.line ? this->col < other.col : this->line < other.line;
				}

				constexpr inline bool operator>=(const file_indexer& other) const noexcept {
					return !this->operator<(other);
				}

				constexpr inline bool operator<=(const file_indexer& other) const noexcept {
					return !this->operator>(other);
				}

		};

		struct SHIFT_COMPILER_API token {
			public:
				/**
				 * Structure of token types:
				 *
				 * [00000000 00000000] [00000000 00000000]
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
				 *
				 */
				enum token_type : uint32_t {
					IDENTIFIER = 1, // [a-zA-Z_][a-zA-Z0-9_]*
					NUMBER_LITERAL, // [-]?[0-9]+
					BINARY_NUMBER, // 0[b|B][0|1]+
					HEX_NUMBER, // 0[x|X][0-9a-fA-F]+
					//FLOATING_POINT_LITERAL = (1 << 5), // [0-9]*.[0-9]+
					FLOAT,  // [0-9]*.[0-9]+(f|F)?
					DOUBLE,  // [0-9]*.[0-9]+(d|D)?
					GREATER_THAN, // >
					LESS_THAN, // <
					MODULO, // %
					OR, // |
					AND, // &
					XOR, // ^
					FLIP_BITS, // ~
					BIT_FLIP = FLIP_BITS,
					NOT, // !
					EXCLAMATION_MARK = NOT, // !
					PLUS, // +
					MINUS, // -
					MULTIPLY, // *
					DIVIDE, // /
					LEFT_BRACKET, // (
					RIGHT_BRACKET, // )
					LEFT_SQUARE_BRACKET, // [
					RIGHT_SQUARE_BRACKET, // ]
					LEFT_SCOPE_BRACKET, // {
					RIGHT_SCOPE_BRACKET, // }
					DOT, // .
					PERIOD = DOT, // . | ^^^^
					COMMA, // ,
					QUESTION_MARK, // ?
					COLON, // :
					SEMICOLON, // ;
					//DOUBLE_QUOTE, // "
					STRING_LITERAL, // ".*"
					CHAR_LITERAL, // '.'
					//SINGLE_QUOTE, // '
					SHIFT_LEFT, // <<
					SHIFT_RIGHT, // >>
					BACKSLASH, // \ <--

					EQUALS = static_cast<uint32_t>(1 << 31), // =
					EQUALS_EQUALS = (1 << 30) | EQUALS, // ==
					GREATER_THAN_OR_EQUAL = GREATER_THAN | EQUALS, // >= (| =>)
					LESS_THAN_OR_EQUAL = LESS_THAN | EQUALS, // <= (| =<)
					MODULO_EQUALS = MODULO | EQUALS, // %=
					OR_EQUALS = OR | EQUALS, // |=
					OR_OR = (1 << 29) | OR, // ||
					AND_EQUALS = AND | EQUALS, // &=
					AND_AND = (1 << 28) | AND, // &&
					XOR_EQUALS = XOR | EQUALS, // ^=
					NOT_EQUAL = NOT | EQUALS, // !=
					PLUS_EQUALS = PLUS | EQUALS, // +=
					PLUS_PLUS = (1 << 27) | PLUS, // ++
					MINUS_EQUALS = MINUS | EQUALS, // -=
					MINUS_MINUS = (1 << 26) | MINUS, // --
					MULTIPLY_EQUALS = MULTIPLY | EQUALS, // *=
					DIVIDE_EQUALS = DIVIDE | EQUALS, // /=
					SHIFT_LEFT_EQUALS = SHIFT_LEFT | EQUALS, // <<=
					SHIFT_RIGHT_EQUALS = SHIFT_RIGHT | EQUALS, // >>=

					NULL_TOKEN = 0x0 // no token
				};
			private:
				std::string_view m_data;
				token_type m_type = NULL_TOKEN;
				file_indexer m_index;
			public:
				static const token null; // NULL token type
			public:
				token(void) noexcept = default;
				token(const std::string& str, token_type type, const file_indexer index) noexcept;
				token(const std::string_view str, token_type type, const file_indexer index) noexcept;
				token(const std::string&& str, token_type type, const file_indexer index) noexcept = delete;

				token(const token&) = default;
				token(token&&) noexcept = default;

				CXX20_CONSTEXPR ~token(void) noexcept = default;

				token& operator=(const token&) = default;
				token& operator=(token&&) = default;

				bool operator==(const token& other) const noexcept;

				inline bool operator!=(const token& other) const noexcept {
					return !this->operator ==(other);
				}

				inline bool operator==(const std::string& other) const noexcept {
					return this->m_data == other;
				}

				inline bool operator!=(const std::string& other) const noexcept {
					return !this->operator ==(other);
				}

				inline bool operator>(const token& other) const noexcept {
					return this->m_index > other.m_index;
				}

				inline bool operator<(const token& other) const noexcept {
					return this->m_index < other.m_index;
				}

				inline const std::string_view get_data(void) const noexcept {
					return this->m_data;
				}

				inline const file_indexer get_file_index(void) const noexcept {
					return this->m_index;
				}

				inline token_type get_token_type(void) const noexcept {
					return this->m_type;
				}

				inline operator const char*(void) const noexcept {
					return this->m_data.data();
				}

				inline operator const std::string_view(void) const noexcept {
					return this->m_data;
				}

				inline operator token_type(void) const noexcept {
					return this->m_type;
				}

				inline operator const file_indexer(void) const noexcept {
					return this->m_index;
				}

				bool is_null(void) const noexcept;
				bool is_public(void) const noexcept;
				bool is_protected(void) const noexcept;
				bool is_private(void) const noexcept;
				bool is_static(void) const noexcept;
				bool is_binary(void) const noexcept; // currently unused
				bool is_const(void) const noexcept;
				bool is_void(void) const noexcept;
				bool is_req(void) const noexcept;  // unsused
				bool is_use(void) const noexcept;
				bool is_module(void) const noexcept;
				bool is_namespace(void) const noexcept; // unsused
				bool is_unsafe(void) const noexcept; // currently unused
				bool is_extern(void) const noexcept;
				bool is_class(void) const noexcept;
				bool is_init(void) const noexcept;
				bool is_operator(void) const noexcept;
				bool is_constructor(void) const noexcept;
				bool is_destructor(void) const noexcept;
				bool is_if(void) const noexcept;
				bool is_else(void) const noexcept;
				bool is_while(void) const noexcept;
				bool is_do(void) const noexcept;
				bool is_return(void) const noexcept;
				bool is_continue(void) const noexcept;
				bool is_break(void) const noexcept;
				bool is_for(void) const noexcept;
				bool is_valid_class_name(void) const noexcept;
				bool is_this(void) const noexcept;
				bool is_base(void) const noexcept;
				bool is_access_specifier(void) const noexcept;
				bool is_prefix_overload_operator(void) const noexcept;
				bool is_suffix_overload_operator(void) const noexcept;
				bool is_overload_operator(void) const noexcept;
				bool is_unary_operator(void) const noexcept;
				bool is_binary_operator(void) const noexcept;
				bool is_number(void) const noexcept;
				bool is_negative_number(void) const noexcept;
				bool is_literal(void) const noexcept;
				bool is_keyword(void) const noexcept;
				bool is_alias(void) const noexcept;
				bool is_true(void) const noexcept;
				bool is_false(void) const noexcept;
				bool is_asm(void) const noexcept;
				bool is_asm_compatible(void) const noexcept; // whether this token can be found in assembly (i.e. compatible with asm blocks)

				inline bool is_identifier(void) const noexcept {
					return (this->m_type == IDENTIFIER);
				}

				inline bool is_number_literal(void) const noexcept {
					return (this->m_type == NUMBER_LITERAL);
				}

				inline bool is_string_literal(void) const noexcept {
					return (this->m_type == STRING_LITERAL);
				}

				inline bool is_char_literal(void) const noexcept {
					return (this->m_type == CHAR_LITERAL);
				}

				inline bool is_semicolon(void) const noexcept {
					return (this->m_type == token_type::SEMICOLON);
				}

				inline bool is_left_scope(void) const noexcept {
					return (this->m_type == token_type::LEFT_SCOPE_BRACKET);
				}

				inline bool is_left_scope_bracket(void) const noexcept {
					return is_left_scope();
				}

				inline bool is_right_scope(void) const noexcept {
					return (this->m_type == token_type::RIGHT_SCOPE_BRACKET);
				}

				inline bool is_right_scope_bracket(void) const noexcept {
					return is_right_scope();
				}

				inline bool is_left_bracket(void) const noexcept {
					return (this->m_type == token_type::LEFT_BRACKET);
				}

				inline bool is_right_bracket(void) const noexcept {
					return (this->m_type == token_type::RIGHT_BRACKET);
				}

				inline bool is_left_square(void) const noexcept {
					return (this->m_type == token_type::LEFT_SQUARE_BRACKET);
				}

				inline bool is_left_square_bracket(void) const noexcept {
					return is_left_square();
				}

				inline bool is_right_square(void) const noexcept {
					return (this->m_type == token_type::RIGHT_SQUARE_BRACKET);
				}

				inline bool is_right_square_bracket(void) const noexcept {
					return is_right_square();
				}

				inline bool is_left_bracket_type(void) const noexcept {
					return is_left_bracket() || is_left_scope_bracket() || is_left_square_bracket();
				}

				inline bool is_right_bracket_type(void) const noexcept {
					return is_right_bracket() || is_right_scope_bracket() || is_right_square_bracket();
				}

				inline bool is_dot(void) const noexcept {
					return (this->m_type == token_type::DOT);
				}

				inline bool is_period(void) const noexcept {
					return is_dot();
				}

				inline bool is_comma(void) const noexcept {
					return (this->m_type == token_type::COMMA);
				}

				inline bool is_question_mark(void) const noexcept {
					return (this->m_type == token_type::QUESTION_MARK);
				}

				inline bool is_equals(void) const noexcept {
					return (this->m_type == token_type::EQUALS);
				}

				inline bool has_equals(void) const noexcept {
					return (this->m_type & token_type::EQUALS);
				}

				inline bool is_null_token(void) const noexcept {
					return (this->m_type == NULL_TOKEN);
				}
		};

		class SHIFT_COMPILER_API tokenizer {
			protected:
				shift::io::file m_file;
//				shift::compiler::shift_error_handler m_error_handler;
				shift::compiler::shift_error_handler* m_error_handler;
				bool m_del_error_handler = false;
				std::list<token> m_tokens;
				std::vector<std::string_view> m_lines;
				std::string m_filedata;
				std::stack<typename std::list<token>::const_iterator> m_token_marks;
				typename std::list<token>::const_iterator m_token_index;
			public:
				tokenizer(const shift::io::file&, shift::compiler::shift_error_handler* = nullptr);
				tokenizer(shift::io::file&&, shift::compiler::shift_error_handler* = nullptr);

				~tokenizer(void) noexcept;
				tokenizer(const tokenizer&) = delete;
				tokenizer(tokenizer&&);
				tokenizer& operator=(const tokenizer&) = delete;
				tokenizer& operator=(tokenizer&&);

				void tokenize(void);
				void mark(void) noexcept;
				void rollback(void) noexcept;
				void clear_marks(typename std::stack<typename std::list<token>::const_iterator>::size_type count = -1) noexcept;
				const token* current_token(void) const noexcept;
				const token* next_token(typename std::list<token>::size_type advance_count = 1) noexcept;
				const token* reverse_token(typename std::list<token>::size_type count = 1) noexcept;
				const token* peek_token(typename std::list<token>::size_type count = 1) const noexcept;
				const token* reverse_peek_token(typename std::list<token>::size_type count = 1) const noexcept;
				const token* token_at(const file_indexer& index) const noexcept;
				const token* token_before(const file_indexer& index) const noexcept;
				const token* token_after(const file_indexer& index) const noexcept;
				typename std::list<token>::const_iterator set_index(const typename std::list<token>::const_iterator index) noexcept;

				inline typename std::list<token>::const_iterator get_index(void) const noexcept {
					return this->m_token_index;
				}

				inline typename std::list<token>::const_iterator& get_index(void) noexcept {
					return this->m_token_index;
				}

				inline const token* token_at(const typename std::list<token>::const_iterator it) const noexcept {
					return it == this->m_tokens.cend() || it._M_node == nullptr ? &token::null : it.operator ->();
				}

				inline const token* token_before(typename std::list<token>::const_iterator it) const noexcept {
					return token_at(--it);
				}

				inline const token* token_after(typename std::list<token>::const_iterator it) const noexcept {
					return token_at(++it);
				}

				inline const token* token_before(const token& token) const noexcept {
					return this->token_before(token.get_file_index());
				}

				inline const token* token_before(const token* const token) const noexcept {
					return this->token_before(token->get_file_index());
				}

				inline const shift::io::file& get_file(void) const noexcept {
					return m_file;
				}

				inline const std::vector<std::string_view>& get_lines(void) const noexcept {
					return this->m_lines;
				}

				inline const std::list<token>& get_tokens(void) const noexcept {
					return this->m_tokens;
				}

				inline const shift::compiler::shift_error_handler* get_error_handler(void) const noexcept {
					return this->m_error_handler;
				}

				inline shift::compiler::shift_error_handler* get_error_handler(void) noexcept {
					return this->m_error_handler;
				}
		};

	}
}

#endif /* SHIFT_TOKENIZER_H_ */
