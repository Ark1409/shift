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
namespace shift {
	/** Namespace compiler */
	namespace compiler {

		struct file_indexer {
			size_t line = 0, col = 0;

			constexpr inline bool operator==(const file_indexer other) const noexcept { return this->col == other.col && this->line == other.line; }

			constexpr inline bool operator!=(const file_indexer& other) const noexcept { return !this->operator==(other); }

			constexpr inline bool operator>(const file_indexer other) const noexcept { return this->line == other.line ? this->col > other.col : this->line > other.line; }

			constexpr inline bool operator<(const file_indexer other) const noexcept { return this->line == other.line ? this->col < other.col : this->line < other.line; }

			constexpr inline bool operator>=(const file_indexer& other) const noexcept { return !this->operator<(other); }

			constexpr inline bool operator<=(const file_indexer& other) const noexcept { return !this->operator>(other); }

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
			enum token_type : uint_fast16_t {
				IDENTIFIER = 1, // [a-zA-Z_][a-zA-Z0-9_]*
				INTEGER_LITERAL, // [-]?[0-9]+
				NUMBER_LITERAL = INTEGER_LITERAL,
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

				EQUALS = static_cast<uint_fast16_t>(1 << 15), // =
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
			constexpr token(void) noexcept = default;
			inline token(const std::string& str, token_type type, const file_indexer index) noexcept;
			constexpr inline token(const std::string_view str, token_type type, const file_indexer index) noexcept;
			token(const std::string&& str, token_type type, const file_indexer index) noexcept = delete;

			constexpr token(const token&) noexcept = default;
			constexpr token(token&&) noexcept = default;

			~token() noexcept = default;

			constexpr token& operator=(const token&) noexcept = default;
			constexpr token& operator=(token&&) noexcept = default;

			constexpr inline bool operator==(const token& other) const noexcept { return this->m_type == other.m_type && this->m_index == other.m_index && this->m_data == other.m_data; }
			constexpr inline bool operator==(const std::string_view other) const noexcept { return this->m_data == other; }
			inline bool operator==(const std::string& other) const noexcept { return this->m_data == other; }

			constexpr inline bool operator!=(const token& other) const noexcept { return !this->operator ==(other); }
			constexpr inline bool operator!=(const std::string_view& other) const noexcept { return !this->operator ==(other); }
			inline bool operator!=(const std::string& other) const noexcept { return !this->operator ==(std::string_view(other.c_str(), other.length())); }

			constexpr inline bool operator>(const token& other) const noexcept { return this->m_index > other.m_index; }
			constexpr inline bool operator<(const token& other) const noexcept { return this->m_index < other.m_index; }

			constexpr inline std::string_view get_data(void) const noexcept { return this->m_data; }

			constexpr inline file_indexer get_file_index(void) const noexcept { return this->m_index; }

			constexpr inline token_type get_token_type(void) const noexcept { return this->m_type; }

			constexpr inline operator std::string_view(void) const noexcept { return this->m_data; }

			constexpr inline operator token_type(void) const noexcept { return this->m_type; }

			constexpr inline operator file_indexer(void) const noexcept { return this->m_index; }

			constexpr inline bool is_null(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "null")); }

