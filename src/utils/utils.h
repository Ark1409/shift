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

#ifdef SHIFT_DEBUG
#   define debug_log(X) std::cout << X << '\n'
#else
#   define debug_log(X)
#endif

#ifdef SHIFT_DEBUG
#	define SHIFT_FUNCTION_BENCHMARK_BEGIN debug_log("Starting benchmark for function: " << __func__); typename std::chrono::high_resolution_clock::time_point func_begin = std::chrono::high_resolution_clock::now(); size_t func_bytes_alloced = ::bytes_alloced;
#	define SHIFT_FUNCTION_BENCHMARK_END typename std::chrono::high_resolution_clock::time_point func_end = std::chrono::high_resolution_clock::now(); debug_log("Ended benchmark for function: " << __func__ << "; Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(func_end - func_begin) << "ms" << "; Bytes used: " << (::bytes_alloced-func_bytes_alloced) << " (" << ((double) ((double)::bytes_alloced-(double)func_bytes_alloced) / (double)1024)  << " KiB)");
#else
#	define SHIFT_FUNCTION_BENCHMARK_BEGIN
#	define SHIFT_FUNCTION_BENCHMARK_END
#endif

#define is_between_in(val,min,max) (((val)>=(min))&&((val)<=(max))) // inclusive
#define is_between_ex(val,min,max) (((val)>(min))&&((val)<(max))) // exclusive
#define shift_min(val, min) ((val) > (min) ? (min) : (val))
#define shift_max(val, max) ((val) < (max) ? (max) : (val))
#define shift_clamp(val, min, max) shift_min(shift_max(val, min), max)
#define is_whitespace(__str) std::isspace(__str)
#define is_whitespace_ext(__str, _CharT) ((__str)==static_cast<_CharT>('\t')||(__str)==static_cast<_CharT>('\v')||(__str)==static_cast<_CharT>(' ')||(__str)==static_cast<_CharT>('\n')||(__str)==static_cast<_CharT>('\r')||(__str)==static_cast<_CharT>('\f'))

namespace shift {
    namespace utils {
        template<typename CharT>
        constexpr inline size_t strlen(const CharT* const str) noexcept {
            size_t i = 0;
            for (; str[i] != CharT(0x0); ++i);
            return i;
        }

        template<typename T>
        std::vector<T> list_to_vector(const std::list<T>& list) noexcept(std::is_nothrow_copy_constructible_v<T>) {
            std::vector<T> vec;
            vec.reserve(list.size());
            for (T const& val : list) {
                vec.push_back(val);
            }

            return vec;
        }

        template<typename T>
        std::vector<T> list_to_vector(std::list<T>&& list) noexcept(std::is_nothrow_move_constructible_v<T>) {
            std::vector<T> vec;
            vec.reserve(list.size());
            for (T&& val : list) {
                vec.push_back(std::move(val));
            }

            return vec;
        }

        template<typename T>
        std::list<T> vector_to_list(const std::vector<T>& vec) noexcept(std::is_nothrow_copy_constructible_v<T>) {
            std::list<T> list;
            for (T const& val : vec) {
                list.push_back(val);
            }

            return list;
        }

        template<typename T>
        std::list<T> vector_to_list(std::vector<T>&& vec) noexcept(std::is_nothrow_move_constructible_v<T>) {
            std::list<T> list;
            for (T&& val : vec) {
                list.push_back(std::move(val));
            }

            return list;
        }

        template<typename T>
        inline std::vector<T> to_vector(const std::list<T>& list) noexcept(std::is_nothrow_copy_constructible_v<T>) {
            return list_to_vector(list);
        }

        template<typename T>
        inline std::vector<T> to_vector(std::list<T>&& list) noexcept(std::is_nothrow_move_constructible_v<T>) {
            return list_to_vector(std::move(list));
        }

        template<typename T>
        inline std::list<T> to_list(const std::vector<T>& vec) noexcept(std::is_nothrow_copy_constructible_v<T>) {
            return vector_to_list(vec);
        }

