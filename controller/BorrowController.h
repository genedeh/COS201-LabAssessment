#ifndef BORROW_CONTROLLER_H
#define BORROW_CONTROLLER_H

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>

#include "../LibraryDatabase.h"
#include "../ReservationQueue.h"
#include "../UserAccount.h"

enum class BorrowResult {
    Borrowed,
    Reserved,
    ItemNotFound
};

// Coordinates borrowing rules between the catalogue, user, and waitlists.
class BorrowController {
private:
    // Each item ID has its own first-in, first-out reservation queue.
    std::unordered_map<std::string, ReservationQueue> reservationsByItem;

public:
    BorrowResult borrowItem(LibraryDatabase& database,
                            UserAccount& user,
                            const std::string& itemId,
                            const std::string& borrowedOn) {
        LibraryItem* item = database.findItemById(itemId);
        if (item == nullptr) {
            return BorrowResult::ItemNotFound;
        }

        if (!item->isAvailable()) {
            reservationsByItem[itemId].addReservation(user.getUserId());
            return BorrowResult::Reserved;
        }

        item->setAvailable(false);
        user.addBorrowingRecord(BorrowingRecord(itemId, borrowedOn));
        return BorrowResult::Borrowed;
    }

    std::size_t getWaitingCount(const std::string& itemId) const {
        const auto queue = reservationsByItem.find(itemId);
        if (queue == reservationsByItem.end()) {
            return 0;
        }
        return queue->second.getWaitingCount();
    }

    std::optional<std::string> takeNextReservation(const std::string& itemId) {
        const auto queue = reservationsByItem.find(itemId);
        if (queue == reservationsByItem.end()) {
            return std::nullopt;
        }
        return queue->second.takeNextUser();
    }
};

#endif
