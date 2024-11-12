#ifndef SHIFT_UTILS_VARIANT_H_
#define SHIFT_UTILS_VARIANT_H_ 1

#include <variant>
#include <type_traits>
#include <concepts>

#include "utils/type_traits.h"

namespace shift::utils {
    template<class... Ts>
    struct visit_overloader : Ts ... {
        constexpr explicit visit_overloader(Ts&& ... ts) noexcept((std::is_nothrow_move_constructible_v<Ts> && ...)) :
            Ts(std::move(ts))... {}

        constexpr explicit visit_overloader(const Ts& ... ts) noexcept((std::is_nothrow_copy_constructible_v<Ts> && ...)) : Ts(ts)... {}

        using Ts::operator()...;
    };

    template<typename, typename>
    struct variant_index;

    template<typename T, typename... Types>
    struct variant_index<T, std::variant<Types...>> : type_index<std::is_same, T, Types...> {};

    template<typename T, typename VariantT>
    constexpr std::size_t variant_index_v = variant_index<T, VariantT>::value;

    template<typename, typename>
    struct variant_has_alternative;

    template<typename T, typename... Types>
    struct variant_has_alternative<T, std::variant<Types...>> : has_type<std::is_same, T, Types...> {};

    template<typename T, typename VariantT>
    constexpr bool variant_has_alternative_v = variant_has_alternative<T, VariantT>::value;

    namespace detail {
        template<typename T>
        struct ensure_variant_pointer { typedef T* type; };

        template<typename T>
        struct ensure_variant_pointer<T*> {};

        template<typename T> requires (!std::is_pointer_v<T>)
        struct ensure_variant_pointer<T*> { typedef T* type; };

        template<>
        struct ensure_variant_pointer<std::monostate> { typedef std::monostate type; };

        template<>
        struct ensure_variant_pointer<std::nullptr_t> { typedef std::nullptr_t type; };

        template<typename T>
        using ensure_variant_pointer_t = typename ensure_variant_pointer<T>::type;

        template<typename T>
        concept variant_pointer = std::same_as<ensure_variant_pointer_t<T>, T>;

        template<typename NewVariantT, typename OldVariantT>
        struct castable_variant_helper;

        template<typename... NewVariantArgs, typename... OldVariantArgs>
        struct castable_variant_helper<std::variant<NewVariantArgs...>, std::variant<OldVariantArgs...>> :
            std::bool_constant<(castable_to<remove_pointer_extent_t<OldVariantArgs>, remove_pointer_extent_t<NewVariantArgs>> && ...)> {
        };

        template<typename NewVariantT, typename OldVariantT>
        concept castable_variant = castable_variant_helper<NewVariantT, OldVariantT>::value;

        template<typename T1, typename T2>
        struct ensure_variant_pointer_compat {
            typedef std::enable_if_t<castable_to<remove_pointer_extent_t<T2>, remove_pointer_extent_t<T1>>,
                                     std::conditional_t<std::is_const_v<remove_pointer_extents_t<T2>>,
                                                        add_pointer_const_t<ensure_variant_pointer_t<T1>>,
                                                        ensure_variant_pointer_t<T1>>> type;
        };

        template<typename T1, typename T2>
        using ensure_variant_pointer_compat_t = typename ensure_variant_pointer_compat<T1, T2>::type;

        template<typename NewVariantT, typename OldVariantT>
        struct ensure_all_variant_pointer_compat;

        template<typename... Types, typename... OldTypes>
        struct ensure_all_variant_pointer_compat<std::variant<Types...>, std::variant<OldTypes...>> {
            typedef std::variant<ensure_variant_pointer_compat_t<Types, OldTypes>...> type;
        };

        template<typename NewVariantT, typename OldVariantT>
        using ensure_all_variant_pointer_compat_t = typename ensure_all_variant_pointer_compat<NewVariantT, OldVariantT>::type;

        template<typename T>
        concept variant_cast_all = detail::variant_pointer<std::remove_cvref_t<T>>;

        template<typename T>
        concept variant_cast_pointer = variant_cast_all<T> && std::is_pointer_v<std::remove_cvref_t<T>>;

        template<typename NewVariantT, typename CasterT, typename OldVariantT> requires (detail::castable_variant<NewVariantT,
                                                                                                                  std::remove_cvref_t<
                                                                                                                      OldVariantT>>)
        auto variant_cast(OldVariantT&& v, CasterT&& caster) {
            using variant_type = detail::ensure_all_variant_pointer_compat_t<NewVariantT, std::remove_cvref_t<OldVariantT>>;
            return std::visit(utils::visit_overloader{
                [&](auto& value) {
                    if constexpr (variant_has_alternative_v<std::remove_reference_t<decltype(value)>, std::remove_cvref_t<decltype(v)>>) {
                        return variant_type{
                            caster.template operator()<std::variant_alternative_t<
                                variant_index_v<std::remove_cvref_t<decltype(value)>, std::remove_cvref_t<decltype(v)>>,
                                variant_type>>(&value)
                        };
                    } else {
                        return variant_type{
                            caster.template operator()<std::variant_alternative_t<
                                variant_index_v<std::remove_cvref_t<decltype(value)>, std::remove_cvref_t<decltype(v)>>,
                                variant_type>>(&value)
                        };
                    }
                },
                [&](variant_cast_all auto& value) {
                    return variant_type{ value };
                },
                [&](variant_cast_pointer auto& value) {
                    return variant_type{
                        caster.template operator()<std::variant_alternative_t<
                            variant_index_v<std::remove_cvref_t<decltype(value)>, std::remove_cvref_t<decltype(v)>>,
                            variant_type>>(value) };
                }
            }, std::forward<OldVariantT>(v));
        }
    }

    template<typename NewVariantT, typename... OldTypes> requires (detail::castable_variant<NewVariantT, std::variant<OldTypes...>>)
    auto variant_cast(std::variant<OldTypes ...>& v) {
        return detail::variant_cast<NewVariantT>(v, []<typename NewT>(auto& old_v) { return dynamic_cast<NewT>(old_v); });
    }

    template<typename NewVariantT, typename... OldTypes> requires (detail::castable_variant<NewVariantT, std::variant<OldTypes...>>)
    auto variant_cast(const std::variant<OldTypes ...>& v) {
        return detail::variant_cast<NewVariantT>(v, []<typename NewT>(auto& old_v) { return dynamic_cast<NewT>(old_v); });
    }

    template<typename NewVariantT, typename... OldTypes> requires (detail::castable_variant<NewVariantT, std::variant<OldTypes...>>)
    auto variant_cast(const std::variant<OldTypes ...>&& v) = delete;

    template<typename NewVariantT, typename... OldTypes> requires (detail::castable_variant<NewVariantT, std::variant<OldTypes...>>)
    auto variant_static_cast(std::variant<OldTypes ...>& v) {
        return detail::variant_cast<NewVariantT>(v, []<typename NewT>(auto& old_v) { return static_cast<NewT>(old_v); });
    }

    template<typename NewVariantT, typename... OldTypes> requires (detail::castable_variant<NewVariantT, std::variant<OldTypes...>>)
    auto variant_static_cast(const std::variant<OldTypes ...>& v) {
        return detail::variant_cast<NewVariantT>(v, []<typename NewT>(auto& old_v) { return static_cast<NewT>(old_v); });
    }

    template<typename NewVariantT, typename... OldTypes> requires (detail::castable_variant<NewVariantT, std::variant<OldTypes...>>)
    auto variant_static_cast(const std::variant<OldTypes ...>&& v) = delete;
}


#endif //SHIFT_UTILS_VARIANT_H_
