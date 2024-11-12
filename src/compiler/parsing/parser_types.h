#ifndef SHIFT_PARSER_TYPES_H_
#define SHIFT_PARSER_TYPES_H_

#include "compiler/lexing/lexer.h"
#include "compiler/mods.h"
#include "compiler/analyzing/analyzer_fwd.h"
#include "compiler/parsing/parser_fwd.h"
#include "compiler/parsing/types.h"

#include "utils/range.h"
#include "utils/iterator_wrapper.h"
#include "utils/ordered_set.h"

#include <vector>
#include <string>
#include <concepts>
#include <iterator>
#include <ranges>
#include <memory>
#include <type_traits>
#include <variant>
#include <optional>

namespace shift::compiler::parsing::detail {
    template<template<typename> typename Transformer, typename Base, std::derived_from<Base>... ExprTs>
    auto variant_type_generator() -> std::variant<Transformer < Base>, Transformer<ExprTs>

    ...> {
    static_assert(false);
}
}

#define SHIFT_TYPES_GENERATOR(name, base, ...)                                                                                           \
    using name##_types = decltype(shift::compiler::parsing::detail::variant_type_generator<std::type_identity_t, base, __VA_ARGS__>());  \
    using name##_ptr_types = decltype(shift::compiler::parsing::detail::variant_type_generator<std::add_pointer_t, base, __VA_ARGS__>());

namespace shift::compiler::parsing {
    struct token_group {
        using iterator = lexing::token_stream::const_iterator;

        utils::range<lexing::token_stream::const_iterator> source;

        iterator begin() const { return source.begin(); }

        iterator end() const { return source.end(); }

        /// @brief Obtains the length of the name in tokens.
        auto length() const { return source.size(); }

        bool empty() const { return source.empty(); }

        SHIFT_API std::string to_string() const;

        inline explicit operator std::string() const { return to_string(); }

        SHIFT_API bool operator==(const token_group& other) const;

        inline bool operator!=(const token_group& other) const { return !operator==(other); }

        inline bool operator==(const std::string& str) const { return to_string() == str; }

        inline bool operator!=(const std::string& str) const { return !operator==(str); }

        inline bool operator==(const std::string_view str) const { return to_string() == str; }

        inline bool operator!=(const std::string_view& str) const { return !operator==(str); }
    };

    struct parser_module : shift_module {
        using iterator = utils::iterator_wrapper<token_group::iterator>;

        token_group name;

        std::size_t depth() const override { return (name.length() + 1) / 2; }

        inline std::string to_string() const override { return name.to_string(); }

        SHIFT_API bool is_sub_module(const parser_module& child) const;

        SHIFT_API std::partial_ordering operator<=>(const parser_module& other) const;

        SHIFT_API std::partial_ordering operator<=>(std::string_view str) const;

        SHIFT_API iterator begin() const noexcept;

        SHIFT_API iterator end() const noexcept;

        const lexing::token& operator[](lexing::token_stream::size_type s) const { return *std::next(begin(), s); }

        template<std::ranges::input_range R> requires utils::range_of<R, const lexing::token>
        static parser_module from_source(R&& r);

    private:
        constexpr parser_module() noexcept = default;
    };

    template<std::ranges::input_range R> requires utils::range_of<R, const lexing::token>
    parser_module parser_module::from_source(R&& r) {
        for (bool expect_dot = false; const lexing::token& tok : std::views::all(r)) {
            if ((expect_dot && !tok.is_dot()) || (!expect_dot && !tok.is_identifier())) {
                throw std::invalid_argument("shift_module::from_source");
            }
            expect_dot ^= 1;
        }

        return {{ utils::range(r) }};
    }

    struct parser_type : shift_type {
        struct dimension {
            std::size_t count{};
            enum class dimension_type : std::uint_fast8_t { pointer = 1, array } type;
            shift_mods mods{};
            std::unique_ptr<shift_expression> size_expr;

            inline bool operator==(const dimension& other) const noexcept {
                return count == other.count && type == other.type && mods == other.mods;
            }

            inline bool operator!=(const dimension& other) const noexcept {
                return !operator==(other);
            }
        };

        token_group name;
        std::vector<dimension> dimensions;
        bool panic{ false };

        inline void add_pointer_dimensions(size_t count) {
            dimensions.push_back({ .count = count, .type = dimension::dimension_type::pointer });
        }

        inline void add_array_dimensions(size_t count, size_t size = -1) {
            dimensions.push_back({ .count = count, .type = dimension::dimension_type::array });
        }

        inline const std::vector<dimension>& get_dimensions() const noexcept { return dimensions; }
    };
}

template<>
struct std::hash<shift::compiler::parsing::token_group> {
    inline std::size_t operator()(const shift::compiler::parsing::token_group& name) const {
        return std::hash<std::string>()(name.to_string());
    }
};

template<>
struct std::hash<shift::compiler::parsing::parser_module> {
    inline std::size_t operator()(const shift::compiler::parsing::parser_module& module_) const {
        return std::hash<std::string>()(module_.to_string());
    }
};

