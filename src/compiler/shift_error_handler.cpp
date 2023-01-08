/**
 * @file compiler/error_handler.cpp
 */
#include "shift_error_handler.h"

#include "utils/utils.h"
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
			if ((type & message_type::warning) && !this->m_warnings) {
				m_message_stream.str("");
				return;
			}

			type = (type & message_type::warning) && this->m_werror ? ((type & ~message_type::warning) | message_type::error) : type;

			this->m_messages.push_back(_message_pair_type(m_message_stream.str(), type));
			m_message_stream.str("");
		}

		void error_handler::print(const bool color) noexcept {
			for (const auto& [message, type] : this->m_messages) {
				if (type & message_type::error) {
					if (color && logging::has_colored_console()) {
						std::cerr << logging::lred << message << logging::creset;
					} else {
						std::cerr << message;
					}
				} else if (type & message_type::warning) {
					if (color && logging::has_colored_console()) {
						std::cout << logging::lyellow << message << logging::creset;
					} else {
						std::cout << message;
					}
				} else if (type & message_type::info) {
					if (color && logging::has_colored_console()) {
						std::cout << logging::lblue << message << logging::creset;
					} else {
						std::cout << message;
					}
				} else {
					std::cout << message;
				}
			}

			this->m_messages.clear();
			std::cerr.flush();
			std::cout.flush();
		}

		void error_handler::print_exit(const bool color) noexcept {
			bool __exit = false;

			for (const auto& [message, type] : this->m_messages) {
				if (type & message_type::error) {
					__exit = true;
					if (color && logging::has_colored_console()) {
						std::cerr << logging::lred << message << logging::creset;
					} else {
						std::cerr << message;
					}
				} else if (type & message_type::warning) {
					if (color && logging::has_colored_console()) {
						std::cout << logging::lyellow << message << logging::creset;
					} else {
						std::cout << message;
					}
				} else if (type & message_type::info) {
					if (color && logging::has_colored_console()) {
						std::cout << logging::lblue << message << logging::creset;
					} else {
						std::cout << message;
					}
				} else {
					std::cout << message;
				}
			}

			this->m_messages.clear();
			std::cerr.flush();
			std::cout.flush();

			if (__exit) shift::utils::exit(1);
		}

		size_t error_handler::get_error_count(void) const {
			size_t ret = 0;
			for (const auto& [m, type] : this->m_messages) {
				if (type & message_type::error) ret++;
			}
			return ret;
		}

		size_t error_handler::get_warning_count(void) const {
			size_t ret = 0;
			for (const auto& [m, type] : this->m_messages) {
				if (type & message_type::warning) ret++;
			}
			return ret;
		}

		void error_handler::rollback(void) noexcept {
			if (this->m_marks.empty()) return;

			const auto mark = this->m_marks.top();

			if (mark < this->m_messages.size())
				this->m_messages.resize(mark);

			this->m_marks.pop();
		}

		void error_handler::clear_marks(typename std::stack<typename std::list<_message_pair_type>::size_type>::size_type count) noexcept {
			count = std::min(count, this->m_marks.size());

			// Remove only *count* items
			for (; count; count--) this->m_marks.pop();
		}
	}
}
