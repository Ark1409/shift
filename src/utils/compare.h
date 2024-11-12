#ifndef SHIFT_UTILS_COMPARE_H_
#define SHIFT_UTILS_COMPARE_H_ 1

#include <compare>

inline std::strong_ordering operator!(const std::strong_ordering order) noexcept {
    if (order == std::strong_ordering::equal) return std::strong_ordering::equal;
    return order == std::strong_ordering::less ? std::strong_ordering::greater : std::strong_ordering::less;
}

#endif //SHIFT_UTILS_COMPARE_H_
