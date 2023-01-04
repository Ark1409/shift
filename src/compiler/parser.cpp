/**
 * @file parser.cpp
 */
#include "parser.h"
#include <functional>
#include <algorithm>

#define SHIFT_PARSER_ERROR_PREFIX 				"error: " << std::filesystem::relative(this->m_tokenizer->get_file().get_path()).native() << ": " // std::filesystem::relative call every time probably isn't that optimal
#define SHIFT_PARSER_WARNING_PREFIX 			"warning: " << std::filesystem::relative(this->m_tokenizer->get_file().get_path()).native() << ": " // std::filesystem::relative call every time probably isn't that optimal

#define SHIFT_PARSER_ERROR_PREFIX_EXT_(__line__, __col__) "error: " << std::filesystem::relative(this->m_tokenizer->get_file().get_path()).native() << ":" << __line__ << ":" << __col__ << ": " // std::filesystem::relative call every time probably isn't that optimal
#define SHIFT_PARSER_WARNING_PREFIX_EXT_(__line__, __col__) "warning: " << std::filesystem::relative(this->m_tokenizer->get_file().get_path()).native() << ":" << __line__ << ":" << __col__ << ": " // std::filesystem::relative call every time probably isn't that optimal

#define SHIFT_PARSER_ERROR_PREFIX_EXT(__token) SHIFT_PARSER_ERROR_PREFIX_EXT_((__token)->get_file_index().line, (__token)->get_file_index().col)
#define SHIFT_PARSER_WARNING_PREFIX_EXT(__token) SHIFT_PARSER_WARNING_PREFIX_EXT_((__token)->get_file_index().line, (__token)->get_file_index().col)

#define SHIFT_PARSER_PRINT() this->m_error_handler->print_and_exit()

#define SHIFT_PARSER_WARNING(__WARN__) 			this->m_error_handler->stream() << SHIFT_PARSER_WARNING_PREFIX << __WARN__ << "\n"; this->m_error_handler->flush_stream(SHIFT_WARNING_MESSAGE_TYPE)
#define SHIFT_PARSER_FATAL_WARNING(__WARN__) 		SHIFT_PARSER_WARNING(__WARN__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_WARNING_LOG(__WARN__) 		this->m_error_handler->stream() << __WARN__ << "\n"; this->m_error_handler->flush_stream(SHIFT_WARNING_MESSAGE_TYPE)
#define SHIFT_PARSER_FATAL_WARNING_LOG(__WARN__)  SHIFT_PARSER_WARNING_LOG(__WARN__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_ERROR(__ERR__) 			this->m_error_handler->stream() << SHIFT_PARSER_ERROR_PREFIX << __ERR__ << "\n"; this->m_error_handler->flush_stream(SHIFT_ERROR_MESSAGE_TYPE)
#define SHIFT_PARSER_FATAL_ERROR(__ERR__) 		SHIFT_PARSER_ERROR(__ERR__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_ERROR_LOG(__ERR__) 		this->m_error_handler->stream() << __ERR__ << "\n"; this->m_error_handler->flush_stream(SHIFT_ERROR_MESSAGE_TYPE)
#define SHIFT_PARSER_FATAL_ERROR_LOG(__ERR__)  SHIFT_PARSER_ERROR_LOG(__ERR__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_WARNING_(__token, __WARN__) 	this->m_error_handler->stream() << SHIFT_PARSER_WARNING_PREFIX_EXT(__token) << __WARN__ << "\n"; this->m_error_handler->flush_stream(SHIFT_WARNING_MESSAGE_TYPE)
#define SHIFT_PARSER_FATAL_WARNING_(__token, __WARN__) 		SHIFT_PARSER_WARNING(__token, __WARN__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_WARNING_LOG_(__token, __WARN__) 		this->m_error_handler->stream() << __WARN__ << std::endl; this->m_error_handler->flush_stream(SHIFT_WARNING_MESSAGE_TYPE)
#define SHIFT_PARSER_FATAL_WARNING_LOG_(__token, __WARN__)  SHIFT_PARSER_WARNING_LOG_(__token, __WARN__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_ERROR_(__token, __ERR__) 			this->m_error_handler->stream() << SHIFT_PARSER_ERROR_PREFIX_EXT(__token) << __ERR__ << "\n"; this->m_error_handler->flush_stream(SHIFT_ERROR_MESSAGE_TYPE)
#define SHIFT_PARSER_FATAL_ERROR_(__token, __ERR__) 		SHIFT_PARSER_ERROR_(__token, __ERR__); SHIFT_PARSER_PRINT()

#define SHIFT_PARSER_ERROR_LOG_(__token, __ERR__) 		this->m_error_handler->stream() << __ERR__ << "\n"; this->m_error_handler->flush_stream(SHIFT_ERROR_MESSAGE_TYPE)
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

static shift::compiler::mods parse_access_specifier(const shift::compiler::token* const token) noexcept {
	using namespace shift::compiler;
	if (token->is_public()) {
		return mods::PUBLIC;
	} else if (token->is_protected()) {
		return mods::PROTECTED;

	} else if (token->is_private()) {
		return mods::PRIVATE;

	} else if (token->is_static()) {
		return mods::STATIC;

	} else if (token->is_const()) {
		return mods::CONST_;

	} else if (token->is_extern()) {
		return mods::EXTERN;

	} else if (token->is_binary()) {
		return mods::BINARY;
	}

	return static_cast<mods>(0x0);
}

static inline bool is_void(const shift::compiler::shift_name name) noexcept {
	return name.length() == 1 && name.begin()->is_void();
}

static constexpr uint_fast8_t operator_priority(const shift::compiler::token::token_type type) noexcept {
	using token_type = shift::compiler::token::token_type;

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
				return operator_priority((token_type) (type & ~token_type::EQUALS)) - 1;
			}
			return 0;
		}
	}
}

static constexpr inline uint_fast8_t operator_priority(const shift::compiler::token* const token) noexcept {
	return operator_priority(token->get_token_type());
}

static constexpr inline uint_fast8_t operator_priority(const std::list<shift::compiler::token>::const_iterator it) noexcept {
	return operator_priority(it.operator ->());
}

static constexpr shift::compiler::token::token_type opposite_bracket(const shift::compiler::token::token_type type) noexcept {
	switch (type) {
		case shift::compiler::token::token_type::LEFT_BRACKET:
			return shift::compiler::token::token_type::RIGHT_BRACKET;
		case shift::compiler::token::token_type::RIGHT_BRACKET:
			return shift::compiler::token::token_type::LEFT_BRACKET;

		case shift::compiler::token::token_type::LEFT_SCOPE_BRACKET:
			return shift::compiler::token::token_type::RIGHT_SCOPE_BRACKET;
		case shift::compiler::token::token_type::RIGHT_SCOPE_BRACKET:
			return shift::compiler::token::token_type::LEFT_SCOPE_BRACKET;

		case shift::compiler::token::token_type::LEFT_SQUARE_BRACKET:
			return shift::compiler::token::token_type::RIGHT_SQUARE_BRACKET;
		case shift::compiler::token::token_type::RIGHT_SQUARE_BRACKET:
			return shift::compiler::token::token_type::LEFT_SQUARE_BRACKET;
		default:
			return 0;
	}
}

