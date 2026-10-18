#ifndef BORROWING_RECORD_H
#define BORROWING_RECORD_H

#include <optional>
#include <string>

// A history record refers to a library item by ID; it does not own the item.
class BorrowingRecord {
private:
    std::string itemId;
    std::string borrowedOn;
    std::optional<std::string> returnedOn;

public:
    BorrowingRecord(const std::string& itemId, const std::string& borrowedOn)
        : itemId(itemId), borrowedOn(borrowedOn), returnedOn(std::nullopt) {}

    const std::string& getItemId() const { return itemId; }
    const std::string& getBorrowedOn() const { return borrowedOn; }
    const std::optional<std::string>& getReturnedOn() const { return returnedOn; }

    bool isReturned() const { return returnedOn.has_value(); }

    void markReturned(const std::string& date) {
        returnedOn = date;
    }
};

#endif
