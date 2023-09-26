/**
 * @file compiler/shift_parser.h
 */
#ifndef SHIFT_PARSER_H_
#define SHIFT_PARSER_H_ 1
#include "compiler/shift_tokenizer.h"
#include "compiler/shift_error_handler.h"
#include "utils/ordered_set.h"
#include "utils/ordered_map.h"

#include <string>
#include <string_view>
#include <list>
#include <vector>
#include <utility>
#include <algorithm>
#include <functional>

 // Outline
namespace shift::compiler {
    class parser;

    struct shift_name;
    struct shift_type;
    struct shift_expression;
    struct shift_variable;
    struct shift_statement;
    struct shift_function;
    struct shift_class;

    using shift_module = shift_name;
}

// Allow hashing of shift_name
namespace shift::compiler {
    struct shift_name {
        typename std::vector<token>::const_iterator begin, end;

        inline auto size() const noexcept { return end - begin; }

        std::string to_string() const {
            std::string str;

            for (auto b = begin; b != end; ++b) {
                str += b->get_data();

                auto const next = b + 1;
                if (next != end && next->is_identifier() && b->is_identifier()) {
                    str += ' ';
                }
            }

            return str;
        }

        inline operator std::string() const { return to_string(); }

        inline bool operator==(const shift_name& other) const { return to_string() == other.to_string(); }
        inline bool operator!=(const shift_name& other) const { return !operator==(other); }
        inline bool operator==(const std::string& str) const { return to_string() == str; }
        inline bool operator!=(const std::string& str) const { return !operator==(str); }
    };
}

template<>
struct std::hash<shift::compiler::shift_name> {
    inline std::size_t operator()(const shift::compiler::shift_name& name) const {
        return std::hash<std::string>()(name.to_string());
    }
};

// Non-hashable structures
namespace shift::compiler {
    enum shift_mods : uint_fast8_t {
        PUBLIC = 0x1,
        PROTECTED = 0x2,
        PRIVATE = 0x4,
        STATIC = 0x8,
        CONST_ = 0x10,
        BINARY = 0x20,
        EXTERN = 0x40,
        EXPLICIT = 0x80
    };

    struct shift_type {
        shift_mods mods = shift_mods(0x0);
        shift_name name;
        shift_class* name_class = nullptr;
        size_t array_dimensions = 0;

        inline bool operator==(const shift_type& other) const noexcept { return name_class == other.name_class && array_dimensions == other.array_dimensions; }
        inline bool operator!=(const shift_type& other) const noexcept { return !operator==(other); }

        std::string get_fqn() const;
    };

    struct shift_expression {
        token::token_type type = token::token_type::NULL_TOKEN;
        shift_expression* parent = nullptr;
        typename std::vector<token>::const_iterator begin, end;
        std::list<shift_expression> sub;

        shift_type expr_type;
        //std::vector<shift_variable*> variables;
        shift_variable* variable = nullptr;
        shift_function* function = nullptr;
        shift_class* clazz = nullptr;

        inline std::string to_string() const noexcept { return to_name().to_string(); }

        inline shift_name to_name() const noexcept {
            shift_name name;
            name.begin = begin;
            name.end = end;
            return name;
        }

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
        shift_module* module_ = nullptr;
        shift_class* clazz = nullptr;
        shift_function* function = nullptr;
        parser* parser_ = nullptr;
        size_t implicit_use_statements = 0;

        std::string get_fqn() const;
    };

    struct shift_statement {
        enum class statement_type : uint_fast8_t {
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
            shift_statement* statement = nullptr;
        } data[2];

        std::list<shift_statement> sub;

        shift_statement* parent = nullptr;

        inline void set_if(const token* const token) noexcept {
            type = statement_type::if_;
            data[0].token = token;
        }

        inline void set_if_condition(const shift_expression& expr) { data[0].expr = expr; }