static constexpr const char* bracket_string(const shift::compiler::token::token_type type) noexcept {
	switch (type) {
		case shift::compiler::token::token_type::LEFT_BRACKET:
			return "(";
		case shift::compiler::token::token_type::RIGHT_BRACKET:
			return ")";

		case shift::compiler::token::token_type::LEFT_SCOPE_BRACKET:
			return "{";
		case shift::compiler::token::token_type::RIGHT_SCOPE_BRACKET:
			return "}";

		case shift::compiler::token::token_type::LEFT_SQUARE_BRACKET:
			return "[";
		case shift::compiler::token::token_type::RIGHT_SQUARE_BRACKET:
			return "]";
		default:
			return nullptr;
	}
}

/** Namespace shift */
namespace shift {
	/** Namespace compiler */
	namespace compiler {
		SHIFT_COMPILER_API parser::parser(tokenizer* const tokenizer) noexcept : m_tokenizer(tokenizer), m_error_handler(
				tokenizer->get_error_handler()) {
		}

		SHIFT_COMPILER_API void parser::parse(void) {
			//this->m_tokenizer->set_index(this->m_tokenizer->get_tokens().cbegin());
			for (const token* token = this->m_tokenizer->current_token(); !token->is_null_token(); token = this->m_tokenizer->next_token()) {
				if (token->is_use()) {
					// use statement
					parse_use();
					continue;
				}

				if (token->is_class()) {
					// creating class, dont forget template args
					parse_class();
					continue;
				}

				if (token->is_access_specifier()) {
					this->parse_access_specifier();
					continue;
				}

				if (token->is_module()) {
					if (this->m_module.name.begin() == this->m_module.name.end()) {
						// module statement; expected the least (only once)
						parse_module();
					} else {
						this->token_error(token, "module already defined");
						this->skip_until(token::token_type::SEMICOLON);
					}
					continue;
				}

				if (token->is_identifier()) {
					// function / field in global scope?
				}

			}

			if (this->m_mods.size() > 0) {
				const token* const error_token = this->m_mods.front().second;
				this->token_error(error_token, "unexpected access specifier '" + std::string(error_token->get_data()) + "'");
			}
		}

		void parser::parse_module(void) {
			if (this->m_mods.size() > 0) {
				// list types and print them (or general error)
				this->token_error(this->m_mods.front().second, "unexpected access specifier");
				this->skip_until(token::token::SEMICOLON);
				return;
			}
			this->m_tokenizer->next_token(); // skip 'module' keyword

			this->m_module.name.begin_ = this->m_tokenizer->get_index();

			{
				token::token_type last_type = 0x0;

				for (const token* token = this->m_tokenizer->current_token(); true; token = this->m_tokenizer->next_token()) {
					if (token->is_access_specifier() && this->m_module.name.size() <= 0) {
						this->token_error(token, "expected valid module name after 'module'");
					}

					if (token->is_identifier()) {
						if (token->is_keyword()) {
							// error, not keywords in module names
							this->token_error(token, "module name cannot contain keyword");
							this->skip_until(token::token_type::SEMICOLON);
							return;
						}

						if (last_type == token::token_type::IDENTIFIER) {
							// cannot have two identifiers in a row in a module name
							this->token_error(token, "unexpected identifier");
							this->skip_until(token::token_type::SEMICOLON);
							return;
						}

						last_type = token::token_type::IDENTIFIER;
						continue;
					}

					if (token->get_token_type() == token::token_type::DOT) {
						if (last_type == token::token_type::DOT || last_type != token::token_type::IDENTIFIER) {
							// cannot have two identifiers in a row in a module name
							this->token_error(token, "unexpected '.' inside module declaration");
							this->skip_until(token::token_type::SEMICOLON);
							return;
						}

						last_type = token::token_type::DOT;
						continue;
					}

					if (token->is_semicolon()) {
						if (last_type == 0x0) {
							this->token_error(token, "expected module name before ';'");
							this->skip_until(token::token_type::SEMICOLON);
							return;
						}

						if (last_type == token::token_type::DOT) {
							this->token_error(this->m_tokenizer->reverse_peek_token(), "invalid module name");
							this->skip_until(token::token_type::SEMICOLON);
							return;
						}
						this->m_module.name.end_ = this->m_tokenizer->get_index();
						break;
					}

					if (token->is_null_token()) {
						this->token_error(this->m_tokenizer->reverse_token(), "expected ';' before end of file");
						return;
					}

					this->token_error(token, "unexpected '" + std::string(token->get_data()) + "' in module name");
					this->skip_until(token::token_type::SEMICOLON);
					return;
				}
			}

		}

		void parser::parse_use(void) {
			if (this->m_mods.size() > 0) {
				// list types and print them (or general error)
				this->token_error(this->m_mods.front().second, "unexpected access specifier");
				this->skip_until(token::token::SEMICOLON);
				return;
			}
			this->m_tokenizer->next_token(); // skip 'use' keyword

			shift_module module;
			module.name.begin_ = this->m_tokenizer->get_index();

			{
				token::token_type last_type = 0x0;

				for (const token* token = this->m_tokenizer->current_token(); true; token = this->m_tokenizer->next_token()) {
					if (token->is_access_specifier() && module.name.size() <= 0) {
						this->token_error(token, "expected valid module name after 'module'");
					}

					if (token->is_identifier()) {
						if (token->is_keyword()) {
							// error, not keywords in module names
							this->token_error(token, "module name cannot contain keyword");
							this->skip_until(token::token_type::SEMICOLON);
							return;
						}

						if (last_type == token::token_type::IDENTIFIER) {
							// cannot have two identifiers in a row in a module name
							this->token_error(token, "unexpected identifier");
							this->skip_until(token::token_type::SEMICOLON);
							return;
						}

						last_type = token::token_type::IDENTIFIER;
						continue;
					}

					if (token->get_token_type() == token::token_type::DOT) {
						if (last_type == token::token_type::DOT || last_type != token::token_type::IDENTIFIER) {
							// cannot have two identifiers in a row in a module name
							this->token_error(token, "unexpected '.' inside module declaration");
							this->skip_until(token::token_type::SEMICOLON);
							return;
						}

						last_type = token::token_type::DOT;
						continue;
					}

					if (token->is_semicolon()) {
						if (last_type == 0x0) {
							this->token_error(token, "expected module name before ';'");
							this->skip_until(token::token_type::SEMICOLON);
							return;
						}

						if (last_type == token::token_type::DOT) {
							this->token_error(this->m_tokenizer->reverse_peek_token(), "invalid module name");
							this->skip_until(token::token_type::SEMICOLON);
							return;
						}

						module.name.end_ = this->m_tokenizer->get_index();
						break;
					}

					if (token->is_null_token()) {
						this->token_error(this->m_tokenizer->reverse_token(), "expected ';' before end of file");
						return;
					}

					this->token_error(token, "unexpected '" + std::string(token->get_data()) + "' in module name");
					this->skip_until(token::token_type::SEMICOLON);
					return;
				}
			}

			m_used_modules.push_back(std::move(module));
		}

