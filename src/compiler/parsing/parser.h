#ifndef SHIFT_PARSER_H_
#define SHIFT_PARSER_H_ 1

#include "compiler/lexing/lexer.h"
#include "compiler/error_handler.h"

#include "compiler/mods.h"
#include "compiler/parsing/parser_fwd.h"
#include "compiler/parsing/parser_types.h"

#include "utils/ordered_set.h"
#include "utils/ordered_map.h"
#include "utils/range.h"
#include "utils/optional.h"
#include "utils/utility.h"

#include <string>
#include <string_view>
#include <deque>
#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
#include <array>
#include <memory>
#include <ranges>
#include <numeric>

#include <more_concepts/more_concepts.hpp>

namespace shift::compiler::analyzing {
    // fwd decl analyzer class for friend
    class analyzer;
}

namespace shift::compiler::parsing {
    class parser {
    public:
        inline parser(error_handler* const eh, const lexing::lexer& lex);

        parser(error_handler* const eh, const lexing::lexer&& lex) noexcept = delete;

        parser(const parser&) = delete;

        parser(parser&&) noexcept = default;

        parser& operator=(const parser&) = delete;

        parser& operator=(parser&&) noexcept = default;

        inline const lexing::lexer& get_lexer() const noexcept { return *m_lexer; }

        inline error_handler* get_error_handler() noexcept { return m_error_handler; }

        inline const error_handler* get_error_handler() const noexcept { return m_error_handler; }

        inline auto get_module() noexcept {
            return utils::make_optional(this->m_module.get());
        }

        inline auto get_module() const noexcept {
            return utils::make_optional(utils::as_const_ptr(this->m_module.get()));
        }

        inline auto& get_classes() noexcept { return m_classes; }

        inline const auto& get_classes() const noexcept { return m_classes; }

        inline auto& get_functions() noexcept { return m_functions; }

        inline const auto& get_functions() const noexcept { return m_functions; }

        inline auto& get_variables() noexcept { return m_variables; }

        inline const auto& get_variables() const noexcept { return m_variables; }

        inline auto& get_global_uses() noexcept { return m_global_uses; }

        inline const auto& get_global_uses() const noexcept { return m_global_uses; }

        SHIFT_API static std::uint8_t operator_priority(const lexing::token::type type, const bool prefix = false) noexcept;

#ifdef SHIFT_DEBUG
        SHIFT_API void print_tree();
#endif
    private:
        SHIFT_API void parse();

        struct parse_state;

    private:
        shift_mods parse_modifier(parse_state& state);

        parser_module parse_use(parse_state&);

        void consume_use(parse_state&, utils::ordered_set<parser_module>& modules);

        void consume_module(parse_state& state);

        void consume_class(parse_state& state);

        parser_function* consume_function(parse_state& state, parser_type&& return_type);

        parser_variable parse_variable_header(parse_state& state, parser_type&& type);

        // void consume_class(parser_class&);
        void consume_function_body(parse_state& state);

        void consume_block(parse_state& state, block_statement&, size_t count = -1);

        void consume_body(parse_state&);

        expression_types parse_expression(parse_state& state, const utils::predicate<std::vector<lexing::token>::const_iterator>& end_func);

        inline expression_types
        parse_expression(parse_state& state, const lexing::token::type end_type = lexing::token::type::SEMICOLON) {
            return parse_expression(state, [end_type](const std::vector<lexing::token>::const_iterator it) {
                return it->get_token_type() == end_type;
            });
        }

        expression_types expect_expression(parse_state& state, std::string_view expr_origin,
            lexing::token::type end_type = lexing::token::type::SEMICOLON);

        void consume_modifiers(parse_state& state);

        token_group parse_name(parse_state& state, std::string_view name_type);

        token_group expect_name(parse_state& state, std::string_view name_type);

        parser_type parse_type(parse_state& state, std::string_view);

        std::string token_underline(const lexing::token&);

        std::string token_message_header(error_handler::message_type type, const lexing::token&);

        void token_error(const lexing::token& token_, std::string_view msg);

        void token_warning(const lexing::token& token_, std::string_view msg);

        std::string_view get_line(const lexing::token&) const noexcept;

        bool is_module_defined() const noexcept;

        void ensure_no_mods(parse_state& state);
        void ensure_no_mods(parse_state& state, std::string_view);

    private:
        // Tokenized file. Tokenization must have passed with no errors in order to be usable in the parsing stage
        const lexing::lexer* m_lexer;

        // Error handler (if desired)
        error_handler* m_error_handler{nullptr};

        // The current module for the file
        std::unique_ptr<parser_module> m_module;

        // Storage for all global use statements in the module. 
        // It simply contains all the 'use' statements in sequential order
        utils::ordered_set<parser_module> m_global_uses;

        // List of classes found inside current module inside current file
        std::deque<parser_class> m_classes;

        // List of functions which are tied solely to the current module inside the current file (not inside a class)
        std::deque<parser_function> m_functions;

        // List of variables which are tied solely to the current module inside the current file (not inside a class)
        std::deque<parser_variable> m_variables;


        friend class analyzing::analyzer;
    };

    inline parser::parser(error_handler* const eh, const lexing::lexer& lex) : m_lexer(&lex), m_error_handler(eh) {
        parse();
    }


    struct parser::parse_state {
        explicit parse_state(const parser& p) : position{p.get_lexer()} {}

        parse_state(const parser&& p) = delete;

        // Current lexing::token position in the parsing process
        lexing::token_stream position;

        mods_holder mods{};

        parser_class* clazz{nullptr};
        parser_function* func{nullptr};
        parser_variable* variable{nullptr};
    };
}

#endif