#include "compiler/shift_tokenizer.h"
#include "compiler/shift_error_handler.h"

#include <string>
#include <string_view>
#include <list>
#include <vector>
#include <utility>
#include <algorithm>

namespace shift {
    namespace compiler {
        class parser {
        public:
            inline parser(tokenizer* const tokenizer) noexcept;
            inline parser(error_handler* const error_handler, tokenizer* const tokenizer) noexcept;

            void parse();

            inline tokenizer* get_tokenizer() const noexcept { return m_tokenizer; }
            inline void set_tokenizer(tokenizer* const tokenizer) noexcept { m_tokenizer = tokenizer; }
            inline error_handler* get_error_handler() noexcept { return m_error_handler; }
            inline const error_handler* get_error_handler() const noexcept { return m_error_handler; }
            inline void set_error_handler(error_handler* const error_handler) noexcept { m_error_handler = error_handler; }

            static uint_fast8_t operator_priority(const token::token_type type, const bool prefix = false) noexcept;
        public:
            enum mods: uint_fast8_t {
                PUBLIC = 0x1,
                PROTECTED = 0x2,
                PRIVATE = 0x4,
                STATIC = 0x8,
                CONST_ = 0x10,
                BINARY = 0x20,
                EXTERN = 0x40
            };
        private:
            struct shift_name;
            struct shift_type;
            struct shift_expression;
            struct shift_variable;
            struct shift_statement;
            struct shift_function;
            struct shift_class;

            using shift_module = shift_name;

            struct shift_name {
                typename std::vector<token>::const_iterator begin, end;

                inline auto size() const noexcept { return end - begin; }
            };

            struct shift_type {
                parser::mods mods = parser::mods(0x0);
                shift_name name;
            };

            struct shift_expression {
                token::token_type type = token::token_type::NULL_TOKEN;
                shift_expression* parent = nullptr;
                typename std::vector<token>::const_iterator begin, end;
                std::list<shift_expression> sub;

                inline auto size() const noexcept { return end - begin; }

                inline bool is_bracket() const noexcept { return type == token::token_type::LEFT_BRACKET; }
                inline bool is_function_call() const noexcept { return type == token::token_type::LEFT_SCOPE_BRACKET; }
                inline bool is_array() const noexcept { return type == token::token_type::LEFT_SQUARE_BRACKET; }

                inline void set_bracket() noexcept { type = token::token_type::LEFT_BRACKET; }
                inline void set_function_call() noexcept { type = token::token_type::LEFT_SCOPE_BRACKET; }
                inline void set_array() noexcept { type = token::token_type::LEFT_SQUARE_BRACKET; }

                inline bool has_left() const noexcept { return sub.size() == 1 || sub.size() == 2; }
                inline bool has_right() const noexcept { return sub.size() == 2; }

                inline shift_expression* get_left() noexcept { return has_left() ? &sub.front() : nullptr; }
                inline shift_expression* get_right() noexcept { return has_right() ? &sub.back() : nullptr; }

                inline const shift_expression* get_left() const noexcept { return has_left() ? &sub.front() : nullptr; }
                inline const shift_expression* get_right() const noexcept { return has_right() ? &sub.back() : nullptr; }

                inline void set_left(const shift_expression& expr) {
                    sub.resize(shift_clamp(sub.size(), 1, 2));
                    sub.front() = expr;
                    sub.front().parent = this;
                }

                inline void set_left(shift_expression&& expr) {
                    sub.resize(shift_clamp(sub.size(), 1, 2));
                    sub.front() = std::move(expr);
                    sub.front().parent = this;
                }

                inline void set_left() { set_left(shift_expression()); }

                inline void set_right(const shift_expression& expr) {
                    sub.resize(2);
                    sub.back() = expr;
                    sub.back().parent = this;
                }

                inline void set_right(shift_expression&& expr) {
                    sub.resize(2);
                    sub.back() = std::move(expr);
                    sub.back().parent = this;
                }

                inline void set_right() { set_right(shift_expression()); }
            };

            struct shift_variable {
                shift_type type;
                const token* name = nullptr;
                shift_expression value;
                shift_class* clazz = nullptr;
                shift_function* function = nullptr;
            };

            struct shift_statement {
                enum class statement_type: uint_fast8_t {
                    expression = 1,
                    variable_alloc,
                    scope_begin, // {
                    use,
                    if_,
                    else_,
                    while_,
                    for_,
                    return_,
                    continue_,
                    break_
                } type;

                struct {
                    shift_expression expr;
                    shift_module module_;
                    shift_variable variable;
                    const token* token = nullptr;
                } data[2];

                std::list<shift_statement> sub;

                inline void set_if(const token* const token) noexcept {
                    type = statement_type::if_;
                    data[0].token = token;
                }

                inline void set_if_condition(const shift_expression& expr) { data[0].expr = expr; }

                inline void set_if_condition(shift_expression&& expr) noexcept { data[0].expr = std::move(expr); }

                inline const token* get_if() const noexcept { return data[0].token; }

                inline const shift_expression& get_if_condition() const noexcept { return data[0].expr; }

