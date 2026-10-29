#ifndef SCAPMS_CONTROLLER_REPORT_CONTROLLER_H
#define SCAPMS_CONTROLLER_REPORT_CONTROLLER_H

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "../model/ClinicDatabase.h"
#include "../model/PatientAccessCache.h"
#include "../model/WaitingQueue.h"

namespace scapms::controller {

struct FrequentPatient { std::string patientId; std::string name; std::size_t visits; };
struct PendingPatient { std::string patientId; std::string name; std::size_t appointments; };
struct CategoryCount { std::string category; std::size_t count; };
struct DailySummary { std::string date; std::size_t total=0, completed=0, pending=0, cancelled=0; };

class ReportController {
public:
    std::vector<FrequentPatient> mostFrequentlyAttended(
        const model::ClinicDatabase& database) const {
        std::unordered_map<std::string, std::size_t> counts;
        for (const auto& appointment : database.getAppointments())
            if (appointment.getStatus() == model::AppointmentStatus::Completed)
                ++counts[appointment.getPatientId()];
        std::vector<FrequentPatient> result;
        for (const auto& [id, count] : counts) {
            const auto* patient = database.findPatient(id);
            if (patient) result.push_back({id, patient->getName(), count});
        }
        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            return a.visits != b.visits ? a.visits > b.visits : a.patientId < b.patientId;
        });
        return result;
    }
    std::vector<PendingPatient> patientsWithPendingAppointments(
        const model::ClinicDatabase& database) const {
        std::unordered_map<std::string, std::size_t> counts;
        for (const auto& appointment : database.getAppointments())
            if (appointment.getStatus() == model::AppointmentStatus::Pending)
                ++counts[appointment.getPatientId()];
        std::vector<PendingPatient> result;
        for (const auto& [id, count] : counts) {
            const auto* patient = database.findPatient(id);
            if (patient) result.push_back({id, patient->getName(), count});
        }
        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            return a.name < b.name;
        });
        return result;
    }
    std::vector<CategoryCount> categoryDistribution(
        const model::ClinicDatabase& database) const {
        std::map<std::string, std::size_t> counts;
        for (const auto& patient : database.getPatients()) ++counts[patient->category()];
        std::vector<CategoryCount> result;
        for (const auto& [category, count] : counts) result.push_back({category, count});
        return result;
    }
    DailySummary dailySummary(const model::ClinicDatabase& database,
                              const std::string& date) const {
        DailySummary summary; summary.date = date;
        for (const auto& appointment : database.getAppointments()) {
            if (appointment.getDateTime().substr(0, 10) != date) continue;
            ++summary.total;
            switch (appointment.getStatus()) {
            case model::AppointmentStatus::Completed: ++summary.completed; break;
            case model::AppointmentStatus::Pending: ++summary.pending; break;
            case model::AppointmentStatus::Cancelled: ++summary.cancelled; break;
            }
        }
        return summary;
    }
    std::vector<model::PatientAccessCache::Entry> frequentAccesses(
        const model::PatientAccessCache& cache) const {
        std::vector<model::PatientAccessCache::Entry> result;
        for (std::size_t i = 0; i < cache.size(); ++i) result.push_back(cache.at(i));
        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            return a.accesses != b.accesses ? a.accesses > b.accesses : a.patientId < b.patientId;
        });
        return result;
    }
};

} // namespace scapms::controller

#endif
