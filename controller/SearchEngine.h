#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "../Book.h"
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

    const LibraryItem* recursiveSearchByTitle(
        const std::vector<const LibraryItem*>& sortedItems,
        const std::string& targetTitle,
        std::size_t first,
        std::size_t last) const {
        // Base case: there are no positions left to search.
        if (first >= last) {
            return nullptr;
        }

        const std::size_t middle = first + (last - first) / 2;
        const std::string middleTitle =
            toLowercase(sortedItems[middle]->getTitle());

        if (middleTitle == targetTitle) {
            return sortedItems[middle];
        }

        if (middleTitle < targetTitle) {
            return recursiveSearchByTitle(
                sortedItems, targetTitle, middle + 1, last);
        }

        return recursiveSearchByTitle(
            sortedItems, targetTitle, first, middle);
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

    // Only Books have authors. Other item types are skipped safely.
    std::vector<const LibraryItem*> searchByAuthor(const LibraryDatabase& database,
                                                    const std::string& author) {
        const std::string targetAuthor = toLowercase(author);
        std::vector<const LibraryItem*> matches;

        for (std::size_t index = 0; index < database.getItemCount(); ++index) {
            const LibraryItem* item = database.getItemAt(index);
            const Book* book = dynamic_cast<const Book*>(item);

            if (book != nullptr &&
                toLowercase(book->getAuthor()) == targetAuthor) {
                matches.push_back(book);
            }
        }

        return matches;
    }

    // Makes a sorted view of item pointers with insertion sort. The database
    // keeps ownership and its original item order is not changed.
    std::vector<const LibraryItem*> makeTitleSortedView(
        const LibraryDatabase& database) const {
        std::vector<const LibraryItem*> sortedItems;
        sortedItems.reserve(database.getItemCount());

        for (std::size_t index = 0; index < database.getItemCount(); ++index) {
            const LibraryItem* item = database.getItemAt(index);
            if (item != nullptr) {
                sortedItems.push_back(item);
            }
        }

        // The left side (before index) is sorted. Hold the next item as the
        // key, shift larger titles right, and place the key into the gap.
        for (std::size_t index = 1; index < sortedItems.size(); ++index) {
            const LibraryItem* keyItem = sortedItems[index];
            const std::string keyTitle = toLowercase(keyItem->getTitle());
            std::size_t position = index;

            while (position > 0 &&
                   toLowercase(sortedItems[position - 1]->getTitle()) > keyTitle) {
                sortedItems[position] = sortedItems[position - 1];
                --position;
            }

            sortedItems[position] = keyItem;
        }

        return sortedItems;
    }

    // Binary search requires this pointer list to be sorted by title.
    const LibraryItem* binarySearchByTitle(
        const std::vector<const LibraryItem*>& sortedItems,
        const std::string& title) const {
        const std::string targetTitle = toLowercase(title);
        std::size_t first = 0;
        std::size_t last = sortedItems.size(); // One past the final position.

        while (first < last) {
            const std::size_t middle = first + (last - first) / 2;
            const std::string middleTitle =
                toLowercase(sortedItems[middle]->getTitle());

            if (middleTitle == targetTitle) {
                return sortedItems[middle];
            }

            if (middleTitle < targetTitle) {
                first = middle + 1;
            } else {
                last = middle;
            }
        }

        return nullptr;
    }

    // Recursive binary search; sortedItems must be ordered by title.
    const LibraryItem* recursiveSearchByTitle(
        const std::vector<const LibraryItem*>& sortedItems,
        const std::string& title) const {
        return recursiveSearchByTitle(
            sortedItems, toLowercase(title), 0, sortedItems.size());
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
