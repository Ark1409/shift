/**
 * @file compiler/shift_parser.h
 */
#ifndef SHIFT_PARSER_H_
#define SHIFT_PARSER_H_ 1

#include "compiler/shift_tokenizer.h"
#include "compiler/shift_error_handler.h"
#include "utils/ordered_set.h"
#include "utils/ordered_map.h"
#include "utils/range.h"

#include <string>
#include <string_view>
#include <deque>
#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
#include <array>
#include <optional>
#include <memory>
#include <ranges>
#include <numeric>

// Outline
namespace shift::compiler {
    struct shift_name;
    struct shift_type;
    struct shift_expression;
    struct shift_variable;
    struct shift_statement;
    struct shift_function;
    struct shift_class;

    class shift_module;

    class parser;
}


template<>
struct std::hash<shift::compiler::shift_name>;

template<>
struct std::hash<shift::compiler::shift_module>;

template<>
struct std::hash<shift::compiler::shift_type>;


namespace shift::compiler {
    enum shift_mods : uint_fast16_t {
        NONE = 0x0,
        PUBLIC = 0x1,
        PROTECTED = 0x2,
        PRIVATE = 0x4,
        STATIC = 0x8,
        IMUT = 0x10,
        CONST_ = 0x20 | IMUT,
        BINARY = 0x40,
        EXTERN = 0x80,
        EXPLICIT = 0x100
    };

    constexpr shift::compiler::shift_mods
    operator^(const shift::compiler::shift_mods f, const shift::compiler::shift_mods other) noexcept {
        return shift::compiler::shift_mods(
            std::underlying_type_t<shift::compiler::shift_mods>(f) ^
            std::underlying_type_t<shift::compiler::shift_mods>(other));
    }

    constexpr shift::compiler::shift_mods&
    operator^=(shift::compiler::shift_mods& f, const shift::compiler::shift_mods other) noexcept {
        return f = operator^(f, other);
    }

    constexpr shift::compiler::shift_mods
    operator|(const shift::compiler::shift_mods f, const shift::compiler::shift_mods other) noexcept {
        return shift::compiler::shift_mods(
            std::underlying_type_t<shift::compiler::shift_mods>(f) |
            std::underlying_type_t<shift::compiler::shift_mods>(other));
    }

    constexpr shift::compiler::shift_mods&
    operator|=(shift::compiler::shift_mods& f, const shift::compiler::shift_mods other) noexcept {
        return f = operator|(f, other);
    }

    constexpr shift::compiler::shift_mods
    operator&(const shift::compiler::shift_mods f, const shift::compiler::shift_mods other) noexcept {
        return shift::compiler::shift_mods(
            std::underlying_type_t<shift::compiler::shift_mods>(f) &
            std::underlying_type_t<shift::compiler::shift_mods>(other));
    }

    constexpr shift::compiler::shift_mods&
    operator&=(shift::compiler::shift_mods& f, const shift::compiler::shift_mods other) noexcept {
        return f = operator&(f, other);
    }

    constexpr shift::compiler::shift_mods operator~(const shift::compiler::shift_mods f) noexcept {
        return shift::compiler::shift_mods(~std::underlying_type_t<shift::compiler::shift_mods>(f));
    }
}

namespace shift::compiler {
    struct shift_name {
        std::vector<token>::const_iterator begin, end;

        // Length in tokens
        inline auto size() const noexcept { return std::distance(begin, end); }

        // Length in tokens
        inline auto length() const noexcept { return std::distance(begin, end); }

        inline bool empty() const noexcept { return begin == end; }

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

        inline bool operator==(const shift_name& other) const {
            if constexpr (std::random_access_iterator<decltype(begin)>) {
                if (std::distance(begin, end) != std::distance(other.begin, other.end)) return false;
            }
            auto b = begin, b2 = other.begin;
            for (; b != end && b2 != other.end; ++b, ++b2) {
                if (b->get_data() != b2->get_data()) return false;
            }
            return b == end && b2 == other.end;
        }

        inline bool operator!=(const shift_name& other) const { return !operator==(other); }

        inline bool operator==(const std::string& str) const { return to_string() == str; }

        inline bool operator!=(const std::string& str) const { return !operator==(str); }

        inline bool operator==(const std::string_view str) const { return to_string() == str; }

        inline bool operator!=(const std::string_view& str) const { return !operator==(str); }
    };

    struct shift_module {
        shift_name name;
        shift_mods mods = shift_mods(0x0);

        std::string to_string() const { return name.to_string(); }

        inline operator std::string() const { return name.operator std::string(); }

        bool is_submodule_of(const shift_module& other) const noexcept {
            if (name.size() <= other.name.size()) return false;
            auto b = name.begin, b2 = other.name.begin;
            for (; b2 != other.name.end; ++b, ++b2) {
                if (b->get_data() != b2->get_data()) return false;
            }
            return true;
        }

        inline bool operator==(const shift_module& other) const { return name == other.name; }

