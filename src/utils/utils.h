/**
 * @file utils/utils.h
 */

#ifndef SHIFT_UTILS_H_
#define SHIFT_UTILS_H_ 1

#include "shift_config.h"

#include <iostream>
#include <memory>
#include <chrono>
#include <list>
#include <vector>
#include <stack>
#include <utility>
#include <string>
#include <string_view>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <cstring>
#include <type_traits>
#include <concepts>
#include <ranges>
#include <numeric>
#include <array>

#ifdef SHIFT_DEBUG
#   define debug_log(X) std::cout << X << '\n'
#else
#   define debug_log(X)
#endif

// Naive "benchmark" implementation
#ifdef SHIFT_DEBUG
#	define SHIFT_FUNCTION_BENCHMARK_BEGIN debug_log("Starting benchmark for function: " << __func__); auto func_begin = std::chrono::high_resolution_clock::now(); //size_t func_bytes_alloced = ::bytes_alloced;
#	define SHIFT_FUNCTION_BENCHMARK_END auto func_end = std::chrono::high_resolution_clock::now(); debug_log("Ended benchmark for function: " << __func__ << "; Time: " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(func_end - func_begin) << "ms"); //<< "; Bytes used: " << (::bytes_alloced-func_bytes_alloced) << " (" << ((double) ((double)::bytes_alloced-(double)func_bytes_alloced) / (double)1024)  << " KiB)");
#else
#	define SHIFT_FUNCTION_BENCHMARK_BEGIN
#	define SHIFT_FUNCTION_BENCHMARK_END
#endif

#define is_between_in(val, min, max) (((val)>=(min))&&((val)<=(max))) // inclusive
#define is_between_ex(val, min, max) (((val)>(min))&&((val)<(max))) // exclusive
#define shift_min(val, min) ((val) > (min) ? (min) : (val))
#define shift_max(val, max) ((val) < (max) ? (max) : (val))
#define shift_clamp(val, min, max) shift_min(shift_max(val, min), max)
#define is_whitespace(__str) std::isspace(__str)
#define is_whitespace_ext(__str, _CharT) ((__str)==static_cast<_CharT>('\t')||(__str)==static_cast<_CharT>('\v')||(__str)==static_cast<_CharT>(' ')||(__str)==static_cast<_CharT>('\n')||(__str)==static_cast<_CharT>('\r')||(__str)==static_cast<_CharT>('\f'))

namespace shift::utils {
    template<typename NumT>
    NumT str_to_num(const std::string& str) {
        NumT ret;
        if constexpr (std::is_floating_point_v<NumT>) {
            const long double temp = std::stold(str);
            if (temp > std::numeric_limits<NumT>::max() || temp < std::numeric_limits<NumT>::lowest())
                throw std::runtime_error("shift::utils::str_to_num");
            ret = static_cast<NumT>(temp);
        } else {
            if constexpr (!std::is_signed_v<NumT>) {
                const unsigned long long temp = std::stoull(str);
                if (temp > std::numeric_limits<NumT>::max()) { throw std::runtime_error("shift::utils::str_to_num"); }
                ret = static_cast<NumT>(temp);
            } else {
                const signed long long temp = std::stoll(str);
                if (temp > std::numeric_limits<NumT>::max() || temp < std::numeric_limits<NumT>::lowest()) {
                    throw std::runtime_error("shift::utils::str_to_num");
                }
                ret = static_cast<NumT>(temp);
            }
        }
        return ret;
    }

    template<typename NumT>
    inline NumT str_to_num(const std::string_view str) { return str_to_num<NumT>(std::string(str)); }

    template<typename NumT>
    inline NumT str_to_num(const char* const str) { return str_to_num<NumT>(std::string(str)); }

    template<typename CharT>
    constexpr size_t strlen(const CharT* const str) noexcept {
        size_t i = 0;
        for (; str[i] != CharT(0x0); ++i);
        return i;
    }

    template<>
    inline size_t strlen(const char* const str) noexcept { return std::strlen(str); }

    template<typename T, typename Seq>
    inline std::stack<T, Seq>& clear_stack(std::stack<T, Seq>& stack) {
        std::stack<T, Seq>().swap(stack);
        return stack;
    }

    template<typename T, typename Seq>
    inline std::stack<T, Seq>&
    pop_stack(std::stack<T, Seq>& stack, typename std::stack<T, Seq>::size_type count = typename std::stack<T, Seq>::size_type(1)) {
        if (count >= stack.size()) { return clear_stack(stack); }

        for (; count > 0; count--) { stack.pop(); }

        return stack;
    }

