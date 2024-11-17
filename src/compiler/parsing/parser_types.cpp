#include "compiler/parsing/parser_types.h"

#include <algorithm>

#include "utils/variant.h"

namespace shift::compiler::parsing {
    SHIFT_API std::string token_group::to_string() const {
        std::string ret;

        const auto beg = begin(), end_ = end();

        for (auto cur = begin(); cur != end_; ++cur) {
            if (cur != beg) {
                const auto prev = std::prev(cur);
                if (cur->is_identifier() && prev->is_identifier()) {
                    ret += ' ';
                }
            }
            ret += cur->get_data();
        }

        return ret;
    }

    SHIFT_API bool token_group::operator==(const token_group& other) const {
        if (length() != other.length())return false;
        auto b = begin(), b2 = other.begin();
        const auto e = end(), e2 = other.end();
        for (; b != e && b2 != e2; ++b, ++b2) {
            if (b->get_data() != b2->get_data()) return false;
        }
        return b == e && b2 == e2;
    }

    SHIFT_API bool parser_module::is_sub_module(const parser_module& child) const {
        if (depth() >= child.depth()) return false;
        auto it = begin(), other_it = child.begin();
        const auto end_it = end();
        for (; it != end_it; ++it, ++other_it) {
            if (it->get_data() != other_it->get_data()) return false;
        }
        return true;
    }

    SHIFT_API std::partial_ordering parser_module::operator<=>(const parser_module& other) const {
        auto [pos, other_pos] = std::mismatch(begin(), end(), other.begin(), other.end(),
            [](const auto a, const auto b) { return a.get_data() == b.get_data(); });

        if (pos == end()) {
            if (other_pos == other.end()) {
                return std::partial_ordering::equivalent;
            }
            return std::partial_ordering::less;
        }
        if (other_pos == other.end()) {
            return std::partial_ordering::greater;
        }
        return std::partial_ordering::unordered;
    }

    SHIFT_API std::partial_ordering parser_module::operator<=>(std::string_view str) const {
        std::string module_string = to_string();
        auto module_sv = std::string_view{ module_string };

        if (module_sv == str) {
            return std::partial_ordering::equivalent;
        } else if (module_sv.starts_with(str)) {
            return std::partial_ordering::greater;
        } else if (str.starts_with(module_sv)) {
            return std::partial_ordering::less;
        }
        return std::partial_ordering::unordered;
    }

    SHIFT_API parser_module::iterator parser_module::begin() const noexcept {
        return { name.begin(),
                 [&](auto it, auto diff) {
                     auto dis_beg = std::distance(name.begin(), it);
                     auto dis_end = std::distance(it, name.end());
                     return std::next(it, std::clamp(diff * 2, -dis_beg, dis_end));
                 },
                 [](auto a_it, auto b_it) {
                     return std::distance(a_it, b_it) / 2;
                 }
        };
    }

    SHIFT_API parser_module::iterator parser_module::end() const noexcept {
        return { name.end(),
                 [&](auto it, auto diff) {
                     auto dis_beg = std::distance(name.begin(), it);
                     auto dis_end = std::distance(it, name.end());
                     return std::next(it, std::clamp(diff * 2, -dis_beg, dis_end));
                 },
                 [](auto a_it, auto b_it) {
                     return std::distance(a_it, b_it) / 2;
                 }
        };
    }

    static std::string to_string(shift_mods mods) {
        std::string ret;

        if (mods & shift_mods::PUBLIC) {
            if (!ret.empty()) ret += ' ';
            ret += "public";
        } else if (mods & shift_mods::PROTECTED) {
            if (!ret.empty()) ret += ' ';
            ret += "protected";
        } else if (mods & shift_mods::PRIVATE) {
            if (!ret.empty()) ret += ' ';
            ret += "private";
        }

        if (mods & shift_mods::STATIC) {
            if (!ret.empty()) ret += ' ';
            ret += "static";
        }

        if (mods & shift_mods::EXPLICIT) {
            if (!ret.empty()) ret += ' ';
            ret += "explicit";
        }

        if (mods & shift_mods::EXTERN) {
            if (!ret.empty()) ret += ' ';
            ret += "extern";
        }

        if (mods & shift_mods::CONST_) {
            if (!ret.empty()) ret += ' ';
            ret += "const";
        } else if (mods & shift_mods::IMUT) {
            if (!ret.empty()) ret += ' ';
            ret += "imut";
        }

        return ret;
    }

