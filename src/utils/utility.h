#ifndef SHIFT_UTILS_UTILITY_H_
#define SHIFT_UTILS_UTILITY_H_ 1

#include <cstdint>
#include <cstddef>
#include <concepts>
#include <utility>
#include <functional>

namespace shift::utils {
    constexpr std::size_t hash_combine(const std::size_t first, const std::size_t second) noexcept {
        // Stolen from https://stackoverflow.com/a/2595226
        return second + 0x9e3779b9 + (first << 6) + (first >> 2);
    }

    template<std::convertible_to<std::size_t>... Ts>
    constexpr std::size_t hash_combine(const std::size_t first, const std::size_t second, const Ts... rest) noexcept {
        std::size_t ret = first;
        for (std::size_t d : { second, rest... }) {
            ret = hash_combine(ret, d);
        }
        return ret;
    }
}

template<typename T1, typename T2>
struct std::hash<std::pair<T1, T2>> {
    constexpr std::size_t operator()(const std::pair<T1, T2>& p) const {
        return shift::utils::hash_combine(std::hash<T1>()(p.first), std::hash<T2>()(p.second));
    }
};

#endif //SHIFT_UTILS_UTILITY_H_