			constexpr inline bool is_module(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "module")); }

			// unsused
			constexpr inline bool is_namespace(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "namespace")); }

			constexpr inline bool is_const(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "const")); }

			constexpr inline bool is_public(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "public")); }

			constexpr inline bool is_protected(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "protected")); }

			constexpr inline bool is_private(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "private")); }

			constexpr inline bool is_static(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "static")); }

			// currently unused
			constexpr inline bool is_binary(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "binary")); }

			// constexpr inline bool is_floating_point_literal(void) const noexcept {	return (this->m_type == FLOATING_POINT_LITERAL);}

			constexpr inline bool is_void(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "void")); }

			// unsused
			constexpr inline bool is_req(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "req")); }

			constexpr inline bool is_use(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "use")); }

			// currently unused
			constexpr inline bool is_unsafe(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "unsafe")); }

			constexpr inline bool is_extern(void) const noexcept { return ((this->is_identifier()) && (this->m_data == "extern" || this->m_data == "ext")); }

			constexpr inline bool is_class(void) const noexcept { return (this->is_identifier()) && (this->m_data == "class"); }

			constexpr inline bool is_init(void) const noexcept { return (this->is_identifier()) && (this->m_data == "init"); }

			constexpr inline bool is_operator(void) const noexcept { return (this->is_identifier()) && (this->m_data == "operator"); }

			constexpr inline bool is_constructor(void) const noexcept { return (this->is_identifier()) && (this->m_data == "constructor"); }

			constexpr inline bool is_destructor(void) const noexcept { return (this->is_identifier()) && (this->m_data == "destructor"); }

			constexpr inline bool is_if(void) const noexcept { return (this->is_identifier()) && (this->m_data == "if"); }

			constexpr inline bool is_else(void) const noexcept { return (this->is_identifier()) && (this->m_data == "else"); }

			constexpr inline bool is_while(void) const noexcept { return (this->is_identifier()) && (this->m_data == "while"); }

			constexpr inline bool is_do(void) const noexcept { return (this->is_identifier()) && (this->m_data == "do"); }

			constexpr inline bool is_return(void) const noexcept { return (this->is_identifier()) && (this->m_data == "return"); }

			constexpr inline bool is_continue(void) const noexcept { return (this->is_identifier()) && (this->m_data == "continue"); }

			constexpr inline bool is_break(void) const noexcept { return (this->is_identifier()) && (this->m_data == "break"); }

			constexpr inline bool is_for(void) const noexcept { return (this->is_identifier()) && (this->m_data == "for"); }

			constexpr inline bool is_valid_class_name(void) const noexcept { return (this->is_identifier()) && (!this->is_keyword()); }

			constexpr inline bool is_this(void) const noexcept { return (this->is_identifier()) && (this->m_data == "this"); }

			constexpr inline bool is_base(void) const noexcept { return (this->is_identifier()) && (this->m_data == "base"); }

			constexpr inline bool is_new(void) const noexcept { return (this->is_identifier()) && (this->m_data == "new"); }

			constexpr inline bool is_throw(void) const noexcept { return (this->is_identifier()) && (this->m_data == "throw"); }

			constexpr inline bool is_access_specifier(void) const noexcept {
				return (this->is_identifier())
					&& (this->is_public() || this->is_protected() || this->is_private() || this->is_static() || this->is_const() || this->is_extern()
						|| this->is_binary() || this->is_unsafe());
			}

			constexpr inline bool is_overload_operator(void) const noexcept { return this->is_prefix_overload_operator() || this->is_suffix_overload_operator() || this->is_binary_operator(); }

			constexpr inline bool is_prefix_overload_operator(void) const noexcept { return this->is_unary_operator(); }

			constexpr inline bool is_strictly_prefix_overload_operator(void) const noexcept { return this->is_prefix_overload_operator() && !this->is_suffix_overload_operator() && !this->is_binary_operator(); }

			constexpr inline bool is_suffix_overload_operator(void) const noexcept {
				return this->m_type == token_type::MINUS_MINUS || this->m_type == token_type::PLUS_PLUS;
			}

			constexpr inline bool is_strictly_suffix_overload_operator(void) const noexcept { return !this->is_prefix_overload_operator() && this->is_suffix_overload_operator(); }

			constexpr inline bool is_unary_operator(void) const noexcept {
				return (!this->is_identifier())
					&& ((this->m_type == token_type::BIT_FLIP) || (this->m_type == token_type::PLUS_PLUS)
						|| (this->m_type == token_type::MINUS_MINUS) || (this->m_type == token_type::MINUS)
						|| (this->m_type == token_type::NOT));
			}

			constexpr inline bool is_binary_operator(void) const noexcept {
				// Old version, manually checking token type of each binary operator
				return (!this->is_identifier())
					&& (((this->m_type & token_type::EQUALS) == token_type::EQUALS)
						|| (this->m_type == token_type::AND) || (this->m_type == token_type::AND_AND)
						|| (this->m_type == token_type::DIVIDE) || (this->m_type == token_type::GREATER_THAN)
						|| (this->m_type == token_type::LESS_THAN) || (this->m_type == token_type::MINUS)
						|| (this->m_type == token_type::MODULO) || (this->m_type == token_type::MULTIPLY)
						|| (this->m_type == token_type::NOT_EQUAL) || (this->m_type == token_type::OR)
						|| (this->m_type == token_type::OR_OR) || (this->m_type == token_type::OR_EQUALS)
						|| (this->m_type == token_type::PLUS) || (this->m_type == token_type::XOR)
						|| (this->m_type == token_type::SHIFT_LEFT) || (this->m_type == token_type::SHIFT_RIGHT));

				// This way is more efficient, but should be removed if some how tertiary operators are introduced
				//return !this->is_unary_operator(); // does not work 100%, cuz of things like CHAR_LITERAL; revert back to old method
			}

			constexpr inline bool is_number(void) const noexcept {
				return this->m_type == token_type::INTEGER_LITERAL || this->m_type == token_type::FLOAT
					|| this->m_type == token_type::DOUBLE || this->m_type == token_type::BINARY_NUMBER
					|| this->m_type == token_type::HEX_NUMBER;
			}

			constexpr inline bool is_negative_number(void) const noexcept { return this->is_number() && this->get_data().at(0) == char('-'); }

			constexpr inline bool is_literal(void) const noexcept { return this->is_string_literal() || this->is_number_literal() || this->is_char_literal(); }

			constexpr inline bool is_keyword(void) const noexcept {
				return (this->is_identifier())
					&& (this->is_binary() || this->is_const() || this->is_extern() || this->is_module() || this->is_namespace() || this->is_private()
						|| this->is_protected() || this->is_public() || this->is_req() || this->is_unsafe() || this->is_use() || this->is_void()
						|| this->is_class() || this->is_init() || this->is_operator() || this->is_constructor() || this->is_destructor()
						|| this->is_this() || this->is_base() || this->is_if() || this->is_else() || this->is_while() || this->is_do() || this->is_return()
						|| this->is_continue() || this->is_break() || this->is_for() || this->is_true() || this->is_false() || this->is_access_specifier());
			}

			constexpr inline bool is_alias(void) const noexcept { return (this->is_identifier()) && (this->m_data == "alias"); }

			constexpr inline bool is_true(void) const noexcept { return (this->is_identifier()) && (this->m_data == "true"); }

			constexpr inline bool is_false(void) const noexcept { return (this->is_identifier()) && (this->m_data == "false"); }

			constexpr inline bool is_asm(void) const noexcept { return (this->is_identifier()) && (this->m_data == "asm" || this->m_data == "_asm_" || this->m_data == "__asm__"); }

			// whether this token can be found in assembly (i.e. compatible with asm blocks)
			constexpr inline bool is_asm_compatible(void) const noexcept {
				return (this->is_identifier()) || (this->m_type == token_type::LEFT_SQUARE_BRACKET)
					|| (this->m_type == token_type::RIGHT_SQUARE_BRACKET) || (this->m_type == token_type::LEFT_BRACKET)
					|| (this->m_type == token_type::RIGHT_BRACKET) || (this->is_number()) || (this->m_type == token_type::DOT)
					|| (this->m_type == token_type::PLUS) || (this->m_type == token_type::MINUS)
					|| (this->m_type == token_type::MULTIPLY) || (this->m_type == token_type::COMMA);
			}

			constexpr inline bool is_identifier(void) const noexcept { return (this->m_type == IDENTIFIER); }

			constexpr inline bool is_number_literal(void) const noexcept { return (this->m_type == INTEGER_LITERAL); }

			constexpr inline bool is_integer_literal(void) const noexcept { return is_number_literal(); }

			constexpr inline bool is_string_literal(void) const noexcept { return (this->m_type == STRING_LITERAL); }

			constexpr inline bool is_char_literal(void) const noexcept { return (this->m_type == CHAR_LITERAL); }

			constexpr inline bool is_character_literal(void) const noexcept { return is_char_literal(); }

			constexpr inline bool is_colon(void) const noexcept { return (this->m_type == token_type::COLON); }

			constexpr inline bool is_semicolon(void) const noexcept { return (this->m_type == token_type::SEMICOLON); }

			constexpr inline bool is_left_scope(void) const noexcept { return (this->m_type == token_type::LEFT_SCOPE_BRACKET); }

			constexpr inline bool is_left_scope_bracket(void) const noexcept { return is_left_scope(); }

			constexpr inline bool is_right_scope(void) const noexcept { return (this->m_type == token_type::RIGHT_SCOPE_BRACKET); }

			constexpr inline bool is_right_scope_bracket(void) const noexcept { return is_right_scope(); }

			constexpr inline bool is_left_bracket(void) const noexcept { return (this->m_type == token_type::LEFT_BRACKET); }

			constexpr inline bool is_right_bracket(void) const noexcept { return (this->m_type == token_type::RIGHT_BRACKET); }

			constexpr inline bool is_left_square(void) const noexcept { return (this->m_type == token_type::LEFT_SQUARE_BRACKET); }

			constexpr inline bool is_left_square_bracket(void) const noexcept { return is_left_square(); }

			constexpr inline bool is_right_square(void) const noexcept { return (this->m_type == token_type::RIGHT_SQUARE_BRACKET); }

			constexpr inline bool is_right_square_bracket(void) const noexcept { return is_right_square(); }

			constexpr inline bool is_left_bracket_type(void) const noexcept { return is_left_bracket() || is_left_scope_bracket() || is_left_square_bracket(); }

			constexpr inline bool is_right_bracket_type(void) const noexcept { return is_right_bracket() || is_right_scope_bracket() || is_right_square_bracket(); }

			constexpr inline bool is_dot(void) const noexcept { return (this->m_type == token_type::DOT); }

			constexpr inline bool is_period(void) const noexcept { return is_dot(); }

			constexpr inline bool is_comma(void) const noexcept { return (this->m_type == token_type::COMMA); }

			constexpr inline bool is_question_mark(void) const noexcept { return (this->m_type == token_type::QUESTION_MARK); }

			constexpr inline bool is_equals(void) const noexcept { return (this->m_type == token_type::EQUALS); }

			constexpr inline bool has_equals(void) const noexcept { return (this->m_type & token_type::EQUALS) == token_type::EQUALS; }

			constexpr inline bool is_null_token(void) const noexcept { return (this->m_type == NULL_TOKEN); }

		private:
			std::string_view m_data;
			token_type m_type = NULL_TOKEN;
			file_indexer m_index;
		};

		inline constexpr token token::null = token();

		inline token::token(const std::string& str, const token_type type, const file_indexer index) noexcept : m_data(str.c_str(),
			str.length()), m_type(type), m_index(index) {}

		constexpr inline token::token(const std::string_view str, const token_type type, const file_indexer index) noexcept : m_data(str), m_type(
			type), m_index(index) {}

		class tokenizer {
		public:
			inline tokenizer(error_handler* const, const filesystem::file&);
			inline tokenizer(error_handler* const, filesystem::file&&);
			~tokenizer() noexcept = default;
			tokenizer(const tokenizer&) = default;
			tokenizer(tokenizer&&) noexcept = default;
			tokenizer& operator=(const tokenizer&) = default;
			tokenizer& operator=(tokenizer&&) noexcept = default;

			void tokenize(void);
			inline void mark(void) noexcept { return this->m_token_marks.push(this->m_token_index); }
			void rollback(void) noexcept;
			inline void pop_mark() noexcept { return pop_marks(1); }
			inline void pop_marks(typename std::stack<typename std::vector<token>::const_iterator>::size_type count = -1) noexcept { utils::pop_stack(this->m_token_marks, count); }

			inline typename std::vector<token>::iterator begin() noexcept { return this->m_tokens.begin(); }
			inline typename std::vector<token>::iterator end() noexcept { return this->m_tokens.end(); }

			inline typename std::vector<token>::const_iterator cbegin() const noexcept { return this->m_tokens.cbegin(); }
			inline typename std::vector<token>::const_iterator cend() const noexcept { return this->m_tokens.cend(); }

			inline const token& operator[](const typename std::vector<token>::size_type index) const { return this->m_tokens[index]; }
			inline const token& current_token(void) const noexcept { return this->token_at(this->m_token_index); }
			const token& next_token(typename std::vector<token>::size_type advance_count = 1) noexcept;
			const token& reverse_token(typename std::vector<token>::size_type count = 1) noexcept;
			const token& peek_token(typename std::vector<token>::size_type count = 1) const noexcept;
			const token& reverse_peek_token(typename std::vector<token>::size_type const count = 1) const noexcept;
			const token& token_at(const file_indexer index) const noexcept;
			const token& token_before(const file_indexer index) const noexcept;
			const token& token_after(const file_indexer index) const noexcept;
			typename std::vector<token>::const_iterator set_index(const typename std::vector<token>::const_iterator index) noexcept;
			inline typename std::vector<token>::const_iterator set_index(const typename std::vector<token>::size_type index) noexcept { return set_index(cbegin() + index); }

			inline typename std::vector<token>::const_iterator get_index(void) const noexcept { return this->m_token_index; }
			inline typename std::vector<token>::const_iterator& get_index(void) noexcept { return this->m_token_index; }

			inline const token& token_at(const typename std::vector<token>::const_iterator it) const noexcept { return it == this->m_tokens.cend() ? token::null : *it; }

			inline const token& token_before(typename std::vector<token>::const_iterator it) const noexcept { return it == this->m_tokens.cbegin() ? token::null : token_at(--it); }

			inline const token& token_after(typename std::vector<token>::const_iterator it) const noexcept { return it == this->m_tokens.cend() ? token::null : token_at(++it); }

			inline const token& token_before(const token& token) const noexcept { return this->token_before(token.get_file_index()); }

			inline const token& token_before(const token* const token) const noexcept { return this->token_before(token->get_file_index()); }

			inline const filesystem::file& get_file(void) const noexcept { return m_file; }

			inline const std::vector<std::string_view>& get_lines(void) const noexcept { return this->m_lines; }

			inline const std::vector<token>& get_tokens(void) const noexcept { return this->m_tokens; }

			inline error_handler* get_error_handler() noexcept { return m_error_handler; }

			inline const error_handler* get_error_handler() const noexcept { return m_error_handler; }

			inline void set_error_handler(error_handler* const error_handler) noexcept { m_error_handler = error_handler; }
		protected:
			error_handler* m_error_handler;
			filesystem::file m_file;
			std::string m_filedata;
			std::vector<std::string_view> m_lines;
			std::vector<token> m_tokens;
			std::stack<typename std::vector<token>::const_iterator> m_token_marks;
			typename std::vector<token>::const_iterator m_token_index;
		};

		inline tokenizer::tokenizer(error_handler* const handler, const filesystem::file& file) : m_error_handler(handler),
			m_file(file) {}

		inline tokenizer::tokenizer(error_handler* const handler, filesystem::file&& file) : m_error_handler(handler),
			m_file(std::move(file)) {}
	}
}