        template<typename T>
        inline std::list<T> to_list(std::vector<T>&& vec) noexcept(std::is_nothrow_move_constructible_v<T>) {
            return vector_to_list(std::move(vec));
        }

        template<typename T>
        inline std::stack<T>& clear_stack(std::stack<T>& stack) {
            std::stack<T>().swap(stack);
            return stack;
        }

        template<typename T>
        inline std::stack<T>& pop_stack(std::stack<T>& stack, typename std::stack<T>::size_type count = typename std::stack<T>::size_type(1)) {
            if (std::min(count, stack.size()) == stack.size()) return clear_stack(stack);

            for (; count > 0 && !stack.empty(); count--)
                stack.pop();
            return stack;
        }

        /// @brief Creates a list from the elements contained within another list in the range [index, index+length)
        /// @tparam T The list type
        /// @param list The list to search from.
        /// @param index The index to begin searching from.
        /// @param length the length of the search.
        /// @return A list containing the selected elements.
        template<typename T>
        std::list<T> sub_list(const std::list<T>& list, const typename std::list<T>::size_type index, const typename std::list<T>::size_type length) {
            using namespace std::string_literals;
            if (index < 0 || index >= list.size())
                throw std::out_of_range(std::to_string(index) + " is not in range [0," + std::to_string(list.size()) + ")");

            if (length < 0 || (index + length) >= list.size()) {
                throw std::out_of_range(std::to_string(index + length) + " is not in range [0, " + std::to_string(list.size()) + ")");
            }

            std::list<T> sub;
            typename std::list<T>::size_type i = 0;
            for (typename std::list<T>::const_iterator it = list.cbegin(); it != list.cend() && i < (index + length); it++, i++) {
                if (i >= index) {
                    sub.push_back(*it);
                }
            }

            return sub;
        }

        template<typename T>
        std::list<T> sub_list(std::list<T>&& list, const typename std::list<T>::size_type index, const typename std::list<T>::size_type length) {
            using namespace std::string_literals;
            if (index < 0 || index >= list.size())
                throw std::out_of_range(std::to_string(index) + " is not in range [0," + std::to_string(list.size()) + ")");

            if (length < 0 || (index + length) >= list.size()) {
                throw std::out_of_range(std::to_string(index + length) + " is not in range [0, " + std::to_string(list.size()) + ")");
            }

            std::list<T> sub;
            typename std::list<T>::size_type i = 0;
            for (typename std::list<T>::iterator it = list.begin(); it != list.end() && i < (index + length); it++, i++) {
                if (i >= index) {
                    sub.push_back(std::move(*it));
                }
            }

            return sub;
        }
        template<typename T>
        std::vector<T> sub_vector(const std::vector<T>& vec, const typename std::vector<T>::size_type index, const typename std::vector<T>::size_type length) {
            using namespace std::string_literals;
            if (index < 0 || index >= vec.size())
                throw std::out_of_range(std::to_string(index) + " is not in range [0," + std::to_string(vec.size()) + ")");

            if (length < 0 || (index + length) >= vec.size()) {
                throw std::out_of_range(std::to_string(index + length) + " is not in range [0, " + std::to_string(vec.size()) + ")");
            }

            return std::vector(vec.cbegin() + index, vec.cbegin() + index + length);
        }

        template<typename T>
        std::vector<T> sub_vector(std::vector<T>&& vec, const typename std::vector<T>::size_type index, const typename std::vector<T>::size_type length) {
            using namespace std::string_literals;
            if (index < 0 || index >= vec.size())
                throw std::out_of_range(std::to_string(index) + " is not in range [0," + std::to_string(vec.size()) + ")");

            if (length < 0 || (index + length) >= vec.size()) {
                throw std::out_of_range(std::to_string(index + length) + " is not in range [0, " + std::to_string(vec.size()) + ")");
            }

            return std::vector(vec.begin() + index, vec.begin() + index + length);
        }