template<>
struct std::hash<shift::compiler::parsing::parser_type::dimension> {
    inline std::size_t operator()(const shift::compiler::parsing::parser_type::dimension& dim) const {
        constexpr auto hasher = std::hash<std::size_t>{};
        return shift::utils::hash_combine(hasher(static_cast<std::size_t>(dim.mods)), hasher(static_cast<std::size_t>(dim.count)),
            hasher(static_cast<std::size_t>(dim.type)));
    }
};

template<>
struct std::hash<shift::compiler::parsing::parser_type> {
    inline std::size_t operator()(const shift::compiler::parsing::parser_type& type) const {
        std::size_t ret = type.type_info ? std::hash<decltype(type.type_info)>()(type.type_info)
                                         : std::hash<decltype(type.name)>()(type.name);
        if (!type.type_info) {
            for (auto const& dim : type.dimensions) {
                ret = shift::utils::hash_combine(ret, std::hash<shift::compiler::parsing::parser_type::dimension>()(dim));
            }
        }
        ret = shift::utils::hash_combine(ret, std::hash<std::size_t>()(static_cast<std::size_t>(type.ref_type)),
            std::hash<std::size_t>()(static_cast<std::size_t>(type.mods)));
        return ret;
    }
};

namespace shift::compiler::parsing {
    struct shift_expression {
        token_group source;
        shift_expression* parent{ nullptr };
        analyzing::type_info* type_info{ nullptr };
        bool panic{ false };

        virtual ~shift_expression() noexcept = default;

        inline std::string to_string() const { return source.to_string(); }

        virtual void update_children() {}
    };

    struct unary_expression : shift_expression {
        lexing::token::type op{ lexing::token::type::NULL_TOKEN };
        enum { prefix, suffix } side;
        std::unique_ptr<shift_expression> sub_expr;

        void update_children() override {
            if (sub_expr) {
                sub_expr->parent = this;
                sub_expr->update_children();
            }
        }
    };

    struct binary_expression : shift_expression {
        lexing::token::type op{ lexing::token::type::NULL_TOKEN };
        std::unique_ptr<shift_expression> left, right;

        void update_children() override {
            if (left) {
                left->parent = this;
                left->update_children();
            }
            if (right) {
                right->parent = this;
                right->update_children();
            }
        }
    };

    struct cp_expression : shift_expression {
        std::unique_ptr<shift_expression> sub_expr;

        void update_children() override {
            if (sub_expr) {
                sub_expr->parent = this;
                sub_expr->update_children();
            }
        }
    };

    struct mv_expression : shift_expression {
        std::unique_ptr<shift_expression> sub_expr;

        void update_children() override {
            if (sub_expr) {
                sub_expr->parent = this;
                sub_expr->update_children();
            }
        }
    };

    struct bracket_expression : shift_expression {
        std::unique_ptr<shift_expression> sub_expr;

        void update_children() override {
            if (sub_expr) {
                sub_expr->parent = this;
                sub_expr->update_children();
            }
        }
    };

    struct cast_expression : shift_expression {
        parser_type cast_type;
        std::unique_ptr<shift_expression> sub_expr;

        void update_children() override {
            if (sub_expr) {
                sub_expr->parent = this;
                sub_expr->update_children();
            }
        }
    };

    struct array_expression : shift_expression {
        std::unique_ptr<shift_expression> sub_expr;
        std::vector<std::unique_ptr<shift_expression>> indexers;

        void update_children() override {
            if (sub_expr) {
                sub_expr->parent = this;
                sub_expr->update_children();
            }
            for (auto& indexer : indexers) {
                if (indexer) {
                    indexer->parent = this;
                    indexer->update_children();
                }
            }
        }
    };

    struct function_call_expression : shift_expression {
        std::unique_ptr<shift_expression> function_expr;
        std::vector<std::unique_ptr<shift_expression>> arguments;

        void update_children() override {
            if (function_expr) {
                function_expr->parent = this;
                function_expr->update_children();
            }
            for (auto& arg : arguments) {
                if (arg) {
                    arg->parent = this;
                    arg->update_children();
                }
            }
        }
    };

    struct new_expression : shift_expression {
        std::variant<function_call_expression, array_expression> call;

        void update_children() override {
            std::visit([](auto& e) { e.update_children(); }, call);
        }
    };

    struct del_expression : shift_expression {
        std::unique_ptr<shift_expression> sub_expr;

        void update_children() override {
            if (sub_expr) {
                sub_expr->parent = this;
                sub_expr->update_children();
            }
        }
    };

    struct comma_expression : shift_expression {
        std::vector<std::unique_ptr<shift_expression>> expressions;

        void update_children() override {
            for (auto& expr : expressions) {
                if (expr) {
                    expr->parent = this;
                    expr->update_children();
                }
            }
        }
    };

    struct dotted_expression : shift_expression {
        std::vector<std::unique_ptr<shift_expression>> expressions;