        inline void set_if_condition(shift_expression&& expr) noexcept { data[0].expr = std::move(expr); }

        inline const token* get_if() const noexcept { return data[0].token; }

        inline shift_expression& get_if_condition() noexcept { return data[0].expr; }

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

        inline std::list<shift_statement>& get_else_statements() noexcept { return sub; }

        inline const std::list<shift_statement>& get_else_statements() const noexcept { return sub; }

        inline void connect_else(shift_statement* else_statement) noexcept {
            data[0].statement = else_statement;
        }

        inline void attach_else(shift_statement* else_statement) noexcept {
            return connect_else(else_statement);
        }

        inline shift_statement* get_connected_else() const noexcept { return data[0].statement; }
        inline shift_statement* get_attached_else() const noexcept { return data[0].statement; }

        inline void set_while(const token* const token) noexcept {
            type = statement_type::while_;
            data[0].token = token;
        }

        inline void set_while_condition(const shift_expression& expr) { data[0].expr = expr; }

        inline void set_while_condition(shift_expression&& expr) noexcept { data[0].expr = std::move(expr); }

        inline const token* get_while() const noexcept { return data[0].token; }

        inline shift_expression& get_while_condition() noexcept { return data[0].expr; }

        inline const shift_expression& get_while_condition() const noexcept { return data[0].expr; }

        inline std::list<shift_statement>& get_while_statements() noexcept { return sub; }

        inline const std::list<shift_statement>& get_while_statements() const noexcept { return sub; }

        inline void set_for(const token* const token) noexcept {
            type = statement_type::for_;
            data[0].token = token;
        }

        inline const token* get_for() const noexcept { return data[0].token; }

        inline void set_for_initializer(const shift_statement& statement) {
            sub.resize(std::max<size_t>(sub.size(), 1));
            sub.front() = (statement);
        }

        inline void set_for_initializer(shift_statement&& statement) {
            sub.resize(std::max<size_t>(sub.size(), 1));
            sub.front() = std::move(statement);
        }

        inline void set_for_condition(const shift_expression& expr) { data[0].expr = expr; }

        inline void set_for_condition(shift_expression&& expr) noexcept { data[0].expr = std::move(expr); }

        inline void set_for_increment(const shift_expression& expr) { data[1].expr = expr; }

        inline void set_for_increment(shift_expression&& expr) noexcept { data[1].expr = std::move(expr); }

        inline shift_statement& get_for_initializer() noexcept { return sub.front(); }

        inline const shift_statement& get_for_initializer() const noexcept { return sub.front(); }

        inline shift_expression& get_for_condition() noexcept { return data[0].expr; }

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

        inline shift_expression& get_return_statement() noexcept { return data[0].expr; }

        inline const shift_expression& get_return_statement() const noexcept { return data[0].expr; }

        inline void set_expression() noexcept { type = statement_type::expression; }

        inline void set_expression(const shift_expression& expr) { data[0].expr = expr; }

        inline void set_expression(shift_expression&& expr) noexcept { data[0].expr = std::move(expr); }

        inline shift_expression& get_expression() noexcept { return data[0].expr; }

        inline const shift_expression& get_expression() const noexcept { return data[0].expr; }

        inline void set_variable() noexcept { type = statement_type::variable_alloc; }

        inline void set_variable(const shift_variable& var_) { data[0].variable = var_; }

        inline void set_variable(shift_variable&& var_) noexcept { data[0].variable = std::move(var_); }

        inline shift_variable& get_variable() noexcept { return data[0].variable; }

        inline const shift_variable& get_variable() const noexcept { return data[0].variable; }

        inline void set_block(const token* const token) noexcept {
            type = statement_type::scope_begin;
            data[0].token = token;
        }

        inline void set_block(const std::list<shift_statement>& sub) noexcept { this->sub = sub; }

        inline void set_block(std::list<shift_statement>&& sub) noexcept { this->sub = std::move(sub); }

