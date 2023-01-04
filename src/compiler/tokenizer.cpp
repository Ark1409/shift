/**
 * @file tokenizer.cpp
 */

#include "tokenizer.h"
#include <fstream>

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

#define SHIFT_TOKENIZER_ERROR_PREFIX(_line_, _col_) 				"error: " << std::filesystem::relative(this->m_file.get_path()).string() << ":" << line << ":" << col << ": "

#define SHIFT_TOKENIZER_ERROR_LOG(__ERR__) 		this->m_error_handler->stream() << __ERR__ << '\n'; this->m_error_handler->flush_stream(SHIFT_ERROR_MESSAGE_TYPE)
#define SHIFT_TOKENIZER_FATAL_ERROR_LOG(__ERR__)  SHIFT_TOKENIZER_ERROR_LOG(__ERR__); //this->m_error_handler->print_and_exit()

#define SHIFT_TOKENIZER_ERROR(_line_, _col_, _len_, __ERR__) \
{\
	this->m_error_handler->stream() << SHIFT_TOKENIZER_ERROR_PREFIX(_line_, _col_) << __ERR__ << '\n'; this->m_error_handler->flush_stream(SHIFT_ERROR_MESSAGE_TYPE);\
	std::string_view __temp_line; shift_tokenizer_get_full_line(__temp_line); SHIFT_TOKENIZER_ERROR_LOG(__temp_line);\
	for(size_t loop_col = 0; loop_col < (_col_-1); loop_col++) this->m_error_handler->stream() << ' ';\
	for(size_t loop_item = 0; loop_item < (_len_); loop_item++) this->m_error_handler->stream() << '^';\
	this->m_error_handler->stream() << '\n';\
	this->m_error_handler->flush_stream(SHIFT_ERROR_MESSAGE_TYPE);\
}

#define SHIFT_TOKENIZER_FATAL_ERROR(_line_, _col_, _len_, __ERR__) 		SHIFT_TOKENIZER_ERROR(_line_, _col_, _len_, __ERR__); //this->m_error_handler->print_and_exit()

/** Namespace shift */
namespace shift {
	/** Namespace compiler */
	namespace compiler {
//		SHIFT_COMPILER_API file_indexer::file_indexer(const std::initializer_list<size_t> list) : line(list.begin()[0]), col(list.begin()[1]) {
//			if (list.size() < size_t(2)) {
//				// error ? it may have already crashed
//			}
//		}

		SHIFT_COMPILER_API const token token::null = token();

		SHIFT_COMPILER_API token::token(const std::string& str, const token_type type, const file_indexer index) noexcept : m_data(str.c_str(),
				str.size()), m_type(type), m_index(index) {
		}

		SHIFT_COMPILER_API token::token(const std::string_view str, const token_type type, const file_indexer index) noexcept : m_data(str), m_type(
				type), m_index(index) {
		}

		SHIFT_COMPILER_API bool token::operator==(const token& other) const noexcept {
			return this->m_data == other.m_data && this->m_index == other.m_index && this->m_type == other.m_type;
		}