    template<typename T>
    T remove(std::list<T>& list, typename std::list<T>::size_type index) {
        if (index < 0 || index >= list.size())
            throw std::out_of_range(std::to_string(index) + " is not in range [0," + std::to_string(list.size()) + ")");

        typename std::list<T>::iterator it = list.begin();
        std::advance(it, index);
        T ret = std::move(*it);
        list.erase(it);
        return ret;
    }

    template<typename CharT, typename Traits, typename Alloc = std::allocator<CharT>>
    [[nodiscard]] std::basic_string<CharT, Traits, Alloc> repeat(const std::basic_string_view<CharT, Traits> str, const std::size_t count) {
        typename std::basic_string_view<CharT, Traits>::size_type const size = str.size();
        std::basic_string<CharT, Traits, Alloc> ret(size * count, CharT(0x0));
        for (std::size_t i = 0; i < count; i++) {
            Traits::copy(ret.data() + (i * size), str.data(), size);
        }
        return ret;
    }

    template<typename CharT, typename Traits, typename Alloc>
    [[nodiscard]] inline std::basic_string<CharT, Traits, Alloc>
    repeat(const std::basic_string<CharT, Traits, Alloc>& str, const std::size_t count) {
        return repeat<CharT, Traits, Alloc>(std::basic_string_view<CharT, Traits>(str.data(), str.length()), count);
    }

    template<typename CharT, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
    [[nodiscard]] inline std::basic_string<CharT, Traits, Alloc> repeat(const CharT ch, const std::size_t count) {
        return std::basic_string<CharT, Traits, Alloc>(count, ch);
    }

    template<typename CharT, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
    [[nodiscard]] inline std::basic_string<CharT, Traits, Alloc> repeat(const CharT* ch, const std::size_t count) {
        return repeat<CharT, Traits, Alloc>(std::basic_string_view<CharT, Traits>(ch), count);
    }

    template<typename CharT, typename Traits, typename Alloc>
    void replace_all(std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string_view<CharT, Traits> find,
        const std::basic_string_view<CharT, Traits> replace) {
        size_t start_pos = 0;
        while ((start_pos = str.find(find, start_pos)) != std::basic_string<CharT, Traits, Alloc>::npos) {
            str.replace(start_pos, find.length(), replace);
            start_pos += replace.length(); // Handles case where 'to' is a substring of 'from'
        }
    }

    template<typename CharT, typename Traits, typename Alloc>
    [[nodiscard]] inline std::basic_string<CharT, Traits, Alloc>
    replace_all(const std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string_view<CharT, Traits> find,
        const std::basic_string_view<CharT, Traits> replace) {
        std::basic_string<CharT, Traits, Alloc> ret = str;
        replace_all(ret, find, replace);
        return ret;
    }

    template<typename CharT, typename Traits>
    typename std::basic_string_view<CharT, Traits>::size_type
    count(const std::basic_string_view<CharT, Traits> str, const std::basic_string_view<CharT, Traits> delim) noexcept {
        typedef typename std::basic_string_view<CharT, Traits>::size_type size_type;

        size_type count = 0;

        const size_type delim_len = delim.length();

        if (delim_len > 0) {
            for (size_type begin = str.find(delim);
                 begin != std::basic_string_view<CharT, Traits>::npos; begin = str.find(delim, begin += delim_len)) {
                count++;
            }
        }

        return count;
    }

    template<typename CharT, typename Traits>
    std::vector<std::basic_string_view<CharT, Traits>> split(const std::basic_string_view<CharT, Traits> str,
        const std::basic_string_view<CharT, Traits> delim) {
        typedef std::basic_string_view<CharT, Traits> string_view_type;
        typedef std::vector<string_view_type> vector_type;
        typedef typename string_view_type::size_type size_type;

        vector_type tokens;

        tokens.reserve(count(str, delim) + 1);

        const size_type delim_len = delim.length();

        size_type last = 0;
        if (delim_len > 0) {
            for (size_type begin = str.find(delim);
                 begin != std::basic_string_view<CharT, Traits>::npos; begin += delim_len, last = begin, begin = str.find(delim, begin)) {
                tokens.emplace_back(&str[last], begin - last);
            }
        }
        tokens.emplace_back(&str[last], str.length() - last);

        return tokens;
    }

    template<typename CharT, typename Traits, typename Alloc>
    inline std::vector<std::basic_string_view<CharT, Traits>> split(const std::basic_string<CharT, Traits, Alloc>& str,
        const std::basic_string_view<CharT, Traits>& delim) {
        return split<CharT, Traits>(std::basic_string_view<CharT, Traits>(str.data(), str.length()), delim);
    }