    SHIFT_API std::vector<const shift_class*> parser_class::get_sub_classes() const {
        std::vector<const shift_class*> ret;
        ret.reserve(sub_classes.size());
        for (const auto& c : sub_classes) {
            ret.push_back(&c);
        }
        return ret;
    }

    SHIFT_API std::vector<const shift_function*> parser_class::get_functions() const {
        std::vector<const shift_function*> ret;
        ret.reserve(functions.size());
        for (const auto& f : functions) {
            ret.push_back(&f);
        }
        return ret;
    }

    SHIFT_API std::vector<const shift_variable*> parser_class::get_fields() const {
        std::vector<const shift_variable*> ret;
        ret.reserve(fields.size());
        for (const auto& f : fields) {
            ret.push_back(&f);
        }
        return ret;
    }

    SHIFT_API std::string parser_class::get_fqn() const {
        std::string str;

        if (m_parent) {
            str = static_cast<parser_class*>(m_parent)->get_fqn();
        } else if (m_module) {
            str = static_cast<parser_module*>(m_module)->to_string();
        }

        if (str.size() > 0) str += '.';

        str += name->get_data();

        return str;
    }

    SHIFT_API std::string parser_function::get_fqn() const {
        std::string str;

        std::visit(utils::visit_overloader{
            [](std::nullptr_t) {},
            [&](shift_module* m) {
                str += static_cast<parser_module*>(m)->to_string();
            },
            [&](shift_class* c) {
                str += static_cast<parser_class*>(c)->get_fqn();
            }
        }, m_parent);

        if (!str.empty()) str += '.';

        return str += name.to_string();
    }

    SHIFT_API std::string parser_function::get_signature() const noexcept {
        std::string str = get_fqn() + "(";
        for (bool past_first = false; auto const& [name, v] : parameters) {
            if (past_first) { str += ", "; }
            str += v.type.name.to_string();
            past_first = true;
        }
        return str += ")";
    }

    SHIFT_API std::variant<std::nullptr_t, parser_module*, parser_class*> parser_function::get_parent() const {
        return utils::variant_static_cast<decltype(get_parent())>(m_parent);
    }

    SHIFT_API void parser_function::set_parent(std::variant<std::nullptr_t, parser_module*, parser_class*> v) {
        std::visit(utils::visit_overloader{
            [&](auto p) {
                m_parent = std::move(p);
            }
        }, v);
    }

    SHIFT_API utils::ordered_map<std::string_view, const shift_variable*> parser_function::get_parameters() const {
        utils::ordered_map<std::string_view, const shift_variable*> ret;
        for (const auto& [var_name, var] : parameters) {
            ret.push_back({ var_name, &var });
        }
        return ret;
    }

    std::string parser_variable::get_fqn() const {
        std::string str;
        std::visit(utils::visit_overloader{
            [](std::nullptr_t) {},
            [&](shift_module* m) {
                str = static_cast<parser_module*>(m)->to_string();
            },
            [&](shift_class* c) {
                str = static_cast<parser_class*>(c)->get_fqn();
            },
            [&](shift_function* f) {
                str = static_cast<parser_function*>(f)->get_fqn();
            },
        }, m_parent);
        if (!str.empty()) str += '.';

        return str += name->get_data();
    }

    SHIFT_API std::variant<std::nullptr_t, parser_module*, parser_class*, parser_function*> parser_variable::get_parent() const {
        return utils::variant_static_cast<decltype(get_parent())>(m_parent);
    }

    void parser_variable::set_parent(std::variant<std::nullptr_t, parser_module*, parser_class*, parser_function*> v) {
        std::visit(utils::visit_overloader{
            [&](auto p) {
                m_parent = std::move(p);
            }
        }, v);
    }
}