        inline bool operator!=(const shift_module& other) const { return !operator==(other); }

        inline bool operator==(const shift_name& other) const { return name == other; }

        inline bool operator!=(const shift_name& other) const { return name != other; }

        inline bool operator==(const std::string& str) const { return to_string() == str; }

        inline bool operator!=(const std::string& str) const { return !operator==(str); }

        inline bool operator==(const std::string_view str) const { return to_string() == str; }

        inline bool operator!=(const std::string_view& str) const { return !operator==(str); }
    };


    struct shift_type {
        shift_mods mods = shift_mods(0x0);
        struct {
            shift_name name;
            shift_class* name_clazz = nullptr; // class of name variable
            shift_class* clazz = nullptr; // will be different from name_clazz if this is an array, otherwise same as name_clazz
        } name;

        enum class reference_type { none, ref, tref } ref_type = reference_type::none;

        struct dimension {
            std::size_t count{};
            enum class dimension_type : uint_fast8_t { pointer = 1, array } type;
            shift_mods mods{};

            inline bool operator==(const dimension& other) const noexcept {
                return count == other.count && type == other.type && mods == other.mods;
            }

            inline bool operator!=(const dimension& other) const noexcept {
                return !operator==(other);
            }
        };

        std::vector<dimension> dimensions;

        bool tried_resolve = false;

        inline bool operator==(const shift_type& other) const noexcept {
            return name.clazz == other.name.clazz && dimensions == other.dimensions && ref_type == other.ref_type && mods == other.mods;
        }

        inline bool operator!=(const shift_type& other) const noexcept { return !operator==(other); }

        inline bool weakly_equal(const shift_type& other) const noexcept {
            return name.clazz == other.name.clazz && dimensions == other.dimensions;
        }

        SHIFT_API std::string get_fqn() const;

        SHIFT_API std::string get_printable_fqn() const;

        inline void add_pointer_dimensions(size_t count) {
            dimensions.push_back({ count, dimension::dimension_type::pointer });
        }

        inline void add_array_dimensions(size_t count) {
            dimensions.push_back({ count, dimension::dimension_type::array });
        }

        inline bool is_resolved() const noexcept { return name.clazz; }

        SHIFT_API bool is_conversion_needed(const shift_type& to) const noexcept;
    };
}

template<>
struct std::hash<shift::compiler::shift_name> {
    inline std::size_t operator()(const shift::compiler::shift_name& name) const {
        return std::hash<std::string>()(name.to_string());
    }
};

template<>
struct std::hash<shift::compiler::shift_module> {
    inline std::size_t operator()(const shift::compiler::shift_module& module_) const {
        return std::hash<std::string>()(module_.to_string());
    }
};

template<>
struct std::hash<shift::compiler::shift_type::dimension> {
    inline std::size_t operator()(const shift::compiler::shift_type::dimension& dim) const {
        static_assert(std::is_standard_layout_v<shift::compiler::shift_type::dimension>);
        return std::hash<std::string_view>()(std::string_view((const char*) std::addressof(dim), sizeof(dim)));
    }
};

template<>
struct std::hash<shift::compiler::shift_type> {
    inline std::size_t operator()(const shift::compiler::shift_type& type) const {
        std::size_t ret = type.is_resolved() ? std::hash<shift::compiler::shift_class*>()(type.name.clazz) : std::hash<
            shift::compiler::shift_name>()(type.name.name);
        for (auto const& dim : type.dimensions) {
            ret = shift::utils::hash_combine(ret, std::hash<shift::compiler::shift_type::dimension>()(dim));
        }
        ret = shift::utils::hash_combine(ret, std::hash<std::size_t>()(static_cast<std::size_t>(type.ref_type)));
        return ret;
    }
};


namespace shift::compiler {
    struct shift_expression {
        token::type type = token::type::NULL_TOKEN;
        shift_expression* parent = nullptr;
        std::vector<token>::const_iterator begin, end;
        std::list<shift_expression> sub;

        struct resolution_info {
            shift_type type;
            shift_variable* variable = nullptr;
            shift_function* function = nullptr;
            shift_class* clazz = nullptr;
            shift_module* module_ = nullptr;
        } resolved;

        bool is_resolved() const noexcept { return resolved.variable || resolved.function || resolved.clazz || resolved.module_; }

        bool is_type_resolved() const noexcept { return resolved.type.name.clazz; }

        inline std::string to_string() const noexcept { return to_name().to_string(); }

        inline shift_name to_name() const noexcept {
            shift_name name;
            name.begin = begin;
            name.end = end;
            return name;
        }

        inline auto size() const noexcept { return std::distance(begin, end); }

        inline bool empty() const noexcept { return begin == end; }

        inline bool is_bracket() const noexcept { return type == token::type::LEFT_BRACKET; }

        inline bool is_function_call() const noexcept {
            return type == token::type::LEFT_SCOPE_BRACKET;
        }

        inline bool is_array() const noexcept { return type == token::type::LEFT_SQUARE_BRACKET; }

