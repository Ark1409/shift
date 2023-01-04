/**
 * @file include/parser.h
 */

#ifndef SHIFT_PARSER_H_
#define SHIFT_PARSER_H_ 1

#include "../shift_config.h"
#include "shift_error_handler.h"
#include "tokenizer.h"
#include "../stdutils.h"

/** Namespace shift */
namespace shift {
	/** Namespace compiler */
	namespace compiler {
		typedef struct shift_name {
				std::list<token>::const_iterator begin_, end_;

				inline std::list<token>::const_iterator begin(void) const noexcept {
					return begin_;
				}

				inline std::list<token>::const_iterator end(void) const noexcept {
					return end_;
				}

				inline std::list<token>::const_iterator cbegin(void) const noexcept {
					return begin_;
				}

				inline std::list<token>::const_iterator cend(void) const noexcept {
					return end_;
				}

				inline bool operator==(const shift_name& name) const noexcept {
					return this->begin_ == name.begin_ && this->end_ == name.end_;
				}

				inline bool operator!=(const shift_name& name) const noexcept {
					return !operator==(name);
				}

				std::list<token>::size_type length(void) const noexcept {
					return end_ - begin_;
				}

				inline std::list<token>::size_type size(void) const noexcept {
					return length();
				}

				typename std::string_view::size_type string_length(void) const noexcept {
					typename std::string_view::size_type len = 0;

					for (typename std::list<token>::const_iterator begin__ = begin(); begin__ != end_; ++begin__) {
						len += begin__->get_data().length();
					}

					return len;
				}

		} shift_name, shift_type;

		enum mods : uint_fast8_t {
			PUBLIC = 0x1, PROTECTED = 0x2, PRIVATE = 0x4, STATIC = 0x8, CONST_ = 0x10, BINARY = 0x20, EXTERN = 0x40
		};

		typedef std::underlying_type_t<mods> mods_t;

		struct shift_module;
		struct shift_expression;
		struct shift_function;
		struct shift_class;

		struct shift_module {
				shift_name name;
		};

		struct shift_expression {
				token::token_type type = token::token_type::NULL_TOKEN; // im lazy and dont wanna create a new type so I will re-use token_type with some exceptions
//				shift_expression* left = nullptr, * right = nullptr; // left and right expressions must be free'd;
				shift_name data;
				std::vector<shift_expression> sub; // replaces left and right hand sides, left will be [0], right will be [1]

				void clear(void) {
					type = token::token_type::NULL_TOKEN;
					data.begin_ = std::list<token>::const_iterator();
					data.end_ = std::list<token>::const_iterator();
					sub.clear();
				}

				bool is_null_expr(void) const noexcept {
					return type == token::token_type::NULL_TOKEN || is_cast();
				}

				bool is_cast(void) const noexcept {
					return type == token::token_type::LEFT_BRACKET;
				}

				bool is_name(void) const noexcept {
					return type == token::token_type::IDENTIFIER;
				}

				bool is_function_call(void) const noexcept {
					return type == token::token_type::LEFT_SCOPE_BRACKET;
				}

				void set_cast(void) noexcept {
					type = token::token_type::LEFT_BRACKET;
				}

				void set_name(void) noexcept {
					type = token::token_type::IDENTIFIER;
				}

				void set_function_call(void) noexcept {
					type = token::token_type::LEFT_SCOPE_BRACKET;
				}

				shift_expression& set_left(const shift_expression* const l) {
					if (sub.size() < 1) {
						sub.push_back(*l);
					} else {
						sub[0] = *l;
					}

					return sub[0];
				}

				shift_expression& set_right(const shift_expression* const r) {
					if (sub.size() < 2) {
						if (sub.size() == 0)
							sub.push_back(shift_expression()); // set to null???
						sub.push_back(*r);
					} else {
						sub[1] = *r;
					}

					return sub[1];
				}

				inline shift_expression& set_left(const shift_expression& l) {
					return set_left(&l);
				}

				inline shift_expression& set_right(const shift_expression& r) {
					return set_right(&r);
				}

				shift_expression& set_left(shift_expression&& l) {
					if (sub.size() < 1) {
						sub.push_back(std::move(l));
					} else {
						sub[0] = std::move(l);
					}

					return sub[0];
				}

				shift_expression& set_right(shift_expression&& r) {
					if (sub.size() < 2) {
						if (sub.size() == 0)
							sub.push_back(shift_expression()); // set to null???
						sub.push_back(std::move(r));
					} else {
						sub[1] = std::move(r);
					}

					return sub[1];
				}

				shift_expression* left(void) {
					return has_left() ? &sub[0] : nullptr;
				}

				shift_expression* right(void) {
					return has_right() ? &sub[1] : nullptr;
				}

				const shift_expression* left(void) const {
					return has_left() ? &sub[0] : nullptr;
				}

				const shift_expression* right(void) const {
					return has_right() ? &sub[1] : nullptr;
				}

				bool has_left(void) const noexcept {
					return sub.size() >= 1 && sub.size() <= 2 && sub[0].type != token::token_type::NULL_TOKEN;
				}

				bool has_right(void) const noexcept {
					return sub.size() == 2;
				}
		};

