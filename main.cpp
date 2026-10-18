#include <iostream>
#include <cstddef>
#include <memory>

#include "Book.h"
#include "Journal.h"
#include "LibraryDatabase.h"
#include "Magazine.h"

// This function accepts the common base type. The correct itemType()
// implementation is chosen at runtime for each child object (polymorphism).
void printItem(const LibraryItem& item) {
    std::cout << item.itemType() << ": "
              << item.getTitle() << " ("
              << item.getPublicationYear() << ")\n";
}

int main() {
    LibraryDatabase database;

    database.addItem(std::make_unique<Book>(
        "B001", "Things Fall Apart", 1958, "Chinua Achebe"));
    database.addItem(std::make_unique<Magazine>(
        "M001", "Science Monthly", 2025));
    database.addItem(std::make_unique<Journal>(
        "J001", "Journal of Computing", 2024));

    for (std::size_t index = 0; index < database.getItemCount(); ++index) {
        const LibraryItem* item = database.getItemAt(index);
        if (item != nullptr) {
            printItem(*item);
        }
    }

    std::cout << "Total items: " << database.getItemCount() << "\n";
}
