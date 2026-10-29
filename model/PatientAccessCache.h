#ifndef SCAPMS_MODEL_PATIENT_ACCESS_CACHE_H
#define SCAPMS_MODEL_PATIENT_ACCESS_CACHE_H

#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>

namespace scapms::model {

// Fixed-size frequency cache required by the assessment.
class PatientAccessCache {
public:
    struct Entry { std::string patientId; std::size_t accesses = 0; };
private:
    std::array<Entry, 10> entries{};
    std::size_t count = 0;
    std::unordered_map<std::string, std::size_t> totalAccesses;

    static bool ranksBefore(const Entry& left, const Entry& right) {
        return left.accesses != right.accesses
            ? left.accesses > right.accesses
            : left.patientId < right.patientId;
    }

    void rebuildTopTen() {
        entries = {};
        count = 0;
        for (const auto& [patientId, accesses] : totalAccesses) {
            const Entry candidate{patientId, accesses};
            std::size_t position = 0;
            while (position < count && !ranksBefore(candidate, entries[position]))
                ++position;
            if (position >= entries.size()) continue;

            const std::size_t lastPosition = count < entries.size()
                ? count
                : entries.size() - 1;
            for (std::size_t i = lastPosition; i > position; --i)
                entries[i] = entries[i - 1];
            entries[position] = candidate;
            if (count < entries.size()) ++count;
        }
    }

public:
    void record(const std::string& id) {
        ++totalAccesses[id];
        rebuildTopTen();
    }
    std::size_t size() const { return count; }
    const Entry& at(std::size_t index) const { return entries.at(index); }
    void clear() { entries = {}; count = 0; totalAccesses.clear(); }
};

} // namespace scapms::model

#endif
