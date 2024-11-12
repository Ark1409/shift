#ifndef SHIFT_ITERATOR_WRAPPER_H_
#define SHIFT_ITERATOR_WRAPPER_H_ 1

#include <concepts>
#include <iterator>
#include <functional>
#include <type_traits>

namespace shift::utils {
    template<std::input_or_output_iterator Iterator>
    struct iterator_wrapper {
        typedef Iterator iterator_type;
        typedef std::iter_value_t<iterator_type> value_type;
        typedef std::iter_reference_t<iterator_type> reference;
        typedef std::remove_reference_t<reference>* pointer;
        typedef std::iter_difference_t<iterator_type> difference_type;
        typedef typename std::iterator_traits<iterator_type>::iterator_category iterator_category;

        iterator_wrapper(Iterator it) : m_it{ it } {}

        iterator_wrapper(Iterator it, const std::function<iterator_type(iterator_type, difference_type)>& advance_func,
            const std::function<difference_type(iterator_type, iterator_type)>& distance_func)
            : m_it{ it }, m_advance_func{ advance_func }, m_distance_func{ distance_func } {}

        iterator_wrapper(Iterator it, std::function<iterator_type(iterator_type, difference_type)>&& advance_func,
            std::function<difference_type(iterator_type, iterator_type)>&& distance_func)
            : m_it{ it }, m_advance_func{ std::move(advance_func) }, m_distance_func{ std::move(distance_func) } {}

        reference operator*() const { return *m_it; }

        pointer operator->() const { return m_it.operator->(); }

        iterator_wrapper& operator++() {
            m_it = m_advance_func(m_it, 1);
            return *this;
        }

        iterator_wrapper operator++(int) {
            iterator_wrapper ret = *this;
            ++*this;
            return ret;
        }

        iterator_wrapper& operator--() requires std::bidirectional_iterator<iterator_type> {
            m_it = m_advance_func(m_it, -1);
            return *this;
        }

        iterator_wrapper operator--(int) requires std::bidirectional_iterator<iterator_type> {
            iterator_wrapper ret = *this;
            --*this;
            return ret;
        }

        iterator_wrapper& operator+=(difference_type diff) requires std::random_access_iterator<iterator_type> {
            m_it = m_advance_func(m_it, diff);
            return *this;
        }

        iterator_wrapper operator+(difference_type diff) requires std::random_access_iterator<iterator_type> {
            iterator_wrapper ret = *this;
            *this += diff;
            return ret;
        }

        iterator_wrapper& operator-=(difference_type diff) requires std::random_access_iterator<iterator_type> {
            return operator+=(-diff);
        }

        iterator_wrapper operator-(difference_type diff) requires std::random_access_iterator<iterator_type> {
            return operator+(-diff);
        }

        difference_type operator-(const iterator_wrapper& other) requires std::random_access_iterator<iterator_type> {
            return m_distance_func(m_it, other.m_it);
        }

        reference operator[](difference_type diff) requires std::random_access_iterator<iterator_type> {
            if (diff == 0) return operator*();
            iterator_wrapper it{ *this };
            it += diff;
            return *it;
        }

        template<std::sentinel_for<Iterator> Sentinel = Iterator>
        bool operator==(const iterator_wrapper<Sentinel>& other) const { return m_it == other.m_it; }

        template<std::sentinel_for<Iterator> Sentinel = Iterator>
        bool operator!=(const iterator_wrapper<Sentinel>& other) const { return m_it != other.m_it; }

        bool operator<(const iterator_wrapper& other) const requires std::random_access_iterator<iterator_type> {
            return m_it < other.m_it;
        }

        bool operator<=(const iterator_wrapper& other) const requires std::random_access_iterator<iterator_type> {
            return m_it <= other.m_it;
        }

        bool operator>(const iterator_wrapper& other) const requires std::random_access_iterator<iterator_type> {
            return m_it > other.m_it;
        }

        bool operator>=(const iterator_wrapper& other) const requires std::random_access_iterator<iterator_type> {
            return m_it >= other.m_it;
        }

        const auto& get_advance_function() const noexcept { return m_advance_func; }

        void set_advance_function(const std::function<iterator_type(iterator_type, difference_type)>& func) {
            m_advance_func = func;
        }

        void set_advance_function(std::function<iterator_type(iterator_type, difference_type)>&& func) {
            m_advance_func = std::move(func);
        }

        const auto& get_distance_function() const noexcept { return m_distance_func; }

        void set_distance_function(const std::function<difference_type(iterator_type, iterator_type)>& func) {
            m_distance_func = func;
        }

        void set_distance_function(std::function<difference_type(iterator_type, iterator_type)>&& func) {
            m_distance_func = std::move(func);
        }

        const iterator_type& base() const noexcept { return m_it; }

    private:
        Iterator m_it;

        std::function<iterator_type(iterator_type, difference_type)> m_advance_func = [](auto it, auto diff) {
            return std::next(it, diff);
        };

        std::function<difference_type(iterator_type, iterator_type)> m_distance_func = [](auto a, auto b) {
            return std::distance(a, b);
        };
    };
}

#endif //SHIFT_ITERATOR_WRAPPER_H
