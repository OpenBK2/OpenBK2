#pragma once

#include <list>
#include <unordered_map>
#include <utility>
#include <stdexcept>
#include <initializer_list>
#include <type_traits>

template <
    class Key,
    class T,
    class Hash = std::hash<Key>,
    class KeyEqual = std::equal_to<Key>
>
class det_map {
public:
    using key_type        = Key;
    using mapped_type     = T;
    using value_type      = std::pair<const Key, T>;
    using size_type       = std::size_t;
    using hasher          = Hash;
    using key_equal       = KeyEqual;

private:
    using list_type       = std::list<value_type>;
    using list_iterator   = typename list_type::iterator;

    list_type items_;

    // Key is duplicated here.
    // The hash table gives O(1) average lookup.
    std::unordered_map<Key, list_iterator, Hash, KeyEqual> index_;

public:
    using iterator       = typename list_type::iterator;
    using const_iterator = typename list_type::const_iterator;

public:
    det_map() = default;

    det_map(std::initializer_list<value_type> init) {
        reserve(init.size());
        for (const auto& item : init) {
            insert(item);
        }
    }

    // ------------------------------------------------------------
    // basic capacity
    // ------------------------------------------------------------

    bool empty() const noexcept {
        return items_.empty();
    }

    size_type size() const noexcept {
        return items_.size();
    }

    void clear() noexcept {
        index_.clear();
        items_.clear();
    }

    // ------------------------------------------------------------
    // iteration: deterministic insertion order
    // ------------------------------------------------------------

    iterator begin() noexcept {
        return items_.begin();
    }

    iterator end() noexcept {
        return items_.end();
    }

    const_iterator begin() const noexcept {
        return items_.begin();
    }

    const_iterator end() const noexcept {
        return items_.end();
    }

    const_iterator cbegin() const noexcept {
        return items_.cbegin();
    }

    const_iterator cend() const noexcept {
        return items_.cend();
    }

    // ------------------------------------------------------------
    // lookup
    // ------------------------------------------------------------

    iterator find(const Key& key) {
        auto found = index_.find(key);
        if (found == index_.end()) {
            return items_.end();
        }
        return found->second;
    }

    const_iterator find(const Key& key) const {
        auto found = index_.find(key);
        if (found == index_.end()) {
            return items_.end();
        }
        return found->second;
    }

    bool contains(const Key& key) const {
        return index_.find(key) != index_.end();
    }

    T& at(const Key& key) {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("det_map::at: key not found");
        }
        return it->second;
    }

    const T& at(const Key& key) const {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("det_map::at: key not found");
        }
        return it->second;
    }

    // ------------------------------------------------------------
    // operator[]
    // If key does not exist, inserts it at the end with T{}.
    // ------------------------------------------------------------

    T& operator[](const Key& key) {
        auto found = index_.find(key);
        if (found != index_.end()) {
            return found->second->second;
        }

        items_.emplace_back(key, T{});
        auto it = std::prev(items_.end());
        index_.emplace(it->first, it);
        return it->second;
    }

    T& operator[](Key&& key) {
        auto found = index_.find(key);
        if (found != index_.end()) {
            return found->second->second;
        }

        items_.emplace_back(std::move(key), T{});
        auto it = std::prev(items_.end());
        index_.emplace(it->first, it);
        return it->second;
    }

    // ------------------------------------------------------------
    // insertion
    // Returns {iterator, true} if inserted.
    // Returns {existing_iterator, false} if key already existed.
    // Existing keys keep their original insertion position.
    // ------------------------------------------------------------

    std::pair<iterator, bool> insert(const value_type& value) {
        auto found = index_.find(value.first);
        if (found != index_.end()) {
            return {found->second, false};
        }

        items_.push_back(value);
        auto it = std::prev(items_.end());
        index_.emplace(it->first, it);
        return {it, true};
    }

    std::pair<iterator, bool> insert(value_type&& value) {
        auto found = index_.find(value.first);
        if (found != index_.end()) {
            return {found->second, false};
        }

        items_.push_back(std::move(value));
        auto it = std::prev(items_.end());
        index_.emplace(it->first, it);
        return {it, true};
    }

    template <class... Args>
    std::pair<iterator, bool> emplace(const Key& key, Args&&... args) {
        auto found = index_.find(key);
        if (found != index_.end()) {
            return {found->second, false};
        }

        items_.emplace_back(key, T(std::forward<Args>(args)...));
        auto it = std::prev(items_.end());
        index_.emplace(it->first, it);
        return {it, true};
    }

    template <class... Args>
    std::pair<iterator, bool> try_emplace(const Key& key, Args&&... args) {
        return emplace(key, std::forward<Args>(args)...);
    }

    // Insert or assign.
    // If key exists, value is replaced but position is unchanged.
    // If key does not exist, it is appended at the end.
    template <class U>
    std::pair<iterator, bool> insert_or_assign(const Key& key, U&& value) {
        auto found = index_.find(key);
        if (found != index_.end()) {
            found->second->second = std::forward<U>(value);
            return {found->second, false};
        }

        items_.emplace_back(key, std::forward<U>(value));
        auto it = std::prev(items_.end());
        index_.emplace(it->first, it);
        return {it, true};
    }

    // ------------------------------------------------------------
    // erase
    // ------------------------------------------------------------

    bool erase(const Key& key) {
        auto found = index_.find(key);
        if (found == index_.end()) {
            return false;
        }

        items_.erase(found->second);
        index_.erase(found);
        return true;
    }

    iterator erase(iterator pos) {
        if (pos == items_.end()) {
            return pos;
        }

        index_.erase(pos->first);
        return items_.erase(pos);
    }

    // ------------------------------------------------------------
    // hash-table management
    // ------------------------------------------------------------

    void reserve(size_type count) {
        index_.reserve(count);
    }

    size_type bucket_count() const {
        return index_.bucket_count();
    }

    float load_factor() const {
        return index_.load_factor();
    }

    void max_load_factor(float value) {
        index_.max_load_factor(value);
    }

    float max_load_factor() const {
        return index_.max_load_factor();
    }
};