        template<typename T>
        std::remove_reference_t<T> remove(std::list<T>& list, typename std::list<T>::size_type index) {
            if (index < 0 || index >= list.size())
                throw std::out_of_range(std::to_string(index) + " is not in range [0," + std::to_string(list.size()) + ")");

            typename std::list<T>::iterator it = list.begin();
            it = it + index;
            std::remove_reference_t<T> ret = std::move(*it);
            list.erase(it);
            return ret;
        }

        template<typename CharT, typename Traits, typename Alloc>
        void replace_all(std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string<CharT, Traits, Alloc>& find,
            const std::basic_string<CharT, Traits, Alloc>& replace) {
            size_t start_pos = 0;
            while ((start_pos = str.find(find, start_pos)) != std::basic_string<CharT, Traits, Alloc>::npos) {
                str.replace(start_pos, find.length(), replace);
                start_pos += replace.length(); // Handles case where 'to' is a substring of 'from'
            }
        }

        template<typename CharT, typename Traits, typename Alloc>
        inline std::basic_string<CharT, Traits, Alloc> replace_all(const std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string<CharT, Traits, Alloc>& find,
            const std::basic_string<CharT, Traits, Alloc>& replace) {
            std::basic_string<CharT, Traits, Alloc> ret = str;
            replace_all(ret, find, replace);
            return ret;
        }

        template<typename CharT, typename Traits, typename Alloc>
        void replace_all(std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string_view<CharT, Traits> find,
            const std::basic_string<CharT, Traits, Alloc>& replace) {
            size_t start_pos = 0;
            while ((start_pos = str.find(find, start_pos)) != std::basic_string<CharT, Traits, Alloc>::npos) {
                str.replace(start_pos, find.length(), replace);
                start_pos += replace.length(); // Handles case where 'to' is a substring of 'from'
            }
        }

        template<typename CharT, typename Traits, typename Alloc>
        inline std::basic_string<CharT, Traits, Alloc> replace_all(const std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string_view<CharT, Traits> find,
            const std::basic_string<CharT, Traits, Alloc>& replace) {
            std::basic_string<CharT, Traits, Alloc> ret = str;
            replace_all(ret, find, replace);
            return ret;
        }

        template<typename CharT, typename Traits, typename Alloc>
        void replace_all(std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string<CharT, Traits, Alloc>& find,
            const std::basic_string_view<CharT, Traits> replace) {
            size_t start_pos = 0;
            while ((start_pos = str.find(find, start_pos)) != std::basic_string<CharT, Traits, Alloc>::npos) {
                str.replace(start_pos, find.length(), replace);
                start_pos += replace.length(); // Handles case where 'to' is a substring of 'from'
            }
        }

