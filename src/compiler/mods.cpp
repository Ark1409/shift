#include "compiler/mods.h"
#include "compiler/lexing/token.h"

#include <optional>
#include <algorithm>

namespace shift::compiler {
    shift_mods to_mod(std::string_view sv) {
        if (sv == "public") { return shift_mods::PUBLIC; }
        if (sv == "protected") { return shift_mods::PROTECTED; }
        if (sv == "private") { return shift_mods::PRIVATE; }
        if (sv == "static") { return shift_mods::STATIC; }
        if (sv == "imut") { return shift_mods::IMUT; }
        if (sv == "const") { return shift_mods::CONST_; }
        if (sv == "binary") { return shift_mods::BINARY; }
        if (sv == "extern") { return shift_mods::EXTERN; }
        if (sv == "explicit") { return shift_mods::EXPLICIT; }
        return shift_mods::NONE;
    }

    void mods_holder::add(const lexing::token& tok) noexcept {
        auto mod = to_mod(tok.get_data());
        if (has_mod(mod)) { return; }
        return unsafe_add(mod, tok);
    }

    void mods_holder::unsafe_add(shift_mods mod, const lexing::token& tok) noexcept {
        m_accum |= mod;
        for (auto mod_num = mods_t(mod); mod_num != 0;) {
            auto rb = std::countr_zero(mod_num);
            m_mods[rb] = &tok;
            mod_num ^= mods_t(1) << rb;
        }
    }

    void mods_holder::remove(shift_mods mod) noexcept {
        if (!has_mod(mod)) { return; }
        for (auto mod_num = mods_t(mod); mod_num != 0;) {
            auto rb = std::countr_zero(mod_num);
            m_mods[rb] = nullptr;
            mod_num ^= mods_t(1) << rb;
        }
        m_accum &= ~mod;
    }

    const lexing::token* mods_holder::find(shift_mods mod) const noexcept {
        if (!has_mod(mod)) { return nullptr; }
        std::optional<const lexing::token*> tok;
        for (auto mod_num = mods_t(mod); mod_num != 0;) {
            auto rb = std::countr_zero(mod_num);
            auto* new_tok = m_mods[rb];

            if (!tok.has_value()) { tok = new_tok; }
            else if (*tok != new_tok) { return nullptr; }

            mod_num ^= mods_t(1) << rb;
        }
        return tok.value_or(nullptr);
    }

    const lexing::token* mods_holder::find_any(shift_mods mods) const noexcept {
        auto common_mods = m_accum & mods;
        if (!common_mods) return nullptr;
        return m_mods[std::countr_zero(mods_t(common_mods))];
    }

    const lexing::token& mods_holder::front() const noexcept {
        return *std::ranges::min(m_mods, [](const lexing::token* a, const lexing::token* b) {
            if (!a) return false;
            if (!b) return true;
            return a->get_file_position() < b->get_file_position();
        });
    }

    const lexing::token& mods_holder::back() const noexcept {
        return *std::ranges::max(m_mods, [](const lexing::token* a, const lexing::token* b) {
            if (!a) return false;
            if (!b) return true;
            return a->get_file_position() > b->get_file_position();
        });
    }

    std::vector<std::pair<const lexing::token*, shift_mods>> mods_holder::sorted() const noexcept {
        auto mods_copy = m_mods;
        std::ranges::sort(mods_copy, [](const lexing::token* a, const lexing::token* b) {
            return !b || (a && a->get_file_position() < b->get_file_position());
        });
        auto mod_tokens_end = std::ranges::find(mods_copy, nullptr);

        std::vector<std::pair<const lexing::token*, shift_mods>> ret;
        ret.reserve(mod_tokens_end - mods_copy.begin());
        for (auto it = mods_copy.begin(); it != mod_tokens_end; ++it) {
            auto next_it = it + 1;
            if (next_it == mod_tokens_end || *next_it != *it) {
                const lexing::token* tok = *it;
                ret.emplace_back(tok, to_mod(tok->get_data()));
            }
        }
        return ret;
    }
}