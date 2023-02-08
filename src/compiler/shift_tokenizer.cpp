/**
 * @file tokenizer.cpp
 */

#include "compiler/shift_tokenizer.h"
#include "utils/utils.h"
#include <fstream>
#include <cctype>
#include <algorithm>

#define shift_tokenizer_can_peek(__peek_count) (((i)+(__peek_count)) < (filesize))
#define shift_tokenizer_can_peek_() shift_tokenizer_can_peek(1)
#define shift_tokenizer_peek(__peek_count) ((i + (__peek_count)) >= filesize ? char(0x0) : this->m_filedata[i+(__peek_count)])
#define shift_tokenizer_peek_() shift_tokenizer_peek(1)
#define shift_tokenizer_is_whitespace(ch) is_whitespace_ext(ch, char)
#define shift_tokenizer_advance(__count) i+=__count, col+=__count, current=this->m_filedata[i]
#define shift_tokenizer_advance_() i++, col++, current=this->m_filedata[i]
#define shift_tokenizer_pre_advance(__count) shift_tokenizer_advance(__count)
#define shift_tokenizer_pre_advance_() ++i, ++col, current=this->m_filedata[i]
#define shift_tokenizer_next_line() this->m_lines.push_back(std::string_view(&this->m_filedata[last_line], i-(last_line))), last_line = i+1, line++, col = 0

#define shift_tokenizer_char_equal(__char, __eq) ((__char) == char((__eq)))
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

#define SHIFT_TOKENIZER_ERROR_PREFIX(_line_, _col_) 				"error: " << std::filesystem::relative(this->m_file.raw_path()).string() << ":" << line << ":" << col << ": "
#define SHIFT_TOKENIZER_WARNING_PREFIX(_line_, _col_) 				"warning: " << std::filesystem::relative(this->m_file.raw_path()).string() << ":" << line << ":" << col << ": "

#define SHIFT_TOKENIZER_ERROR_LOG(__ERR__) 		if(m_error_handler) this->m_error_handler->stream() << __ERR__ << '\n', this->m_error_handler->flush_stream(error_handler::message_type::error)
#define SHIFT_TOKENIZER_FATAL_ERROR_LOG(__ERR__)  SHIFT_TOKENIZER_ERROR_LOG(__ERR__); if(m_error_handler) this->m_error_handler->print_exit_clear()

#define SHIFT_TOKENIZER_WARNING_LOG(__ERR__) 		if(m_error_handler) this->m_error_handler->stream() << __ERR__ << '\n', this->m_error_handler->flush_stream(error_handler::message_type::warning)

#define SHIFT_TOKENIZER_ERROR(_line_, _col_, _len_, __ERR__) \
if(this->m_error_handler) {\
	this->m_error_handler->stream() << SHIFT_TOKENIZER_ERROR_PREFIX(_line_, _col_) << __ERR__ << '\n'; this->m_error_handler->flush_stream(error_handler::message_type::error);\
	std::string_view __temp_line; shift_tokenizer_get_full_line(__temp_line); SHIFT_TOKENIZER_ERROR_LOG(__temp_line);\
	this->m_error_handler->stream() << std::string((_col_)-1, ' ');\
	this->m_error_handler->stream() << std::string(_len_, '^');\
	this->m_error_handler->stream() << '\n';\
	this->m_error_handler->flush_stream(error_handler::message_type::error);\
}

#define SHIFT_TOKENIZER_WARNING(_line_, _col_, _len_, __ERR__) \
if(this->m_error_handler) {\
	this->m_error_handler->stream() << SHIFT_TOKENIZER_WARNING_PREFIX(_line_, _col_) << __ERR__ << '\n'; this->m_error_handler->flush_stream(error_handler::message_type::warning);\
	std::string_view __temp_line; shift_tokenizer_get_full_line(__temp_line); SHIFT_TOKENIZER_WARNING_LOG(__temp_line);\
	this->m_error_handler->stream() << std::string((_col_)-1, ' ');\
	this->m_error_handler->stream() << std::string(_len_, '^');\
	this->m_error_handler->stream() << '\n';\
	this->m_error_handler->flush_stream(error_handler::message_type::warning);\
}

