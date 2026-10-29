#ifndef SCAPMS_MODEL_PATIENT_ACCESS_CACHE_H
#define SCAPMS_MODEL_PATIENT_ACCESS_CACHE_H

#include <array>
#include <cstddef>
#include <string>

namespace scapms::model {

// Fixed-size frequency cache required by the assessment.
class PatientAccessCache {
public:
    struct Entry { std::string patientId; std::size_t accesses = 0; };
private:
    std::array<Entry, 10> entries{};
    std::size_t count = 0;
    std::size_t replacementCursor = 0;
public:
    void record(const std::string& id) {
        for (std::size_t i = 0; i < count; ++i) {
            if (entries[i].patientId == id) { ++entries[i].accesses; return; }
        }
        if (count < entries.size()) entries[count++] = {id, 1};
        else { entries[replacementCursor] = {id, 1}; replacementCursor = (replacementCursor + 1) % entries.size(); }
    }
    std::size_t size() const { return count; }
    const Entry& at(std::size_t index) const { return entries.at(index); }
    void clear() { entries = {}; count = 0; replacementCursor = 0; }
};

} // namespace scapms::model

#endif
