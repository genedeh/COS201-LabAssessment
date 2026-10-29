#ifndef SCAPMS_MODEL_WAITING_QUEUE_H
#define SCAPMS_MODEL_WAITING_QUEUE_H

#include <algorithm>
#include <queue>
#include <string>
#include <utility>
#include <vector>

namespace scapms::model {

// Patient IDs enter at the back and are called from the front (FIFO).
class WaitingQueue {
private:
    std::queue<std::string> patients;

public:
    bool enqueue(const std::string& patientId) {
        if (contains(patientId)) return false;
        patients.push(patientId);
        return true;
    }
    bool contains(const std::string& id) const {
        auto copy = patients;
        while (!copy.empty()) {
            if (copy.front() == id) return true;
            copy.pop();
        }
        return false;
    }
    bool remove(const std::string& id) {
        std::queue<std::string> retained;
        bool removed = false;
        while (!patients.empty()) {
            const std::string current = patients.front();
            patients.pop();
            if (current == id && !removed) removed = true;
            else retained.push(current);
        }
        patients = std::move(retained);
        return removed;
    }
    std::string callNext() {
        if (patients.empty()) return {};
        const std::string next = patients.front();
        patients.pop();
        return next;
    }
    std::vector<std::string> toVector() const {
        std::vector<std::string> result;
        auto copy = patients;
        while (!copy.empty()) {
            result.push_back(copy.front());
            copy.pop();
        }
        return result;
    }
    void restore(const std::vector<std::string>& ids) {
        patients = {};
        for (const auto& id : ids) enqueue(id);
    }
    std::size_t size() const { return patients.size(); }
    bool empty() const { return patients.empty(); }
};

} // namespace scapms::model

#endif
