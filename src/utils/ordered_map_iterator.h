#ifndef SHIFT_ORDERED_MAP_ITERATOR_H_
#define SHIFT_ORDERED_MAP_ITERATOR_H_ 1

#include <unordered_map>
#include <list>
#include <type_traits>
#include <utility>
#include <functional>
#include <stdexcept>

namespace shift::utils {
    template<typename K, typename V, typename Hash, typename Pred, typename Alloc>
    class ordered_map;

    template<typename K, typename V, typename Hash = std::hash<K>, typename Pred = std::equal_to<K>,
        typename Alloc = std::allocator<std::pair<const K, V>>>
    struct ordered_map_iterator {
    private:
        typedef std::unordered_map<K, std::remove_const_t<std::remove_reference_t<V>>, Hash, Pred, Alloc> base_unordered_map_type;
        typedef std::conditional_t<std::is_const_v<std::remove_reference_t<V>>, base_unordered_map_type, base_unordered_map_type const>
            unordered_map_type;

        typedef ordered_map_iterator<K, V, Hash, Pred, Alloc> ordered_map_iterator_type;
        typedef ordered_map<K, std::remove_const_t<std::remove_reference_t<V>>, Hash, Pred, Alloc> base_ordered_map_type;

        typedef typename std::list<typename base_ordered_map_type::value_type*>::const_iterator iterator_type;
    public:
        typedef std::conditional_t<std::is_const_v<std::remove_reference_t<V>>, base_ordered_map_type const, base_ordered_map_type> ordered_map_type;
        typedef std::conditional_t<std::is_const_v<std::remove_reference_t<V>>, typename ordered_map_type::value_type const, typename ordered_map_type::value_type> value_type;
        typedef value_type& reference_type;
        typedef value_type* pointer_type;
    public:
        template<typename K1, typename = std::enable_if_t<std::is_const_v<std::remove_reference_t<V>> && std::is_same_v<K1, K>>>
        inline ordered_map_iterator(const ordered_map_iterator<K1, std::remove_const_t<std::remove_reference_t<V>>, Hash, Pred, Alloc>& other) noexcept
            : m_order_it(other.m_order_it) {}

        template<typename K1, typename = std::enable_if_t<std::is_const_v<std::remove_reference_t<V>> && std::is_same_v<K1, K>>>
        inline ordered_map_iterator(ordered_map_iterator<K1, std::remove_const_t<std::remove_reference_t<V>>, Hash, Pred, Alloc>&& other) noexcept
            : m_order_it(std::move(other.m_order_it)) {}

        inline ordered_map_iterator(iterator_type it) noexcept
            : m_order_it(it) {}

        ordered_map_iterator(const ordered_map_iterator&) noexcept = default;
        ordered_map_iterator(ordered_map_iterator&&) noexcept = default;

        constexpr ordered_map_iterator& operator=(const ordered_map_iterator&) noexcept = default;
        constexpr ordered_map_iterator& operator=(ordered_map_iterator&&) noexcept = default;

        inline reference_type operator*() const { return **m_order_it; }
        inline pointer_type operator->() const { return *m_order_it; }

        inline ordered_map_iterator_type& operator++() noexcept { ++m_order_it; return *this; }
        inline ordered_map_iterator_type operator++(int) noexcept {
            auto ret = *this;
            ++*this;
            return ret;
        }

        inline ordered_map_iterator_type& operator--() noexcept { --m_order_it; return *this; }
        inline ordered_map_iterator_type operator--(int) noexcept {
            auto ret = *this;
            --*this;
            return ret;
        }

        inline bool operator==(const ordered_map_iterator_type& other) const noexcept { return m_order_it == other.m_order_it; }
        inline bool operator!=(const ordered_map_iterator_type& other) const noexcept { return !operator==(other); }
    private:
        iterator_type m_order_it;

        friend class ordered_map<K, std::remove_const_t<std::remove_reference_t<V>>, Hash, Pred, Alloc>;

        friend struct ordered_map_iterator<K, std::remove_const_t<std::remove_reference_t<V>>, Hash, Pred, Alloc>;
        friend struct ordered_map_iterator<K, V const, Hash, Pred, Alloc>;
    };
}

#endif