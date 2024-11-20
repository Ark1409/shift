#ifndef SHIFT_MODS_H_
#define SHIFT_MODS_H_ 1

#include <string_view>
#include <cstddef>
#include <cstdint>
#include <climits>
#include <array>
#include <vector>

#include "utils/enum.h"
#include "utils/optional.h"
#include "utils/utility.h"

namespace shift::compiler {
    enum shift_mods : std::uint_fast16_t {
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

    shift_mods to_mod(std::string_view sv);

    constexpr shift::compiler::shift_mods operator|(const shift::compiler::shift_mods f, const shift::compiler::shift_mods other) noexcept {
        return shift_mods(utils::to_underlying(f) | utils::to_underlying(other));
    }

    constexpr shift::compiler::shift_mods& operator|=(shift::compiler::shift_mods& f,
        const shift::compiler::shift_mods other) noexcept {
        return f = operator|(f, other);
    }

    constexpr shift::compiler::shift_mods operator&(const shift::compiler::shift_mods f, const shift::compiler::shift_mods other) noexcept {
        return shift::compiler::shift_mods(utils::to_underlying(f) & utils::to_underlying(other));
    }

    constexpr shift::compiler::shift_mods& operator&=(shift::compiler::shift_mods& f,
        const shift::compiler::shift_mods other) noexcept {
        return f = operator&(f, other);
    }

    constexpr shift::compiler::shift_mods operator~(const shift::compiler::shift_mods f) noexcept {
        return shift::compiler::shift_mods(~utils::to_underlying(f));
    }

    namespace lexing {
        struct token; // token struct forward decl for mods_holder class below
    }

    namespace parsing {
        class parser; // parser class for friend access
    }

    class mods_holder {
    private:
        typedef std::underlying_type_t<shift_mods> mods_t;

    public:
        void add(const lexing::token& tok) noexcept;

        void add(const lexing::token* tok) noexcept {
            if (tok) { add(*tok); }
        }

        void remove(shift_mods mod) noexcept;

        void clear() noexcept {
            m_accum = shift_mods::NONE;
            m_mods.fill(nullptr);
        }

        const lexing::token* find(shift_mods mod) const noexcept;

        const lexing::token* find_any(shift_mods mods) const noexcept;

        bool has_mod(shift_mods mod) const noexcept { return (m_accum & mod) == mod; }

        operator shift_mods() const noexcept { return m_accum; }

        mods_holder& operator|=(const lexing::token& tok) noexcept {
            add(tok);
            return *this;
        }

        bool operator==(shift_mods mod) const noexcept { return m_accum == mod; }

        const lexing::token& front() const noexcept;

        const lexing::token& back() const noexcept;

        std::vector<std::pair<const lexing::token*, shift_mods>> sorted() const noexcept;

    private:
        void unsafe_add(shift_mods, const lexing::token&) noexcept;

    private:
        std::array<const lexing::token*, sizeof(mods_t) * CHAR_BIT> m_mods{};
        shift_mods m_accum{shift_mods::NONE};

        friend class parsing::parser;
    };

}

#endif //SHIFT_MODS_H_
