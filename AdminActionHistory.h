#ifndef ADMIN_ACTION_HISTORY_H
#define ADMIN_ACTION_HISTORY_H

#include <cstddef>
#include <optional>
#include <stack>

#include "AdminAction.h"

// Stores admin actions so the newest one can be undone first (LIFO).
class AdminActionHistory {
private:
    std::stack<AdminAction> actions;

public:
    void recordAction(const AdminAction& action) {
        actions.push(action);
    }

    std::size_t getActionCount() const {
        return actions.size();
    }

    // Removes and returns the newest action, or no value if there are none.
    std::optional<AdminAction> takeMostRecentAction() {
        if (actions.empty()) {
            return std::nullopt;
        }

        AdminAction action = actions.top();
        actions.pop();
        return action;
    }
};

#endif
