/**
 * @file utils/utils.h
 */

#ifndef SHIFT_UTILS_H_
#define SHIFT_UTILS_H_ 1

#include "shift_config.h"
#include "utils/type_traits.h"
#include "utils/compare.h"
#include "utils/containers.h"
#include "utils/variant.h"
#include "utils/utility.h"
#include "utils/strings.h"
#include "utils/range.h"
#include "utils/enum.h"
#include "utils/optional.h"

#include <chrono>
#include <concepts>
#include <ranges>
#include <functional>
#include <deque>
#include <algorithm>
#include <numeric>

#ifdef SHIFT_DEBUG

#   include <iostream>

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
    [[noreturn]] SHIFT_API void exit(int status = EXIT_FAILURE) noexcept;

    template<typename... Args>
    using predicate = std::function<bool(Args...)>;

// Type definition of (incomplete) "deque", i.e a deque that supports incomplete types.
// MSVC does not support incomplete types in std::deque as it is not required by the standard as of C++20
    template<typename T>
#if defined(__GNUC__) || defined(__clang__)
    using ideque = std::deque<T>;
#else
    using ideque = std::list<T>;
#endif
}

#ifdef SHIFT_DEBUG

template<typename CharT, typename Traits, typename Rep, typename Period>
inline std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& _os,
    const std::chrono::duration<Rep, Period>& dur) {
    return _os << dur.count();
}


#endif

#endif /* SHIFT_UTILS_H_ */