        inline const token* get_block() const noexcept { return data[0].token; }

        inline std::list<shift_statement>& get_block_statements() noexcept { return sub; }

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

        inline const token* get_break() const noexcept { return data[0].token; }
        inline const token* get_continue() const noexcept { return data[0].token; }

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

        inline shift_statement* get_break_link() const noexcept { return data[0].statement; }
        inline void set_break_link(shift_statement* const stat) { data[0].statement = stat; }

        inline shift_statement* get_continue_link() const noexcept { return data[0].statement; }
        inline void set_continue_link(shift_statement* const stat) { data[0].statement = stat; }
    };

    struct shift_class {
        shift_module* module_ = nullptr;
        struct {
            shift_name name;
            shift_class* clazz = nullptr;
        } parent, base;
        const token* name = nullptr;
        shift_mods mods = shift_mods(0x0);
        size_t implicit_use_statements = 0;
        utils::ordered_set<shift_module> use_statements;
        std::list<shift_function> functions;
        std::list<shift_variable> variables;
        shift_variable this_var, base_var;

        parser* parser_ = nullptr;

        inline size_t has_base(const shift_class* const test) const noexcept {
            size_t count = 1;
            for (const shift_class* current = base.clazz; current; current = current->base.clazz, count++) {
                if (current == test) return count;
            }
            return 0;
        }

        inline size_t has_parent(const shift_class* const test) const noexcept {
            size_t count = 1;
            for (const shift_class* current = parent.clazz; current; current = current->parent.clazz, count++) {
                if (current == test) return count;
            }
            return 0;
        }

        inline std::string get_fqn() const noexcept {
            std::string str;

            if (parent.clazz) str = parent.clazz->get_fqn();
            else if (module_) str = module_->to_string();
            if (str.size() > 0) str += '.';

            str += name->get_data();

            return str;
        }
    };

    struct shift_function {
        shift_module* module_ = nullptr;
        shift_class* clazz = nullptr;
        parser* parser_ = nullptr;
        shift_name name;
        shift_mods mods = shift_mods(0x0);
        shift_type return_type;
        utils::ordered_map<std::string_view, shift_variable> parameters;
        std::list<shift_statement> statements;
        size_t implicit_use_statements = 0;

        inline std::string get_fqn() const noexcept {
            std::string str;

            if (clazz) str = clazz->get_fqn();
            else if (module_) str = module_->to_string();

            if (str.size() > 0) str += '.';

            return str += name.to_string();
        }

        inline std::string get_fqn(const size_t id) const noexcept {
            std::string str = get_fqn();
            str += '@' + std::to_string(id);
            return str;
        }

        inline std::string get_signature() const noexcept {
            std::string str = get_fqn() + "(";
            size_t param_index = 1;
            for (auto const& [name, v] : parameters) {
                if (v.type.name_class) {
                    str += v.type.name_class->get_fqn();
                } else {
                    str += "<unknown>";
                }
                if (param_index < parameters.size())
                    str += ", ";
                param_index++;
            }
            return str += ")";
        }
    };
}