        inline void set_bracket() noexcept { type = token::type::LEFT_BRACKET; }

        inline void set_function_call() noexcept { type = token::type::LEFT_SCOPE_BRACKET; }

        inline void set_array() noexcept { type = token::type::LEFT_SQUARE_BRACKET; }

        inline void set_mv_expression(const shift_expression& expr) {
            sub.resize(1);
            sub.front() = expr;
            sub.front().update_parents(this);
        }

        inline void set_mv_expression(shift_expression&& expr) {
            sub.resize(1);
            sub.front() = std::move(expr);
            sub.front().update_parents(this);
        }

        inline void set_cp_expression(const shift_expression& expr) {
            set_mv_expression(expr);
        }

        inline void set_cp_expression(shift_expression&& expr) {
            set_mv_expression(std::move(expr));
        }

        inline shift_expression* get_mv_expression() noexcept {
            return sub.size() == 1 ? &sub.front() : nullptr;
        }

        inline const shift_expression* get_mv_expression() const noexcept {
            return sub.size() == 1 ? &sub.front() : nullptr;
        }

        inline shift_expression* get_cp_expression() noexcept {
            return get_mv_expression();
        }

        inline const shift_expression* get_cp_expression() const noexcept {
            return get_mv_expression();
        }

        inline bool is_cp() const noexcept { return sub.size() == 1 && !empty() && begin->is_cp(); }

        inline bool is_mv() const noexcept { return sub.size() == 1 && !empty() && begin->is_mv(); }

        inline void set_function_call_object(const shift_expression& object) {
            sub.resize(std::max(sub.size(), size_t(1)));
            sub.front() = object;
            sub.front().update_parents(this);
            set_function_call();
        }

        inline void set_function_call_object(shift_expression&& object) {
            sub.resize(std::max(sub.size(), size_t(1)));
            sub.front() = std::move(object);
            sub.front().update_parents(this);
            set_function_call();
        }

        inline void add_function_call_arguments(const shift_expression& args) {
            sub.resize(std::max(sub.size(), size_t(1)));
            sub.push_back(args);
            sub.back().update_parents(this);
            set_function_call();
        }

        inline void add_function_call_arguments(shift_expression&& args) {
            sub.resize(std::max(sub.size(), size_t(1)));
            sub.push_back(std::move(args));
            sub.back().update_parents(this);
            set_function_call();
        }

        inline shift_expression* get_function_call_object() noexcept {
            return sub.empty() ? nullptr : &sub.front();
        }

        inline const shift_expression*
        get_function_call_object() const noexcept { return sub.empty() ? nullptr : &sub.front(); }

        inline auto get_function_call_arguments() noexcept {
            return utils::range(sub.empty() ? sub.begin() : ++sub.begin(), sub.end());
        }

        inline auto get_function_call_arguments() const noexcept {
            return utils::range(sub.empty() ? sub.begin() : ++sub.begin(), sub.end());
        }

        inline shift_expression* get_array_object() noexcept {
            return sub.empty() ? nullptr : &sub.front();
        }

        inline const shift_expression* get_array_object() const noexcept {
            return sub.empty() ? nullptr : &sub.front();
        }

        inline shift_expression* get_array_dimension(const size_t index) noexcept {
            return index + 1 >= sub.size() ? nullptr : &*std::next(sub.begin(), index + 1);
        }

        inline const shift_expression* get_array_dimension(const size_t index) const noexcept {
            return index + 1 >= sub.size() ? nullptr : &*std::next(sub.begin(), index + 1);
        }

        inline auto get_array_dimensions() noexcept {
            return utils::range(sub.empty() ? sub.begin() : ++sub.begin(), sub.end());
        }

        inline auto get_array_dimensions() const noexcept {
            return utils::range(sub.empty() ? sub.begin() : ++sub.begin(), sub.end());
        }

        inline void set_array_object(const shift_expression& object) {
            sub.resize(std::max(sub.size(), size_t(1)));
            sub.front() = object;
            sub.front().update_parents(this);
            set_array();
        }

        inline void set_array_object(shift_expression&& object) {
            sub.resize(std::max(sub.size(), size_t(1)));
            sub.front() = std::move(object);
            sub.front().update_parents(this);
            set_array();
        }

        inline void add_array_dimension(const shift_expression& indexer) {
            sub.resize(std::max(sub.size(), size_t(1)));
            shift_expression& obj = sub.emplace_back();
            obj.set_array_indexer(indexer);
            obj.update_parents(this);
            set_array();
        }

        inline void add_array_dimension(shift_expression&& indexer) {
            sub.resize(std::max(sub.size(), size_t(1)));
            shift_expression& obj = sub.emplace_back();
            obj.set_array_indexer(std::move(indexer));
            obj.update_parents(this);
            set_array();
        }

        inline void clear_array_dimensions() {
            sub.resize(std::min<std::list<shift_expression>::size_type>(sub.size(), 1));
        }

