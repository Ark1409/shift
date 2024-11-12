#ifndef SHIFT_UTILS_POINTERS_H_
#define SHIFT_UTILS_POINTERS_H_ 1

#include <type_traits>

namespace shift::utils {
    namespace detail {
        template<template<typename, typename> typename, std::size_t, typename...>
        struct type_index_impl;

        template<template<typename, typename> typename Comparer, std::size_t I, typename T>
        struct type_index_impl<Comparer, I, T> {};

        template<template<typename, typename> typename Comparer, std::size_t I, typename T, typename T2, typename... Types>
        struct type_index_impl<Comparer, I, T, T2, Types...> : std::conditional_t<Comparer<T, T2>::value,
                                                                                  std::integral_constant<std::size_t, I>,
                                                                                  type_index_impl<Comparer, I + 1, T, Types...>> {
        };

        template<template<typename, typename> typename, typename...>
        struct has_type_impl;

        template<template<typename, typename> typename Comparer, typename T>
        struct has_type_impl<Comparer, T> : std::false_type {};

        template<template<typename, typename> typename Comparer, typename T, typename... Types>
        struct has_type_impl<Comparer, T, Types...> : std::bool_constant<(Comparer<T, Types>::value || ...)> {
        };
    }

    template<template<typename, typename> typename Comparer, typename T, typename... Types>
    struct type_index : detail::type_index_impl<Comparer, 0, T, Types...> {};

    template<template<typename, typename> typename Comparer, typename T, typename... Types>
    struct has_type : detail::has_type_impl<Comparer, T, Types...> {};

    template<template<typename, typename> typename Comparer, typename T, typename... Types>
    constexpr bool has_type_v = has_type<Comparer, T, Types...>::value;

    template<typename T>
    struct pointer_rank : std::integral_constant<std::size_t, 0> {};

    template<typename T>
    struct pointer_rank<T*> : std::integral_constant<std::size_t, pointer_rank<T>::value + 1> {};

    template<typename T>
    struct pointer_rank<T&> : pointer_rank<T> {};

    template<typename T>
    struct pointer_rank<T&&> : pointer_rank<T> {};

    template<typename T>
    constexpr std::size_t pointer_rank_v = pointer_rank<T>::value;

    template<typename T, std::size_t>
    struct remove_pointer_extents { typedef T type; };

    template<typename T>
    struct remove_pointer_extents<T, 0> { typedef T type; };

    template<typename T, std::size_t N>
    struct remove_pointer_extents<T*, N> : remove_pointer_extents<T, N - 1> {};

    template<typename T, std::size_t N = pointer_rank_v<T>>
    using remove_pointer_extents_t = typename remove_pointer_extents<T, N>::type;

    template<typename T>
    using remove_pointer_extent_t = remove_pointer_extents_t<T, 1>;

    template<std::size_t, typename T> requires (not std::is_pointer_v<T>)
    T& dereference(T& val) noexcept { return val; }

    template<typename T> requires (not std::is_pointer_v<T>)
    T& dereference(T& val) noexcept { return val; }

    template<std::size_t N, typename T>
    auto dereference(T* p) {
        if constexpr (N == 0) { return p; }
        else { return dereference<N - 1>(*p); }
    }

    template<typename T>
    auto dereference(T* p) { return dereference<pointer_rank_v<T>>(p); }

    template<typename T>
    struct remove_reference_const { typedef T type; };

    template<typename T>
    struct remove_reference_const<const T> : remove_reference_const<T> {};

    template<typename T>
    struct remove_reference_const<T&> { typedef typename remove_reference_const<T>::type& type; };

    template<typename T>
    struct remove_reference_const<T&&> { typedef typename remove_reference_const<T>::type&& type; };

    template<typename T>
    using remove_reference_const_t = typename remove_reference_const<T>::type;

    template<typename T>
    struct add_pointer_const { typedef T type; };

    template<typename T>
    struct add_pointer_const<T*> { typedef const T* type; };

    template<typename T>
    struct add_pointer_const<T* const> { typedef const T* const type; };

    template<typename T>
    struct add_pointer_const<T* volatile> { typedef const T* volatile type; };

    template<typename T>
    struct add_pointer_const<T* const volatile> { typedef const T* const volatile type; };

    template<typename T>
    using add_pointer_const_t = typename add_pointer_const<T>::type;

    template<typename From, typename To>
    concept castable_to = requires(From* p) {
        static_cast<To*>(p);
    };

    template<typename From, typename To>
    concept dynamic_castable_to = requires(From* p) {
        dynamic_cast<To*>(p);
    };
}

#endif //SHIFT_UTILS_POINTERS_H_ 1
