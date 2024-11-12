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
        error_handler() = default;

        inline error_handler(const error_handler&);

        error_handler(error_handler&&) noexcept = default;

        ~error_handler() noexcept = default;

        SHIFT_API error_handler& operator=(const error_handler&);

        error_handler& operator=(error_handler&&) noexcept = default;

        SHIFT_API error_handler& add_info(const std::string& msg) noexcept;

        SHIFT_API error_handler& add_warning(const std::string& msg) noexcept;

        SHIFT_API error_handler& add_error(const std::string& msg) noexcept;

        SHIFT_API error_handler& add_info(std::string&& msg) noexcept;

        SHIFT_API error_handler& add_warning(std::string&& msg) noexcept;

        SHIFT_API error_handler& add_error(std::string&& msg) noexcept;

        // Convenient stream for writing warnings and errors
        inline std::ostringstream& stream() noexcept { return m_message_stream; }

        inline std::ostringstream& get_stream() noexcept { return stream(); }

        // Flushes the std::ostringstream from stream(), considering all the text currently held within it to be a message of this type
        SHIFT_API void flush_stream(message_type type) noexcept;

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

        inline void clear_messages() noexcept { this->m_messages.clear(); }

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
        std::ostringstream m_message_stream;

        friend struct marker<error_handler>;
    };

    inline error_handler::error_handler(const error_handler& other) : m_warnings(other.m_warnings), m_werror(other.m_werror),
                                                                      m_messages(other.m_messages),
                                                                      m_message_stream(other.m_message_stream.str()) {}

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