        template<typename CharT, typename Traits, typename Alloc>
        inline std::basic_string<CharT, Traits, Alloc> replace_all(const std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string<CharT, Traits, Alloc>& find,
            const std::basic_string_view<CharT, Traits> replace) {
            std::basic_string<CharT, Traits, Alloc> ret = str;
            replace_all(ret, find, replace);
            return ret;
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
        inline std::basic_string<CharT, Traits, Alloc> replace_all(const std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string_view<CharT, Traits> find,
            const std::basic_string_view<CharT, Traits> replace) {
            std::basic_string<CharT, Traits, Alloc> ret = str;
            replace_all(ret, find, replace);
            return ret;
        }

        template<typename CharT, typename Traits>
        typename std::basic_string_view<CharT, Traits>::size_type count(const std::basic_string_view<CharT, Traits> str, const std::basic_string_view<CharT, Traits> delim) noexcept {
            typedef typename std::basic_string_view<CharT, Traits>::size_type size_type;

            size_type count = 0;

            const size_type len = str.length();
            const size_type delim_len = delim.length();

            if (delim_len > 0) {
                for (size_type i = 0; i < len && (i + delim_len - 1) < len; i++) {
                    if (Traits::compare(str.data(), delim.data(), delim_len) == 0) {
                        count++;
                        i += delim_len - 1;
                    }
                }
            }

            return count;
        }

        template<typename CharT, typename Traits, typename Alloc>
        inline typename std::basic_string_view<CharT, Traits>::size_type count(const std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string_view<CharT, Traits> delim) {
            return count<CharT, Traits>(std::basic_string_view<CharT, Traits>(str.data(), str.length()), delim);
        }

        template<typename CharT, typename Traits, typename Alloc>
        inline typename std::basic_string_view<CharT, Traits>::size_type count(const std::basic_string_view<CharT, Traits> str, const std::basic_string<CharT, Traits, Alloc>& delim) {
            return count<CharT, Traits>(str, std::basic_string_view<CharT, Traits>(delim.data(), delim.length()));
        }

        template<typename CharT, typename Traits, typename Alloc>
        inline typename std::basic_string_view<CharT, Traits>::size_type count(const std::basic_string<CharT, Traits>& str, const std::basic_string<CharT, Traits, Alloc>& delim) {
            return count<CharT, Traits>(std::basic_string_view<CharT, Traits>(str.data(), str.length()), std::basic_string_view<CharT, Traits>(delim.data(), delim.length()));
        }


        template<typename CharT, typename Traits>
        std::vector<std::basic_string_view<CharT, Traits>> split(const std::basic_string_view<CharT, Traits> str,
            const std::basic_string_view<CharT, Traits> delim) {
            typedef std::basic_string_view<CharT, Traits> string_view_type;
            typedef std::vector<string_view_type> vector_type;
            typedef typename std::vector<string_view_type>::size_type size_type;

            vector_type tokens;

            const size_type len = str.length();
            const size_type delim_len = delim.length();

            tokens.reserve(count(str, delim) + 1);

            {
                size_type i = 0, last_pos = 0;
                if (delim_len > 0) {
                    for (; i < len && (i + delim_len - 1) < len; i++) {
                        if (Traits::compare(str.data(), delim.data(), delim_len) == 0) {
                            i += delim_len - 1;
                            tokens.push_back(string_view_type(&str[last_pos], i - last_pos));
                            last_pos = i + 1;
                        }
                    }
                }

                if (i < len) {
                    tokens.push_back(string_view_type(&str[last_pos], len - last_pos));
                }
            }

            return tokens;
        }

        template<typename CharT, typename Traits, typename Alloc>
        inline std::vector<std::basic_string_view<CharT, Traits>> split(const std::basic_string<CharT, Traits, Alloc>& str,
            const std::basic_string<CharT, Traits, Alloc>& delim) {
            return split<CharT, Traits>(std::basic_string_view<CharT, Traits>(str.data(), str.length()), std::basic_string_view<CharT, Traits>(delim.data(), delim.length()));
        }

        template<typename CharT, typename Traits, typename Alloc>
        inline std::vector<std::basic_string_view<CharT, Traits>> split(const std::basic_string<CharT, Traits, Alloc>& str,
            const std::basic_string_view<CharT, Traits> delim) {
            return split<CharT, Traits>(std::basic_string_view<CharT, Traits>(str.data(), str.length()), delim);
        }

        template<typename CharT, typename Traits, typename Alloc>
        inline std::vector<std::basic_string_view<CharT, Traits>> split(const std::basic_string_view<CharT, Traits> str,
            const std::basic_string<CharT, Traits, Alloc>& delim) {
            return split<CharT, Traits>(str, std::basic_string_view<CharT, Traits>(delim.data(), delim.length()));
        }

        template<typename CharT, typename Traits>
        constexpr inline bool starts_with(const std::basic_string_view<CharT, Traits> str, const std::basic_string_view<CharT, Traits> start) {
            return str.length() >= start.length() && Traits::compare(str.data(), start.data(), start.length()) == 0;
        }

        template<typename CharT, typename Traits, typename Alloc>
        constexpr inline bool starts_with(const std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string_view<CharT, Traits> start) {
            return starts_with(std::basic_string_view<CharT, Traits>(str.data(), str.length()), start);
        }

        template<typename CharT, typename Traits, typename Alloc>
        constexpr inline bool starts_with(const std::basic_string_view<CharT, Traits> str, const std::basic_string<CharT, Traits, Alloc>& start) {
            return starts_with(str, std::basic_string_view<CharT, Traits>(start.data(), start.length()));
        }

        template<typename CharT, typename Traits, typename Alloc>
        constexpr inline bool starts_with(const std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string<CharT, Traits, Alloc>& start) {
            return starts_with(std::basic_string_view<CharT, Traits>(str.data(), str.length()), std::basic_string_view<CharT, Traits>(start.data(), start.length()));
        }

        template<typename CharT, typename Traits>
        constexpr inline bool ends_with(const std::basic_string_view<CharT, Traits> str, const std::basic_string_view<CharT, Traits> end) {
            return str.length() >= end.length() && Traits::compare(str.data() + str.length() - end.length(), end.data(), end.length()) == 0;
        }

        template<typename CharT, typename Traits, typename Alloc>
        constexpr inline bool ends_with(const std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string_view<CharT, Traits> end) {
            return ends_with(std::basic_string_view<CharT, Traits>(str.data(), str.length()), end);
        }

        template<typename CharT, typename Traits, typename Alloc>
        constexpr inline bool ends_with(const std::basic_string_view<CharT, Traits> str, const std::basic_string<CharT, Traits, Alloc>& end) {
            return ends_with(str, std::basic_string_view<CharT, Traits>(end.data(), end.length()));
        }

        template<typename CharT, typename Traits, typename Alloc>
        constexpr inline bool ends_with(const std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string<CharT, Traits, Alloc>& end) {
            return ends_with(std::basic_string_view<CharT, Traits>(str.data(), str.length()), std::basic_string_view<CharT, Traits>(end.data(), end.length()));
        }

        [[noreturn]] void exit(int status = EXIT_FAILURE) noexcept;
    }
}

template<typename CharT, typename Traits, typename _Rep, typename _Period>
inline std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& __os, const std::chrono::duration<_Rep, _Period> dur) {
    return __os << dur.count();
}

