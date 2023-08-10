#ifndef SHIFT_ORDERED_SET_ITERATOR_H_
#define SHIFT_ORDERED_SET_ITERATOR_H_ 1

#include <unordered_set>
#include <list>
#include <type_traits>

namespace shift::utils {
    template<typename V, typename Hash, typename Pred, typename Alloc>
    class ordered_set;

    template<typename V, typename Hash = std::hash<V>, typename Pred = std::equal_to<V>, typename Alloc = std::allocator<V>>
    struct ordered_set_iterator {
    private:
        typedef std::unordered_set<std::remove_const_t<std::remove_reference_t<V>>, Hash, Pred, Alloc> base_unordered_set_type;
        typedef std::conditional_t<std::is_const_v<std::remove_reference_t<V>>, base_unordered_set_type, base_unordered_set_type const>
            unordered_set_type;

        typedef ordered_set_iterator<V, Hash, Pred, Alloc> ordered_set_iterator_type;
        typedef ordered_set<std::remove_const_t<std::remove_reference_t<V>>, Hash, Pred, Alloc> base_ordered_set_type;

        typedef typename std::list<typename base_ordered_set_type::value_type const*>::const_iterator iterator_type;
    public:
        typedef std::conditional_t<std::is_const_v<std::remove_reference_t<V>>, base_ordered_set_type const, base_ordered_set_type> ordered_set_type;
        typedef std::conditional_t<std::is_const_v<std::remove_reference_t<V>>, typename ordered_set_type::value_type const, typename ordered_set_type::value_type> const value_type;
        typedef value_type& reference_type;
        typedef value_type* pointer_type;
    public:
        template<typename V1, typename = std::enable_if_t<std::is_const_v<std::remove_reference_t<V>> && std::is_same_v<std::remove_const_t<std::remove_reference_t<V>>, std::remove_const_t<std::remove_reference_t<V1>>>>>
        inline ordered_set_iterator(const ordered_set_iterator<std::remove_const_t<std::remove_reference_t<V>>, Hash, Pred, Alloc>& other) noexcept
            : m_order_it(other.m_order_it) {}

        template<typename V1, typename = std::enable_if_t<std::is_const_v<std::remove_reference_t<V>> && std::is_same_v<std::remove_const_t<std::remove_reference_t<V>>, std::remove_const_t<std::remove_reference_t<V1>>>>>
        inline ordered_set_iterator(ordered_set_iterator<V1, Hash, Pred, Alloc>&& other) noexcept
            : m_order_it(std::move(other.m_order_it)) {}

        inline ordered_set_iterator(iterator_type it) noexcept
            : m_order_it(it) {}

        ordered_set_iterator(const ordered_set_iterator&) noexcept = default;
        ordered_set_iterator(ordered_set_iterator&&) noexcept = default;

        constexpr ordered_set_iterator& operator=(const ordered_set_iterator&) noexcept = default;
        constexpr ordered_set_iterator& operator=(ordered_set_iterator&&) noexcept = default;

        inline reference_type operator*() const { return **m_order_it; }
        inline pointer_type operator->() const { return *m_order_it; }

        inline ordered_set_iterator_type& operator++() noexcept { ++m_order_it; return *this; }
        inline ordered_set_iterator_type operator++(int) noexcept {
            auto ret = *this;
            ++*this;
            return ret;
        }

        inline ordered_set_iterator_type& operator--() noexcept { --m_order_it; return *this; }
        inline ordered_set_iterator_type operator--(int) noexcept {
            auto ret = *this;
            --*this;
            return ret;
        }

        inline bool operator==(const ordered_set_iterator_type& other) const noexcept { return m_order_it == other.m_order_it; }
        inline bool operator!=(const ordered_set_iterator_type& other) const noexcept { return !operator==(other); }
    private:
        iterator_type m_order_it;

        friend class ordered_set<std::remove_const_t<std::remove_reference_t<V>>, Hash, Pred, Alloc>;

        friend struct ordered_set_iterator<std::remove_const_t<std::remove_reference_t<V>>, Hash, Pred, Alloc>;
        friend struct ordered_set_iterator<V const, Hash, Pred, Alloc>;
    };
}

#endif