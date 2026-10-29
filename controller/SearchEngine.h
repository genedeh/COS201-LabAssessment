#ifndef SCAPMS_CONTROLLER_SEARCH_ENGINE_H
#define SCAPMS_CONTROLLER_SEARCH_ENGINE_H

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "../model/ClinicDatabase.h"

namespace scapms::controller {

enum class SearchMethod { LinearId, BinaryId, RecursiveName, Category };
enum class SortAlgorithm { Insertion, Selection };
enum class SortField { Name, PatientId, Age, AppointmentDate };

class SearchEngine {
private:
    static std::string lower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }
    static bool before(const model::PatientRecord* a, const model::PatientRecord* b,
                       SortField field, const model::ClinicDatabase& database) {
        if (field == SortField::PatientId) return a->getPatientId() < b->getPatientId();
        if (field == SortField::Age) return a->getAge() < b->getAge();
        if (field == SortField::AppointmentDate) {
            std::string dateA, dateB;
            for (const auto& appt : database.getAppointments()) {
                if (appt.getPatientId() == a->getPatientId() && appt.getStatus() == model::AppointmentStatus::Pending &&
                    (dateA.empty() || appt.getDateTime() < dateA)) dateA = appt.getDateTime();
                if (appt.getPatientId() == b->getPatientId() && appt.getStatus() == model::AppointmentStatus::Pending &&
                    (dateB.empty() || appt.getDateTime() < dateB)) dateB = appt.getDateTime();
            }
            if (dateA.empty()) dateA = "9999";
            if (dateB.empty()) dateB = "9999";
            return dateA < dateB;
        }
        return lower(a->getName()) < lower(b->getName());
    }
    std::vector<const model::PatientRecord*> recursiveName(
        const model::ClinicDatabase& database, const std::string& query,
        std::size_t index, std::vector<const model::PatientRecord*> matches) const {
        if (index >= database.patientCount()) return matches;
        const auto* patient = database.getPatients()[index].get();
        if (lower(patient->getName()).find(query) != std::string::npos) matches.push_back(patient);
        return recursiveName(database, query, index + 1, std::move(matches));
    }

public:
    std::vector<const model::PatientRecord*> linearId(
        const model::ClinicDatabase& database, const std::string& id) const {
        for (const auto& patient : database.getPatients())
            if (patient->getPatientId() == id) return {patient.get()};
        return {};
    }
    std::vector<const model::PatientRecord*> binaryId(
        const model::ClinicDatabase& database, const std::string& id) const {
        auto sorted = sortedPatients(database, SortField::PatientId, SortAlgorithm::Insertion);
        std::size_t first = 0, last = sorted.size();
        while (first < last) {
            const std::size_t middle = first + (last - first) / 2;
            const std::string& candidate = sorted[middle]->getPatientId();
            if (candidate == id) return {sorted[middle]};
            if (candidate < id) first = middle + 1; else last = middle;
        }
        return {};
    }
    std::vector<const model::PatientRecord*> recursiveNameSearch(
        const model::ClinicDatabase& database, std::string query) const {
        return recursiveName(database, lower(std::move(query)), 0, {});
    }
    std::vector<const model::PatientRecord*> categorySearch(
        const model::ClinicDatabase& database, const std::string& category) const {
        std::vector<const model::PatientRecord*> result;
        const std::string target = lower(category);
        for (const auto& patient : database.getPatients())
            if (lower(patient->category()) == target) result.push_back(patient.get());
        return result;
    }
    std::vector<const model::PatientRecord*> sortedPatients(
        const model::ClinicDatabase& database, SortField field,
        SortAlgorithm algorithm) const {
        std::vector<const model::PatientRecord*> result;
        for (const auto& patient : database.getPatients()) result.push_back(patient.get());
        if (algorithm == SortAlgorithm::Insertion) {
            for (std::size_t i = 1; i < result.size(); ++i) {
                const auto* key = result[i];
                std::size_t j = i;
                while (j > 0 && before(key, result[j - 1], field, database)) {
                    result[j] = result[j - 1];
                    --j;
                }
                result[j] = key;
            }
        } else {
            for (std::size_t start = 0; start + 1 < result.size(); ++start) {
                std::size_t smallest = start;
                for (std::size_t j = start + 1; j < result.size(); ++j)
                    if (before(result[j], result[smallest], field, database)) smallest = j;
                std::swap(result[start], result[smallest]);
            }
        }
        return result;
    }
};

} // namespace scapms::controller

#endif
