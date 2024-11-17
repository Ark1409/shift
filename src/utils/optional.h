#ifndef SHIFT_UTILS_OPTIONAL_H_
#define SHIFT_UTILS_OPTIONAL_H_

#include <optional>
#include <functional>
#include <type_traits>
#include <concepts>
#include <memory>

namespace shift::utils {
    template<typename T>
    using optional_ref = std::optional<std::reference_wrapper<T>>;

    namespace detail {
        template<typename D, typename T>
        class optional_base : public std::optional<T> {
        protected:
            using optional_type = std::optional<T>;
        public:
            using optional_type::optional_type;
        public:
            template<class F>
            constexpr auto and_then(F&& f)& {
                if (*this)
                    return std::invoke(std::forward<F>(f), static_cast<D*>(this)->value());
                else
                    return std::remove_cvref_t<std::invoke_result_t<F, T&>>{};
            }

            template<class F>
            constexpr auto and_then(F&& f) const& {
                if (*this)
                    return std::invoke(std::forward<F>(f), static_cast<D*>(this)->value());
                else
                    return std::remove_cvref_t<std::invoke_result_t<F, const T&>>{};
            }

            template<class F>
            constexpr auto and_then(F&& f)&& {
                if (*this)
                    return std::invoke(std::forward<F>(f), std::move(static_cast<D*>(this)->value()));
                else
                    return std::remove_cvref_t<std::invoke_result_t<F, T>>{};
            }

            template<class F>
            constexpr auto and_then(F&& f) const&& {
                if (*this)
                    return std::invoke(std::forward<F>(f), static_cast<D*>(this)->value());
                else
                    return std::remove_cvref_t<std::invoke_result_t<F, T&>>{};
            }

            template<class F>
            constexpr auto transform(F&& f)& {
                if (*this)
                    return std::invoke(std::forward<F>(f), static_cast<D*>(this)->value());
                else
                    return D{};
            }

            template<class F>
            constexpr auto transform(F&& f) const& {
                if (*this)
                    return std::invoke(std::forward<F>(f), static_cast<D*>(this)->value());
                else
                    return D{};
            }

            template<class F>
            constexpr auto transform(F&& f)&& {
                if (*this)
                    return std::invoke(std::forward<F>(f), std::move(static_cast<D*>(this)->value()));
                else
                    return D{};
            }

            template<class F>
            constexpr auto or_else(F&& f) const& {
                if (*this)
                    return *this;
                else
                    return std::invoke(std::forward<F>(f), static_cast<D*>(this)->value());
            }

            template<class F>
            constexpr auto or_else(F&& f)&& {
                if (*this)
                    return *this;
                else
                    return std::invoke(std::forward<F>(f), std::move(static_cast<D*>(this)->value()));
            }
        };
    }

    template<typename T>
    class optional : public detail::optional_base<optional<T>, T> {
    private:
        using base_type = detail::optional_base<optional<T>, T>;
    public:
        using base_type::base_type;
    };

    template<typename T>
    class optional<T&> : public detail::optional_base<optional<T&>, T*> {
    private:
        using base_type = detail::optional_base<optional<T&>, T*>;
    public:
        using value_type = T&;

        using base_type::base_type;

        template<typename U> requires std::same_as<std::decay_t<U>, T*>
        constexpr optional(U&& p) noexcept {
            if (p) emplace(p);
        }

        constexpr optional(T& value) noexcept : optional(&value) {}

        constexpr const T* operator->() const noexcept { return *base_type::operator->(); }

        constexpr T* operator->() noexcept { return *base_type::operator->(); }

        constexpr const T& operator*()& noexcept { return *base_type::operator*(); }

        constexpr const T& operator*()&& noexcept { return *base_type::operator*(); }

        constexpr const T& operator*() const& noexcept { return *base_type::operator*(); }

        constexpr const T& operator*() const&& noexcept { return *base_type::operator*(); }

        constexpr T& value()& {
            if (!**this) { this->reset(); }
            return *base_type::value();
        }

        constexpr const T& value() const& {
            if (!**this) { this->reset(); }
            return *base_type::value();
        }

        constexpr T& value()&& {
            if (!**this) { this->reset(); }
            return *base_type::value();
        }

        constexpr const T& value() const&& {
            if (!**this) { this->reset(); }
            return *base_type::value();
        }

        constexpr T& value_or(T& default_value) {
            return this->has_value() ? this->value() : default_value;
        }

        constexpr T& value_or(T& default_value) const {
            return this->has_value() ? this->value() : default_value;
        }

        template<typename U> requires std::same_as<std::decay_t<U>, T*>
        T* emplace(U&& p) {
            base_type::emplace(std::forward<U>(p));
            if (!p) this->reset();
            return p;
        }
    };

    template<typename T>
    auto make_optional(T&& value) noexcept(noexcept(utils::optional<T>{value})) {
        return utils::optional<T>{value};
    }

    template<typename T, typename... Args>
    auto make_optional(Args&& ... args) noexcept(noexcept(utils::optional{std::in_place, std::forward<Args>(args)...})) {
        return utils::optional<T>{std::in_place, std::forward<Args>(args)...};
    }

    template<typename T>
    auto make_optional(T* p) noexcept(noexcept(utils::optional<T&>(p))) {
        return utils::optional<T&>(p);
    }
}

#endif //SHIFT_UTILS_OPTIONAL_H_