		void parser::parse_class(shift_class* const parent) {
			shift_class clazz;
			clazz.module = &this->m_module;
			clazz.parent = parent; // if this class is found within another one

			{ // parse access specifiers
				for (clazz.name = this->m_tokenizer->next_token(); clazz.name->is_access_specifier(); clazz.name = this->m_tokenizer->next_token()) {
					this->parse_access_specifier();
				}

				// verify access specifiers
				constexpr mods_t allowed_class_mods = (mods::PUBLIC | mods::PROTECTED | mods::PRIVATE);

				for (const std::pair<mods_t, const token*>& mods_ : this->m_mods) {
					if ((mods_.first & allowed_class_mods) == 0x0) {
						this->token_error(mods_.second, "unexpected access specifier on class");
					}
				}

				// set mods
				clazz.mods = this->get_mods();
				this->clear_mods();
			}
			{ // TODO parse name here

			}

			{ // TODO parse template types over here

			}

			{ // TODO parse super class here
				clazz.super = &shift_class::object;
			}

			{ // parse open bracket '{'
				const token* open_bracket = this->m_tokenizer->next_token();

				if (!open_bracket->is_left_scope()) {
					if (open_bracket->is_null_token()) {
						open_bracket = &this->m_tokenizer->get_tokens().back();
					}
					this->token_error(open_bracket, "expected '{' after declaration of class '" + clazz.full_name() + "'");
					this->end_tokenizer();
					return;
				}
			}

			// parse body of class
			parse_class_(&clazz);

			{ // parse close bracket '}'
				const token* close_bracket = this->m_tokenizer->current_token();

				if (!close_bracket->is_left_scope()) {
					if (close_bracket->is_null_token()) {
						close_bracket = &this->m_tokenizer->get_tokens().back();
					}
					this->token_error(close_bracket, "expected '}' to close declaration of class '" + clazz.full_name() + "'");
					this->end_tokenizer();
					return;
				}
			}

		}

		void parser::parse_class_(shift_class* clazz) {
			for (const token* token = this->m_tokenizer->next_token(); !token->is_null_token(); token = this->m_tokenizer->next_token()) {
				if (token->is_access_specifier()) {
					this->parse_access_specifier();
					continue;
				}

				if (token->is_init()) {
					// do things
					continue;
				}

				if (token->is_constructor()) {
					// do things
					continue;
				}

				if (token->is_destructor()) {
					// do things
					continue;
				}

				if (token->is_use()) {
					//
				}

				if (token->is_right_scope()) {
					return;
				}

				if (token->is_module()) {
					this->token_error(token, "module cannot be defined within a class");
					this->skip_until(token::token_type::SEMICOLON);
					continue;
				}

				if (token->is_identifier()) {
					/*
					 * public void func()
					 *        ^
					 * or
					 *
					 * public shift.int func()
					 *        ^
					 *
					 * or
					 *
					 * public shift.int field;
					 * 	      ^
					 */
					shift_type type = this->parse_type(this->m_tokenizer->get_index());

					shift_name name;
					{
						const shift::compiler::token* name_ = this->m_tokenizer->current_token();

						for (; name_->is_access_specifier() && !name_->is_null_token(); name_ = this->m_tokenizer->next_token()) {
							this->parse_access_specifier();
						}

						name.begin_ = this->m_tokenizer->get_index();

						if (!name_->is_identifier()) {
							this->token_error(name_->is_null_token() ? &this->m_tokenizer->get_tokens().back() : name_,
									"expected identifier after type declaration");
							//continue;
						}

						if (name_->is_operator()) {
							const shift::compiler::token* operator_ = this->m_tokenizer->next_token();

							if (!operator_->is_overload_operator()) {
								this->token_error(operator_->is_null_token() ? &this->m_tokenizer->get_tokens().back() : operator_,
										"expected valid operator for function name after keyword 'operator'");
							}
						} else if (name_->is_keyword()) {
							this->token_error(name_, "unexpected keyword '" + std::string(name_->get_data()) + "'");
						}

						this->m_tokenizer->next_token();
						name.end_ = this->m_tokenizer->get_index();
					}

					const shift::compiler::token* const equal_or_bracket = this->m_tokenizer->current_token();

					if (equal_or_bracket->is_left_bracket()) {
						// function definition

						// parse params
						// parse left bracket
						// parse blocks
						// parse right bracket
					} else if (equal_or_bracket->is_equals() || equal_or_bracket->is_semicolon()) {
						// variable definition
						// parse expression
						// parse semicolon
					} else {
						if (equal_or_bracket->has_equals()) {
							this->token_error(equal_or_bracket,
									"expected ';' or '=' in variable definition, got '" + std::string(equal_or_bracket->get_data()) + "'");
						} else {
							this->token_error(equal_or_bracket->is_null_token() ? &this->m_tokenizer->get_tokens().back() : equal_or_bracket,
									"expected variable or function definition after type");
						}
					}

					continue;
				}
			}
		}

		const token* parser::skip_until(const std::string& str) noexcept {
			for (; !this->m_tokenizer->current_token()->is_null_token() && this->m_tokenizer->current_token()->get_data() != str;
					this->m_tokenizer->next_token());
			return this->m_tokenizer->current_token();
		}

		const token* parser::skip_until(typename token::token_type type) noexcept {
			for (; !this->m_tokenizer->current_token()->is_null_token() && this->m_tokenizer->current_token()->get_token_type() != type;
					this->m_tokenizer->next_token());
			return this->m_tokenizer->current_token();
		}

		const token* parser::skip_after(const std::string& str) noexcept {
			skip_until(str);
			return this->m_tokenizer->next_token();
		}

		const token* parser::skip_after(typename token::token_type type) noexcept {
			skip_until(type);
			return this->m_tokenizer->next_token();
		}

		typename std::list<const token*>::size_type parser::skip_until(const std::vector<const token*>& tokens,
				typename std::list<const token*>::size_type begin_index, const std::string& str) noexcept {
			for (; (begin_index < tokens.size()) && tokens[begin_index]->get_data() != str; ++begin_index);
			return begin_index;
		}

		typename std::list<const token*>::size_type parser::skip_after(const std::vector<const token*>& tokens,
				typename std::list<const token*>::size_type begin_index, const std::string& str) noexcept {
			return skip_until(tokens, begin_index, str) + 1;
		}

