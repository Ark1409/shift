#ifndef SHIFT_UTILS_ENUM_H_
#define SHIFT_UTILS_ENUM_H_ 1

#include <type_traits>

namespace shift::utils {
    template<typename T> requires std::is_enum_v<T>
    constexpr auto to_underlying(T t) noexcept { return static_cast<std::underlying_type_t<T>>(t); }
}

#endif //SHIFT_UTILS_ENUM_H_
