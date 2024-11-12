#include "compiler/parsing/types.h"

namespace shift::compiler::parsing {
    SHIFT_API std::partial_ordering shift_module::operator<=>(std::string_view other_sv) const {
        std::string module_str = to_string();
        auto module_sv = std::string_view{ module_str };
        auto [pos, other_pos] = std::mismatch(module_sv.begin(), module_sv.end(), other_sv.begin(), other_sv.end(),
            [](const auto a, const auto b) { return std::string_view::traits_type::compare(&a, &b, 1); });

        if (pos == module_sv.end()) {
            if (other_pos == other_sv.end()) {
                return std::partial_ordering::equivalent;
            }
            return std::partial_ordering::less;
        }
        if (other_pos == other_sv.end()) {
            return std::partial_ordering::greater;
        }
        return std::partial_ordering::unordered;
    }

    SHIFT_API size_t shift_class::has_parent(const shift_class* const test) const noexcept {
        size_t count = 1;
        for (const shift_class* current = m_parent; current; current = current->m_parent, count++) {
            if (current == test) return count;
        }
        return 0;
    }

    SHIFT_API std::string shift_variable::get_fqn() const {
        std::string str;
        std::visit(utils::visit_overloader{
            [](std::nullptr_t) {},
            [&](shift_module* m) {
                str += m->to_string();
            },
            [&](shift_class* c) {
                str += c->get_fqn();
            },
            [&](shift_function* f) {
                str += f->get_fqn();
            }
        }, m_parent);
        if (!str.empty()) str += '.';
        return str += get_name();
    }

    SHIFT_API std::string shift_class::get_fqn() const {
        std::string str;
        if (m_parent) {
            str += m_parent->get_fqn();
        } else if (m_module) {
            str += m_module->to_string();
        }
        if (!str.empty()) str += '.';
        return str += get_name();
    }

    SHIFT_API std::string shift_function::get_fqn() const {
        std::string str;
        std::visit(utils::visit_overloader{
            [](std::nullptr_t) {},
            [&](shift_module* m) {
                str += m->to_string();
            },
            [&](shift_class* c) {
                str += c->get_fqn();
            }
        }, m_parent);
        if (!str.empty()) str += '.';
        return str += get_name();
    }
}