#ifndef CHIP8_MAP_H
#define CHIP8_MAP_H

#include <array>
#include <optional>
#include <algorithm>
#include <stdexcept>

template<typename Key, typename Value, std::size_t Size>
struct Map {
    std::array<std::pair<Key, Value>, Size> data;

    [[nodiscard]] constexpr Value at(const Key &key) const
    {
        const auto itr =
          std::find_if(begin(data), end(data), [&key](const auto &v) { return v.first == key; });
        if (itr != end(data)) {
            return itr->second;
        } else {
            throw std::range_error("Not found");
        }
    }

    [[nodiscard]] constexpr std::optional<Value> maybeAt(const Key &key) const
    {
        const auto itr =
                std::find_if(begin(data), end(data), [&key](const auto &v) { return v.first == key; });
        if (itr != end(data)) {
            return itr->second;
        } else {
            return std::nullopt;
        }
    }
};

#endif// CHIP8_MAP_H