#define SHIFT_TOKENIZER_FATAL_ERROR(_line_, _col_, _len_, __ERR__) 		SHIFT_TOKENIZER_ERROR(_line_, _col_, _len_, __ERR__); if(m_error_handler) this->m_error_handler->print_exit_clear()

/** Namespace shift */
namespace shift {
	/** Namespace compiler */
	namespace compiler {
		void tokenizer::rollback(void) noexcept {
			if (this->m_token_marks.empty()) return;

			this->m_token_index = this->m_token_marks.top();
			this->m_token_marks.pop();
		}

		typename std::vector<token>::const_iterator tokenizer::set_index(const typename std::vector<token>::const_iterator index) noexcept {
			typename std::vector<token>::const_iterator ret = this->m_token_index;
			this->m_token_index = index;
			return ret;
		}

		const token& tokenizer::token_at(const file_indexer index) const noexcept {
			for (const token& token_ : this->m_tokens) {
				if (token_.get_file_index() == index)
					return token_;
			}
			return token::null;
		}

		const token& tokenizer::token_before(const file_indexer index) const noexcept {
			const token* last_token = &token::null;

			for (const token& token : this->m_tokens) {
				if (token.get_file_index() == index)
					return *last_token;
				last_token = &token;
			}

			return token::null;
		}

		const token& tokenizer::token_after(const file_indexer index) const noexcept {
			bool next = false;

			for (const token& token : this->m_tokens) {
				if (next)
					return token;
				if (token.get_file_index() == index)
					next = true;
			}

			return token::null;
		}

		const token& tokenizer::peek_token(typename std::vector<token>::size_type count) const noexcept {
			count = std::min<typename std::vector<token>::size_type>(count, this->cend() - this->m_token_index);
			return this->token_at(this->m_token_index + count);
		}

		const token& tokenizer::reverse_token(typename std::vector<token>::size_type count) noexcept {
			count = std::min<typename std::vector<token>::size_type>(count, this->m_token_index - this->cbegin());
			return this->token_at(this->m_token_index -= count);
		}

		const token& tokenizer::reverse_peek_token(typename std::vector<token>::size_type const count) const noexcept {
			if (count > typename std::vector<token>::size_type(this->m_token_index - this->cbegin())) return token::null;
			return this->token_at(this->m_token_index - count);
		}

		const token& tokenizer::next_token(typename std::vector<token>::size_type count) noexcept {
			count = std::min<typename std::vector<token>::size_type>(count, this->cend() - this->m_token_index);
			return this->token_at(this->m_token_index += count);
		}