        inline void remove_array_dimension(const std::list<shift_expression>::size_type index) {
            if (!is_array()) return;
            if (index + 1 >= sub.size()) return;
            return remove_array_dimension(std::next(sub.begin(), index + 1));
        }

        inline void remove_array_dimension(std::list<shift_expression>::const_iterator it) {
            if (!is_array()) return;
            if (it == sub.end() || it == sub.begin()) return;
            sub.erase(it);
        }

        inline shift_expression* get_array_indexer_expression() {
            return sub.size() == 1 ? &sub.front() : nullptr;
        }

        inline const shift_expression* get_array_indexer_expression() const {
            return sub.size() == 1 ? &sub.front() : nullptr;
        }

        inline void set_array_indexer(const shift_expression& indexer) {
            sub.resize(1);
            sub.front() = indexer;
            sub.front().update_parents(this);
        }

        inline void set_array_indexer(shift_expression&& indexer) {
            sub.resize(1);
            sub.front() = std::move(indexer);
            sub.front().update_parents(this);
        }

        inline bool is_comma() const noexcept { return type == token::type::COMMA; }

        inline void set_comma() noexcept { type = token::type::COMMA; }

        inline void add_comma_expression(const shift_expression& expr) {
            sub.push_back(expr);
            sub.back().update_parents(this);
            set_comma();
        }

        inline void add_comma_expression(shift_expression&& expr) {
            sub.push_back(std::move(expr));
            sub.back().update_parents(this);
            set_comma();
        }

        inline auto& get_comma_expressions() noexcept { return sub; }

        inline const auto& get_comma_expressions() const noexcept { return sub; }

        inline void set_dotted_expression() noexcept { type = token::type::DOT; }

        inline void add_dotted_expression(const shift_expression& expr) {
            sub.push_back(expr);
            sub.back().update_parents(this);
            set_dotted_expression();
        }

        inline void add_dotted_expression(shift_expression&& expr) {
            sub.push_back(std::move(expr));
            sub.back().update_parents(this);
            set_dotted_expression();
        }

        inline auto& get_dotted_expressions() noexcept { return sub; }

        inline const auto& get_dotted_expressions() const noexcept { return sub; }

        inline bool is_dotted_expression() const noexcept {
            return (type == token::type::DOT) && sub.size() > 0;
        }

        inline void set_new_expression(const shift_expression& type) {
            sub.resize(1);
            sub.front() = type;
            sub.front().update_parents(this);
        }

        inline void set_new_expression(shift_expression&& type) {
            sub.resize(1);
            sub.front() = std::move(type);
            sub.front().update_parents(this);
        }

        inline bool is_new_expression() const noexcept {
            return type == token::type::IDENTIFIER && sub.size() == 1 && begin != end &&
                   begin->is_new();
        }

        inline shift_expression* get_new_expression() noexcept {
            return is_new_expression() ? &sub.front() : nullptr;
        }

        inline const shift_expression*
        get_new_expression() const noexcept { return is_new_expression() ? &sub.front() : nullptr; }

        inline shift_expression* get_bracket_expression() noexcept {
            return is_bracket() && !sub.empty() ? &sub.front() : nullptr;
        }

        inline const shift_expression* get_bracket_expression() const noexcept {
            return is_bracket() && !sub.empty() ? &sub.front() : nullptr;
        }

        inline void set_bracket_expression(const shift_expression& expr) {
            sub.resize(1);
            sub.front() = expr;
            sub.front().update_parents(this);
        }

        inline void set_bracket_expression(shift_expression&& expr) {
            sub.resize(1);
            sub.front() = std::move(expr);
            sub.front().update_parents(this);
        }

        inline bool has_left() const noexcept { return sub.size() == 1 || sub.size() == 2; }

        inline bool has_right() const noexcept { return sub.size() == 2; }

        inline shift_expression* get_left() noexcept { return has_left() ? &sub.front() : nullptr; }

        inline shift_expression* get_right() noexcept {
            return has_right() ? &sub.back() : nullptr;
        }

        inline const shift_expression* get_left() const noexcept {
            return has_left() ? &sub.front() : nullptr;
        }

        inline const shift_expression* get_right() const noexcept {
            return has_right() ? &sub.back() : nullptr;
        }

        inline void clear_left() noexcept {
            if (sub.size() > 1) {
                sub.front() = shift_expression();
            } else {
                sub.clear();
            }
        }

        inline void clear_right() noexcept {
            sub.resize(std::min<std::list<shift_expression>::size_type>(sub.size(), 1));
        }

        inline void clear_children() noexcept { sub.clear(); }

        inline bool has_type() const noexcept { return type != token::type::NULL_TOKEN; }

        inline bool has_deduced_type() const noexcept {
            return resolved.type.name.clazz != nullptr;
        }

        inline void set_left(const shift_expression& expr) {
            sub.resize(std::clamp<size_t>(sub.size(), 1, 2));
            sub.front() = expr;
            sub.front().update_parents(this);
        }