        void update_children() override {
            for (auto& expr : expressions) {
                if (expr) {
                    expr->parent = this;
                    expr->update_children();
                }
            }
        }
    };

    template<std::derived_from<shift_expression> ExprT, typename... Args>
    std::unique_ptr<ExprT> make_expression(Args&& ... args) {
        // TODO arena allocator instead of default new
        return std::unique_ptr(new ExprT{ std::forward<Args>(args)... });
    }

    SHIFT_TYPES_GENERATOR(expression, shift_expression, unary_expression, binary_expression, cp_expression, mv_expression,
        bracket_expression, cast_expression, array_expression, function_call_expression, new_expression, del_expression,
        comma_expression, dotted_expression)

    struct parser_variable : shift_variable {
        parser_type type;
        const lexing::token* name{ nullptr };
        expression_types value;
        size_t implicit_use_statements = 0;

        const parser_type& get_type() const override { return type; }

        std::string_view get_name() const override { return name->get_data(); }

        SHIFT_API std::string get_fqn() const override;

        SHIFT_API std::variant<std::nullptr_t, parser_module*, parser_class*, parser_function*> get_parent() const;

        SHIFT_API void set_parent(std::variant<std::nullptr_t, parser_module*, parser_class*, parser_function*>);
    };

    struct shift_statement {
        token_group source;
        shift_statement* parent{ nullptr };
        bool panic{ false };
    };

    struct block_statement : shift_statement {
        std::vector<std::unique_ptr<shift_statement>> statements;
    };

    struct if_statement : shift_statement {
        expression_types condition;
        block_statement sub_statements;
        std::optional<block_statement> else_statements;
    };

    struct while_statement : shift_statement {
        expression_types condition;
        block_statement sub_statements;
    };

    struct do_while_statement : shift_statement {
        block_statement sub_statements;
        expression_types condition;
    };

    struct expression_statement : shift_statement {
        expression_types expr;
    };

    struct variable_def_statement : shift_statement {
        parser_variable variable;
    };

    struct for_statement : shift_statement {
        std::variant<std::monostate, expression_statement, variable_def_statement> init;
        expression_types condition;
        expression_statement increment;
        block_statement sub_statements;
    };

    struct use_statement : shift_statement {
        parser_module module_;
    };

    struct continue_statement : shift_statement {
        shift_statement* link{ nullptr };
    };

    struct break_statement : shift_statement {
        shift_statement* link{ nullptr };
    };

    struct return_statement : shift_statement {
        expression_types expr;
    };

    SHIFT_TYPES_GENERATOR(statement, shift_statement, block_statement, if_statement, while_statement, do_while_statement,
        expression_statement, variable_def_statement, for_statement, use_statement, continue_statement, break_statement, return_statement)

    struct parser_class : shift_class {
        // Name of base class
        token_group base_name;

        // Name of the class
        const lexing::token* name{ nullptr };

        // Amount of implicit 'use' statements inherited from direct parent class (if this class is at the top level, this will be
        // the amount of 'use' statements in the global file scope)
        size_t implicit_use_statements = 0;

        // List of 'use' statements for this class
        utils::ordered_set<parser_module> use_statements;

        utils::ideque<parser_class> sub_classes;

        // List of functions in this class
        utils::ideque<parser_function> functions;

        // List of variables in this class
        utils::ideque<parser_variable> fields;

        // Variables representing 'this' and 'base'
        parser_variable this_var, base_var;

        parser* parser_{ nullptr };

        // Get the fully qualified name of this class
        // Class hierarchy for this class should be resolved prior to this
        SHIFT_API std::string get_fqn() const override;

        SHIFT_API std::vector<const shift_class*> get_sub_classes() const override;

        SHIFT_API std::vector<const shift_function*> get_functions() const override;

        SHIFT_API std::vector<const shift_variable*> get_fields() const override;

        inline void set_module(parser_module* module_) noexcept { this->m_module = module_; }

        inline void set_parent(parser_class* parent) noexcept { this->m_parent = parent; }
    };

    struct parser_function : shift_function {
        parser* parser_{ nullptr };
        token_group name;
        parser_type return_type;

        // Amount of implicit 'use' statements inherited from direct parent class (if this function is at the top level, this will be
        // the amount of 'use' statements in the global file scope)
        size_t implicit_use_statements = 0;

        utils::ordered_map<std::string_view, parser_variable> parameters;
        std::deque<statement_types> statements;

        inline std::string get_name() const override { return name.to_string(); }

        SHIFT_API std::string get_fqn() const override;

        SHIFT_API std::string get_signature() const noexcept;

        SHIFT_API utils::ordered_map<std::string_view, const shift_variable*> get_parameters() const override;

        inline const parser_type& get_return_type() const override { return return_type; }

        SHIFT_API std::variant<std::nullptr_t, parser_module*, parser_class*> get_parent() const;

        SHIFT_API void set_parent(std::variant<std::nullptr_t, parser_module*, parser_class*> v);
    };
}

#undef SHIFT_TYPES_GENERATOR
#endif //SHIFT_PARSER_TYPES_H_
