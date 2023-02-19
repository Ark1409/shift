/**
 * @file compiler/shift_analyzer.h
 */
#ifndef SHIFT_ANALYZER_H_
#define SHIFT_ANALYZER_H_ 1
#include "compiler/shift_parser.h"

#include <unordered_map>
#include <unordered_set>

namespace shift {
    namespace compiler {
        class analyzer {
        public:
            inline analyzer(error_handler* const handler, std::list<parser>* const parsers) noexcept;
            inline analyzer(error_handler* const handler, std::list<parser>& parsers) noexcept;

            void analyze();

            inline const error_handler* get_error_handler() const noexcept { return m_error_handler; }
            inline void set_error_handler(error_handler* const handler) noexcept { m_error_handler = handler; }

            inline std::list<parser>* get_parsers() noexcept { return m_parsers; }
            inline const std::list<parser>* get_parsers() const noexcept { return m_parsers; }
        private:
            void m_token_error(const parser& parser_, const token& token_, const std::string_view msg);
            void m_token_error(const parser& parser_, const token& token_, const std::string& msg);
            void m_token_error(const parser& parser_, const token& token_, const char* const msg);

            void m_token_warning(const parser& parser_, const token& token_, const std::string_view msg);
            void m_token_warning(const parser& parser_, const token& token_, const std::string& msg);
            void m_token_warning(const parser& parser_, const token& token_, const char* const msg);

            void m_error(const parser& parser_, const std::string_view msg);
            void m_error(const parser& parser_, const std::string& msg);
            void m_error(const parser& parser_, const char* const msg);

            void m_warning(const parser& parser_, const std::string_view msg);
            void m_warning(const parser& parser_, const std::string& msg);
            void m_warning(const parser& parser_, const char* const msg);

            std::string_view m_get_line(const parser&, const token&) const noexcept;

            bool m_contains_module(const parser::shift_module& m) const noexcept;
            bool m_contains_module(const std::string& m) const noexcept;
        private:
            error_handler* m_error_handler;
            std::list<parser>* m_parsers;
        private:
            enum data_type : uint_fast8_t {
                _module=1,
                _class,
                _function,
                _variable
            };
            std::unordered_set<std::string> m_modules;
            std::unordered_map<std::string, parser::shift_class*> m_classes;
        };

        inline analyzer::analyzer(error_handler* const handler, std::list<parser>* const parsers) noexcept: m_error_handler(handler), m_parsers(parsers) {}
        inline analyzer::analyzer(error_handler* const handler, std::list<parser>& parsers) noexcept: analyzer(handler, &parsers) {}
    }
}
#endif