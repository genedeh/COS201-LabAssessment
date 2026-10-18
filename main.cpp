#include <iostream>

#include "Book.h"
#include "Journal.h"
#include "Magazine.h"

// This function accepts the common base type. The correct itemType()
// implementation is chosen at runtime for each child object (polymorphism).
void printItem(const LibraryItem& item) {
    std::cout << item.itemType() << ": "
              << item.getTitle() << " ("
              << item.getPublicationYear() << ")\n";
}

int main() {
    Book book("B001", "Things Fall Apart", 1958, "Chinua Achebe");
    Magazine magazine("M001", "Science Monthly", 2025);
    Journal journal("J001", "Journal of Computing", 2024);

    printItem(book);
    printItem(magazine);
    printItem(journal);

    std::cout << "Book author: " << book.getAuthor() << "\n";
    std::cout << "Book available: "
              << (book.isAvailable() ? "yes" : "no") << "\n";
}
