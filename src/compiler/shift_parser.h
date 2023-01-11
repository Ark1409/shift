#include "compiler/shift_tokenizer.h"
#include "compiler/shift_error_handler.h"

#include <string>
#include <string_view>
#include <list>
#include <vector>
#include <utility>

namespace shift {
    namespace compiler {
        class parser {
        public:
            inline parser(tokenizer* const tokenizer) noexcept;
            inline parser(error_handler* const error_handler, tokenizer* const tokenizer) noexcept;

            void parse(void);

            inline tokenizer* get_tokenizer() const noexcept { return m_tokenizer; }
            inline void set_tokenizer(tokenizer* const tokenizer) noexcept { m_tokenizer = tokenizer; }
            inline error_handler* get_error_handler() noexcept { return m_error_handler; }
            inline const error_handler* get_error_handler() const noexcept { return m_error_handler; }
            inline void set_error_handler(error_handler* const error_handler) noexcept { m_error_handler = error_handler; }
        public:
            enum mods: uint_fast8_t {
                PUBLIC = 0x1, PROTECTED = 0x2, PRIVATE = 0x4, STATIC = 0x8, CONST_ = 0x10, BINARY = 0x20, EXTERN = 0x40
            };
        private:
            struct shift_name {
                typename std::vector<token>::const_iterator begin;
                typename std::vector<token>::const_iterator end;

                inline auto size() const noexcept { return end - begin; }
            };
        private:
            using shift_module = shift_name;
        private:
            void m_parse_access_specifier(void);
            void m_parse_use(void);
            void m_parse_module(void);
            shift_name m_parse_name(const char* const);

            void m_token_error(const token& token_, const std::string_view msg);
            void m_token_error(const token& token_, const std::string& msg);
            void m_token_error(const token& token_, const char* const msg);

            void m_token_warning(const token& token_, const std::string_view msg);
            void m_token_warning(const token& token_, const std::string& msg);
            void m_token_warning(const token& token_, const char* const msg);

            std::string_view m_get_line(const token&) const noexcept;

            const token& m_skip_until(const std::string_view) noexcept;
            const token& m_skip_until(const std::string&) noexcept;
            const token& m_skip_until(const char* const) noexcept;
            const token& m_skip_until(const typename token::token_type) noexcept;

            const token& m_skip_after(const std::string_view) noexcept;
            const token& m_skip_after(const std::string&) noexcept;
            const token& m_skip_after(const char* const) noexcept;
            const token& m_skip_after(const typename token::token_type) noexcept;

            mods m_get_mods(void) const noexcept;
            void m_add_mod(mods, const token&) noexcept;
            void m_clear_mods(void) noexcept;
        private:
            tokenizer* m_tokenizer;
            error_handler* m_error_handler;
        private:
            std::list<std::pair<mods, const token*>> m_mods;
            shift_module m_module;
            std::list<shift_module> m_global_uses;
        };

        inline parser::parser(tokenizer* const tokenizer) noexcept: m_tokenizer(tokenizer), m_error_handler(tokenizer->get_error_handler()) {}
        inline parser::parser(error_handler* const error_handler, tokenizer* const tokenizer) noexcept: m_tokenizer(tokenizer), m_error_handler(error_handler) {}
    }
}

constexpr inline shift::compiler::parser::mods operator^(const shift::compiler::parser::mods f, const shift::compiler::parser::mods other) noexcept { return shift::compiler::parser::mods(std::underlying_type_t<shift::compiler::parser::mods>(f) ^ std::underlying_type_t<shift::compiler::parser::mods>(other)); }
constexpr inline shift::compiler::parser::mods& operator^=(shift::compiler::parser::mods& f, const shift::compiler::parser::mods other) noexcept { return f = operator^(f, other); }
constexpr inline shift::compiler::parser::mods operator|(const shift::compiler::parser::mods f, const shift::compiler::parser::mods other) noexcept { return shift::compiler::parser::mods(std::underlying_type_t<shift::compiler::parser::mods>(f) | std::underlying_type_t<shift::compiler::parser::mods>(other)); }
constexpr inline shift::compiler::parser::mods& operator|=(shift::compiler::parser::mods& f, const shift::compiler::parser::mods other) noexcept { return f = operator|(f, other); }
constexpr inline shift::compiler::parser::mods operator&(const shift::compiler::parser::mods f, const shift::compiler::parser::mods other) noexcept { return shift::compiler::parser::mods(std::underlying_type_t<shift::compiler::parser::mods>(f) & std::underlying_type_t<shift::compiler::parser::mods>(other)); }
constexpr inline shift::compiler::parser::mods& operator&=(shift::compiler::parser::mods& f, const shift::compiler::parser::mods other) noexcept { return f = operator&(f, other); }
constexpr inline shift::compiler::parser::mods operator~(const shift::compiler::parser::mods f) noexcept { return shift::compiler::parser::mods(~std::underlying_type_t<shift::compiler::parser::mods>(f)); }