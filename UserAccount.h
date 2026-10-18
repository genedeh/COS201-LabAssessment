#ifndef USER_ACCOUNT_H
#define USER_ACCOUNT_H

#include <string>
#include <vector>

#include "BorrowingRecord.h"

// A user owns their borrowing-history records, but not the library items
// named by those records.
class UserAccount {
private:
    std::string userId;
    std::string name;
    std::vector<BorrowingRecord> borrowingHistory;

public:
    UserAccount(const std::string& userId, const std::string& name)
        : userId(userId), name(name) {}

    const std::string& getUserId() const { return userId; }
    const std::string& getName() const { return name; }

    void addBorrowingRecord(const BorrowingRecord& record) {
        borrowingHistory.push_back(record);
    }

    // Mark the most recent active loan for this item as returned.
    bool markItemReturned(const std::string& itemId, const std::string& date) {
        for (auto record = borrowingHistory.rbegin();
             record != borrowingHistory.rend();
             ++record) {
            if (record->getItemId() == itemId && !record->isReturned()) {
                record->markReturned(date);
                return true;
            }
        }

        return false;
    }

    const std::vector<BorrowingRecord>& getBorrowingHistory() const {
        return borrowingHistory;
    }
};

#endif
