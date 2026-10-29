#ifndef SCAPMS_MODEL_ADMIN_ACTION_HISTORY_H
#define SCAPMS_MODEL_ADMIN_ACTION_HISTORY_H

#include <optional>
#include <stack>
#include "AdminAction.h"

namespace scapms::model {

class AdminActionHistory {
private:
    std::stack<AdminAction> actions;
public:
    void record(AdminAction action) { actions.push(std::move(action)); }
    std::optional<AdminAction> takeLast() {
        if (actions.empty()) return std::nullopt;
        AdminAction action = std::move(actions.top());
        actions.pop();
        return action;
    }
    std::size_t size() const { return actions.size(); }
    bool empty() const { return actions.empty(); }
};

} // namespace scapms::model

#endif
