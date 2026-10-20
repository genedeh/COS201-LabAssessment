#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "../ItemAccessCache.h"
#include "../LibraryDatabase.h"

// Handles catalogue searches. It borrows the database and never owns items.
class SearchEngine {
private:
    ItemAccessCache<3> accessCache;

    static std::string toLowercase(std::string text) {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char character) {
                           return static_cast<char>(std::tolower(character));
                       });
        return text;
    }

public:
    // Case-insensitive linear search. Returns nullptr if there is no match.
    const LibraryItem* searchByTitle(const LibraryDatabase& database,
                                     const std::string& title) {
        const std::string targetTitle = toLowercase(title);

        for (std::size_t index = 0; index < database.getItemCount(); ++index) {
            const LibraryItem* item = database.getItemAt(index);
            if (item != nullptr && toLowercase(item->getTitle()) == targetTitle) {
                return item;
            }
        }

        return nullptr;
    }

    // Returns every matching item because a library can contain many items
    // of the same type.
    std::vector<const LibraryItem*> searchByType(const LibraryDatabase& database,
                                                  const std::string& type) {
        const std::string targetType = toLowercase(type);
        std::vector<const LibraryItem*> matches;

        for (std::size_t index = 0; index < database.getItemCount(); ++index) {
            const LibraryItem* item = database.getItemAt(index);
            if (item != nullptr && toLowercase(item->itemType()) == targetType) {
                matches.push_back(item);
            }
        }

        return matches;
    }

    // Call this when a user opens or selects one specific item.
    void recordItemAccess(const LibraryItem& item) {
        accessCache.recordAccess(item.getId());
    }

    std::size_t getCachedItemCount() const {
        return accessCache.getItemCount();
    }

    std::optional<ItemAccessCache<3>::CacheEntry> getMostFrequentlyAccessedAt(
        std::size_t rank) const {
        return accessCache.getMostFrequentAt(rank);
    }
};

#endif