		typename std::list<const token*>::size_type parser::skip_until(const std::vector<const token*>& tokens,
				typename std::list<const token*>::size_type begin_index, typename token::token_type type) noexcept {
			for (; (begin_index < tokens.size()) && tokens[begin_index]->get_token_type() != type; ++begin_index);
			return begin_index;
		}

		typename std::list<const token*>::size_type parser::skip_after(const std::vector<const token*>& tokens,
				typename std::list<const token*>::size_type begin_index, typename token::token_type type) noexcept {
			return skip_until(tokens, begin_index, type) + 1;
		}

		void parser::end_tokenizer(void) noexcept {
			this->m_tokenizer->set_index(this->m_tokenizer->get_tokens().cend());
		}

		mods_t parser::get_mods(void) const noexcept {
			mods_t mods = 0x0;

			for (const std::pair<mods_t, const token*>& mods_ : this->m_mods) {
				mods |= mods_.first;
			}
			return mods;
		}

		bool parser::has_mods(mods_t m) const noexcept {
			return (get_mods() & m) == m;
		}

		void parser::clear_mods(void) noexcept {
			this->m_mods.clear();
		}

		void parser::token_error(const token* const token_, const std::string& msg) {
			SHIFT_PARSER_ERROR_(token_, msg);
			const std::string_view line = this->get_line(token_);

			size_t use_col = token_->get_file_index().col;
			std::for_each(line.begin(), line.end(), [&use_col](char& ch) {
				if (ch == '\t') {
					ch = ' ';
					use_col -= 3;
				}
			});

			std::string indexer;
			indexer.reserve((use_col - 1) + token_->get_data().size());
			indexer.resize(use_col - 1, ' ');
			indexer.resize((use_col - 1) + token_->get_data().size(), '^');

			SHIFT_PARSER_ERROR_LOG(line);
			SHIFT_PARSER_ERROR_LOG(indexer);
		}

		void parser::token_error(const token* const token_, const char* const msg) {
			SHIFT_PARSER_ERROR_(token_, msg);
			const std::string_view line = this->get_line(token_);

			size_t use_col = token_->get_file_index().col;
			std::for_each(line.begin(), line.end(), [&use_col](char& ch) {
				if (ch == '\t') {
					ch = ' ';
					use_col -= 3;
				}
			});

			std::string indexer;
			indexer.reserve((use_col - 1) + token_->get_data().size());
			indexer.resize(use_col - 1, ' ');
			indexer.resize((use_col - 1) + token_->get_data().size(), '^');

			SHIFT_PARSER_ERROR_LOG(line);
			SHIFT_PARSER_ERROR_LOG(indexer);
		}

		void parser::token_warning(const token* const token_, const std::string& msg) {
			SHIFT_PARSER_WARNING_(token_, msg);
			const std::string_view line = this->get_line(token_);

			size_t use_col = token_->get_file_index().col;
			std::for_each(line.begin(), line.end(), [&use_col](char& ch) {
				if (ch == '\t') {
					ch = ' ';
					use_col -= 3;
				}
			});

			std::string indexer;
			indexer.reserve((use_col - 1) + token_->get_data().size());
			indexer.resize(use_col - 1, ' ');
			indexer.resize((use_col - 1) + token_->get_data().size(), '^');

			SHIFT_PARSER_WARNING_LOG(line);
			SHIFT_PARSER_WARNING_LOG(indexer);
		}

		void parser::token_warning(const token* const token_, const char* const msg) {
			SHIFT_PARSER_WARNING_(token_, msg);
			const std::string_view line = this->get_line(token_);

			size_t use_col = token_->get_file_index().col;
			std::for_each(line.begin(), line.end(), [&use_col](char& ch) {
				if (ch == '\t') {
					ch = ' ';
					use_col -= 3;
				}
			});

			std::string indexer;
			indexer.reserve((use_col - 1) + token_->get_data().size());
			indexer.resize(use_col - 1, ' ');
			indexer.resize((use_col - 1) + token_->get_data().size(), '^');

			SHIFT_PARSER_WARNING_LOG(line);
			SHIFT_PARSER_WARNING_LOG(indexer);
		}

		std::string_view parser::get_line(const token* const t) const noexcept {
			return this->m_tokenizer->get_lines()[t->get_file_index().line - 1];
		}

		void parser::parse_access_specifier(void) {
			const token* const current_token = this->m_tokenizer->current_token();
			const mods mod = ::parse_access_specifier(current_token);
			const mods_t current_mods = this->get_mods();

			if ((mod) & (mods::PUBLIC | mods::PROTECTED | mods::PRIVATE)) {
				if ((current_mods) & (mods::PUBLIC | mods::PROTECTED | mods::PRIVATE)) {
					// if we are trying to add public, protected or private when we already have public protected or private, we may have an error
					if (((current_mods) & (mod)) == 0x0) {
						// error if the one we have (i.e. the public, protected or private that is currently specified (the one being stored in current_mods)) is NOT the one now being parsed
						this->token_error(current_token, "unexpected visibility specifier");
					}
				}
			}

			if ((current_mods & mod) == mod) {
				// send a warning if we are just adding the same modifier twice
				this->token_warning(current_token, "redundant '" + std::string(current_token->get_data()) + "' specifier");
			} else {
				this->add_mod(mod, current_token);
			}
		}

		void parser::add_mod(mods_t mod, const token* token) {
			this->m_mods.push_back( { mod, token });
		}

		shift_name parser::parse_name(std::list<token>::const_iterator& begin) {
			shift_name ret;

			// assumes the token (index) given is an identifier
			ret.begin_ = begin;

			token::token_type last_type = token::token_type::IDENTIFIER;

			for (++begin; begin._M_node; ++begin) {
				if (begin->is_identifier()) {
					if (last_type == token::token_type::IDENTIFIER)
						break;
					if (begin->is_keyword()) {
						this->token_error(begin.operator ->(), "unexpected keyword in name");
					}

					last_type = token::token_type::IDENTIFIER;
					continue;
				}

				if (begin->is_dot()) {
					if (last_type == token::token_type::DOT)
						this->token_error(begin.operator ->(), "unexpected '.' in type");
					last_type = token::token_type::DOT;
					continue;
				}

				break;
			}

			ret.end_ = begin;
			return ret;
		}

