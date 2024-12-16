#ifndef SHIFT_PARSER_TYPES_H_
#define SHIFT_PARSER_TYPES_H_ 1

#include "compiler/lexing/lexer.h"
#include "compiler/mods.h"
#include "compiler/analyzing/analyzer_fwd.h"
#include "compiler/parsing/parser_fwd.h"
#include "compiler/parsing/types.h"

#include "utils/range.h"
#include "utils/iterator_wrapper.h"
#include "utils/ordered_set.h"
#include "utils/variant.h"

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
    template<template<typename> typename Transformer, typename Base, typename... DerivedTs>
    auto variant_type_generator()
    -> std::decay_t<decltype(std::declval<std::variant<Transformer < Base>, Transformer < DerivedTs>...>>
    ())> {
    static_assert(false);
}
}

#define SHIFT_TYPES_GENERATOR(name, base, ...)                                                                                         \
    using name##_types = decltype(shift::compiler::parsing::detail::variant_type_generator<std::type_identity_t, base __VA_OPT__(,)    \
        __VA_ARGS__>());                                                                                                               \
    using name##_ptr_types = decltype(shift::compiler::parsing::detail::variant_type_generator<std::add_pointer_t, base __VA_OPT__(,)  \
        __VA_ARGS__>());

namespace shift::compiler::parsing {
    struct token_group {
        using iterator = lexing::token_stream::const_iterator;

        utils::range<lexing::token_stream::const_iterator> source;

        bool panic{false};

        iterator begin() const { return source.begin(); }

        iterator end() const { return source.end(); }

        const auto& front() const { return *begin(); }

        const auto& back() const { return *std::prev(end()); }

        /// @brief Obtains the length of the name in position.
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

        constexpr parser_module(const token_group& tg) : name(tg) {}

        token_group name;

        std::size_t depth() const override { return (name.length() + 1) / 2; }

        inline std::string to_string() const override { return name.to_string(); }

        SHIFT_API bool is_sub_module(const parser_module& child) const;

        SHIFT_API std::partial_ordering operator<=>(const parser_module& other) const;

        SHIFT_API std::partial_ordering operator<=>(std::string_view str) const;

        SHIFT_API iterator begin() const noexcept;

        SHIFT_API iterator end() const noexcept;

        SHIFT_API const lexing::token& operator[](lexing::token_stream::size_type s) const;
    };

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
        bool panic{false};

        inline void add_pointer_dimensions(size_t count) {
            dimensions.push_back({.count = count, .type = dimension::dimension_type::pointer});
        }

        inline void add_array_dimensions(size_t count, size_t size = -1) {
            dimensions.push_back({.count = count, .type = dimension::dimension_type::array});
        }

        inline const std::vector<dimension>& get_dimensions() const noexcept { return dimensions; }

        std::string to_string() const;
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
#define APPLY(F, ...) F(__VA_ARGS__)
#define SHIFT_EXPRESSION_TYPES \
        shift_expression, literal_expression, unary_expression, binary_expression, cp_expression, mv_expression,\
        bracket_expression, cast_expression, array_expression, function_call_expression, new_expression, del_expression,\
        comma_expression, dotted_expression

    APPLY(SHIFT_TYPES_GENERATOR, expression, SHIFT_EXPRESSION_TYPES);

#define SHIFT_EXPRESSION_VISITABLE() \
    private:                                 \
        virtual void visit(expression_visitor& v); \
        friend struct utils::visit_dispatcher<shift_expression, expression_visitor>

    using expression_visitor = utils::type_visitor<SHIFT_EXPRESSION_TYPES>;

//#undef SHIFT_EXPRESSION_TYPES
#undef APPLY

    struct shift_expression {
        token_group source;
        shift_expression* parent{nullptr};
        analyzing::type_info* type_info{nullptr};
        bool panic{false};

        virtual ~shift_expression() noexcept;

        inline std::string to_string() const { return source.to_string(); }

        virtual void update_children() {}

