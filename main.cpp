#include <iostream>
#include <cstddef>
#include <memory>
#include <vector>

#include "AdminActionHistory.h"
#include "Book.h"
#include "BorrowingRecord.h"
#include "controller/BorrowController.h"
#include "controller/SearchEngine.h"
#include "Journal.h"
#include "LibraryDatabase.h"
#include "Magazine.h"
#include "UserAccount.h"

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

    UserAccount user("U001", "Ada");
    user.addBorrowingRecord(BorrowingRecord("B001", "2026-10-01"));
    user.markItemReturned("B001", "2026-10-18");

    const BorrowingRecord& record = user.getBorrowingHistory().front();
    std::cout << user.getName() << " borrowed item " << record.getItemId()
              << " on " << record.getBorrowedOn() << "\n";
    if (record.getReturnedOn().has_value()) {
        std::cout << "Returned on: " << record.getReturnedOn().value() << "\n";
    }

    BorrowController borrowController;
    UserAccount sam("U002", "Sam");
    UserAccount alex("U003", "Alex");

    const BorrowResult samResult = borrowController.borrowItem(
        database, sam, "B001", "2026-10-29");
    const BorrowResult alexResult = borrowController.borrowItem(
        database, alex, "B001", "2026-10-29");

    std::cout << "Sam's borrow request: "
              << (samResult == BorrowResult::Borrowed ? "borrowed" : "not borrowed")
              << "\n";
    std::cout << "Alex's request: "
              << (alexResult == BorrowResult::Reserved ? "reserved" : "not reserved")
              << "\n";
    std::cout << "Users waiting for B001: "
              << borrowController.getWaitingCount("B001") << "\n";

    AdminActionHistory adminHistory;
    adminHistory.recordAction({
        AdminAction::Kind::ItemAdded,
        "B002", "Book", "Dune", 1965, "Frank Herbert"});
    adminHistory.recordAction({
        AdminAction::Kind::ItemDeleted,
        "M002", "Magazine", "Nature Weekly", 2024, ""});

    const auto actionToUndo = adminHistory.takeMostRecentAction();
    if (actionToUndo.has_value()) {
        std::cout << "Most recent action to undo: "
                  << actionToUndo->itemType << " "
                  << actionToUndo->title << "\n";
    }

    SearchEngine searchEngine;
    searchEngine.searchByTitle(database, "things fall apart");
    searchEngine.searchByTitle(database, "science monthly");
    searchEngine.searchByTitle(database, "journal of computing");
    const LibraryItem* found =
        searchEngine.searchByTitle(database, "things fall apart");

    if (found != nullptr) {
        std::cout << "Search found: ";
        printItem(*found);
    }

    const std::vector<const LibraryItem*> books =
        searchEngine.searchByType(database, "BOOK");
    std::cout << "Books found by type: " << books.size() << "\n";
    for (const LibraryItem* book : books) {
        if (book != nullptr) {
            printItem(*book);
        }
    }

    const std::vector<const LibraryItem*> achebeBooks =
        searchEngine.searchByAuthor(database, "CHINUA ACHEBE");
    std::cout << "Books found by author: " << achebeBooks.size() << "\n";
    for (const LibraryItem* item : achebeBooks) {
        if (item != nullptr) {
            printItem(*item);
        }
    }

    const std::vector<const LibraryItem*> titleSortedItems =
        searchEngine.makeTitleSortedView(database);
    std::cout << "Titles sorted with insertion sort: ";
    for (const LibraryItem* item : titleSortedItems) {
        std::cout << item->getTitle() << " | ";
    }
    std::cout << "\n";

    const std::vector<const LibraryItem*> selectionSortedTitles =
        searchEngine.makeSelectionSortedView(database, SortField::Title);
    std::cout << "Titles sorted with selection sort: ";
    for (const LibraryItem* item : selectionSortedTitles) {
        std::cout << item->getTitle() << " | ";
    }
    std::cout << "\n";

    const std::vector<const LibraryItem*> yearSortedItems =
        searchEngine.makeSortedView(database, SortField::PublicationYear);
    std::cout << "Items sorted by publication year: ";
    for (const LibraryItem* item : yearSortedItems) {
        std::cout << item->getPublicationYear() << " - "
                  << item->getTitle() << " | ";
    }
    std::cout << "\n";

    const std::vector<const LibraryItem*> authorSortedItems =
        searchEngine.makeSortedView(database, SortField::Author);
    std::cout << "Items sorted by author: ";
    for (const LibraryItem* item : authorSortedItems) {
        std::cout << item->itemType() << " - " << item->getTitle() << " | ";
    }
    std::cout << "\n";

    const LibraryItem* binaryFound =
        searchEngine.binarySearchByTitle(titleSortedItems, "SCIENCE MONTHLY");
    if (binaryFound != nullptr) {
        std::cout << "Binary search found: ";
        printItem(*binaryFound);
    }

    const LibraryItem* recursiveFound =
        searchEngine.recursiveSearchByTitle(titleSortedItems, "science monthly");
    if (recursiveFound != nullptr) {
        std::cout << "Recursive search found: ";
        printItem(*recursiveFound);
    }

    // Simulate the user opening the book three times, and each other item once.
    if (found != nullptr) {
        searchEngine.recordItemAccess(*found);
        searchEngine.recordItemAccess(*found);
        searchEngine.recordItemAccess(*found);
    }
    const LibraryItem* magazine = database.getItemAt(1);
    const LibraryItem* journal = database.getItemAt(2);
    if (magazine != nullptr) {
        searchEngine.recordItemAccess(*magazine);
    }
    if (journal != nullptr) {
        searchEngine.recordItemAccess(*journal);
    }

    std::cout << "Most frequently accessed item IDs: ";
    for (std::size_t index = 0; index < searchEngine.getCachedItemCount(); ++index) {
        const auto cachedItem = searchEngine.getMostFrequentlyAccessedAt(index);
        if (cachedItem.has_value()) {
            std::cout << cachedItem->itemId << " ("
                      << cachedItem->accessCount << " accesses) ";
        }
    }
    std::cout << "\n";
}