		shift_type parser::parse_type(std::list<token>::const_iterator& begin) {
			shift_type ret;

			// assumes the token (index) given is an identifier
			ret.begin_ = begin;

			token::token_type last_type = token::token_type::IDENTIFIER;

			if (!begin->is_void()) {
				for (++begin; begin._M_node; ++begin) {
					if (begin->is_identifier()) {
						if (last_type == token::token_type::IDENTIFIER || last_type == token::token_type::RIGHT_SQUARE_BRACKET)
							break;
						if (last_type == token::token_type::LEFT_SQUARE_BRACKET)
							this->token_error(begin.operator ->(), "unexpected identifier in type, expected ']'");
						if (begin->is_keyword())
							this->token_error(begin.operator ->(), "unexpected keyword in type");

						last_type = token::token_type::IDENTIFIER;
						continue;
					}

					if (begin->is_dot()) {
						if (last_type == token::token_type::RIGHT_SQUARE_BRACKET)
							break;
						if (last_type == token::token_type::DOT || last_type == token::token_type::LEFT_SQUARE_BRACKET)
							this->token_error(begin.operator ->(), "unexpected '.' in type");
						last_type = token::token_type::DOT;
						continue;
					}

					if (begin->is_left_square()) {
						if (last_type == token::token_type::DOT) {
							this->token_error(begin.operator ->(), "unexpected '[' in type");
						} else if (last_type == token::token_type::LEFT_SQUARE_BRACKET) {
							this->token_error(begin.operator ->(), "unexpected '[' in type, expected ']'");
						} else if (last_type == token::token_type::IDENTIFIER || last_type == token::token_type::RIGHT_SQUARE_BRACKET) {
							// good
						}
						continue;
					}

					if (begin->is_right_square()) {
						if (last_type == token::token_type::DOT || last_type == token::token_type::IDENTIFIER) {
							this->token_error(begin.operator ->(), "unexpected ']' in type");
						} else if (last_type == token::token_type::RIGHT_SQUARE_BRACKET) {
							this->token_error(begin.operator ->(), "unexpected ']' in type, expected '[' before ']'");
						} else if (last_type == token::token_type::LEFT_SQUARE_BRACKET) {
							// good
						}
						continue;
					}

					break;
				}
			} else {
				++begin;
			}

			ret.end_ = begin;
			return ret;
		}

		bool parser::skip_bracket(std::list<token>::const_iterator& begin, const std::list<token>::const_iterator end) {
			const token::token_type orig_type = begin->get_token_type();
			const token::token_type opp_type = ::opposite_bracket(orig_type);
			{
				size_t bracket_count = 1;
				for (++begin; begin != end; begin++) {
					if (begin->get_token_type() == orig_type) {
						bracket_count++;
					} else if (begin->get_token_type() == opp_type) {
						bracket_count--;
						if (bracket_count == 0) {
							++begin;
							return true;
						}
					}
				}
			}

			return false;
		}

		shift_expression parser::parse_expression(std::list<token>::const_iterator begin, const std::list<token>::const_iterator end) {
			shift_expression expr;

			for (const std::list<token>::const_iterator old_begin = begin; begin != end; ++begin) {
				const token* const token = begin.operator ->();

				if (token->is_left_bracket()) {
					auto const orig_begin = begin;

					if (!expr.is_null_expr()) {
						this->token_error(token, "unexpected '(' within expression");
						return expr;
					}

					auto const bracket_begin = ++begin;
					if (!skip_bracket(--begin, end)) {
						this->token_error(&this->m_tokenizer->get_tokens().back(), "expected ')' before end of file");
					}

					auto const bracket_end = --begin;

					{
						constexpr auto bracket_priority = ::operator_priority(token::token_type::LEFT_BRACKET);
						auto priority_token = orig_begin;
						for (++begin; begin != end; ++begin) {
							if (begin->is_left_bracket_type() || begin->is_right_bracket_type()) {
								if (begin->is_left_bracket_type()) {
									skip_bracket(begin, end);
									--begin;
								}
								continue;
							}
							if (begin->is_overload_operator()
									&& (::operator_priority(begin->get_token_type()) <= ::operator_priority(priority_token->get_token_type()))) {
								priority_token = begin;
							}
						}

						if (priority_token->get_token_type() != token::token_type::LEFT_BRACKET) {
							begin = --priority_token;
							continue;
						}
					}

					begin = bracket_end;

					// (int)3 // cast
					// (int) + 3 // bracket
					// ++(int)3 // cast
					// (int)++3 // cast
					// (int)++ // bracket
					(std::string()) +++3;
					if ((++begin)->is_binary_operator()) {

					} else {
						// cast

						if (begin != end) {
							--begin;
							continue;
						}

						expr.set_cast();
						expr.set_left(parse_expression(bracket_begin, bracket_end));
						expr.set_right(parse_expression(++bracket_end, end));
						return expr;
					}
					continue;
				}

				if (token->is_unary_operator()) {
					continue;
				}

				if (token->is_binary_operator()) {
					continue;
				}

				if (token->is_number()) {
					if (!expr.is_null_expr()) {
						this->token_error(token, "unexpected number literal within expression");
					}

					expr.type = token->get_token_type();
					expr.data.begin_ = begin;
					expr.data.end_ = begin + 1;
					continue;
				}

				if (token->is_string_literal()) {
					if (!expr.is_null_expr()) {
						this->token_error(token, "unexpected string literal within expression");
					}

					expr.type = token::token_type::STRING_LITERAL;
					expr.data.begin_ = begin;
					expr.data.end_ = begin + 1;
					continue;
				}

				if (token->is_char_literal()) {
					if (!expr.is_null_expr()) {
						this->token_error(token, "unexpected char literal within expression");
					}

					expr.type = token::token_type::CHAR_LITERAL;
					expr.data.begin_ = begin;
					expr.data.end_ = begin + 1;
					continue;
				}
			}

			return shift_expression();
		}