    SHIFT_EXPRESSION_VISITABLE();
    };

    struct literal_expression : shift_expression{
        lexing::token::type type{lexing::token::type::NULL_TOKEN};

    SHIFT_EXPRESSION_VISITABLE();
    };

    struct unary_expression : shift_expression {
        lexing::token::type op{lexing::token::type::NULL_TOKEN};
        enum { prefix, suffix } side;
        std::unique_ptr<shift_expression> sub_expr;

        void update_children() override {
            if (sub_expr) {
                sub_expr->parent = this;
                sub_expr->update_children();
            }
        }

    SHIFT_EXPRESSION_VISITABLE();
    };

    struct binary_expression : shift_expression {
        lexing::token::type op{lexing::token::type::NULL_TOKEN};
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

    SHIFT_EXPRESSION_VISITABLE();
    };

    struct cp_expression : shift_expression {
        std::unique_ptr<shift_expression> sub_expr;

        void update_children() override {
            if (sub_expr) {
                sub_expr->parent = this;
                sub_expr->update_children();
            }
        }

    SHIFT_EXPRESSION_VISITABLE();
    };

    struct mv_expression : shift_expression {
        std::unique_ptr<shift_expression> sub_expr;

        void update_children() override {
            if (sub_expr) {
                sub_expr->parent = this;
                sub_expr->update_children();
            }
        }

    SHIFT_EXPRESSION_VISITABLE();
    };

    struct bracket_expression : shift_expression {
        std::unique_ptr<shift_expression> sub_expr;

        void update_children() override {
            if (sub_expr) {
                sub_expr->parent = this;
                sub_expr->update_children();
            }
        }

    SHIFT_EXPRESSION_VISITABLE();
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

    SHIFT_EXPRESSION_VISITABLE();
    };

    struct array_expression : shift_expression {
        std::unique_ptr<shift_expression> sub_expr;
        std::vector<expression_types> indexers;

        void update_children() override;
    SHIFT_EXPRESSION_VISITABLE();
    };

    struct function_call_expression : shift_expression {
        std::unique_ptr<shift_expression> function_expr;
        std::vector<expression_types> arguments;

        void update_children() override;

    SHIFT_EXPRESSION_VISITABLE();
    };

    struct new_expression : shift_expression {
        std::variant<function_call_expression, array_expression> call;

        void update_children() override {
            std::visit([](auto& e) { e.update_children(); }, call);
        }

    SHIFT_EXPRESSION_VISITABLE();
    };

    struct del_expression : shift_expression {
        std::unique_ptr<shift_expression> sub_expr;

        void update_children() override {
            if (sub_expr) {
                sub_expr->parent = this;
                sub_expr->update_children();
            }
        }

    SHIFT_EXPRESSION_VISITABLE();
    };

    struct comma_expression : shift_expression {
        std::vector<expression_types> expressions;

        void update_children() override;

    SHIFT_EXPRESSION_VISITABLE();
    };

    struct dotted_expression : shift_expression {
        std::vector<expression_types> expressions;

        void update_children() override;

    SHIFT_EXPRESSION_VISITABLE();
    };

#undef SHIFT_EXPRESSION_VISITABLE

    template<std::derived_from<shift_expression> ExprT, typename... Args>
    std::unique_ptr<ExprT> make_expression(Args&& ... args) {
        // TODO arena allocator instead of default new
        return std::unique_ptr<ExprT>(new ExprT{std::forward<Args>(args)...});
    }

    struct parser_variable : shift_variable {
        parser_type type;
        const lexing::token* name{nullptr};
        expression_types value;
        size_t implicit_use_statements = 0;
        bool panic{false};

        const parser_type& get_type() const override { return type; }

        std::string_view get_name() const override { return name->get_data(); }

        SHIFT_API std::string get_fqn() const override;

        SHIFT_API std::variant<std::nullptr_t, parser_module*, parser_class*, parser_function*> get_parent() const;

