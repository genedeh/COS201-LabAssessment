#ifndef RESERVATION_QUEUE_H
#define RESERVATION_QUEUE_H

#include <queue>
#include <optional>
#include <string>

// Keeps user IDs waiting for an item in first-in, first-out order.
class ReservationQueue {
private:
    std::queue<std::string> waitingUserIds;

public:
    void addReservation(const std::string& userId) {
        waitingUserIds.push(userId);
    }

    bool isEmpty() const {
        return waitingUserIds.empty();
    }

    std::size_t getWaitingCount() const {
        return waitingUserIds.size();
    }

    // Removes and returns the next user. No value means the queue was empty.
    std::optional<std::string> takeNextUser() {
        if (waitingUserIds.empty()) {
            return std::nullopt;
        }

        std::string nextUserId = waitingUserIds.front();
        waitingUserIds.pop();
        return nextUserId;
    }
};

#endif
