#ifndef ITEM_ACCESS_CACHE_H
#define ITEM_ACCESS_CACHE_H

#include <array>
#include <cstddef>
#include <string>

// A small fixed-size FIFO cache of recently accessed item IDs.
template <std::size_t Capacity>
class ItemAccessCache {
private:
    std::array<std::string, Capacity> itemIds{};
    std::size_t nextWriteIndex = 0;
    std::size_t itemCount = 0;

public:
    static_assert(Capacity > 0, "The cache capacity must be greater than zero.");

    void recordAccess(const std::string& itemId) {
        itemIds[nextWriteIndex] = itemId;
        nextWriteIndex = (nextWriteIndex + 1) % Capacity;

        if (itemCount < Capacity) {
            ++itemCount;
        }
    }

    std::size_t getItemCount() const {
        return itemCount;
    }

    // Returns the cached ID in oldest-to-newest order, or an empty string
    // when the requested cache position is not currently populated.
    std::string getItemIdAt(std::size_t position) const {
        if (position >= itemCount) {
            return "";
        }

        const std::size_t oldestIndex =
            (nextWriteIndex + Capacity - itemCount) % Capacity;
        const std::size_t arrayIndex = (oldestIndex + position) % Capacity;
        return itemIds[arrayIndex];
    }
};

#endif