		shift_expression parser::parse_expression(std::list<token>::const_iterator begin, const std::list<token>::const_iterator end) {
			/* does not parse variable or function declarations */

			shift_expression expr;

//			bool has_operator = false;
//			 for casting?
//			std::for_each(begin, end, [](const token& token) {
//
//			});

			// int i = (int)5;
			// int i = (int)5 + 3;
			// int i = (int.3)5 + 3;
			// int i = (int.)5 + 3;

			// parse bracket
			//

			for (const std::list<token>::const_iterator old_begin = begin; begin != end; ++begin) {
				const token* const token = begin.operator ->();

				if (token->is_left_bracket()) {
					/*
					 if (expr.is_cast()) {
					 shift_expression* real_expr = &expr;
					 while (real_expr->has_right()) {
					 real_expr = real_expr->right();
					 }
					 if (real_expr->has_right()) {
					 this->token_error(token, "unexpected '(' inside expression");
					 continue;
					 }
					 }

					 if (!expr.is_null_expr()) {
					 this->token_error(token, "unexpected '(' inside expression");
					 continue;
					 }

					 size_t bracket_count = 1;
					 typename std::list<token>::const_iterator bracket_begin;
					 for (bracket_begin = ++begin; begin != end; ++begin) {
					 if (begin->is_left_bracket()) {
					 bracket_count++;
					 } else if (begin->is_right_bracket()) {
					 bracket_count--;
					 if (bracket_count == 0)
					 break;
					 }
					 }
					 // (int)(4)(4)
					 if (bracket_count != 0) {
					 this->token_error(begin - 1, "expected ')' before end of file");
					 }
					 // ((5+5)*3)-9
					 shift_expression sub_expr = parse_expression(bracket_begin, begin);
					 // (5)(5)
					 // luckily cast takes priority over most operations, or this would never work
					 if (sub_expr.is_name() && !expr.is_cast()) {
					 expr.set_left(std::move(sub_expr));
					 expr.set_cast(); // left_bracket
					 } else {
					 if (expr.is_cast()) {
					 shift_expression* real_expr = &expr;
					 while (real_expr->has_right()) {
					 real_expr = real_expr->right();
					 }
					 real_expr->set_right(std::move(sub_expr));

					 //expr.set_right(std::move(sub_expr));
					 } else
					 expr = std::move(sub_expr);

					 }
					 */
					// re- think/do this
					// check if there are operators within this
					// if there are, just skip
					// if not, parse
					// (var + 3)(3,9)
					typename std::list<shift::compiler::token>::const_iterator bracket_begin;
					{
						size_t bracket_count = 1;
						for (bracket_begin = ++begin; begin != end; ++begin) {
							if (begin->is_left_bracket()) {
								bracket_count++;
							} else if (begin->is_right_bracket()) {
								bracket_count--;
								if (bracket_count == 0)
									break;
							}
						}
						// (int)(4)(4)
						if (bracket_count != 0) {
							this->token_error(begin - 1, "expected ')' before end of file");
							continue;
						}
					}

					const typename std::list<shift::compiler::token>::const_iterator bracket_end = begin;
					constexpr auto cast_priority = ::operator_priority(token::token_type::LEFT_BRACKET); // left_bracket = cast

					auto priority = cast_priority;
					for (++begin; begin != end; ++begin) {
						if (begin->is_left_bracket()) {
							size_t bracket_count = 1;
							for (++begin; begin != end && bracket_count > 0; ++begin) {
								if (begin->is_left_bracket()) {
									bracket_count++;
								} else if (begin->is_right_bracket()) {
									bracket_count--;
								}
							}

							if (bracket_count != 0) {
								this->token_error(--begin, "expected ')' before end of file");
							} else {
								--begin;
							}
							continue;
						}
						// (int)5 * 3 + 2
						// (5)5 * 3 + 2

						if (begin->is_binary_operator()) {
							const auto token_priority = ::operator_priority(begin.operator ->());

							if (token_priority < priority)
								// skip cast
								break;
							continue;
						}

						if (begin->is_unary_operator()) {

						}
						/*
						 if (token_priority < cast_priority) {
						 // (int)5 + 3 -----> GOOD
						 // (int)5 + test [3] -----> GOOD
						 // (int)(5 + test)[3] -----> ???
						 // skip entire cast
						 priority = std::min(priority, token_priority);
						 continue;
						 }

						 if (token_priority > cast_priority) {
						 // (int)test[3] -----> GOOD
						 // (int)test[3] + 3 -----> NOT GOOD
						 // (int)(5 + test)[3] -----> ???
						 // (int) 5 + (intfa)+ 4
						 // ~~parse~~ plan to parse rest of cast
						 priority = std::min(priority, token_priority);
						 continue;
						 }

						 if (cast_begin->is_left_bracket()) {

						 }

						 if (token_priority == cast_priority) {
						 // (int)5 + 3 -----> GOOD
						 // (int)(int)5 + 3 -----> GOOD
						 // (int)(int)5 + (int)3 -----> GOOD
						 // (int)((int)5 + (int)3) -----> ? parse rest of cast
						 // (int)5 + ((int)5 + (int)3) -----> ? skip rest of cast
						 // (int)5++; = (int)(5++) -----> ?
						 // skip entire cast
						 }*/
						// (5 + 3) + 9;
					}

					if (begin == end) {
						// cast should be parsed
						shift_expression cast_expr = parse_expression(bracket_begin, bracket_end);
						if (!cast_expr.is_name()) {
							// (5+3)/9
							// (3)
							// (3)myVar
							if ((bracket_end + 1) != end) {
								this->token_error(bracket_begin - 1, "unexpected '(' inside expression");
							} else {
								expr = std::move(cast_expr);
							}
						} else {
							//(int)(long)5
							// (int)
							// var[99](2,3);
							if ((bracket_end + 1) == end) {
								// (myVar)
								expr = std::move(cast_expr);
							} else {
								// (int)5
								// is cast
								expr.set_cast();
								expr.set_left(std::move(cast_expr));
								expr.set_right(parse_expression(bracket_end + 1, end));
								break;
							}
						}
						--begin;
					} else {
						--begin;
					}

					continue;
				}

				if (token->is_right_bracket()) {
					this->token_error(token, "unexpected ')' inside expression");
					continue;
				}

				if (token->is_left_square_bracket()) {
					// (int)(4)[3]
					// (int)(4)[3](7)
					// (myint + 4)[3](7)
					// (4)[3]
					// var3[0] + var1[var2[0] / (3 + 9) & 1] = 3;
					// var3[0][1][2] = 9;
					// var3[0][1][2]();
					// (var3 + 9)[0][1][2]();
					// (var3 + 9)[0][1][2]()[3]()[2](5,3);
					// (var3 + 9)[0][1][2]()[3]()[2](5,3)++ * -9 = 3;
					//
					// Cast vs bracketed-expression: bracket if not is operator or nothing, cast otherwise

					constexpr auto indexer_priority = ::operator_priority(token::token_type::LEFT_SQUARE_BRACKET); // LEFT_SQUARE_BRACKET = array operator
					auto priority = indexer_priority;

					for (; begin != end; ++begin) {
						if (begin->is_left_bracket_type()) {
							const compiler::token* const loop_token = begin.operator ->();
							if (!this->skip_bracket(begin, end)) {
								this->token_error(begin - 1,
										"expected '" + std::string(bracket_string(opposite_bracket(loop_token->get_token_type())))
												+ "' before end of file");
							}
							--begin;
							continue;
						}

						if (begin->is_right_bracket_type()) {
							this->token_error(begin, "unexpected '" + std::string(bracket_string(begin->get_token_type())) + "' inside expression");
							continue;
						}
					}

					typename std::list<shift::compiler::token>::const_iterator bracket_begin = begin + 1;
					if (!this->skip_bracket(begin, end)) {
						this->token_error(begin - 1, "expected ']' before end of file");
					}

					const typename std::list<shift::compiler::token>::const_iterator bracket_end = begin - 1;

					/*

					 //					typename std::list<token>::const_iterator bracket_begin;
					 //					{
					 //						size_t bracket_count = 1;
					 //						for (bracket_begin = ++begin; begin != end; ++begin) {
					 //							if (begin->is_left_square_bracket()) {
					 //								bracket_count++;
					 //							} else if (begin->is_right_square_bracket()) {
					 //								bracket_count--;
					 //								if (bracket_count == 0)
					 //									break;
					 //							}
					 //						}
					 //
					 //						if (bracket_count != 0) {
					 //							this->token_error(begin - 1, "expected ']' before end of file");
					 //							continue;
					 //						}
					 //					}

					 typename std::list<compiler::token>::const_iterator bracket_begin = begin + 1;
					 if (!this->skip_bracket(begin, end)) {
					 this->token_error(begin - 1, "expected ']' before end of file");
					 }

					 const typename std::list<shift::compiler::token>::const_iterator bracket_end = begin - 1;
					 constexpr auto indexer_priority = ::operator_priority(token::token_type::LEFT_SQUARE_BRACKET); // LEFT_SQUARE_BRACKET = array operator

					 auto priority = indexer_priority;
					 for (; begin != end; ++begin) {

					 if (begin->is_left_bracket_type()) {
					 const compiler::token* const loop_token = begin.operator ->();
					 if (!this->skip_bracket(begin, end)) {
					 this->token_error(begin - 1,
					 "expected '" + std::string(bracket_string(opposite_bracket(loop_token->get_token_type())))
					 + "' before end of file");
					 }
					 --begin;
					 continue;
					 }

					 if (begin->is_right_bracket_type()) {
					 this->token_error(begin, "unexpected '" + std::string(bracket_string(begin->get_token_type())) + "' inside expression");
					 continue;
					 }

					 //						if (begin->is_left_square_bracket()) {
					 //							size_t bracket_count = 1;
					 //							for (++begin; begin != end && bracket_count > 0; ++begin) {
					 //								if (begin->is_left_square_bracket()) {
					 //									bracket_count++;
					 //								} else if (begin->is_right_square_bracket()) {
					 //									bracket_count--;
					 //								}
					 //							}
					 //
					 //							if (bracket_count != 0) {
					 //								this->token_error(--begin, "expected ']' before end of file");
					 //							} else {
					 //								--begin;
					 //							}
					 //							continue;
					 //						}
					 // (int)5 * 3 + 2
					 // (5)5 * 3 + 2

					 if (begin->is_overload_operator()) {
					 const auto token_priority = ::operator_priority(begin.operator ->());
					 if (token_priority < priority)
					 // skip cast
					 break;
					 }

					 }

					 if (begin == end) {
					 // cast should be parsed
					 shift_expression cast_expr = parse_expression(bracket_begin, bracket_end);
					 if (!cast_expr.is_name()) {
					 // (5+3)/9
					 // (3)
					 // (3)myVar
					 if ((bracket_end + 1) != end) {
					 this->token_error(bracket_begin - 1, "unexpected '(' inside expression");
					 } else {
					 expr = std::move(cast_expr);
					 }
					 } else {
					 //(int)(long)5
					 // (int)
					 if ((bracket_end + 1) == end) {
					 // (myVar)
					 expr = std::move(cast_expr);
					 } else {
					 // (int)5
					 // is cast
					 expr.set_cast();
					 expr.set_left(std::move(cast_expr));
					 expr.set_right(parse_expression(bracket_end + 1, end));
					 break;
					 }
					 }
					 --begin;
					 } else {
					 --begin;
					 }
					 */
					continue;
				}

				if (token->is_right_square_bracket()) {
					this->token_error(token, "unexpected ']' inside expression");
					continue;
				}

				if (token->is_binary_operator()) {
					typename std::list<shift::compiler::token>::const_iterator index = begin;
					auto current_priority = operator_priority(token);

					for (++begin; begin != end; ++begin) {
						if (begin->is_left_bracket()) {
							size_t bracket_count = 1;
							for (++begin; begin != end && bracket_count > 0; ++begin) {
								if (begin->is_left_bracket()) {
									bracket_count++;
								} else if (begin->is_right_bracket()) {
									bracket_count--;
								}
							}

							if (bracket_count != 0) {
								this->token_error(--begin, "expected ')' before end of file");
							} else {
								--begin;
							}
							continue;
						}
						// if is_operator?
						const auto priority = operator_priority(begin.operator ->());
						if (priority <= current_priority) {
							current_priority = priority;
							index = begin;
						}
					}

					expr.clear();
					expr.type = index->get_token_type();
					expr.data.begin_ = index;
					expr.data.end_ = index + 1;

					if (index->is_binary_operator()) {
						if (expr.data.begin() != old_begin) {
							expr.set_left(parse_expression(old_begin, expr.data.begin()));
						} else {
							// nothing on left side: error
							this->token_error(index,
									"unexpected '" + std::string(index->get_data()) + "' in expression, equation does not define a left hand side");
						}

						if (expr.data.end() != end) {
							expr.set_right(parse_expression(expr.data.end(), end));
						} else {
							// nothing on right side: error
							this->token_error(index,
									"unexpected '" + std::string(index->get_data()) + "' in expression, equation does not define a right hand side");
						}
					} else {
						// TODO implement unary with less priority
						debug_log("Cannot handle unary operator with less priority than binary operator");
					}

					break;
				}

				if (token->is_unary_operator()) {
					// 5 + ++fff++
					// 5 / ++fff
					// ++5++ / fff++

					// if at beginning or after operator -> prefix
					// else suffix

					expr.type = token->get_token_type();

					typename std::list<shift::compiler::token>::const_iterator unary_begin = begin;
					auto priority = ::operator_priority(token);

					for (++unary_begin; unary_begin != end; ++unary_begin) {
						if (begin->is_left_bracket()) {
							size_t bracket_count = 1;
							for (++unary_begin; unary_begin != end && bracket_count > 0; ++unary_begin) {
								if (unary_begin->is_left_bracket()) {
									bracket_count++;
								} else if (unary_begin->is_right_bracket()) {
									bracket_count--;
								}
							}

							if (bracket_count != 0) {
								this->token_error(--unary_begin, "expected ')' before end of file");
							} else {
								--unary_begin;
							}
							continue;
						}

						if (token->is_right_bracket()) {
							this->token_error(token, "unexpected ')' inside expression");
							continue;
						}

						if (unary_begin->is_binary_operator()) {
							const auto token_priority = operator_priority(unary_begin);
							if (token_priority <= priority) {
								break;
							}
						} else if (unary_begin->is_overload_operator()) {

						}
					}

					if (unary_begin == end) {
						// everything else is of ~~equal or~~ higher priority i.e. this is lowest priority
						// parse everything else
						// ++myInt * 3 / 2;
						expr.data.begin_ = begin;
						expr.data.end_ = begin + 1;
						if (expr.is_null_expr() || (begin - 1)->is_overload_operator()) {
							// i.e. if before
							// ++(things after)
							expr.set_left(parse_expression(++begin, end));
						} else {
							// i.e. if after
							// (things before)++
							// 5 * 9++ - 3
							// 9++ * 5 - 3
							expr.set_left(parse_expression(old_begin, begin));
						}
						break;
					} else {
						// not the thing of lowest priority, will split elsewhere in loop
						begin = --unary_begin;
						continue;
					}
					/*
					 if (expr.is_null_expr() || (begin - 1)->is_overload_operator()) {
					 // ++(things after)
					 // ^

					 // check if other operators
					 // if no, parse rest
					 // if yes, skip

					 typename std::list<shift::compiler::token>::const_iterator unary_begin = begin;
					 auto priority = ::operator_priority(token);

					 for (++unary_begin; unary_begin != end; ++unary_begin) {
					 if (begin->is_left_bracket()) {
					 size_t bracket_count = 1;
					 for (++unary_begin; unary_begin != end && bracket_count > 0; ++unary_begin) {
					 if (unary_begin->is_left_bracket()) {
					 bracket_count++;
					 } else if (unary_begin->is_right_bracket()) {
					 bracket_count--;
					 }
					 }

					 if (bracket_count != 0) {
					 this->token_error(--unary_begin, "expected ')' before end of file");
					 } else {
					 --unary_begin;
					 }
					 continue;
					 }

					 if (token->is_right_bracket()) {
					 this->token_error(token, "unexpected ')' inside expression");
					 continue;
					 }

					 if (unary_begin->is_binary_operator()) {
					 const auto token_priority = operator_priority(unary_begin);
					 if (token_priority <= priority) {
					 break;
					 }
					 } else if (unary_begin->is_overload_operator()) {

					 }
					 }

					 if (unary_begin == end) {
					 // everything else is of ~~equal or~~ higher priority i.e. this is lowest priority
					 // parse everything else
					 // ++myInt * 3 / 2;
					 expr.data.begin_ = begin;
					 expr.data.end_ = begin + 1;
					 expr.set_left(parse_expression(++begin, end));
					 break;
					 } else {
					 // not the thing of lowest priority, will split elsewhere in loop
					 begin = --unary_begin;
					 continue;
					 }
					 } else {
					 // (things before)++
					 //				  ^
					 // 5++ * 9
					 // 9 * 5++

					 }
					 */
				}

				if (token->is_number()) {
					if (!expr.is_null_expr()) {
						this->token_error(token, "unexpected number literal within expression");
					}
//					shift_expression ret;
//					ret.type = token->get_token_type();
//					ret.data.begin_ = begin;
//					ret.data.end_ = begin + 1;
//
//					if (expr.is_cast()) {
//						shift_expression* real_expr = &expr;
//						while (real_expr->has_right()) {
//							real_expr = real_expr->right();
//						}
//						real_expr->set_right(std::move(ret));
//					} else {
//						expr = std::move(ret);
//					}
					expr.type = token->get_token_type();
					expr.data.begin_ = begin;
					expr.data.end_ = begin + 1;
					continue;
				}

				if (token->is_string_literal()) {
					if (!expr.is_null_expr()) {
						this->token_error(token, "unexpected string literal within expression");
					}
//					shift_expression ret;
//					ret.type = token::token_type::STRING_LITERAL;
//					ret.data.begin_ = begin;
//					ret.data.end_ = begin + 1;
//
//					if (expr.is_cast()) {
//						shift_expression* real_expr = &expr;
//						while (real_expr->has_right()) {
//							real_expr = real_expr->right();
//						}
//
//						real_expr->set_right(std::move(ret));
//					} else {
//						expr = std::move(ret);
//					}
					expr.type = token::token_type::STRING_LITERAL;
					expr.data.begin_ = begin;
					expr.data.end_ = begin + 1;
					continue;
				}

				if (token->is_char_literal()) {
					if (!expr.is_null_expr()) {
						this->token_error(token, "unexpected char literal within expression");
					}
//					shift_expression ret;
//					ret.type = token::token_type::CHAR_LITERAL;
//					ret.data.begin_ = begin;
//					ret.data.end_ = begin + 1;
//					if (expr.is_cast()) {
//						shift_expression* real_expr = &expr;
//						while (real_expr->has_right()) {
//							real_expr = real_expr->right();
//						}
//
//						real_expr->set_right(std::move(ret));
//					} else {
//						expr = std::move(ret);
//					}

					expr.type = token::token_type::CHAR_LITERAL;
					expr.data.begin_ = begin;
					expr.data.end_ = begin + 1;
					continue;
				}

				if (token->is_identifier()) {
					// var_name;
					// var_name.help();
					// var_name.test = 5;
					// (type_name)5;
					// (type_name)var_name;
					// var_name("hello");
					// var_name (+/-/*/=)
					// var_name 111;
					{
						bool should_parse = true;
						for (auto temp = begin; temp != end; ++temp) {
							if (temp->is_overload_operator()) {
								begin = --temp;
								should_parse = false;
								break;
							}
						}

						if (!should_parse)
							continue;
					}

					if (!expr.is_null_expr()) {
						this->token_error(token, "unexpected identifier within expression");
					}

					expr.set_name();
					expr.data = parse_name(begin);

					if (begin->is_left_bracket()) {
						// func call
						expr.set_left(expr); // must be done before set_function_call
						expr.set_right(shift_expression());
						expr.set_function_call();

						for (typename std::list<token>::const_iterator param_begin = ++begin; begin != end; ++begin) {
							if (begin->is_comma()) {
								if (param_begin == begin) {
									this->token_error(begin, "expected function parameter within expression");
									param_begin = begin + 1;
									continue;
								}

								expr.right()->sub.push_back(parse_expression(param_begin, begin));
								param_begin = begin + 1;
								continue;
							}

							if (begin->is_left_bracket()) {
								size_t bracket_count = 1;
								for (++begin; begin != end && bracket_count > 0; ++begin) {
									if (begin->is_left_bracket()) {
										bracket_count++;
									} else if (begin->is_right_bracket()) {
										bracket_count--;
									}
								}

								if (bracket_count != 0) {
									this->token_error(--begin, "expected ')' before end of file");
								} else {
									--begin;
								}
								continue;
							}

							if (begin->is_right_bracket()) {
								if (param_begin == begin && ((param_begin - 1)->is_comma())) {
									this->token_error(begin, "expected function parameter within expression");
								} else {
									if (param_begin != begin)
										expr.right()->sub.push_back(parse_expression(param_begin, begin));
								}
								break;
							}

						}

						if (begin == end) {
							this->token_error(--begin, "expected ')' for function call before end of file");
							continue;
						}

					} else {
						// normal identifier
						// nothing more has to be done
					}

					// parse this
					// dont check for "null + 5", "true / 3", etc: that step is for the semantic parser

					continue;
				}

				this->token_error(token, "unexpected '" + std::string(token->get_data()) + "' inside expression");
			}

			if (expr.type == token::token_type::NULL_TOKEN) { // should probably handle inside for loop for each case instead; what if we parse from function
				this->token_error(begin, "expected valid expression");
			}

			return expr;
		}

	}

}
