/**
 * @file compiler/error_handler.cpp
 */
#include "shift_error_handler.h"

#include "logging/console.h"

#include <iostream>
#include <algorithm>

 /** Namespace shift */
namespace shift {
	/** Namespace compiler */
	namespace compiler {
		error_handler& error_handler::operator=(const error_handler& other) noexcept {
			this->m_messages = other.m_messages;
			this->m_message_stream.str(other.m_message_stream.str());
			this->m_marks = other.m_marks;
			return *this;
		}

		error_handler& error_handler::add_info(const std::string& info) noexcept {
			if (!this->m_warnings)
				return *this;
			this->m_messages.push_back(_message_pair_type(info, message_type::info));
			return *this;
		}

		error_handler& error_handler::add_warning(const std::string& warning) noexcept {
			if (!this->m_warnings)
				return *this;
			this->m_messages.push_back(_message_pair_type(warning, this->m_werror ? message_type::error : message_type::warning));
			return *this;
		}

		error_handler& error_handler::add_error(const std::string& error) noexcept {
			this->m_messages.push_back(_message_pair_type(error, message_type::error));
			return *this;
		}

		error_handler& error_handler::add_info(std::string&& info) noexcept {
			if (!this->m_warnings)
				return *this;
			this->m_messages.push_back(_message_pair_type(std::move(info), message_type::info));
			return *this;
		}

		error_handler& error_handler::add_warning(std::string&& warning) noexcept {
			if (!this->m_warnings)
				return *this;
			this->m_messages.push_back(_message_pair_type(std::move(warning), this->m_werror ? message_type::error : message_type::warning));
			return *this;
		}

		error_handler& error_handler::add_error(std::string&& error) noexcept {
			this->m_messages.push_back(_message_pair_type(std::move(error), message_type::error));
			return *this;
		}

		void error_handler::flush_stream(message_type type) noexcept {
			if (type == message_type::warning && !this->m_warnings) {
				m_message_stream.str("");
				return;
			}

			type = type == message_type::warning && this->m_werror ? message_type::error : type;

			this->m_messages.push_back(_message_pair_type(m_message_stream.str(), type));
			m_message_stream.str("");
		}

		void error_handler::print(const bool color, std::ostream& out_stream, std::ostream& err_stream) const {
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

		void error_handler::print_exit(const bool color, std::ostream& out_stream, std::ostream& err_stream) const {
			const bool __exit = std::find_if(this->m_messages.cbegin(), this->m_messages.cend(), [](const _message_pair_type& p) {
				return p.second == message_type::error;
				}) != this->m_messages.cend();

				print(color, out_stream, err_stream);

				if (__exit) shift::utils::exit(EXIT_FAILURE);
		}

		size_t error_handler::get_error_count(void) const {
			return std::count_if(this->m_messages.cbegin(), this->m_messages.cend(), [](const _message_pair_type& p) {
				return p.second == message_type::error;
				});
		}

		size_t error_handler::get_warning_count(void) const {
			return std::count_if(this->m_messages.cbegin(), this->m_messages.cend(), [](const _message_pair_type& p) {
				return p.second == message_type::warning;
				});
		}

		void error_handler::rollback(void) noexcept {
			if (this->m_marks.empty()) return;

			const auto mark = this->m_marks.top();

			this->m_messages.resize(std::min(mark, this->m_messages.size()));

			this->m_marks.pop();
		}
	}
}
