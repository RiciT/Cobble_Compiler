#pragma once

#include <iostream>
#include <unordered_map>
#include <string>
#include <optional>
#include <utility> 
#include <initializer_list>

template<
    typename KeyT, 
    typename ValueT, 
    typename HashKeyT = std::hash<KeyT>, 
    typename EqKeyT = std::equal_to<KeyT>,
    typename HashValueT = std::hash<ValueT>,
    typename EqValueT = std::equal_to<ValueT>
>
class bidirectional_unordered_map {
public:
    using Key = KeyT;
    using Value = ValueT;
    using PairType = std::pair<const Key, Value>;

private:
    using ForwardMap = std::unordered_map<Key, Value, HashKeyT, EqKeyT>;
    using BackwardMap = std::unordered_map<Value, Key, HashValueT, EqValueT>;

    ForwardMap map_forward;
    BackwardMap map_backward;

public:
    // --- Constructors & Assignment ---

    bidirectional_unordered_map() = default;

    bidirectional_unordered_map(const bidirectional_unordered_map& other) = default;

    bidirectional_unordered_map(bidirectional_unordered_map&& other) noexcept = default;

    bidirectional_unordered_map(std::initializer_list<PairType> list) {
        for (const auto& pair : list) {
            // Uses the safe insert() which checks for key/value conflicts.
            insert(pair.first, pair.second);
        }
    }

    bidirectional_unordered_map& operator=(const bidirectional_unordered_map& other) = default;

    bidirectional_unordered_map& operator=(bidirectional_unordered_map&& other) noexcept = default;

    bidirectional_unordered_map& operator=(std::initializer_list<PairType> list) {
        clear();
        for (const auto& pair : list) {
            insert(pair.first, pair.second);
        }
        return *this;
    }

    // --- Friends ---

    template<typename K, typename V, typename HK, typename EK, typename HV, typename EV>
    friend bool operator==(
        const bidirectional_unordered_map<K, V, HK, EK, HV, EV>& lhs,
        const bidirectional_unordered_map<K, V, HK, EK, HV, EV>& rhs);

    // --- Capacity ---

    [[nodiscard]] bool empty() const {
        return map_forward.empty();
    }

    [[nodiscard]] size_t size() const {
        // Both maps should always have the same size.
        return map_forward.size();
    }

    // --- Modifiers ---

    void clear() {
        map_forward.clear();
        map_backward.clear();
    }

    bool insert(const Key& key, const Value& value) {
        // Check for conflicts
        if (map_forward.contains(key) || map_backward.contains(value)) {
            return false; 
        }

        // No conflicts, insert in both directions
        map_forward.emplace(key, value);
        map_backward.emplace(value, key);
        return true;
    }

    void assign(const Key& key, const Value& value) {
        // Check for existing key
        auto fwd_it = map_forward.find(key);
        if (fwd_it != map_forward.end()) {
            // Key exists. We must remove its old corresponding value 
            // from the backward map.
            Value old_value = fwd_it->second;

            // Don't erase if the old value is the same as the new one
            if (old_value != value) {
                map_backward.erase(old_value);
            }
        }

        // Check for existing value
        auto bwd_it = map_backward.find(value);
        if (bwd_it != map_backward.end()) {
            // Value exists. We must remove its old corresponding key
            // from the forward map.
            Key old_key = bwd_it->second;
            
            // Don't erase if the old key is the same as the new one
            if (old_key != key) {
                map_forward.erase(old_key);
            }
        }

        // Now, assign the new pair in both directions
        map_forward[key] = value;
        map_backward[value] = key;
    }

    bool erase_by_key(const Key& key) {
        auto fwd_it = map_forward.find(key);
        if (fwd_it == map_forward.end()) {
            return false; // Key not found
        }

        Value value = fwd_it->second;
        map_forward.erase(fwd_it);
        map_backward.erase(value); // Erase the corresponding value
        return true;
    }

    bool erase_by_value(const Value& value) {
        auto bwd_it = map_backward.find(value);
        if (bwd_it == map_backward.end()) {
            return false; // Value not found
        }

        Key key = bwd_it->second;
        map_backward.erase(bwd_it);
        map_forward.erase(key); // Erase the corresponding key
        return true;
    }

    void swap(bidirectional_unordered_map& other) noexcept {
        map_forward.swap(other.map_forward);
        map_backward.swap(other.map_backward);
    }

    // --- Lookup ---

    const Value& at(const Key& key) const {
        return map_forward.at(key); 
    }

    const Value& at_key(const Key& key) const {
        return map_forward.at(key); 
    }

    const Key& at_value(const Value& value) const {
        return map_backward.at(value);
    }

    ForwardMap::const_iterator find_by_key(const Key& key) const {
        return map_forward.find(key);
    }

    ForwardMap::const_iterator find_by_value(const Value& value) const {
        auto it = map_backward.find(value);

        if (it != map_backward.end()) {
            return it;
        }
        
        return map_forward.end();
    }


    size_t count(const Key& key) const {
        return map_forward.count(key);
    }

    bool contains_key(const Key& key) const {
        return map_forward.contains(key);
    }

    bool contains_value(const Value& value) const {
        return map_backward.contains(value);
    }

    // --- Iterators ---

    ForwardMap::iterator begin() { return map_forward.begin(); }
    ForwardMap::iterator end() { return map_forward.end(); }
    ForwardMap::const_iterator begin() const { return map_forward.cbegin(); }
    ForwardMap::const_iterator end() const { return map_forward.cend(); }
    ForwardMap::const_iterator cbegin() const { return map_forward.cbegin(); }
    ForwardMap::const_iterator cend() const { return map_forward.cend(); }
};

// --- Non-Member Functions ---

template<typename K, typename V, typename HK, typename EK, typename HV, typename EV>
bool operator==(
    const bidirectional_unordered_map<K, V, HK, EK, HV, EV>& lhs,
    const bidirectional_unordered_map<K, V, HK, EK, HV, EV>& rhs) 
{
    // We only need to check one map, as the class invariants
    // ensure the other map is a mirror.
    return lhs.map_forward == rhs.map_forward;
}

template<typename K, typename V, typename HK, typename EK, typename HV, typename EV>
bool operator!=(
    const bidirectional_unordered_map<K, V, HK, EK, HV, EV>& lhs,
    const bidirectional_unordered_map<K, V, HK, EK, HV, EV>& rhs) 
{
    return !(lhs == rhs);
}

template<typename K, typename V, typename HK, typename EK, typename HV, typename EV>
void swap(
    bidirectional_unordered_map<K, V, HK, EK, HV, EV>& lhs,
    bidirectional_unordered_map<K, V, HK, EK, HV, EV>& rhs) noexcept {
    lhs.swap(rhs);
}

template<typename K, typename V, typename HK, typename EK, typename HV, typename EV>
std::ostream& operator<<(
    std::ostream& os, 
    const bidirectional_unordered_map<K, V, HK, EK, HV, EV>& bimap)
{
    os << "{";
    bool first = true;
    for (const auto& pair : bimap) { // Uses public .begin() and .end()
        if (!first) {
            os << ", ";
        }
        os << "(" << pair.first << " <-> " << pair.second << ")";
        first = false;
    }
    os << "}";
    return os;
}