		SHIFT_COMPILER_API bool token::is_null(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "null"));
		}

		SHIFT_COMPILER_API bool token::is_module(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "module"));
		}

		SHIFT_COMPILER_API bool token::is_namespace(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "namespace"));
		}

		SHIFT_COMPILER_API bool token::is_const(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "const"));
		}

		SHIFT_COMPILER_API bool token::is_public(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "public"));
		}

		SHIFT_COMPILER_API bool token::is_protected(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "protected"));
		}

		SHIFT_COMPILER_API bool token::is_private(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "private"));
		}

		SHIFT_COMPILER_API bool token::is_static(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "static"));
		}

		SHIFT_COMPILER_API bool token::is_binary(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "binary"));
		}

		//				SHIFT_COMPILER_API bool token::is_floating_point_literal(void) const noexcept {
		//					return (this->m_type == FLOATING_POINT_LITERAL);
		//				}

		SHIFT_COMPILER_API bool token::is_void(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "void"));
		}

		SHIFT_COMPILER_API bool token::is_req(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "req"));
		}

		SHIFT_COMPILER_API bool token::is_use(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "use"));
		}

		SHIFT_COMPILER_API bool token::is_unsafe(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "unsafe"));
		}

		SHIFT_COMPILER_API bool token::is_extern(void) const noexcept {
			return ((this->is_identifier()) && (this->m_data == "extern" || this->m_data == "ext"));
		}

		SHIFT_COMPILER_API bool token::is_class(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "class");
		}

		SHIFT_COMPILER_API bool token::is_init(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "init");
		}

		SHIFT_COMPILER_API bool token::is_operator(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "operator");
		}

		SHIFT_COMPILER_API bool token::is_constructor(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "constructor");
		}

		SHIFT_COMPILER_API bool token::is_destructor(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "destructor");
		}

		SHIFT_COMPILER_API bool token::is_if(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "if");
		}
		SHIFT_COMPILER_API bool token::is_else(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "else");
		}

		SHIFT_COMPILER_API bool token::is_while(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "while");
		}

		SHIFT_COMPILER_API bool token::is_do(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "do");
		}

		SHIFT_COMPILER_API bool token::is_return(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "return");
		}

		SHIFT_COMPILER_API bool token::is_continue(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "continue");
		}

		SHIFT_COMPILER_API bool token::is_break(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "break");
		}

		SHIFT_COMPILER_API bool token::is_for(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "for");
		}

		SHIFT_COMPILER_API bool token::is_valid_class_name(void) const noexcept {
			return (this->is_identifier()) && (!this->is_keyword());
		}

		SHIFT_COMPILER_API bool token::is_this(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "this");
		}

		SHIFT_COMPILER_API bool token::is_base(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "base");
		}

		SHIFT_COMPILER_API bool token::is_access_specifier(void) const noexcept {
			return (this->is_identifier())
					&& (this->is_public() || this->is_protected() || this->is_private() || this->is_static() || this->is_extern() || this->is_binary()
							|| this->is_const() || this->is_unsafe());
		}

		SHIFT_COMPILER_API bool token::is_overload_operator(void) const noexcept {
			return this->is_prefix_overload_operator() || this->is_suffix_overload_operator();
		}

		SHIFT_COMPILER_API bool token::is_prefix_overload_operator(void) const noexcept {
			return this->is_unary_operator();
		}

		SHIFT_COMPILER_API bool token::is_suffix_overload_operator(void) const noexcept {
			return (!this->is_identifier())
					&& ((this->is_binary_operator()) || this->get_token_type() == token_type::MINUS_MINUS
							|| this->get_token_type() == token_type::PLUS_PLUS || this->get_token_type() == token_type::LEFT_SQUARE_BRACKET
							|| this->get_token_type() == token_type::RIGHT_SQUARE_BRACKET);
		}

		SHIFT_COMPILER_API bool token::is_unary_operator(void) const noexcept {
			return (!this->is_identifier())
					&& ((this->get_token_type() == token_type::LEFT_BRACKET) // cast
					|| (this->get_token_type() == token_type::FLIP_BITS) || (this->get_token_type() == token_type::PLUS_PLUS)
							|| (this->get_token_type() == token_type::MINUS_MINUS) || (this->get_token_type() == token_type::NOT));
		}

		SHIFT_COMPILER_API bool token::is_binary_operator(void) const noexcept {
			/*
			 * Old version, manually checking token type of each binary operator
			 */
			return (!this->is_identifier())
					&& (((this->get_token_type() & token_type::EQUALS) == token_type::EQUALS)
							|| ((this->get_token_type() & token_type::AND) == token_type::AND) || (this->get_token_type() == token_type::DIVIDE)
							|| (this->get_token_type() == token_type::EQUALS) || (this->get_token_type() == token_type::GREATER_THAN)
							|| (this->get_token_type() == token_type::LESS_THAN) || (this->get_token_type() == token_type::MINUS)
							|| (this->get_token_type() == token_type::MODULO) || (this->get_token_type() == token_type::MULTIPLY)
							|| (this->get_token_type() == token_type::NOT_EQUAL) || ((this->get_token_type() & token_type::OR) == token_type::OR)
							|| (this->get_token_type() == token_type::OR_EQUALS) || (this->get_token_type() == token_type::PLUS)
							|| (this->get_token_type() == token_type::XOR) || (this->get_token_type() == token_type::SHIFT_LEFT)
							|| (this->get_token_type() == token_type::SHIFT_RIGHT));

			// This way is more efficient, but should be removed if some how tertiary operators are introduced
			//return !this->is_unary_operator(); // does not work 100%, cuz of things like CHAR_LITERAL; revert back to old method
		}

		SHIFT_COMPILER_API bool token::is_number(void) const noexcept {
			return this->get_token_type() == token_type::NUMBER_LITERAL || this->get_token_type() == token_type::FLOAT
					|| this->get_token_type() == token_type::DOUBLE || this->get_token_type() == token_type::BINARY_NUMBER
					|| this->get_token_type() == token_type::HEX_NUMBER;
		}

		SHIFT_COMPILER_API bool token::is_negative_number(void) const noexcept {
			return this->is_number() && this->get_data().at(0) == char('-');
		}

		SHIFT_COMPILER_API bool token::is_literal(void) const noexcept {
			return this->is_string_literal() || this->is_number_literal() || this->is_char_literal();
		}

		SHIFT_COMPILER_API bool token::is_keyword(void) const noexcept {
			return (this->is_identifier())
					&& (this->is_binary() || this->is_const() || this->is_extern() || this->is_module() || this->is_namespace() || this->is_private()
							|| this->is_protected() || this->is_public() || this->is_req() || this->is_unsafe() || this->is_use() || this->is_void()
							|| this->is_class() || this->is_init() || this->is_operator() || this->is_constructor() || this->is_destructor()
							|| this->is_this() || this->is_if() || this->is_else() || this->is_while() || this->is_return() || this->is_continue()
							|| this->is_break() || this->is_for() || this->is_true() || this->is_false() || this->is_access_specifier());
		}

		SHIFT_COMPILER_API bool token::is_alias(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "alias");
		}

		SHIFT_COMPILER_API bool token::is_true(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "true");
		}

		SHIFT_COMPILER_API bool token::is_false(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "false");
		}

		SHIFT_COMPILER_API bool token::is_asm(void) const noexcept {
			return (this->is_identifier()) && (this->m_data == "asm" || this->m_data == "_asm_" || this->m_data == "__asm__");
		}

		// whether this token can be found in assembly (i.e. compatible with asm blocks)
		SHIFT_COMPILER_API bool token::is_asm_compatible(void) const noexcept {
			return (this->is_identifier()) || (this->get_token_type() == token_type::LEFT_SQUARE_BRACKET)
					|| (this->get_token_type() == token_type::RIGHT_SQUARE_BRACKET) || (this->get_token_type() == token_type::LEFT_BRACKET)
					|| (this->get_token_type() == token_type::RIGHT_BRACKET) || (this->is_number()) || (this->get_token_type() == token_type::DOT)
					|| (this->get_token_type() == token_type::PLUS) || (this->get_token_type() == token_type::MINUS)
					|| (this->get_token_type() == token_type::MULTIPLY) || (this->get_token_type() == token_type::COMMA);
		}