		void tokenizer::tokenize(void) {
			if (!this->m_file) // Immediately exit if file does not exist
				return;

			// Clear all class data in case this function has been called more than once
			this->m_tokens.clear();
			std::string().swap(this->m_filedata);
			this->m_lines.clear();
			utils::clear_stack(this->m_token_marks);

			this->m_token_index = this->m_tokens.cbegin();

			std::uintmax_t filesize = this->m_file.size(); // retrieves real size of file on disk; we know it must be at least this large
			{ // read the file
				std::ifstream input_file(this->m_file.raw_path(), std::ios_base::in);

				// reserve correct amount of bytes within file data string
				this->m_filedata.resize(filesize);

				input_file.read(this->m_filedata.data(), filesize); // read the file fully

				filesize = std::uintmax_t(input_file.gcount()); // change file size to number of characters read
				this->m_filedata.resize(filesize); // resize the string to the right size
				input_file.close();
			}

			{ // tokenizing
				size_t last_line = 0; // index of character after last \n
				char current = this->m_filedata[0]; // Current character (i.e. cursor)
				size_t i, line, col; // index (starts at 0), line # (starts at 1), column # (starts at 1)

				this->m_lines.reserve(utils::count(this->m_filedata, std::string_view("\n")) + 1);

				for (i = 0, line = 1, col = 1; i < filesize; shift_tokenizer_advance_()) {
					if (shift_tokenizer_is_whitespace(current)) {
						if (shift_tokenizer_current_equal('\n')) {
							shift_tokenizer_next_line();
							// col++; // col will be incremented to 1 by shift_tokenizer_advance_() in the for loop
						} else if (shift_tokenizer_current_equal('\t')) {
							col += 3; // tabs are 4 spaces. col with be incremented the 4th time by shift_tokenizer_advance_() in the for loop
						}
						continue;
					}

					if (isalpha(current) || shift_tokenizer_current_equal('_')) {
						const size_t old_col = col;
						const size_t old_i = i;

						for (shift_tokenizer_pre_advance_(); (i < filesize) && (isalnum(current) || shift_tokenizer_current_equal('_'));
							shift_tokenizer_advance_());

						shift_tokenizer_reverse_();
						m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::IDENTIFIER, { line,
								old_col }));
						continue;
					}

					if (isdigit(current)) {
						const size_t old_col = col;
						const size_t old_i = i;

						for (shift_tokenizer_pre_advance_(); i < filesize && isdigit(current); shift_tokenizer_advance_());

						if ((shift_tokenizer_current_equal('b') || shift_tokenizer_current_equal('B'))
							&& ((i - old_i) == 1 && this->m_filedata[old_i] == char('0'))) {
							// binary number
							for (shift_tokenizer_pre_advance_(); i < filesize && shift_tokenizer_is_binary(current); shift_tokenizer_advance_());

							if ((i - old_i) == 2) {
								if (this->m_error_handler) {
									SHIFT_TOKENIZER_ERROR(line, col, 1, "Expected binary digit (bit), got '" << current << "'");
								}
							}

							shift_tokenizer_reverse_();
							m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::BINARY_NUMBER, {
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
							m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::HEX_NUMBER, { line,
									old_col }));
						} else if (shift_tokenizer_current_equal('.') && isdigit(shift_tokenizer_peek_())) {
							for (shift_tokenizer_pre_advance_(); i < filesize && isdigit(current); shift_tokenizer_advance_());

							if (shift_tokenizer_current_equal('f') || shift_tokenizer_current_equal('F')) {
								m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::FLOAT, { line,
										old_col }));
							} else if (shift_tokenizer_current_equal('d') || shift_tokenizer_current_equal('D')) {
								m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::DOUBLE, { line,
										old_col }));
							} else {
								shift_tokenizer_reverse_();
								m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::FLOAT, { line,
										old_col }));
							}

						} else if (shift_tokenizer_current_equal('f') || shift_tokenizer_current_equal('F')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::FLOAT, { line,
									old_col }));
						} else if (shift_tokenizer_current_equal('d') || shift_tokenizer_current_equal('D')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::DOUBLE, { line,
									old_col }));
						} else {
							shift_tokenizer_reverse_();
							m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::NUMBER_LITERAL, {
									line, old_col }));
						}
						continue;
					}

					if (shift_tokenizer_current_equal(';')) {
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::SEMICOLON, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal('!')) {
						if (shift_tokenizer_char_equal(shift_tokenizer_peek_(), '=')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::NOT_EQUAL, { line, col }));
							shift_tokenizer_advance_();
							continue;
						}

						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::NOT, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal('{')) {
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::LEFT_SCOPE_BRACKET, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal('}')) {
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::RIGHT_SCOPE_BRACKET, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal('(')) {
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::LEFT_BRACKET, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal(')')) {
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::RIGHT_BRACKET, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal('[')) {
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::LEFT_SQUARE_BRACKET, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal(']')) {
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::RIGHT_SQUARE_BRACKET, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal('.')) {
						if (isdigit(shift_tokenizer_peek_())) {
							const size_t old_col = col;
							const size_t old_i = i;

							// We already know the next character is a digit
							for (shift_tokenizer_pre_advance(2); i < filesize && isdigit(current); shift_tokenizer_advance_());

							if (shift_tokenizer_current_equal('f') || shift_tokenizer_current_equal('F')) {
								m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::FLOAT, { line,
										old_col }));
							} else if (shift_tokenizer_current_equal('d') || shift_tokenizer_current_equal('D')) {
								m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::DOUBLE, { line,
										old_col }));
							} else {
								shift_tokenizer_reverse_();
								m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::DOUBLE, { line,
										old_col }));
							}

							continue;
						}
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::DOT, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal('=')) {
						const char next = shift_tokenizer_peek_();

						// CHECK FOR GREATHER THAN, LESS THAN, MODULO, !=, ETCCCCCC
						//
						// (actually, not != or -=, since it could be:  "int i =! varName;" = "int i = !varName;" or "int i =- varName;" = "int i = -varName;")

						if (shift_tokenizer_char_equal(next, '=')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::EQUALS_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '%')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::MODULO_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '*')) // If pointers are added into the language, =* might count as a dereferencing and not *=
						{
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::MULTIPLY_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '&')) // If pointers are added into the language, =& might count as 'getting a pointer to' and not &=
						{
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::AND_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '|')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::OR_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '^')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::XOR_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '<')) {
							if (shift_tokenizer_char_equal(shift_tokenizer_peek(2), '<')) {
								m_tokens.push_back(
									token(std::string_view(&this->m_filedata[i], 3), token::token_type::SHIFT_LEFT_EQUALS, { line, col }));
								shift_tokenizer_advance(2);
							} else {
								m_tokens.push_back(
									token(std::string_view(&this->m_filedata[i], 2), token::token_type::LESS_THAN_OR_EQUAL, { line, col }));
								shift_tokenizer_advance_();
							}

						} else if (shift_tokenizer_char_equal(next, '>')) {
							if (shift_tokenizer_char_equal(shift_tokenizer_peek(2), '>')) {
								m_tokens.push_back(
									token(std::string_view(&this->m_filedata[i], 3), token::token_type::SHIFT_RIGHT_EQUALS, { line, col }));
								shift_tokenizer_advance(2);
							} else {
								m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::GREATER_THAN_OR_EQUAL, { line,
										col }));
								shift_tokenizer_advance_();
							}

						} else if (shift_tokenizer_char_equal(next, '/')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::DIVIDE_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '+') && !shift_tokenizer_char_equal(shift_tokenizer_peek(2), '+')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::PLUS_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						}
						//
						//					Cannot transform =- to -=
						//					Reason:
						//					i =- 3; // -= 3 OR = -3 ? (since white spaces are ignored)
						//					
						//					else if (shift_tokenizer_char_equal(next, '-')) {
						//						m_tokens.push_back(
						//								token(std::string_view({next}) + current, token::token_type::MINUS_EQUALS, {line, col}));
						//						shift_tokenizer_advance_();
						//					}
						else {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::EQUALS, { line, col }));
						}
						continue;
					}

					if (shift_tokenizer_current_equal('&')) {
						const char next = shift_tokenizer_peek_();
						if (shift_tokenizer_char_equal(next, '&')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::AND_AND, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '=')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::AND_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::AND, { line, col }));
						}
						continue;
					}

					if (shift_tokenizer_current_equal('|')) {
						const char next = shift_tokenizer_peek_();
						if (shift_tokenizer_char_equal(next, '|')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::OR_OR, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '=')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::OR_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::OR, { line, col }));
						}

						continue;
					}

					if (shift_tokenizer_current_equal('^')) {
						if (shift_tokenizer_char_equal(shift_tokenizer_peek_(), '=')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::XOR_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::XOR, { line, col }));
						}

						continue;
					}

					if (shift_tokenizer_current_equal('?')) {
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::QUESTION_MARK, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal('~')) {
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::FLIP_BITS, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal('\\')) {
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::BACKSLASH, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal(':')) {
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::COLON, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal(',')) {
						m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::COMMA, { line, col }));
						continue;
					}

					if (shift_tokenizer_current_equal('-')) {
						const char next = shift_tokenizer_peek_();

						if (shift_tokenizer_char_equal(next, '=')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::MINUS_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '-')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::MINUS_MINUS, { line, col }));
							shift_tokenizer_advance_();
						} else {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::MINUS, { line, col }));
						}
						continue;
					}

					if (shift_tokenizer_current_equal('+')) {
						const char next = shift_tokenizer_peek_();
						if (shift_tokenizer_char_equal(next, '=')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::PLUS_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '+')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::PLUS_PLUS, { line, col }));
							shift_tokenizer_advance_();
						} else {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::PLUS, { line, col }));
						}

						continue;
					}

					if (shift_tokenizer_current_equal('*')) {
						if (shift_tokenizer_char_equal(shift_tokenizer_peek_(), '=')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::MULTIPLY_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::MULTIPLY, { line, col }));
						}

						continue;
					}

					if (shift_tokenizer_current_equal('>')) {
						const char next = shift_tokenizer_peek_();
						if (shift_tokenizer_char_equal(next, '=')) {
							m_tokens.push_back(
								token(std::string_view(&this->m_filedata[i], 2), token::token_type::GREATER_THAN_OR_EQUAL, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '>')) {
							if (shift_tokenizer_char_equal(shift_tokenizer_peek(2), '=')) {
								m_tokens.push_back(
									token(std::string_view(&this->m_filedata[i], 3), token::token_type::SHIFT_RIGHT_EQUALS, { line, col }));
								shift_tokenizer_advance(2);
							} else {
								m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::SHIFT_RIGHT, { line, col }));
								shift_tokenizer_advance_();
							}
						} else {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::GREATER_THAN, { line, col }));
						}

						continue;
					}

					if (shift_tokenizer_current_equal('<')) {
						const char next = shift_tokenizer_peek_();
						if (shift_tokenizer_char_equal(next, '=')) {
							m_tokens.push_back(
								token(std::string_view(&this->m_filedata[i], 2), token::token_type::LESS_THAN_OR_EQUAL, { line, col }));
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '<')) {
							if (shift_tokenizer_char_equal(shift_tokenizer_peek(2), '=')) {
								m_tokens.push_back(
									token(std::string_view(&this->m_filedata[i], 3), token::token_type::SHIFT_LEFT_EQUALS, { line, col }));
								shift_tokenizer_advance(2);
							} else {
								m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::SHIFT_LEFT, { line, col }));
								shift_tokenizer_advance_();
							}
						} else {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::LESS_THAN, { line, col }));
						}

						continue;
					}

					if (shift_tokenizer_current_equal('%')) {
						if (shift_tokenizer_char_equal(shift_tokenizer_peek_(), '=')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::MODULO_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::MODULO, { line, col }));
						}

						continue;
					}

					if (shift_tokenizer_current_equal('/')) {
						const char next = shift_tokenizer_peek_();
						if (shift_tokenizer_char_equal(next, '/')) {
							// single line comment, loop until line is finished
							for (shift_tokenizer_pre_advance(2); i < filesize && !shift_tokenizer_current_equal('\n'); shift_tokenizer_advance_());
							shift_tokenizer_next_line();
						} else if (shift_tokenizer_char_equal(next, '*')) {
							// Multi line comment, loop until next "*/"
							for (shift_tokenizer_pre_advance(2);
								i < filesize && !(shift_tokenizer_current_equal('*') && shift_tokenizer_char_equal(shift_tokenizer_peek_(), '/'));
								shift_tokenizer_advance_()) {
								if (shift_tokenizer_current_equal('\n')) {
									shift_tokenizer_next_line();
								}
							}
							shift_tokenizer_advance_();
						} else if (shift_tokenizer_char_equal(next, '=')) {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 2), token::token_type::DIVIDE_EQUALS, { line, col }));
							shift_tokenizer_advance_();
						} else {
							m_tokens.push_back(token(std::string_view(&this->m_filedata[i], 1), token::token_type::DIVIDE, { line, col }));
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
								SHIFT_TOKENIZER_ERROR(line, old_col, i - old_i + 1, "String literal must be terminated");
							}
						}

						m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::STRING_LITERAL, { line,
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

						m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::CHAR_LITERAL, { line,
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
			this->m_token_index = this->m_tokens.cbegin();
		}

	}
}
