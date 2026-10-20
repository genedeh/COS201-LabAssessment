#ifndef ITEM_ACCESS_CACHE_H
#define ITEM_ACCESS_CACHE_H

#include <array>
#include <cstddef>
#include <optional>
#include <string>

// A fixed-size cache that counts accesses and ranks cached items by frequency.
template <std::size_t Capacity>
class ItemAccessCache {
public:
    struct CacheEntry {
        std::string itemId;
        std::size_t accessCount;
    };

private:
    struct StoredEntry {
        CacheEntry entry;
        std::size_t insertionOrder;
    };

    std::array<StoredEntry, Capacity> entries{};
    std::size_t itemCount = 0;
    std::size_t nextInsertionOrder = 0;

public:
    static_assert(Capacity > 0, "The cache capacity must be greater than zero.");

    void recordAccess(const std::string& itemId) {
        for (std::size_t index = 0; index < itemCount; ++index) {
            if (entries[index].entry.itemId == itemId) {
                ++entries[index].entry.accessCount;
                return;
            }
        }

        if (itemCount < Capacity) {
            entries[itemCount] = {{itemId, 1}, nextInsertionOrder++};
            ++itemCount;
            return;
        }

        // Choose the least-accessed entry. If tied, choose the one that
        // entered the cache first.
        std::size_t leastUsedIndex = 0;
        for (std::size_t index = 1; index < itemCount; ++index) {
            const bool usedLess = entries[index].entry.accessCount <
                                  entries[leastUsedIndex].entry.accessCount;
            const bool tiedButOlder =
                entries[index].entry.accessCount ==
                    entries[leastUsedIndex].entry.accessCount &&
                entries[index].insertionOrder <
                    entries[leastUsedIndex].insertionOrder;

            if (usedLess || tiedButOlder) {
                leastUsedIndex = index;
            }
        }

        entries[leastUsedIndex] = {{itemId, 1}, nextInsertionOrder++};
    }

    std::size_t getItemCount() const {
        return itemCount;
    }

    // Returns a copy at the requested rank (0 is most accessed), or no value
    // if that rank is outside the populated entries.
    std::optional<CacheEntry> getMostFrequentAt(std::size_t rank) const {
        if (rank >= itemCount) {
            return std::nullopt;
        }

        std::array<bool, Capacity> alreadyRanked{};
        std::size_t selectedIndex = 0;

        for (std::size_t currentRank = 0; currentRank <= rank; ++currentRank) {
            selectedIndex = Capacity;

            for (std::size_t index = 0; index < itemCount; ++index) {
                if (alreadyRanked[index]) {
                    continue;
                }

                const bool usedMore = selectedIndex == Capacity ||
                    entries[index].entry.accessCount >
                        entries[selectedIndex].entry.accessCount;
                const bool tiedButOlder = selectedIndex != Capacity &&
                    entries[index].entry.accessCount ==
                        entries[selectedIndex].entry.accessCount &&
                    entries[index].insertionOrder <
                        entries[selectedIndex].insertionOrder;

                if (usedMore || tiedButOlder) {
                    selectedIndex = index;
                }
            }

            alreadyRanked[selectedIndex] = true;
        }

        return entries[selectedIndex].entry;
    }
};

#endif