//		SHIFT_COMPILER_API tokenizer::tokenizer(const shift::io::file& file, shift::compiler::shift_error_handler& handler) : m_file(file), m_error_handler(
//				&handler) {
//		}

		SHIFT_COMPILER_API tokenizer::tokenizer(const shift::io::file& file, shift::compiler::shift_error_handler* const handler) : m_file(file), m_error_handler(
				handler ? handler : (new shift_error_handler())), m_del_error_handler(!handler) {
		}

//		SHIFT_COMPILER_API tokenizer::tokenizer(const shift::io::file& file, shift::compiler::shift_error_handler&& handler) : m_file(file), m_error_handler(
//				std::move(handler)) {
//		}

//		SHIFT_COMPILER_API tokenizer::tokenizer(shift::io::file&& file, shift::compiler::shift_error_handler& handler) : m_file(std::move(file)), m_error_handler(
//				&handler) {
//		}

		SHIFT_COMPILER_API tokenizer::tokenizer(shift::io::file&& file, shift::compiler::shift_error_handler* const handler) : m_file(
				std::move(file)), m_error_handler(handler ? handler : (new shift_error_handler())), m_del_error_handler(!handler) {
		}

		SHIFT_COMPILER_API tokenizer::tokenizer(tokenizer&& other) : m_file(std::move(other.m_file)), m_error_handler(other.m_error_handler), m_del_error_handler(
				other.m_del_error_handler), m_tokens(std::move(other.m_tokens)), m_lines(std::move(other.m_lines)), m_filedata(
				std::move(other.m_filedata)), m_token_marks(std::move(other.m_token_marks)), m_token_index(std::move(other.m_token_index)) {
			other.m_del_error_handler = false;
			other.m_error_handler = nullptr;
		}

		SHIFT_COMPILER_API tokenizer::~tokenizer(void) noexcept {
			if (m_del_error_handler)
				delete m_error_handler;
		}

		SHIFT_COMPILER_API tokenizer& tokenizer::operator=(tokenizer&& other) {
			m_file = std::move(other.m_file);
			m_error_handler = other.m_error_handler;
			m_del_error_handler = other.m_del_error_handler;
			m_tokens = std::move(other.m_tokens);
			m_lines = std::move(other.m_lines);
			m_filedata = std::move(other.m_filedata);
			m_token_marks = std::move(other.m_token_marks);
			m_token_index = std::move(other.m_token_index);

			other.m_del_error_handler = false;
			other.m_error_handler = nullptr;
			return *this;
		}