        inline void set_left(shift_expression&& expr) {
            sub.resize(std::clamp<size_t>(sub.size(), 1, 2));
            sub.front() = std::move(expr);
            sub.front().update_parents(this);
        }

        inline void set_left() { set_left(shift_expression()); }

        inline void set_right(const shift_expression& expr) {
            sub.resize(2);
            sub.back() = expr;
            sub.back().update_parents(this);
        }

        inline void set_right(shift_expression&& expr) {
            sub.resize(2);
            sub.back() = std::move(expr);
            sub.back().update_parents(this);
        }

        inline void set_right() { set_right(shift_expression()); }

        void update_parents(shift_expression* this_parent) {
            parent = this_parent;
            for (auto& expr : sub) {
                expr.update_parents(this);
            }
        }
    };

    struct shift_variable {
        shift_type type;
        shift_mods mods = shift_mods::NONE;
        const token* name = nullptr;
        shift_expression value;
        shift_module* module_ = nullptr;
        shift_class* clazz = nullptr;
        shift_function* function = nullptr;
        parser* parser_ = nullptr;
        size_t implicit_use_statements = 0;

        SHIFT_API std::string get_fqn() const;
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
            break_,
            do_while
        } type;

        struct statement_data {
            std::array<shift_expression, 2> expr;
            shift_module module_;
            shift_variable variable;
            std::array<const token*, 2> token_{ nullptr, nullptr };
            shift_statement* statement = nullptr;
        } data;

        shift_statement* parent = nullptr;

        inline void set_if(const token* const token) noexcept {
            type = statement_type::if_;
            data.token_[0] = token;
        }

        inline void set_if_condition(const shift_expression& expr) {
            data.expr[0] = expr;
        }

        inline void set_if_condition(shift_expression&& expr) noexcept {
            data.expr[0] = std::move(expr);
        }

        inline const token* get_if() const noexcept { return data.token_[0]; }

        inline shift_expression& get_if_condition() noexcept { return data.expr[0]; }

        inline const shift_expression& get_if_condition() const noexcept { return data.expr[0]; }

        inline utils::ideque<shift_statement>& get_if_statements() noexcept { return m_sub; }

        inline const utils::ideque<shift_statement>&
        get_if_statements() const noexcept { return m_sub; }

        inline void set_else(const token* const token_) noexcept {
            type = statement_type::else_;
            data.token_[0] = token_;
        }

        inline const token* get_else() const noexcept { return data.token_[0]; }

        inline utils::ideque<shift_statement>& get_else_statements() noexcept { return m_sub; }

        inline const utils::ideque<shift_statement>&
        get_else_statements() const noexcept { return m_sub; }

        inline void connect_else(const shift_statement& else_statement) noexcept {
            ensure_storage_size(1);
            m_statement_storage[0] = else_statement;
            data.statement = &m_statement_storage[0];
        }

        inline void connect_else(shift_statement&& else_statement) noexcept {
            ensure_storage_size(1);
            m_statement_storage[0] = std::move(else_statement);
            data.statement = &m_statement_storage[0];
        }

        inline void attach_else(const shift_statement& else_statement) noexcept {
            connect_else(else_statement);
        }

        inline void attach_else(shift_statement&& else_statement) noexcept {
            connect_else(std::move(else_statement));
        }

        inline const shift_statement&
        get_connected_else() const noexcept { return *data.statement; }

        inline const shift_statement& get_attached_else() const noexcept { return *data.statement; }

        inline shift_statement& get_connected_else() noexcept { return *data.statement; }

        inline shift_statement& get_attached_else() noexcept { return *data.statement; }

        inline bool has_connected_else() const noexcept { return data.statement != nullptr; }

        inline bool has_attached_else() const noexcept { return data.statement != nullptr; }

        inline void set_while(const token* const token_) noexcept {
            type = statement_type::while_;
            data.token_[0] = token_;
        }

        inline void set_while_condition(const shift_expression& expr) {
            data.expr[0] = expr;
        }

        inline void set_while_condition(shift_expression&& expr) noexcept {
            data.expr[0] = std::move(expr);
        }

        inline const token* get_while() const noexcept { return data.token_[0]; }

        inline shift_expression& get_while_condition() noexcept { return data.expr[0]; }

        inline const shift_expression& get_while_condition() const noexcept { return data.expr[0]; }

        inline utils::ideque<shift_statement>& get_while_statements() noexcept { return m_sub; }

        inline const utils::ideque<shift_statement>&
        get_while_statements() const noexcept { return m_sub; }

        inline void set_for(const token* const token_) noexcept {
            type = statement_type::for_;
            data.token_[0] = token_;
        }

        inline const token* get_for() const noexcept { return data.token_[0]; }

        inline void set_for_initializer(const shift_statement& statement) {
            ensure_storage_size(1);
            m_statement_storage.front() = (statement);
        }

        inline void set_for_initializer(shift_statement&& statement) {
            ensure_storage_size(1);
            m_statement_storage.front() = std::move(statement);
        }

