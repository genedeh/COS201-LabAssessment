#ifndef SCAPMS_CONTROLLER_APPOINTMENT_CONTROLLER_H
#define SCAPMS_CONTROLLER_APPOINTMENT_CONTROLLER_H

#include <string>

#include "../model/AdminActionHistory.h"
#include "../model/ClinicDatabase.h"
#include "../model/WaitingQueue.h"
#include "../utils/ClinicException.h"
#include "../utils/DateUtils.h"
#include "../utils/IDGenerator.h"

namespace scapms::controller {

class AppointmentController {
private:
    model::AdminActionHistory& history;
    utils::IDGenerator& ids;

public:
    AppointmentController(model::AdminActionHistory& history, utils::IDGenerator& ids)
        : history(history), ids(ids) {}

    std::string schedule(model::ClinicDatabase& database, const std::string& patientId,
                         const std::string& dateTime, const std::string& reason) {
        model::PatientRecord* patient = database.findPatient(patientId);
        if (!patient) throw utils::ClinicException("Patient not found.");
        if (!utils::isValidDateTime(dateTime))
            throw utils::ClinicException("Enter a valid appointment date and time.");
        if (reason.empty()) throw utils::ClinicException("Appointment purpose is required.");
        for (const auto& current : database.getAppointments()) {
            if (current.getPatientId() == patientId && current.getDateTime() == dateTime &&
                current.getStatus() == model::AppointmentStatus::Pending)
                throw utils::ClinicException("This patient already has a pending appointment at that time.");
        }

        const std::string id = ids.nextAvailable("AP", [&database](const std::string& candidate) {
            return database.findAppointment(candidate) != nullptr;
        });
        model::Appointment appointment(id, patientId, dateTime, reason);
        if (!patient->scheduleAppointment(id))
            throw utils::ClinicException("This appointment ID is already linked to the patient.");
        if (!database.addAppointment(appointment)) {
            patient->cancelAppointment(id);
            throw utils::ClinicException("The appointment ID is already in use.");
        }
        history.record(model::AdminAction(model::AdminAction::Kind::AppointmentScheduled, id));
        return id;
    }

    void cancel(model::ClinicDatabase& database, const std::string& id) {
        model::Appointment* appointment = database.findAppointment(id);
        if (!appointment) throw utils::ClinicException("Appointment not found.");
        if (appointment->getStatus() != model::AppointmentStatus::Pending)
            throw utils::ClinicException("Only pending appointments can be cancelled.");
        const model::Appointment before = *appointment;
        appointment->setStatus(model::AppointmentStatus::Cancelled);
        if (auto* patient = database.findPatient(appointment->getPatientId()))
            patient->cancelAppointment(id);
        history.record(model::AdminAction(model::AdminAction::Kind::AppointmentCancelled,
                                          id, nullptr, before));
    }

    void complete(model::ClinicDatabase& database, const std::string& id) {
        model::Appointment* appointment = database.findAppointment(id);
        if (!appointment) throw utils::ClinicException("Appointment not found.");
        if (appointment->getStatus() != model::AppointmentStatus::Pending)
            throw utils::ClinicException("Only pending appointments can be completed.");
        const model::Appointment before = *appointment;
        appointment->setStatus(model::AppointmentStatus::Completed);
        history.record(model::AdminAction(model::AdminAction::Kind::AppointmentUpdated,
                                          id, nullptr, before));
    }

    void joinQueue(model::ClinicDatabase& database, model::WaitingQueue& queue,
                   const std::string& patientId) const {
        if (!database.findPatient(patientId)) throw utils::ClinicException("Patient not found.");
        if (!queue.enqueue(patientId)) throw utils::ClinicException("Patient is already in the waiting queue.");
    }
};

} // namespace scapms::controller

#endif