		struct shift_function {
				shift_module* module = nullptr;
				shift_class* clazz = nullptr;
				shift_name name;
				std::list<shift_name> params;
				std::list<shift_module>::const_iterator usings_begin, usings_end;
				std::list<shift_expression> statements;
		};

		struct shift_class {
				static shift_class object;

				shift_module* module = nullptr;
				shift_class* parent = nullptr, * super = nullptr;
				const token* name = nullptr;
				mods_t mods = 0x0;
				std::list<shift_module>::const_iterator usings_begin, usings_end;
				std::list<shift_function>::const_iterator functions_begin, functions_end;

				std::string full_name(void) const noexcept {
					std::string str;
					if (parent) {
						str = parent->full_name();
						str.reserve(str.size() + 1 + name->get_data().size());
						str += '.';
						str += name->get_data();
					} else {
						str.reserve(name->get_data().size());
						if (module) {
							str.reserve(module->name.string_length() + name->get_data().length() + 1 /* '.' */);
							for (auto it = module->name.begin(); it != module->name.end(); ++it) {
								str += it->get_data();
							}
							str += '.';
						}
						str += name->get_data();
					}
					return str;
				}
		};

		class SHIFT_COMPILER_API parser {
			private:
				tokenizer* m_tokenizer;
				shift_error_handler* m_error_handler;

				typedef std::pair<mods_t, const token*> mod_pair_type;
				std::list<mod_pair_type> m_mods;

				shift_module m_module;
				std::list<shift_module> m_used_modules;
				std::list<shift_class> m_classes;
			public:
				parser(tokenizer* const tokenizer) noexcept;
				CXX20_CONSTEXPR ~parser(void) noexcept = default;
				parser(const parser&) noexcept = default;
				parser(parser&&) noexcept = default;
				parser& operator=(const parser&) noexcept = default;
				parser& operator=(parser&&) noexcept = default;

				void parse(void);
			private:
				void token_error(const token* const token, const char* const msg);
				void token_error(const token* const token, const std::string& msg);
				void token_warning(const token* const token, const char* const msg);
				void token_warning(const token* const token, const std::string& msg);

				inline void token_error(const std::list<token>::const_iterator token, const char* const msg) {
					return token_error(token.operator ->(), msg);
				}

				inline void token_error(const std::list<token>::const_iterator token, const std::string& msg) {
					return token_error(token.operator ->(), msg);
				}

				inline void token_warning(const std::list<token>::const_iterator token, const char* const msg) {
					return token_warning(token.operator ->(), msg);
				}

				inline void token_warning(const std::list<token>::const_iterator token, const std::string& msg) {
					return token_warning(token.operator ->(), msg);
				}

				void parse_module(void);
				void parse_use(void);
				void parse_class(shift_class* const parent = nullptr);
				void parse_class_(shift_class* clazz);

				void parse_access_specifier(void);
				shift_name parse_name(std::list<token>::const_iterator&);
				shift_type parse_type(std::list<token>::const_iterator&);

				bool skip_bracket(std::list<token>::const_iterator& begin, const std::list<token>::const_iterator end);

				shift_expression parse_expression(std::list<token>::const_iterator, const std::list<token>::const_iterator);

				void add_mod(mods_t, const token* = m_tokenizer->current_token());
				mods_t get_mods(void) const noexcept;
				bool has_mods(mods_t) const noexcept;
				void clear_mods(void) noexcept;

				std::string_view get_line(const token* const) const noexcept;
				const token* skip_until(const std::string& str) noexcept;
				const token* skip_until(typename token::token_type type) noexcept;

				template<typename _Predicate>
				inline const token* skip_until(_Predicate __func) noexcept {
					/// bool __func(const token& token) noexcept;
					///
					/// Returns true if desired item was found, false to continue skipping

					for (; !this->m_tokenizer->current_token()->is_null_token() && !__func(this->m_tokenizer->current_token());
							this->m_tokenizer->next_token());
					return this->m_tokenizer->current_token();
				}

				const token* skip_after(const std::string& str) noexcept;
				const token* skip_after(typename token::token_type type) noexcept;

				template<typename _Predicate>
				inline const token* skip_after(_Predicate&& __func) noexcept {
					skip_until(std::forward<_Predicate>(__func));
					return this->m_tokenizer->next_token();
				}

				typename std::list<const token*>::size_type skip_until(const std::vector<const token*>& tokens,
						typename std::list<const token*>::size_type begin_index, const std::string& str) noexcept;

				typename std::list<const token*>::size_type skip_after(const std::vector<const token*>& tokens,
						typename std::list<const token*>::size_type begin_index, const std::string& str) noexcept;

				typename std::list<const token*>::size_type skip_until(const std::vector<const token*>& tokens,
						typename std::list<const token*>::size_type begin_index, typename token::token_type type) noexcept;

				typename std::list<const token*>::size_type skip_after(const std::vector<const token*>& tokens,
						typename std::list<const token*>::size_type begin_index, typename token::token_type type) noexcept;

				void end_tokenizer(void) noexcept;
		};

	}
}

#endif /* SHIFT_PARSER_H_ */