        inline void set_for_condition(const shift_expression& expr) {
            data.expr[0] = expr;
        }

        inline void set_for_condition(shift_expression&& expr) noexcept {
            data.expr[0] = std::move(expr);
        }

        inline void set_for_increment(const shift_expression& expr) {
            data.expr[1] = expr;
        }

        inline void set_for_increment(shift_expression&& expr) noexcept {
            data.expr[1] = std::move(expr);
        }

        inline shift_statement&
        get_for_initializer() noexcept { return m_statement_storage.front(); }

        inline const shift_statement&
        get_for_initializer() const noexcept { return m_statement_storage.front(); }

        inline shift_expression& get_for_condition() noexcept { return data.expr[0]; }

        inline const shift_expression& get_for_condition() const noexcept { return data.expr[0]; }

        inline const shift_expression& get_for_increment() const noexcept { return data.expr[1]; }

        inline utils::ideque<shift_statement>& get_for_statements() noexcept { return m_sub; }

        inline const utils::ideque<shift_statement>&
        get_for_statements() const noexcept { return m_sub; }

        inline void set_return(const token* const token_) noexcept {
            type = statement_type::return_;
            data.token_[0] = token_;
        }

        inline void set_return_statement(const shift_expression& expr) {
            data.expr[0] = expr;
        }

        inline void
        set_return_statement(shift_expression&& expr) noexcept { data.expr[0] = std::move(expr); }

        inline void
        set_return_expression(const shift_expression& expr) { return set_return_statement(expr); }

        inline void set_return_expression(shift_expression&& expr) noexcept {
            return set_return_statement(std::move(expr));
        }

        inline const token* get_return() const noexcept { return data.token_[0]; }

        inline shift_expression& get_return_statement() noexcept { return data.expr[0]; }

        inline const shift_expression&
        get_return_statement() const noexcept { return data.expr[0]; }

        inline void set_expression() noexcept { type = statement_type::expression; }

        inline void set_expression(const shift_expression& expr) { data.expr[0] = expr; }

        inline void set_expression(shift_expression&& expr) noexcept {
            data.expr[0] = std::move(expr);
        }

        inline shift_expression& get_expression() noexcept { return data.expr[0]; }

        inline const shift_expression& get_expression() const noexcept { return data.expr[0]; }

        inline void set_variable() noexcept { type = statement_type::variable_alloc; }

        inline void set_variable(const shift_variable& var_) {
            set_variable();
            data.variable = var_;
        }

        inline void set_variable(shift_variable&& var_) noexcept {
            set_variable();
            data.variable = std::move(var_);
        }

        inline shift_variable& get_variable() noexcept { return data.variable; }

        inline const shift_variable& get_variable() const noexcept { return data.variable; }

        inline void set_block(const token* const token_) noexcept {
            type = statement_type::scope_begin;
            data.token_[0] = token_;
        }

        inline void
        set_block(const utils::ideque<shift_statement>& sub) noexcept { this->m_sub = sub; }

        inline void
        set_block(utils::ideque<shift_statement>&& sub) noexcept { this->m_sub = std::move(sub); }

        inline const token* get_block() const noexcept { return data.token_[0]; }

        inline utils::ideque<shift_statement>& get_block_statements() noexcept { return m_sub; }

        inline const utils::ideque<shift_statement>&
        get_block_statements() const noexcept { return m_sub; }

        inline void set_block_end(const token* const token_) noexcept {
            type = statement_type::scope_begin;
            data.token_[1] = token_;
        }

        inline const token* get_block_end() const noexcept { return data.token_[1]; }

        inline void set_continue(const token* const token_) noexcept {
            type = statement_type::continue_;
            data.token_[0] = token_;
        }

        inline void set_break(const token* const token_) noexcept {
            type = statement_type::break_;
            data.token_[0] = token_;
        }

        inline const token* get_break() const noexcept { return data.token_[0]; }

        inline const token* get_continue() const noexcept { return data.token_[0]; }

        inline void set_use(const token* const token_) noexcept {
            type = statement_type::use;
            data.token_[0] = token_;
        }

        inline void set_use_module(const shift_module& module_) noexcept {
            data.module_ = module_;
        }

        inline void set_use_module(shift_module&& module_) noexcept {
            data.module_ = std::move(module_);
        }

        inline const token* get_use() const noexcept { return data.token_[0]; }

        inline const shift_module& get_use_module() const noexcept { return data.module_; }

        inline shift_statement* get_break_link() const noexcept { return data.statement; }

        inline void set_break_link(shift_statement* const stat) {
            data.statement = stat;
        }

        inline shift_statement* get_continue_link() const noexcept { return data.statement; }

        inline void set_continue_link(shift_statement* const stat) {
            data.statement = stat;
        }

    private:
        inline void ensure_sub_size(size_t n) {
            if (m_sub.size() < n) {
                m_sub.resize(n);
            }
        }

        inline void ensure_storage_size(size_t n) {
            if (m_statement_storage.size() < n) {
                m_statement_storage.resize(n);
            }
        }