constexpr inline shift::compiler::shift_mods operator^(const shift::compiler::shift_mods f, const shift::compiler::shift_mods other) noexcept { return shift::compiler::shift_mods(std::underlying_type_t<shift::compiler::shift_mods>(f) ^ std::underlying_type_t<shift::compiler::shift_mods>(other)); }
constexpr inline shift::compiler::shift_mods& operator^=(shift::compiler::shift_mods& f, const shift::compiler::shift_mods other) noexcept { return f = operator^(f, other); }
constexpr inline shift::compiler::shift_mods operator|(const shift::compiler::shift_mods f, const shift::compiler::shift_mods other) noexcept { return shift::compiler::shift_mods(std::underlying_type_t<shift::compiler::shift_mods>(f) | std::underlying_type_t<shift::compiler::shift_mods>(other)); }
constexpr inline shift::compiler::shift_mods& operator|=(shift::compiler::shift_mods& f, const shift::compiler::shift_mods other) noexcept { return f = operator|(f, other); }
constexpr inline shift::compiler::shift_mods operator&(const shift::compiler::shift_mods f, const shift::compiler::shift_mods other) noexcept { return shift::compiler::shift_mods(std::underlying_type_t<shift::compiler::shift_mods>(f) & std::underlying_type_t<shift::compiler::shift_mods>(other)); }
constexpr inline shift::compiler::shift_mods& operator&=(shift::compiler::shift_mods& f, const shift::compiler::shift_mods other) noexcept { return f = operator&(f, other); }
constexpr inline shift::compiler::shift_mods operator~(const shift::compiler::shift_mods f) noexcept { return shift::compiler::shift_mods(~std::underlying_type_t<shift::compiler::shift_mods>(f)); }

namespace shift::compiler {
    // fwd decl analyzer class for friend
    class analyzer;

    class parser {
    public:
        inline parser(tokenizer* const tokenizer) noexcept;
        inline parser(error_handler* const error_handler, tokenizer* const tokenizer) noexcept;
        parser(const parser&) = delete;
        parser(parser&&) noexcept = delete;

        parser& operator=(const parser&) = delete;
        parser& operator=(parser&&) noexcept = delete;

        SHIFT_API void parse();

        inline tokenizer* get_tokenizer() const noexcept { return m_tokenizer; }
        inline void set_tokenizer(tokenizer* const tokenizer) noexcept { m_tokenizer = tokenizer; }
        inline error_handler* get_error_handler() noexcept { return m_error_handler; }
        inline const error_handler* get_error_handler() const noexcept { return m_error_handler; }
        inline void set_error_handler(error_handler* const error_handler) noexcept { m_error_handler = error_handler; }

        SHIFT_API static uint_fast8_t operator_priority(const token::token_type type, const bool prefix = false) noexcept;
    private:
        void m_parse_access_specifier(void);
        void m_parse_use(void);
        void m_parse_use(utils::ordered_set<shift_module>&);
        void m_parse_module(void);
        void m_parse_class(shift_class* parent_class = nullptr);
        // void m_parse_class(shift_class&);
        void m_parse_function(shift_function&);
        void m_parse_function_block(shift_function&, std::list<shift_statement>&, size_t count = -1);
        void m_parse_body(shift_class* = nullptr);
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

        shift_mods m_get_mods(void) const noexcept;
        void m_add_mod(shift_mods, const token&) noexcept;
        void m_clear_mods(void) noexcept;

        bool m_is_module_defined(void) const noexcept;
    private:
        // Tokenized file. Tokenization must have passed with no errors in order to be usable in the parsing stage
        tokenizer* m_tokenizer;

        // Error handler (if desired)
        error_handler* m_error_handler;

        // The current module for the file
        shift_module m_module;

        // Storage for all global use statements in the module. 
        // It simpliy contains all the 'use' statements in sequential order
        utils::ordered_set<shift_module> m_global_uses;

        // List of classes found inside current module inside current file
        std::list<shift_class> m_classes;

        // List of functions which are tied solely to the current module inside the current file (not inside a class)
        std::list<shift_function> m_functions;

        // List of variables which are tied solely to the current module inside the current file (not inside a class)
        std::list<shift_variable> m_variables;

        // Utility variable for holding the current mods specified by the user
        std::list<std::pair<shift_mods, const token*>> m_mods;

        friend class analyzer;
    };

    inline parser::parser(tokenizer* const tokenizer) noexcept : m_tokenizer(tokenizer), m_error_handler(tokenizer->get_error_handler()) {}
    inline parser::parser(error_handler* const error_handler, tokenizer* const tokenizer) noexcept : m_tokenizer(tokenizer), m_error_handler(error_handler) {}
}
#endif