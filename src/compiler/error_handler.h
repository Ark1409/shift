/**
 * @file compiler/shift_error_handler.h
 */
#ifndef SHIFT_ERROR_HANDLER_H_
#define SHIFT_ERROR_HANDLER_H_ 1

#include "utils/utils.h"
#include "compiler/marker.h"

#include <deque>
#include <stack>
#include <string>
#include <sstream>
#include <ostream>

/** Namespace shift */
namespace shift::compiler {
    class error_handler {
    public:
        enum class message_type {
            error = 0x1, // Represents an error message from the compiler.
            warning, // Represents a warning message from the compiler.
            info
        };
    public:
        typedef std::pair<std::string, message_type> message_pair_type;
    public:
        SHIFT_API error_handler& add_message(message_type type, const std::string& message);

        SHIFT_API error_handler& add_message(message_type type, std::string&& message);

        inline error_handler& add_info(const std::string& msg) { return add_message(message_type::info, msg); }

        inline error_handler& add_warning(const std::string& msg) { return add_message(message_type::warning, msg); }

        inline error_handler& add_error(const std::string& msg) { return add_message(message_type::error, msg); }

        inline error_handler& add_info(std::string&& msg) { return add_message(message_type::info, std::move(msg)); }

        inline error_handler& add_warning(std::string&& msg) { return add_message(message_type::warning, std::move(msg)); }

        inline error_handler& add_error(std::string&& msg) { return add_message(message_type::error, std::move(msg)); }

        // Prints without clearing the internal message list
        // Print messages
        SHIFT_API void
        print(const bool color = true, std::ostream& out_stream = std::cout, std::ostream& err_stream = std::cerr) const; // Print messages

        // Print messages and exits the program iff errors were found
        SHIFT_API void print_exit(const bool color = true, std::ostream& out_stream = std::cout,
            std::ostream& err_stream = std::cerr) const; // Print messages and exits the program iff errors were found

        // Prints and clears the internal message list
        inline void print_clear(const bool color = true, std::ostream& out_stream = std::cout, std::ostream& err_stream = std::cerr) {
            print(color, out_stream, err_stream);
            this->m_messages.clear();
        }

        inline void print_exit_clear(const bool color = true, std::ostream& out_stream = std::cout, std::ostream& err_stream = std::cerr) {
            print_exit(color, out_stream, err_stream);
            this->m_messages.clear();
        }

        inline void clear_messages() noexcept {
            this->m_messages.clear();
            this->m_messages.shrink_to_fit();
        }

        SHIFT_API size_t get_error_count() const;

        SHIFT_API size_t get_warning_count() const;

        inline void set_werror(const bool werror = true) noexcept { this->m_werror = werror; }

        inline bool is_werror() const noexcept { return this->m_werror; }

        inline void set_print_warnings(const bool warnings = true) { this->m_warnings = warnings; }

        inline void set_warnings(const bool warnings = true) { return set_print_warnings(warnings); }

        inline void enable_warnings() { return set_warnings(true); }

        inline bool is_print_warnings() const noexcept { return this->m_warnings; }

        inline bool is_warning() const noexcept { return is_print_warnings(); }

        inline const std::deque<message_pair_type>& get_messages() const noexcept { return this->m_messages; }

    private:
        bool m_warnings = false, m_werror = false;
        std::deque<message_pair_type> m_messages;

        friend struct marker<error_handler>;
    };

    /**
     * @brief Convenient stream class for writing error messages to an @ref error_handler
     */
    class error_stream : public std::ostringstream {
    public:
        inline error_stream(error_handler&);

        inline error_stream(const error_stream&);

        error_stream(error_stream&&) noexcept = default;

        inline error_stream& operator=(const error_stream&);

        error_stream& operator=(error_stream&&) noexcept = default;

        /// @brief Flushes the string currently held with the stream as the following message type.
        /// After this call, this class holds an empty string as its string content buffer.
        SHIFT_API void flush(error_handler::message_type type) noexcept;

        error_handler& get_error_handler() noexcept { return *m_error_handler; }

        const error_handler& get_error_handler() const noexcept { return *m_error_handler; }

    private:
        error_handler* m_error_handler{ nullptr };
    };

    inline error_stream::error_stream(error_handler& eh) : std::ostringstream(), m_error_handler(&eh) {}

    inline error_stream::error_stream(const error_stream& es) : std::ostringstream(es.str()),
                                                                m_error_handler(es.m_error_handler) {}

    inline error_stream& error_stream::operator=(const error_stream& es) {
        if (&es != this) { this->str(es.str()); }
        return *this;
    }

    template<>
    struct marker<error_handler> : marker_helper<error_handler, std::deque<error_handler::message_pair_type>::size_type> {
        explicit marker(error_handler& e) : marker_helper(e) {}

        /**
         * Adds a mark to the current list of warnings and errors. Calling this method multiple times
         * will not replace previous marks. Instead, they are added into a stack, with the most
         * recent mark being used when rolling back.
         *
         * @see rollback()
         */
        inline void mark() { this->m_marks.push(this->m_markee.get_messages().size()); } // Mark current warnings and errors

        /**
         * Rolls back to the most recent mark, popping it off the stack to remove it from further use.
         */
        void rollback() {
            if (this->m_marks.empty()) return;

            const auto mark = this->m_marks.top();

            this->m_markee.m_messages.resize(std::min(mark, this->m_markee.get_messages().size()));

            this->m_marks.pop();
        }
    };

    constexpr error_handler::message_type operator^(const error_handler::message_type f, const error_handler::message_type other) noexcept {
        return error_handler::message_type(std::underlying_type_t<error_handler::message_type>(f) ^
                                           std::underlying_type_t<error_handler::message_type>(other));
    }

    constexpr error_handler::message_type& operator^=(error_handler::message_type& f, const error_handler::message_type other) noexcept {
        return f = operator^(f, other);
    }

    constexpr error_handler::message_type operator|(const error_handler::message_type f, const error_handler::message_type other) noexcept {
        return error_handler::message_type(std::underlying_type_t<error_handler::message_type>(f) |
                                           std::underlying_type_t<error_handler::message_type>(other));
    }

    constexpr error_handler::message_type& operator|=(error_handler::message_type& f, const error_handler::message_type other) noexcept {
        return f = operator|(f, other);
    }

    constexpr error_handler::message_type operator&(const error_handler::message_type f, const error_handler::message_type other) noexcept {
        return error_handler::message_type(std::underlying_type_t<error_handler::message_type>(f) &
                                           std::underlying_type_t<error_handler::message_type>(other));
    }

    constexpr error_handler::message_type& operator&=(error_handler::message_type& f, const error_handler::message_type other) noexcept {
        return f = operator&(f, other);
    }

    constexpr error_handler::message_type operator~(const error_handler::message_type f) noexcept {
        return error_handler::message_type(~std::underlying_type_t<error_handler::message_type>(f));
    }
}

inline std::ostream& operator<<(std::ostream& out, const shift::compiler::error_handler& handler) {
    handler.print(false, out, out);
    return out;
}

#endif /* SHIFT_ERROR_HANDLER_H_ */