                inline std::list<shift_statement>& get_if_statements() noexcept { return sub; }

                inline const std::list<shift_statement>& get_if_statements() const noexcept { return sub; }

                inline void set_else_condition(const shift_expression& expr) { data[0].expr = expr; }

                inline void set_else_condition(shift_expression&& expr) noexcept { data[0].expr = std::move(expr); }

                inline void set_else(const token* const token) noexcept {
                    type = statement_type::else_;
                    data[0].token = token;
                }

                inline const token* get_else() const noexcept { return data[0].token; }

                inline const shift_expression& get_else_condition() const noexcept { return data[0].expr; }

                inline std::list<shift_statement>& get_else_statements() noexcept { return sub; }

                inline const std::list<shift_statement>& get_else_statements() const noexcept { return sub; }

                inline void set_while(const token* const token) noexcept {
                    type = statement_type::while_;
                    data[0].token = token;
                }

                inline void set_while_condition(const shift_expression& expr) { data[0].expr = expr; }

                inline void set_while_condition(shift_expression&& expr) noexcept { data[0].expr = std::move(expr); }

                inline const token* get_while() const noexcept { return data[0].token; }

                inline const shift_expression& get_while_condition() const noexcept { return data[0].expr; }

                inline std::list<shift_statement>& get_while_statements() noexcept { return sub; }

                inline const std::list<shift_statement>& get_while_statements() const noexcept { return sub; }

                inline void set_for(const token* const token) noexcept {
                    type = statement_type::for_;
                    data[0].token = token;
                }

                inline const token* get_for() const noexcept { return data[0].token; }

                inline void set_for_initializer(const shift_statement& statement) {
                    sub.clear();
                    sub.push_back(statement);
                }

                inline void set_for_initializer(shift_statement&& statement) {
                    sub.clear();
                    sub.push_back(std::move(statement));
                }

                inline void set_for_condition(const shift_expression& expr) { data[0].expr = expr; }

                inline void set_for_condition(shift_expression&& expr) noexcept { data[0].expr = std::move(expr); }

                inline void set_for_increment(const shift_expression& expr) { data[1].expr = expr; }

                inline void set_for_increment(shift_expression&& expr) noexcept { data[1].expr = std::move(expr); }

                inline const shift_statement& get_for_initializer() const noexcept { return sub.front(); }

                inline const shift_expression& get_for_condition() const noexcept { return data[0].expr; }

                inline const shift_expression& get_for_increment() const noexcept { return data[1].expr; }

                inline std::list<shift_statement>& get_for_statements() noexcept { return sub; }

                inline const std::list<shift_statement>& get_for_statements() const noexcept { return sub; }

                inline void set_return(const token* const token) noexcept {
                    type = statement_type::return_;
                    data[0].token = token;
                }

                inline void set_return_statement(const shift_expression& expr) { data[0].expr = expr; }

                inline void set_return_statement(shift_expression&& expr) noexcept { data[0].expr = std::move(expr); }

                inline void set_return_expression(const shift_expression& expr) { return set_return_statement(expr); }

                inline void set_return_expression(shift_expression&& expr) noexcept { return set_return_statement(std::move(expr)); }

                inline const token* get_return() const noexcept { return data[0].token; }

                inline const shift_expression& get_return_statement() const noexcept { return data[0].expr; }

                inline void set_expression() noexcept { type = statement_type::expression; }

                inline void set_expression(const shift_expression& expr) { data[0].expr = expr; }

                inline void set_expression(shift_expression&& expr) noexcept { data[0].expr = std::move(expr); }

                inline const shift_expression& get_expression() const noexcept { return data[0].expr; }

                inline void set_variable() noexcept { type = statement_type::variable_alloc; }

                inline void set_variable(const shift_variable& var_) { data[0].variable = var_; }

                inline void set_variable(shift_variable&& var_) noexcept { data[0].variable = std::move(var_); }

                inline const shift_variable& get_variable() const noexcept { return data[0].variable; }

                inline void set_block(const token* const token) noexcept {
                    type = statement_type::scope_begin;
                    data[0].token = token;
                }

                inline void set_block(const std::list<shift_statement>& sub) noexcept { this->sub = sub; }

                inline void set_block(std::list<shift_statement>&& sub) noexcept { this->sub = std::move(sub); }

                inline const token* get_block() const noexcept { return data[0].token; }

                inline const std::list<shift_statement>& get_block_statements() const noexcept { return sub; }

                inline void set_block_end(const token* const token) noexcept {
                    type = statement_type::scope_begin;
                    data[1].token = token;
                }

                inline const token* get_block_end() const noexcept { return data[1].token; }

                inline void set_continue(const token* const token) noexcept {
                    type = statement_type::continue_;
                    data[0].token = token;
                }

                inline void set_break(const token* const token) noexcept {
                    type = statement_type::break_;
                    data[0].token = token;
                }

                inline void set_use(const token* const token) noexcept {
                    type = statement_type::use;
                    data[0].token = token;
                }

                inline void set_use_module(const shift_module& module_) noexcept {
                    data[0].module_ = module_;
                }

                inline void set_use_module(shift_module&& module_) noexcept {
                    data[0].module_ = std::move(module_);
                }

