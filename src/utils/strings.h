#ifndef SHIFT_UTILS_STRINGS_H_
#define SHIFT_UTILS_STRINGS_H_ 1

#include <vector>
#include <string>
#include <string_view>
#include <type_traits>
#include <stdexcept>
#include <limits>
#include <cstring>

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
    [[nodiscard]] inline std::basic_string<CharT, Traits, Alloc> repeat(const CharT* str, const std::size_t count) {
        return repeat<CharT, Traits, Alloc>(std::basic_string_view<CharT, Traits>(str), count);
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
}

#endif //SHIFT_UTILS_STRINGS_H_