//		SHIFT_COMPILER_API tokenizer::tokenizer(shift::io::file&& file, shift::compiler::shift_error_handler&& handler) : m_file(std::move(file)), m_error_handler(
//				std::move(handler)) {
//		}

		SHIFT_COMPILER_API void tokenizer::mark(void) noexcept {
			this->m_token_marks.push(this->m_token_index);
		}

		SHIFT_COMPILER_API void tokenizer::rollback(void) noexcept {
			if (!this->m_token_marks.empty()) {
				this->m_token_index = this->m_token_marks.top();
				this->m_token_marks.pop();
			}
		}

		SHIFT_COMPILER_API void tokenizer::clear_marks(typename std::stack<typename std::list<token>::const_iterator>::size_type count) noexcept {
			if (count > this->m_token_marks.size()) {
				count = this->m_token_marks.size();
			}

			for (; count > 0; --count) {
				this->m_token_marks.pop();
			}
		}

		typename std::list<token>::const_iterator tokenizer::set_index(const typename std::list<token>::const_iterator index) noexcept {
			typename std::list<token>::const_iterator ret = this->m_token_index;
			this->m_token_index = index;
			return ret;
		}

		const token* tokenizer::token_at(const file_indexer& index) const noexcept {
			for (const token& token_ : this->m_tokens) {
				if (token_.get_file_index() == index)
					return &token_;
			}
			return &token::null;
		}

		const token* tokenizer::token_before(const file_indexer& index) const noexcept {
			const token* last_token = &token::null;

			for (const token& token : this->m_tokens) {
				if (token.get_file_index() == index)
					return last_token;
				last_token = &token;
			}

			return &token::null;
		}

		const token* tokenizer::token_after(const file_indexer& index) const noexcept {
			bool next = false;

			for (const token& token : this->m_tokens) {
				if (next)
					return &token;
				if (token.get_file_index() == index)
					next = true;
			}

			return &token::null;
		}

		SHIFT_COMPILER_API const token* tokenizer::current_token(void) const noexcept {
			if (this->m_token_index == this->m_tokens.cend()
#ifdef __MINGW32__
					|| this->m_token_index._M_node == nullptr
#endif
							) {
				return &token::null;
			}

			return this->token_at(this->m_token_index);
		}

		SHIFT_COMPILER_API const token* tokenizer::peek_token(typename std::list<token>::size_type count) const noexcept {
			typename std::list<token>::const_iterator copy = this->m_token_index;

			auto const end = this->m_tokens.cend();

			for (; count && copy != end; --count, ++copy);

			if (count) {
				return &token::null;
			}

			return this->token_at(copy);
		}

		SHIFT_COMPILER_API const token* tokenizer::reverse_token(typename std::list<token>::size_type count) noexcept {
			auto const begin = this->m_tokens.cbegin();

			for (; count && this->m_token_index != begin; --count, --this->m_token_index);

			return this->current_token();
		}

		SHIFT_COMPILER_API const token* tokenizer::reverse_peek_token(typename std::list<token>::size_type count) const noexcept {
			typename std::list<token>::const_iterator copy = this->m_token_index;

			auto const begin = this->m_tokens.cbegin();

			for (; count && copy != begin; --count, --copy);

			if (count != 0) {
				return &token::null;
			}

			return this->token_at(copy);
		}

		SHIFT_COMPILER_API const token* tokenizer::next_token(typename std::list<token>::size_type count) noexcept {
			auto const end = this->m_tokens.cend();

			for (; count && this->m_token_index != end; --count, ++this->m_token_index);

			debug_log("next_token() is " << this->current_token());
			return this->current_token();
		}

		SHIFT_COMPILER_API void tokenizer::tokenize(void) {
			if (!this->m_file) // Immediately exit if file does not exist
				return;

			// Clear all class data in case this function has been called more than once
			this->m_tokens.clear();
			this->m_filedata.clear();
			this->m_lines.clear();
			this->m_error_handler->get_messages().clear();
			this->m_filedata.clear();
			clear(this->m_token_marks);
			this->m_token_index = this->m_tokens.cbegin();

			std::uintmax_t filesize = this->m_file.size(); // retrieves real size of file; we know it must be at least this large
			{ // read the file
				std::ifstream input_file(this->m_file.absolute_path(), std::ios_base::in);

				// reserve correct amount of bytes within file data string
				this->m_filedata.resize(filesize, char(0x0));

				input_file.read(this->m_filedata.data(), filesize); // read the file fully

				filesize = std::uintmax_t(input_file.gcount()); // change file size to real file size
				this->m_filedata.resize(filesize); // resize the string to the right size
				input_file.close();
			}

			{ // tokenizing
				size_t last_line = 0; // index of character after last \n
				char current = this->m_filedata[0]; // Current character (i.e. cursor)
				size_t i, line, col; // index (starts at 0), line # (starts at 1), column # (starts at 1)

				this->m_lines.reserve(shift::count(this->m_filedata, std::string_view("\n")) + 1);

				for (i = 0, line = 1, col = 1; i < filesize; shift_tokenizer_advance_()) {
					if (shift_tokenizer_is_whitespace(current)) {
						if (shift_tokenizer_current_equal('\n')) {
							shift_tokenizer_next_line();
							// col++ = 1 // col will be incremented to 1 by shift_tokenizer_advance_() in the for loop
						} else if (shift_tokenizer_current_equal('\t')) {
							col += 3; // tabs are like 4 spaces. col with be increment the 4th time by shift_tokenizer_advance_() in the for loop
						}
						continue;
					}

					if (isalpha(current) || shift_tokenizer_current_equal('_')) {
						const size_t old_col = col;
						const size_t old_i = i;

						for (shift_tokenizer_pre_advance_(); (i < filesize) && (isalnum(current) || shift_tokenizer_current_equal('_'));
								shift_tokenizer_advance_());
						if (i < filesize)
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
								SHIFT_TOKENIZER_FATAL_ERROR(line, col, 1, "Expected binary digit (bit), got '" << current << "'");
							}

							shift_tokenizer_reverse_();
							m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::BINARY_NUMBER, {
									line, old_col }));
						} else if ((shift_tokenizer_current_equal('x') || shift_tokenizer_current_equal('X'))
								&& ((i - old_i) == 1 && this->m_filedata[old_i] == char('0'))) {
							// hex number
							for (shift_tokenizer_pre_advance_(); i < filesize && shift_tokenizer_is_hex(current); shift_tokenizer_advance_());

							if ((i - old_i) == 2) {
								SHIFT_TOKENIZER_FATAL_ERROR(line, col, 1, "Expected hexadecimal digit, got '" << current << "'");
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

							for (shift_tokenizer_pre_advance_(); i < filesize && isdigit(current); shift_tokenizer_advance_());

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

						if (isdigit(next) || shift_tokenizer_char_equal(next, '.')) {
							const size_t old_col = col;
							const size_t old_i = i;
							for (shift_tokenizer_pre_advance_(); i < filesize && isdigit(current); shift_tokenizer_advance_());

							bool has_decimal = false;
							if (shift_tokenizer_current_equal('.') && !shift_tokenizer_char_equal(next, '.')) {
								has_decimal = true;
								for (shift_tokenizer_pre_advance_(); i < filesize && isdigit(current); shift_tokenizer_advance_());
							} else if ((shift_tokenizer_current_equal('b') || shift_tokenizer_current_equal('B'))
									&& ((i - (old_i + 1)) == 1 && next == char('0'))) {
								// binary number
								for (shift_tokenizer_pre_advance_(); i < filesize && shift_tokenizer_is_binary(current); shift_tokenizer_advance_());

								if ((i - (old_i + 1)) == 2) {
									SHIFT_TOKENIZER_FATAL_ERROR(line, col, 1, "Expected binary digit (bit), got '" << current << "'");
								}
								shift_tokenizer_reverse_();
								m_tokens.push_back(
										token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::BINARY_NUMBER, { line,
												old_col }));
							} else if ((shift_tokenizer_current_equal('x') || shift_tokenizer_current_equal('X'))
									&& ((i - (old_i + 1)) == 1 && next == char('0'))) {
								// hex number
								for (shift_tokenizer_pre_advance_(); i < filesize && shift_tokenizer_is_hex(current); shift_tokenizer_advance_());

								if ((i - (old_i + 1)) == 2) {
									SHIFT_TOKENIZER_FATAL_ERROR(line, col, 1, "Expected hexadecimal digit, got '" << current << "'");
								}

								shift_tokenizer_reverse_();
								m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::HEX_NUMBER, {
										line, old_col }));
							}

							if (shift_tokenizer_current_equal('f') || shift_tokenizer_current_equal('F')) {
								m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::FLOAT, { line,
										old_col }));
							} else if (shift_tokenizer_current_equal('d') || shift_tokenizer_current_equal('D')) {
								m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::DOUBLE, { line,
										old_col }));
							} else if (has_decimal) {
								shift_tokenizer_reverse_();
								m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::FLOAT, { line,
										old_col }));
							} else {
								shift_tokenizer_reverse_();
								m_tokens.push_back(
										token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::NUMBER_LITERAL, { line,
												old_col }));
							}

						} else if (shift_tokenizer_char_equal(next, '=')) {
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

								// TODO
								// here, we should warn them if "current" is not a proper escape code
								//

								// TODO THIS MUST BE ACCOUNTED FOR
								// If the string we are parsing is: "hello\\nworld",
								// that is the EXACT same string we will get in the token data,
								// instead of "\n" in real text form (if it is was "hello\nworld", we'd get "\n" in ascii form)
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
							SHIFT_TOKENIZER_FATAL_ERROR(line, old_col, i - old_i + 1, "String literal must be terminated");
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
								// TODO
								// here, we should warn them if "current" is not a proper escape code
								//

							}
						} else if (shift_tokenizer_current_equal('\'')) {
							SHIFT_TOKENIZER_FATAL_ERROR(line, col, 1, "Character literal cannot be empty");
							continue;
						}

						shift_tokenizer_advance_();

						if (!shift_tokenizer_current_equal('\'')) {
							SHIFT_TOKENIZER_FATAL_ERROR(line, col, 1, "Expected ''', got '" << current << "'");
						}

						m_tokens.push_back(token(std::string_view(&this->m_filedata[old_i], i - old_i + 1), token::token_type::CHAR_LITERAL, { line,
								old_col }));
						continue;
					}

					// ALL OTHER CHARACTERS
					SHIFT_TOKENIZER_FATAL_ERROR(line, col, 1, "Unexpected symbol: '" << current << "'");

				}
			}
		}

	}
}
