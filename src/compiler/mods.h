#ifndef SHIFT_MODS_H_
#define SHIFT_MODS_H_ 1

#include <string_view>

namespace shift::compiler {
    enum shift_mods : uint_fast16_t {
        NONE = 0x0,
        PUBLIC = 0x1,
        PROTECTED = 0x2,
        PRIVATE = 0x4,
        STATIC = 0x8,
        IMUT = 0x10,
        CONST_ = 0x20 | IMUT,
        BINARY = 0x40,
        EXTERN = 0x80,
        EXPLICIT = 0x100
    };

    constexpr shift::compiler::shift_mods operator|(const shift::compiler::shift_mods f, const shift::compiler::shift_mods other) noexcept {
        return shift::compiler::shift_mods(
            std::underlying_type_t<shift::compiler::shift_mods>(f) |
            std::underlying_type_t<shift::compiler::shift_mods>(other));
    }

    constexpr shift::compiler::shift_mods& operator|=(shift::compiler::shift_mods& f, const shift::compiler::shift_mods other) noexcept {
        return f = operator|(f, other);
    }

    constexpr shift::compiler::shift_mods operator&(const shift::compiler::shift_mods f, const shift::compiler::shift_mods other) noexcept {
        return shift::compiler::shift_mods(
            std::underlying_type_t<shift::compiler::shift_mods>(f) &
            std::underlying_type_t<shift::compiler::shift_mods>(other));
    }

    constexpr shift::compiler::shift_mods& operator&=(shift::compiler::shift_mods& f, const shift::compiler::shift_mods other) noexcept {
        return f = operator&(f, other);
    }

    constexpr shift::compiler::shift_mods operator~(const shift::compiler::shift_mods f) noexcept {
        return shift::compiler::shift_mods(~std::underlying_type_t<shift::compiler::shift_mods>(f));
    }

}

#endif //SHIFT_MODS_H_