    template<typename CharT, typename Traits>
    constexpr bool
    starts_with(const std::basic_string_view<CharT, Traits> str, const std::basic_string_view<CharT, Traits> start) {
        return str.length() >= start.length() && Traits::compare(str.data(), start.data(), start.length()) == 0;
    }

    template<typename CharT, typename Traits>
    constexpr bool
    ends_with(const std::basic_string_view<CharT, Traits> str, const std::basic_string_view<CharT, Traits> end) {
        return str.length() >= end.length() &&
               Traits::compare(str.data() + str.length() - end.length(), end.data(), end.length()) == 0;
    }

    template<typename CharT, typename Traits>
    constexpr bool contains(const std::basic_string_view<CharT, Traits> str, const CharT c) {
        return str.find(c) != std::basic_string_view<CharT, Traits>::npos;
    }

    template<typename CharT, typename Traits>
    constexpr bool contains(const std::basic_string_view<CharT, Traits> str, const std::basic_string_view<CharT, Traits> find) {
        return str.find(find) != std::basic_string_view<CharT, Traits>::npos;
    }

    [[noreturn]] SHIFT_API void exit(int status = EXIT_FAILURE) noexcept;

    constexpr std::size_t hash_combine(const std::size_t first, const std::size_t second) noexcept {
        // Stolen from https://stackoverflow.com/a/2595226
        return second + 0x9e3779b9 + (first << 6) + (first >> 2);
    }

    template<std::convertible_to<std::size_t>... Ts>
    constexpr std::size_t hash_combine(const std::size_t first, const std::size_t second, const Ts... rest) noexcept {
        std::size_t ret = first;
        for (size_t d : { second, rest... }) {
            ret = hash_combine(ret, d);
        }
        return ret;
    }

    template<class... Ts>
    struct visit_overloader : Ts ... {
        constexpr explicit visit_overloader(Ts&& ... ts) noexcept((std::is_nothrow_move_constructible_v<Ts> && ...)) :
            Ts(std::move(ts))... {}

        constexpr explicit visit_overloader(const Ts& ... ts) noexcept((std::is_nothrow_copy_constructible_v<Ts> && ...)) : Ts(ts)... {}

        using Ts::operator()...;
    };

    template<typename T, std::ranges::input_range R, std::invocable<T, std::ranges::range_reference_t<R>> BinaryOp>
    T accumulate(R&& r, T init, BinaryOp op) {
        return std::accumulate(std::ranges::begin(r), std::ranges::end(r), init, op);
    }

    template<typename... Args>
    using predicate = std::function<bool(Args...)>;

// Type definition of (incomplete) "deque", i.e a deque that supports incomplete types.
// MSVC does not support incomplete types in std::deque as it is not required by the standard as of C++20
#if defined(_MSC_VER) || defined(_MSC_FULL_VER)
    template<typename T>
    using ideque = std::list<T>;
#else
    template<typename T>
    using ideque = std::deque<T>;
#endif
}

inline std::strong_ordering operator!(const std::strong_ordering order) noexcept {
    if (order == std::strong_ordering::equal) return std::strong_ordering::equal;
    return order == std::strong_ordering::less ? std::strong_ordering::greater : std::strong_ordering::less;
}

template<typename CharT, typename Traits, typename Rep, typename Period>
inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& _os, const std::chrono::duration<Rep, Period>& dur) {
    return _os << dur.count();
}

template<typename CharT, typename Traits, typename T>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& _os, const std::vector<T>& _vector) {
    typedef typename std::vector<T>::size_type size_type;
    _os << CharT('[');

    for (size_type i = 0; i < _vector.size(); i++) {
        if (i > 0) { _os << CharT(',') << CharT(' '); }
        _os << _vector[i];
    }
    _os << CharT(']');
    return _os;
}

template<typename CharT, typename Traits, typename T>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& _os, const std::list<T>& _list) {
    _os << CharT('[');
    for (bool past_first = false; T const& _v : _list) {
        if (past_first) { _os << CharT(',') << CharT(' '); }
        _os << _v;
        past_first = true;
    }
    _os << CharT(']');
    return _os;
}

template<typename T1, typename T2>
struct std::hash<std::pair<T1, T2>> {
    constexpr std::size_t operator()(const std::pair<T1, T2>& p) const {
        return shift::utils::hash_combine(std::hash<T1>()(p.first), std::hash<T2>()(p.second));
    }
};

#endif /* SHIFT_UTILS_H_ */