        utils::ideque<shift_statement> m_statement_storage;
        utils::ideque<shift_statement> m_sub;
    };

    struct shift_class {
        // Module this class belongs to
        shift_module* module_ = nullptr;

        // Parent and base class information
        struct {
            shift_name name;
            shift_class* name_clazz = nullptr;
            shift_class* clazz = nullptr;
        } parent, base;

        // Access modifiers for class
        shift_mods mods = shift_mods::NONE;

        // Name of the class
        const token* name = nullptr;

        // Amount of implicit 'use' statements inheritied from direct parent class (if this class is at the top level, this will be 
        // the amount of 'use' statements in the global file scope)
        size_t implicit_use_statements = 0;

        // List of 'use' statements for this class
        utils::ordered_set<shift_module> use_statements;

        // List of functionsin this class
        utils::ideque<shift_function> functions;

        // List of variables in this class
        utils::ideque<shift_variable> variables;

        // Variables representing 'this' and 'base'
        shift_variable this_var, base_var;

        // The parser object that created this class
        parser* parser_ = nullptr;

        // TODO move all of these functions into analyzer class, since they can only be used once class hierarchy is resolved

        // Recursively check bases to see if this class has the specified base class
        // Only works if all base classes for this class have already been resolved
        inline size_t has_base(const shift_class* const test) const noexcept {
            size_t count = 1;
            for (const shift_class* current = base.clazz; current; current = current->base.clazz, count++) {
                if (current == test) return count;
            }
            return 0;
        }

        // Checks if this class can access protected variables from the specified class
        inline bool has_protected_access(const shift_class* const test) const noexcept {
            return test == this || has_base(test) > 0;
        }

        // Recursively check parents to see if this class has the specified parent class
        // A parent class differs from a base in class in that a parent class is the class that directly encloses this class
        inline size_t has_parent(const shift_class* const test) const noexcept {
            size_t count = 1;
            for (const shift_class* current = parent.clazz; current; current = current->parent.clazz, count++) {
                if (current == test) return count;
            }
            return 0;
        }

        // Get the fully qualified name of this class
        // Class hierarchy for this class should be resolved prior to this
        inline std::string get_fqn() const {
            std::string str;

            if (parent.clazz) str = parent.clazz->get_fqn();
            else if (module_) str = module_->name.to_string();
            if (str.size() > 0) str += '.';

            str += name->get_data();

            using namespace std::string_view_literals;
            if (utils::starts_with((std::string_view) str, "shift.array@"sv)) {
                // shift.array@clazz_name@array_dimensions
                // const size_t array_dim = utils::str_to_num<size_t>(str.substr(str.find_last_of('@') + 1));
                // str = str.substr(str.find('@') + 1, str.find_last_of('@') - (str.find('@') + 1));
                // for (size_t i = 0; i < array_dim; i++) {
                //     str += "[]";
                // }
            }

            return str;
        }
    };

    struct shift_function {
        shift_mods mods = shift_mods::NONE;
        shift_module* module_ = nullptr;
        shift_class* clazz = nullptr;
        parser* parser_ = nullptr;
        shift_name name;
        shift_type return_type;

        // Amount of implicit 'use' statements inheritied from direct parent class (if this function is at the top level, this will be 
        // the amount of 'use' statements in the global file scope)
        size_t implicit_use_statements = 0;

        utils::ordered_map<std::string_view, shift_variable> parameters;
        utils::ideque<shift_statement> statements;

        inline std::string get_fqn() const noexcept {
            std::string str;

            if (clazz) str = clazz->get_fqn();
            else if (module_) str = module_->to_string();

            if (!str.empty()) str += '.';

            return str += name.to_string();
        }

        inline std::string get_fqn(const size_t id) const noexcept {
            std::string str = get_fqn();
            str += '@' + std::to_string(id);
            return str;
        }

        inline std::string get_signature() const noexcept {
            std::string str = get_fqn() + "(";
            for (bool past_first = false; auto const& [name, v] : parameters) {
                if (past_first) { str += ", "; }
                if (v.type.name.clazz) {
                    str += v.type.get_printable_fqn();
                } else {
                    str += "<unknown>";
                }
                past_first = true;
            }
            return str += ")";
        }
    };
}

namespace shift::compiler {
    // fwd decl analyzer class for friend
    class analyzer;

    class parser {
    public:
        inline parser(tokenizer* const tokenizer) noexcept;

        inline parser(error_handler* const error_handler, tokenizer* const tokenizer) noexcept;

        inline parser(error_handler* const error_handler, tokenizer& tokenizer) noexcept;

        parser(const parser&) = delete;

        parser(parser&&) noexcept = default;

        parser& operator=(const parser&) = delete;

        parser& operator=(parser&&) noexcept = default;

        SHIFT_API void parse();

        inline tokenizer* get_tokenizer() const noexcept { return m_tokenizer; }

        inline void set_tokenizer(tokenizer* const tokenizer) noexcept { m_tokenizer = tokenizer; }

