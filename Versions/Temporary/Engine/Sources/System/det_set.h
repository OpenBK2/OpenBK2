#pragma once

#include <list>
#include <unordered_map>
#include <initializer_list>
#include <utility>
#include <cstddef>
#include <functional>

template <
    class T,
    class Hash = std::hash<T>,
    class KeyEqual = std::equal_to<T>
>
class det_set {
public:
    using key_type        = T;
    using value_type      = T;
    using size_type       = std::size_t;
    using hasher          = Hash;
    using key_equal       = KeyEqual;

private:
    using list_type       = std::list<T>;
    using list_iterator   = typename list_type::iterator;

    list_type items_;

    // Value is duplicated here.
    // The hash table gives O(1) average lookup.
    std::unordered_map<T, list_iterator, Hash, KeyEqual> index_;

public:
    // Important: expose only const iterators.
    // Set elements must not be mutated through iteration,
    // otherwise the hash index would become invalid.
    using iterator       = typename list_type::const_iterator;
    using const_iterator = typename list_type::const_iterator;

public:
    det_set() = default;

    det_set(std::initializer_list<T> init) {
        reserve(init.size());
        for (const auto& value : init) {
            insert(value);
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

    const_iterator find(const T& value) const {
        auto found = index_.find(value);
        if (found == index_.end()) {
            return items_.end();
        }
        return found->second;
    }

    bool contains(const T& value) const {
        return index_.find(value) != index_.end();
    }

    size_type count(const T& value) const {
        return contains(value) ? 1u : 0u;
    }

    // ------------------------------------------------------------
    // insertion
    // Returns {iterator, true} if inserted.
    // Returns {existing_iterator, false} if value already existed.
    // Existing values keep their original insertion position.
    // ------------------------------------------------------------

    std::pair<const_iterator, bool> insert(const T& value) {
        auto found = index_.find(value);
        if (found != index_.end()) {
            return {found->second, false};
        }

        items_.push_back(value);
        auto it = std::prev(items_.end());
        index_.emplace(*it, it);
        return {it, true};
    }

    std::pair<const_iterator, bool> insert(T&& value) {
        auto found = index_.find(value);
        if (found != index_.end()) {
            return {found->second, false};
        }

        items_.push_back(std::move(value));
        auto it = std::prev(items_.end());
        index_.emplace(*it, it);
        return {it, true};
    }

    template <class... Args>
    std::pair<const_iterator, bool> emplace(Args&&... args) {
        T value(std::forward<Args>(args)...);

        auto found = index_.find(value);
        if (found != index_.end()) {
            return {found->second, false};
        }

        items_.push_back(std::move(value));
        auto it = std::prev(items_.end());
        index_.emplace(*it, it);
        return {it, true};
    }

    // ------------------------------------------------------------
    // erase
    // ------------------------------------------------------------

    bool erase(const T& value) {
        auto found = index_.find(value);
        if (found == index_.end()) {
            return false;
        }

        items_.erase(found->second);
        index_.erase(found);
        return true;
    }

    const_iterator erase(const_iterator pos) {
        if (pos == items_.end()) {
            return pos;
        }

        // std::list::erase needs a non-const iterator in C++17.
        // Convert const_iterator to iterator by walking from begin().
        auto mutable_pos = items_.begin();
        std::advance(mutable_pos, std::distance(items_.cbegin(), pos));

        index_.erase(*mutable_pos);
        return items_.erase(mutable_pos);
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