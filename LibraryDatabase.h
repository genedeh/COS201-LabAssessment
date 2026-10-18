#ifndef LIBRARY_DATABASE_H
#define LIBRARY_DATABASE_H

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "LibraryItem.h"

// Owns all items currently stored in the library.
class LibraryDatabase {
private:
    // Each unique_ptr owns one item. The item itself can be any LibraryItem
    // child, such as Book, Magazine, or Journal.
    std::vector<std::unique_ptr<LibraryItem>> items;

public:
    // Moving the pointer transfers ownership into the database.
    void addItem(std::unique_ptr<LibraryItem> item) {
        if (item) {
            items.push_back(std::move(item));
        }
    }

    std::size_t getItemCount() const {
        return items.size();
    }

    // Returns a borrowed pointer for inspection. The database keeps ownership.
    // nullptr means the requested position does not exist.
    const LibraryItem* getItemAt(std::size_t index) const {
        if (index >= items.size()) {
            return nullptr;
        }

        return items[index].get();
    }
};

#endif
