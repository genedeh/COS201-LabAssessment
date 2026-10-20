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

    // Makes a sorted view of item pointers. The database keeps ownership and
    // its original item order is not changed. We will replace std::sort with
    // a student-written algorithm in the sorting lesson.
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

        std::sort(sortedItems.begin(), sortedItems.end(),
                  [](const LibraryItem* left, const LibraryItem* right) {
                      return toLowercase(left->getTitle()) <
                             toLowercase(right->getTitle());
                  });
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
