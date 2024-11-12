#ifndef SHIFT_MARKER_H_
#define SHIFT_MARKER_H_

#include "utils/utils.h"

namespace shift::compiler {

    /**
     * A type used to place markers at certain sections of the compiling stage to allow for
     * easy rollback of unintended/unforeseen changes.
     *
     * Classes are meant to provide specializations of this interface to enable marking support on their implementations.
     *
     * A specialization of this interface should model the following functions:
     *
     * <ul>
     *  <li><code>void mark()</code>: Adds a mark at the current position within the markee type.
     *      More than one overload may be provided.</li>
     *  <li><code>void rollback()</code>: Rolls back to the markee type to the last generated mark. If no marks exist, this is a no-op.</li>
     *  <li><code>void pop_marks(std::integral auto n)</code>: Removes the first @c n marks within the marker. If @c n is greater than
     *      the number of previously generated marks, clears the marks.</li>
     *  <li><code>void pop_mark()</code>: Equivalent to <code>pop_marks(1)</code></li>
     *  <li><code>auto get_marks() const</code>: Returns a data structure representative of the marks generated with the marker. This
     *      container may be read-only.</li>
     * </ul>
     *
     * @tparam T The markee type. Should be utilized as a reference type.
     */
    template<typename T>
    struct marker;

    /**
     * A helper class for implementing markable types
     * @tparam T The type the marker is meant to hold. Should be utilized as a reference type.
     * @tparam MarkT The type representing a mark within the class.
     */
    template<typename T, typename MarkT>
    struct marker_helper {
        inline void pop_marks(typename std::stack<MarkT, std::vector<MarkT>>::size_type count = -1) {
            utils::pop_stack(this->m_marks, count);
        }

        inline void pop_mark() { return pop_marks(1); }

        const auto& get_marks() const noexcept { return m_marks; }

    protected:
        explicit marker_helper(T& markee) : m_markee{ m_markee } {}

        T& m_markee;
        std::stack<MarkT, std::vector<MarkT>> m_marks;
    };
}

#endif //SHIFT_MARKER_H_