        inline error_handler* get_error_handler() noexcept { return m_error_handler; }

        inline const error_handler* get_error_handler() const noexcept { return m_error_handler; }

        inline void set_error_handler(
            error_handler* const error_handler) noexcept { m_error_handler = error_handler; }

        inline shift_module& get_module() noexcept { return *this->m_module; }

        inline const shift_module& get_module() const noexcept { return *this->m_module; }

        inline std::deque<shift_class>& get_classes() noexcept { return m_classes; }

        inline const std::deque<shift_class>& get_classes() const noexcept { return m_classes; }

        inline std::deque<shift_function>& get_functions() noexcept { return m_functions; }

        inline const std::deque<shift_function>&
        get_functions() const noexcept { return m_functions; }

        inline std::deque<shift_variable>& get_variables() noexcept { return m_variables; }

        inline const std::deque<shift_variable>&
        get_variables() const noexcept { return m_variables; }

        inline utils::ordered_set<shift_module>&
        get_global_uses() noexcept { return m_global_uses; }

        inline const utils::ordered_set<shift_module>&
        get_global_uses() const noexcept { return m_global_uses; }

        inline void clear() noexcept {
            m_module = std::make_unique<shift_module>();
            m_global_uses.clear();
            m_classes.clear();
            m_functions.clear();
            m_variables.clear();
        }

        SHIFT_API static uint_fast8_t
        operator_priority(const token::type type, const bool prefix = false) noexcept;

#ifdef SHIFT_DEBUG

        SHIFT_API void print_tree();

#endif

    private:
        void m_parse_access_specifier();

        void m_parse_use();

        void m_parse_use(utils::ordered_set<shift_module>&);

        void m_parse_module();

        void m_parse_class(shift_class* parent_class = nullptr);

        shift_function* m_parse_function_header(shift_class* parent_class, shift_type& return_type);

        std::optional<shift_variable>
        m_parse_variable_header(shift_class* parent_class, shift_function* parent_function,
            shift_type& type);

        // void m_parse_class(shift_class&);
        void m_parse_function(shift_function&);

        void
        m_parse_function_block(shift_function&, utils::ideque<shift_statement>&, size_t count = -1);

        void m_parse_body(shift_class* = nullptr);

        shift_expression
        m_parse_expression(const utils::predicate<std::vector<token>::const_iterator>& end_func);

        inline shift_expression
        m_parse_expression(const token::type end_type = token::type::SEMICOLON) {
            return m_parse_expression([end_type](const std::vector<token>::const_iterator it) {
                return it->get_token_type() == end_type;
            });
        }

        shift_name m_parse_name(std::string_view);

        std::optional<shift_type> m_parse_type(std::string_view);

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

        const token& m_skip_until(const typename token::type) noexcept;

        const token& m_skip_after(const std::string_view) noexcept;

        const token& m_skip_after(const std::string&) noexcept;

        const token& m_skip_after(const char* const) noexcept;

        const token& m_skip_after(const typename token::type) noexcept;

        const token& m_skip_before(const std::string_view) noexcept;

        const token& m_skip_before(const std::string&) noexcept;

        const token& m_skip_before(const char* const) noexcept;

        const token& m_skip_before(const typename token::type) noexcept;

        const token& m_skip_until_closing(const typename token::type) noexcept;

        shift_mods m_get_mods() const noexcept;

        void m_add_mod(shift_mods, const token&) noexcept;

        void m_clear_mods() noexcept;

        bool m_is_module_defined() const noexcept;

    private:
        // Tokenized file. Tokenization must have passed with no errors in order to be usable in the parsing stage
        tokenizer* m_tokenizer;

        // Error handler (if desired)
        error_handler* m_error_handler;

        // The current module for the file
        std::unique_ptr<shift_module> m_module{ std::make_unique<shift_module>() };

        // Storage for all global use statements in the module. 
        // It simpliy contains all the 'use' statements in sequential order
        utils::ordered_set<shift_module> m_global_uses;

        // List of classes found inside current module inside current file
        std::deque<shift_class> m_classes;

        // List of functions which are tied solely to the current module inside the current file (not inside a class)
        std::deque<shift_function> m_functions;

        // List of variables which are tied solely to the current module inside the current file (not inside a class)
        std::deque<shift_variable> m_variables;

        // Utility variable for holding the current mods specified by the user
        std::vector<std::pair<shift_mods, const token*>> m_mods;

        friend class analyzer;
    };

    inline parser::parser(tokenizer* const tokenizer) noexcept :
        m_tokenizer(tokenizer), m_error_handler(tokenizer->get_error_handler()) {}

    inline parser::parser(error_handler* const error_handler, tokenizer* const tokenizer) noexcept :
        m_tokenizer(tokenizer), m_error_handler(error_handler) {}

    inline parser::parser(error_handler* const error_handler, tokenizer& tokenizer) noexcept :
        parser(error_handler, &tokenizer) {}
}

#endif