        SHIFT_API void set_parent(std::variant<std::nullptr_t, parser_module*, parser_class*, parser_function*>);
    };

    SHIFT_TYPES_GENERATOR(statement, shift_statement, block_statement, if_statement, while_statement, do_while_statement,
        expression_statement, variable_def_statement, for_statement, use_statement, continue_statement, break_statement, return_statement);

    struct shift_statement {
        token_group source;
        shift_statement* parent{nullptr};
        bool panic{false};

        virtual ~shift_statement() noexcept;
    };

    struct block_statement : shift_statement {
        utils::ideque<statement_types> statements;

        bool empty() const noexcept { return statements.empty(); }
    };

    struct if_statement : shift_statement {
        expression_types condition;
        block_statement body;
        std::optional<block_statement> else_body;
    };

    struct while_statement : shift_statement {
        expression_types condition;
        block_statement body;
    };

    struct do_while_statement : shift_statement {
        block_statement body;
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
        expression_types increment;
        block_statement body;
    };

    struct use_statement : shift_statement {
        parser_module module_;
    };

    struct continue_statement : shift_statement {
        shift_statement* link{nullptr};
    };

    struct break_statement : shift_statement {
        shift_statement* link{nullptr};
    };

    struct return_statement : shift_statement {
        std::optional<expression_types> expr;
    };

    struct parser_function : shift_function {
        parser* parser_{nullptr};
        token_group name;
        parser_type return_type;

        // Amount of implicit 'use' statements inherited from direct parent class (if this function is at the top level, this will be
        // the amount of 'use' statements in the global file scope)
        size_t implicit_use_statements = 0;

        utils::ordered_map<std::string_view, parser_variable> parameters;
        block_statement body;

        inline std::string get_name() const override { return name.to_string(); }

        SHIFT_API std::string get_fqn() const override;

        SHIFT_API std::string get_signature() const noexcept;

        SHIFT_API utils::ordered_map<std::string_view, const shift_variable*> get_parameters() const override;

        inline const parser_type& get_return_type() const override { return return_type; }

        SHIFT_API std::variant<std::nullptr_t, parser_module*, parser_class*> get_parent() const;

        SHIFT_API void set_parent(std::variant<std::nullptr_t, parser_module*, parser_class*> v);
    };

    struct parser_class : shift_class {
        // Name of base class
        token_group base_name;

        // Name of the class
        const lexing::token* name{nullptr};

        // Amount of implicit 'use' statements inherited from direct parent class (if this class is at the top level, this will be
        // the amount of 'use' statements in the global file scope)
        size_t implicit_use_statements = 0;

        // List of 'use' statements for this class
        utils::ordered_set<parser_module> use_statements;

        utils::ideque<parser_class> sub_classes;

        // List of functions in this class
        std::deque<parser_function> functions;

        // List of variables in this class
        std::deque<parser_variable> fields;

        parser* parser_{nullptr};

        std::string_view get_name() const override { return name->get_data(); }

        // Get the fully qualified name of this class
        // Class hierarchy for this class should be resolved prior to this
        SHIFT_API std::string get_fqn() const override;

        // All returned classes are of the type parser_class*
        SHIFT_API std::vector<const shift_class*> get_sub_classes() const override;

        // All returned function are of the type parser_function*
        SHIFT_API std::vector<const shift_function*> get_functions() const override;

        // All returned fields are of the type parser_variable*
        SHIFT_API std::vector<const shift_variable*> get_fields() const override;

        inline void set_module(parser_module* module_) noexcept { this->m_module = module_; }

        inline void set_parent(parser_class* parent) noexcept { this->m_parent = parent; }

        const parser_module& get_module() const noexcept { return static_cast<const parser_module&>(*m_module); }

        const parser_class* get_parent() const noexcept { return static_cast<const parser_class*>(m_parent); }
    };
}

#undef SHIFT_TYPES_GENERATOR
#endif //SHIFT_PARSER_TYPES_H_
