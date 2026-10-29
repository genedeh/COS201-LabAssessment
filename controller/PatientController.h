#ifndef SCAPMS_CONTROLLER_PATIENT_CONTROLLER_H
#define SCAPMS_CONTROLLER_PATIENT_CONTROLLER_H

#include <memory>
#include <string>

#include "../model/AdminActionHistory.h"
#include "../model/ClinicDatabase.h"
#include "../model/EmergencyPatient.h"
#include "../model/FollowUpPatient.h"
#include "../model/GeneralPatient.h"
#include "../model/WaitingQueue.h"
#include "../utils/ClinicException.h"

namespace scapms::controller {

class PatientController {
private:
    model::AdminActionHistory& history;

    static std::unique_ptr<model::PatientRecord> makePatient(
        const std::string& category, const std::string& id,
        const std::string& name, int age, const std::string& contact,
        const std::string& detail = "") {
        if (id.empty() || name.empty() || contact.empty())
            throw utils::ClinicException("Patient ID, name, and contact are required.");
        if (age < 0 || age > 120)
            throw utils::ClinicException("Patient age must be between 0 and 120.");
        if (category == "General")
            return std::make_unique<model::GeneralPatient>(id, name, age, contact);
        if (category == "Emergency") {
            int level = 1;
            try { if (!detail.empty()) level = std::stoi(detail); }
            catch (...) { throw utils::ClinicException("Emergency priority must be a number from 1 to 5."); }
            if (level < 1 || level > 5)
                throw utils::ClinicException("Emergency priority must be from 1 to 5.");
            return std::make_unique<model::EmergencyPatient>(id, name, age, contact, level);
        }
        if (category == "Follow-up")
            return std::make_unique<model::FollowUpPatient>(id, name, age, contact,
                detail.empty() ? "Follow-up visit" : detail);
        throw utils::ClinicException("Choose a valid patient category.");
    }

public:
    explicit PatientController(model::AdminActionHistory& history) : history(history) {}

    void add(model::ClinicDatabase& database, const std::string& category,
             const std::string& id, const std::string& name, int age,
             const std::string& contact, const std::string& detail = "") {
        auto patient = makePatient(category, id, name, age, contact, detail);
        const std::string patientId = patient->getPatientId();
        if (!database.addPatient(std::move(patient)))
            throw utils::ClinicException("That patient ID is already registered.");
        history.record(model::AdminAction(model::AdminAction::Kind::PatientAdded, patientId));
    }

    void edit(model::ClinicDatabase& database, const std::string& id,
              const std::string& name, int age, const std::string& contact) {
        model::PatientRecord* patient = database.findPatient(id);
        if (!patient) throw utils::ClinicException("Patient not found.");
        if (name.empty() || contact.empty())
            throw utils::ClinicException("Patient name and contact are required.");
        if (age < 0 || age > 120)
            throw utils::ClinicException("Patient age must be between 0 and 120.");
        auto before = patient->clone();
        patient->updateCommonDetails(name, age, contact);
        history.record(model::AdminAction(model::AdminAction::Kind::PatientEdited,
                                          id, std::move(before)));
    }

    void remove(model::ClinicDatabase& database, model::WaitingQueue& waiting,
                const std::string& id) {
        model::PatientRecord* patient = database.findPatient(id);
        if (!patient) throw utils::ClinicException("Patient not found.");
        for (const auto& appointment : database.getAppointments())
            if (appointment.getPatientId() == id)
                throw utils::ClinicException("This patient has appointment history. Keep the record so appointment reports remain accurate.");
        auto snapshot = patient->clone();
        const auto queueSnapshot = waiting.toVector();
        database.removePatient(id);
        waiting.remove(id);
        history.record(model::AdminAction(model::AdminAction::Kind::PatientDeleted,
                                          id, std::move(snapshot), std::nullopt,
                                          queueSnapshot));
    }
};

} // namespace scapms::controller

#endif