constexpr inline shift::compiler::token::token_type operator^(const shift::compiler::token::token_type f, const shift::compiler::token::token_type other) noexcept { return shift::compiler::token::token_type(std::underlying_type_t<shift::compiler::token::token_type>(f) ^ std::underlying_type_t<shift::compiler::token::token_type>(other)); }
constexpr inline shift::compiler::token::token_type& operator^=(shift::compiler::token::token_type& f, const shift::compiler::token::token_type other) noexcept { return f = operator^(f, other); }
constexpr inline shift::compiler::token::token_type operator|(const shift::compiler::token::token_type f, const shift::compiler::token::token_type other) noexcept { return shift::compiler::token::token_type(std::underlying_type_t<shift::compiler::token::token_type>(f) | std::underlying_type_t<shift::compiler::token::token_type>(other)); }
constexpr inline shift::compiler::token::token_type& operator|=(shift::compiler::token::token_type& f, const shift::compiler::token::token_type other) noexcept { return f = operator|(f, other); }
constexpr inline shift::compiler::token::token_type operator&(const shift::compiler::token::token_type f, const shift::compiler::token::token_type other) noexcept { return shift::compiler::token::token_type(std::underlying_type_t<shift::compiler::token::token_type>(f) & std::underlying_type_t<shift::compiler::token::token_type>(other)); }
constexpr inline shift::compiler::token::token_type& operator&=(shift::compiler::token::token_type& f, const shift::compiler::token::token_type other) noexcept { return f = operator&(f, other); }
constexpr inline shift::compiler::token::token_type operator~(const shift::compiler::token::token_type f) noexcept { return shift::compiler::token::token_type(~std::underlying_type_t<shift::compiler::token::token_type>(f)); }

#endif /* SHIFT_TOKENIZER_H_ */
