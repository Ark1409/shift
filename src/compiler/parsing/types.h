#ifndef SHIFT_PARSING_TYPES_H_
#define SHIFT_PARSING_TYPES_H_ 1

#include "shift_config.h"
#include "compiler/mods.h"
#include "compiler/parsing/types_fwd.h"
#include "compiler/analyzing/analyzer_fwd.h"
#include "utils/ordered_map.h"
#include "utils/ordered_set.h"
#include "utils/utils.h"

#include <string>
#include <string_view>
#include <algorithm>
#include <cstdint>
#include <vector>
#include <variant>
#include <memory>

namespace shift::compiler::parsing {
    struct shift_module {
        shift_mods mods{shift_mods::NONE};

        virtual ~shift_module() noexcept = default;

        inline virtual std::size_t depth() const {
            std::string str = to_string();
            return utils::count(std::string_view{str}, std::string_view("."));
        }

        virtual std::string to_string() const = 0;

        inline explicit operator std::string() const { return to_string(); }

        inline bool is_sub_module(const shift_module& child) const {
            auto str = this->to_string();
            auto child_str = child.to_string();
            return child_str.starts_with(str) && child_str != str;
        }

        inline bool is_sub_module_of(const shift_module& parent) const { return parent.is_sub_module(*this); }

        inline bool operator==(const shift_module& other) const { return to_string() == other.to_string(); }

        inline bool operator!=(const shift_module& other) const { return !operator==(other); }

        inline bool operator==(const std::string& str) const { return to_string() == str; }

        inline bool operator!=(const std::string& str) const { return !operator==(str); }

        inline bool operator==(const std::string_view& str) const { return to_string() == str; }

        inline bool operator!=(const std::string_view& str) const { return !operator==(str); }

        inline std::partial_ordering operator<=>(const shift_module& m) const { return operator<=>(m.to_string()); }

        inline std::partial_ordering operator<=>(const std::string& s) const { return operator<=>((std::string_view) s); }

        SHIFT_API std::partial_ordering operator<=>(std::string_view str) const;
    };

    struct shift_type {
        shift_mods mods{shift_mods::NONE};
        enum class reference_type { none, ref, tref } ref_type = reference_type::none;
        analyzing::type_info* type_info{nullptr};

        virtual ~shift_type() noexcept = default;
    };

    struct shift_variable {
        virtual ~shift_variable() noexcept = default;

        shift_mods mods = shift_mods::NONE;

        virtual const shift_type& get_type() const = 0;

        virtual std::string_view get_name() const = 0;

        SHIFT_API virtual std::string get_fqn() const;

        inline auto get_parent() const noexcept { return m_parent; }

    protected:
        std::variant<std::nullptr_t, shift_module*, shift_class*, shift_function*> m_parent{nullptr};
    };

    struct shift_class {
        // Access modifiers for class
        shift_mods mods{shift_mods::NONE};

        analyzing::class_type_info* type_info{nullptr};

        virtual ~shift_class() noexcept = default;

        const shift_module& get_module() const noexcept { return *m_module; }

        const shift_class* get_parent() const noexcept { return m_parent; }

        // TODO move all of these functions into analyzer class, since they can only be used once class hierarchy is resolved

        virtual std::string_view get_name() const = 0;

        // Get the fully qualified name of this class
        // Class hierarchy for this class should be resolved prior to this
        SHIFT_API virtual std::string get_fqn() const;

        SHIFT_API size_t has_parent(const shift_class* const test) const noexcept;

        virtual std::vector<const shift_class*> get_sub_classes() const = 0;

        virtual std::vector<const shift_function*> get_functions() const = 0;

        virtual std::vector<const shift_variable*> get_fields() const = 0;

    protected:
        // Module this class belongs to
        shift_module* m_module{nullptr};

        shift_class* m_parent{nullptr};
    };

    struct shift_function {
        shift_mods mods{shift_mods::NONE};

        virtual ~shift_function() noexcept = default;

        virtual std::string get_name() const = 0;

        SHIFT_API virtual std::string get_fqn() const;

        virtual utils::ordered_map<std::string_view, const shift_variable*> get_parameters() const = 0;

        virtual const shift_type& get_return_type() const = 0;

        inline auto get_parent() const { return m_parent; }

    protected:
        std::variant<std::nullptr_t, shift_module*, shift_class*> m_parent;
    };
}

#endif //SHIFT_PARSING_TYPES_H_
