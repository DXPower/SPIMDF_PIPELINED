#pragma once

#include <algorithm>
#include <array>
#include <optional>

template<typename T, int N>
class opt_array : public std::array<std::optional<T>, N> {
    using El_t = std::optional<T>;
    public:
    opt_array() : std::array<El_t, N>() {
        this->fill(std::nullopt);
    };

    auto next_slot() {
        return std::find(this->begin(), this->end(), std::nullopt);
    }

    const auto next_slot() const {
        return std::find(this->cbegin(), this->cend(), std::nullopt);
    }

    // If there is room, return iterator to location. Else, return iterator to end()
    typename std::array<El_t, N>::iterator push_back(T&& o) {
        auto slot = next_slot();

        if (is_full())
            return slot;

        *slot = std::move(o);
        return slot;
    }

    // If there is room, return iterator to location. Else, return iterator to end()
    typename std::array<El_t, N>::iterator push_front(T&& o) {
        if (!is_full()) {
            std::rotate(this->rbegin(), this->rbegin() + 1, this->rend()); // Single rotate to right
            (*this)[0] = std::move(o);

            return this->begin();
        } else {
            return this->end();
        }
    }

    // If there is room, return iterator to location. Else, return iterator to end()
    T&& pop_front() {
       T popped = std::move((*this)[0].value()); // Get frontmost element
       (*this)[0] = std::nullopt;

       std::rotate(this->begin(), this->begin() + 1, this->end()); // Single rotate to left

       return std::move(popped);
    }

    T&& pop_back() {
        auto it = std::prev(next_slot());
        T popped = std::move(it->value()); // Get backmost element
        
        *it = std::nullopt;

        return std::move(popped);
    }

    bool is_empty() const {
        return next_slot() == this->begin();
    }

    bool is_full() const {
        return next_slot() == this->end();
    }

    std::size_t num_empty() const {
        return std::distance(next_slot(), this->end());
    }
};