                inline const token* get_use() const noexcept { return data[0].token; }
                inline const shift_module& get_use_module() const noexcept { return data[0].module_; }
            };

            struct shift_function {
                shift_class* clazz = nullptr;
                const token* name = nullptr;
                mods mods = parser::mods(0x0);
                shift_type return_type;
                std::list<std::pair<shift_type, const token*>> parameters;
                std::list<shift_statement> statements;
            };

            struct shift_class {
                shift_module* module_ = nullptr;
                shift_class* parent = nullptr, * base = nullptr;
                const token* name = nullptr;
                mods mods = parser::mods(0x0);
                typename std::list<shift_module>::const_iterator implicit_use_statements;
                std::list<shift_module> use_statements;
                std::list<shift_function> functions;
                std::list<shift_variable> variables;
            };
        private:
            void m_parse_access_specifier(void);
            void m_parse_use(void);
            void m_parse_use(std::list<shift_module>&);
            void m_parse_module(void);
            void m_parse_class(void);
            void m_parse_class(shift_class&);
            void m_parse_function(shift_function&);
            void m_parse_function_block(shift_function&, std::list<shift_statement>&, size_t count = -1);
            shift_expression m_parse_expression(const token::token_type end_type = token::token_type::SEMICOLON);

            shift_name m_parse_name(const char* const);
            shift_type m_parse_type(const char* const);

            void m_token_error(const token& token_, const std::string_view msg);
            void m_token_error(const token& token_, const std::string& msg);
            void m_token_error(const token& token_, const char* const msg);

            void m_token_warning(const token& token_, const std::string_view msg);
            void m_token_warning(const token& token_, const std::string& msg);
            void m_token_warning(const token& token_, const char* const msg);

            std::string_view m_get_line(const token&) const noexcept;

            const token& m_skip_until(const std::string_view) noexcept;
            const token& m_skip_until(const std::string&) noexcept;
            const token& m_skip_until(const char* const) noexcept;
            const token& m_skip_until(const typename token::token_type) noexcept;

            const token& m_skip_after(const std::string_view) noexcept;
            const token& m_skip_after(const std::string&) noexcept;
            const token& m_skip_after(const char* const) noexcept;
            const token& m_skip_after(const typename token::token_type) noexcept;

            const token& m_skip_before(const std::string_view) noexcept;
            const token& m_skip_before(const std::string&) noexcept;
            const token& m_skip_before(const char* const) noexcept;
            const token& m_skip_before(const typename token::token_type) noexcept;

            const token& m_skip_until_closing(const typename token::token_type) noexcept;

            mods m_get_mods(void) const noexcept;
            void m_add_mod(mods, const token&) noexcept;
            void m_clear_mods(void) noexcept;

            bool m_is_module_defined(void) const noexcept;
        private:
            tokenizer* m_tokenizer;
            error_handler* m_error_handler;

            std::list<std::pair<mods, const token*>> m_mods;
            shift_module m_module;
            std::list<shift_module> m_global_uses;
            std::list<shift_class> m_classes;
        };

        inline parser::parser(tokenizer* const tokenizer) noexcept: m_tokenizer(tokenizer), m_error_handler(tokenizer->get_error_handler()) {}
        inline parser::parser(error_handler* const error_handler, tokenizer* const tokenizer) noexcept: m_tokenizer(tokenizer), m_error_handler(error_handler) {}
    }
}

constexpr inline shift::compiler::parser::mods operator^(const shift::compiler::parser::mods f, const shift::compiler::parser::mods other) noexcept { return shift::compiler::parser::mods(std::underlying_type_t<shift::compiler::parser::mods>(f) ^ std::underlying_type_t<shift::compiler::parser::mods>(other)); }
constexpr inline shift::compiler::parser::mods& operator^=(shift::compiler::parser::mods& f, const shift::compiler::parser::mods other) noexcept { return f = operator^(f, other); }
constexpr inline shift::compiler::parser::mods operator|(const shift::compiler::parser::mods f, const shift::compiler::parser::mods other) noexcept { return shift::compiler::parser::mods(std::underlying_type_t<shift::compiler::parser::mods>(f) | std::underlying_type_t<shift::compiler::parser::mods>(other)); }
constexpr inline shift::compiler::parser::mods& operator|=(shift::compiler::parser::mods& f, const shift::compiler::parser::mods other) noexcept { return f = operator|(f, other); }
constexpr inline shift::compiler::parser::mods operator&(const shift::compiler::parser::mods f, const shift::compiler::parser::mods other) noexcept { return shift::compiler::parser::mods(std::underlying_type_t<shift::compiler::parser::mods>(f) & std::underlying_type_t<shift::compiler::parser::mods>(other)); }
constexpr inline shift::compiler::parser::mods& operator&=(shift::compiler::parser::mods& f, const shift::compiler::parser::mods other) noexcept { return f = operator&(f, other); }
constexpr inline shift::compiler::parser::mods operator~(const shift::compiler::parser::mods f) noexcept { return shift::compiler::parser::mods(~std::underlying_type_t<shift::compiler::parser::mods>(f)); }