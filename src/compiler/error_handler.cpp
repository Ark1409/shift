/**
 * @file compiler/shift_error_handler.cpp
 */
#include "error_handler.h"

#include "logging/console.h"

#include <iostream>
#include <algorithm>

/** Namespace shift */
namespace shift::compiler {
    SHIFT_API error_handler& error_handler::add_message(message_type type, const std::string& message) {
        switch (type) {
            case message_type::warning:
                if (!this->m_warnings) { break; }
                if (!this->m_werror) {
                    this->m_messages.emplace_back(message, message_type::warning);
                    break;
                }
                [[fallthrough]];
            case message_type::error:
                this->m_messages.emplace_back(message, message_type::error);
                break;
            case message_type::info:
                this->m_messages.emplace_back(message, message_type::info);
                break;
            default:
                break;
        }
        return *this;
    }

    SHIFT_API error_handler& error_handler::add_message(message_type type, std::string&& message) {
        switch (type) {
            case message_type::warning:
                if (!this->m_warnings) { break; }
                if (!this->m_werror) {
                    this->m_messages.emplace_back(std::move(message), message_type::warning);
                    break;
                }
                [[fallthrough]];
            case message_type::error:
                this->m_messages.emplace_back(std::move(message), message_type::error);
                break;
            case message_type::info:
                this->m_messages.emplace_back(std::move(message), message_type::info);
                break;
            default:
                break;
        }
        return *this;
    }

    SHIFT_API void error_handler::print(const bool color, std::ostream& out_stream, std::ostream& err_stream) const {
        for (const auto& [message, type] : this->m_messages) {
            if (type == message_type::error) {
                if (color && logging::has_colored_console()) {
                    err_stream << logging::lred << message << logging::creset;
                } else {
                    err_stream << message;
                }
            } else if (type == message_type::warning) {
                if (color && logging::has_colored_console()) {
                    out_stream << logging::lyellow << message << logging::creset;
                } else {
                    out_stream << message;
                }
            } else if (type == message_type::info) {
                if (color && logging::has_colored_console()) {
                    out_stream << logging::lblue << message << logging::creset;
                } else {
                    out_stream << message;
                }
            } else {
                out_stream << message;
            }
        }

        err_stream.flush();
        out_stream.flush();
    }

    SHIFT_API void error_handler::print_exit(const bool color, std::ostream& out_stream, std::ostream& err_stream) const {
        const bool __exit = std::find_if(this->m_messages.cbegin(), this->m_messages.cend(), [](const message_pair_type& p) {
            return p.second == message_type::error;
        }) != this->m_messages.cend();

        print(color, out_stream, err_stream);

        if (__exit) shift::utils::exit(EXIT_FAILURE);
    }

    SHIFT_API size_t error_handler::get_error_count() const {
        return std::count_if(this->m_messages.begin(), this->m_messages.end(), [](const message_pair_type& p) {
            return p.second == message_type::error;
        });
    }

    SHIFT_API size_t error_handler::get_warning_count() const {
        return std::count_if(this->m_messages.begin(), this->m_messages.end(), [](const message_pair_type& p) {
            return p.second == message_type::warning;
        });
    }

    SHIFT_API void error_stream::flush(error_handler::message_type type) noexcept {
        this->m_error_handler->add_message(type, str());
        this->str("");
    }
}
