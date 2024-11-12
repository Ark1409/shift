#ifndef SHIFT_UTILS_LAZY_H_
#define SHIFT_UTILS_LAZY_H_ 1

#include <variant>
#include <functional>
#include <concepts>

#include "utils/variant.h"

namespace shift::utils {
    template<typename T>
    class lazy {
    public:
        lazy() noexcept = default;

        lazy(const std::function<T(void)>& generator) : m_data(generator) {}

        lazy(std::function<T(void)>&& generator) : m_data(std::move(generator)) {}

        T& get() {
            return std::visit(utils::visit_overloader{
                [](T& t) { return t; },
                [&](const std::function<T(void)>& gen) { return m_data.emplace(gen()); },
                [](std::monostate) { throw std::bad_variant_access(); }
            }, m_data);
        }

        const T& get() const {
            return std::visit(utils::visit_overloader{
                [](const T& t) { return t; },
                [&](const std::function<T(void)>& gen) { return m_data.emplace(gen()); },
                [](std::monostate) { throw std::bad_variant_access(); }
            }, m_data);
        }

        T* operator->() { return &get(); }

        const T* operator->() const { return &get(); }

        T& operator*() { return get(); }

        const T& operator*() const { return get(); }

        const std::function<T(void)>& get_generator() const {
            return std::visit(utils::visit_overloader{
                [](const std::function<T(void)>& gen) { return gen },
                [](auto&) { throw std::bad_variant_access(); }
            }, m_data);
        }

        void set_generator(const std::function<T(void)>& gen) {
            std::visit(utils::visit_overloader{
                [](const T&) { throw std::bad_variant_access(); },
                [&](auto&) { m_data.emplace(gen); }
            }, m_data);
        }

        void set_generator(std::function<T(void)>&& gen) {
            std::visit(utils::visit_overloader{
                [](const T&) { throw std::bad_variant_access(); },
                [&](auto&) { m_data.emplace(std::move(gen)); }
            }, m_data);
        }

        template<typename... Args> requires std::constructible_from<T, Args...>
        void emplace(Args&& ... args) {
            std::visit(utils::visit_overloader{
                [](const T&) { throw std::bad_variant_access(); },
                [&](auto&) { m_data.template emplace<utils::variant_index_v<T, decltype(m_data)>>(std::forward<Args>(args)...); }
            }, m_data);
        }

        template<typename... Args> requires std::constructible_from<T, Args...>
        static lazy of(Args&& ... args) { return lazy(std::forward<Args>(args)...); }

    private:
        template<typename... Args> requires std::constructible_from<T, Args...>
        explicit lazy(Args&& ... args) : m_data(std::in_place_type<T>, std::forward<Args>(args)...) {}

    private:
        mutable std::variant<std::monostate, T, std::function<T(void)>> m_data;
    };
}

#endif //SHIFT_UTILS_LAZY_H_