template<typename CharT, typename Traits, typename T>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& __os, const std::vector<T>& __vector) {
    typedef typename std::vector<T>::size_type size_type;
    __os << CharT('[');

    for (size_type i = 0; i < __vector.size(); i++) {
        if (i > 0)
            __os << CharT(',') << CharT(' ');
        __os << __vector[i];
    }
    __os << CharT(']');
    return __os;
}

template<typename CharT, typename Traits, typename T>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& __os, const std::list<T>& __list) {
    typedef typename std::list<T>::size_type size_type;
    __os << CharT('[');
    size_type i = 0;
    for (T const& __v : __list) {
        if (i > 0)
            __os << CharT(',') << CharT(' ');
        __os << __v;
        i++;
    }
    __os << CharT(']');
    return __os;
}

template<typename T>
inline typename std::list<T>::iterator operator+(typename std::list<T>::iterator it, typename std::list<T>::size_type const count) noexcept {
    for (typename std::list<T>::size_type i = 0; i < count; i++, ++it);
    return it;
}

template<typename T>
inline typename std::list<T>::const_iterator operator+(typename std::list<T>::const_iterator it, typename std::list<T>::size_type const count) noexcept {
    for (typename std::list<T>::size_type i = 0; i < count; i++, ++it);
    return it;
}

template<typename T>
inline typename std::list<T>::iterator operator-(typename std::list<T>::iterator it, typename std::list<T>::size_type count) noexcept {
    for (; count > 0; --count, --it);
    return it;
}

template<typename T>
inline typename std::list<T>::const_iterator operator-(typename std::list<T>::const_iterator it, typename std::list<T>::size_type count) noexcept {
    for (; count > 0; --count, --it);
    return it;
}

#endif /* SHIFT_UTILS_H_ */
