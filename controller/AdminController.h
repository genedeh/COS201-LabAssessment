#ifndef SCAPMS_CONTROLLER_ADMIN_CONTROLLER_H
#define SCAPMS_CONTROLLER_ADMIN_CONTROLLER_H

#include "../model/AdminActionHistory.h"
#include "../model/ClinicDatabase.h"
#include "../model/WaitingQueue.h"
#include "../utils/ClinicException.h"

namespace scapms::controller {

class AdminController {
public:
    void undoLast(model::ClinicDatabase& database, model::WaitingQueue& waiting,
                  model::AdminActionHistory& history) const {
        auto action = history.takeLast();
        if (!action) throw utils::ClinicException("There is no administrative operation to undo.");
        switch (action->kind) {
        case model::AdminAction::Kind::PatientAdded:
            if (!database.removePatient(action->id))
                throw utils::ClinicException("The added patient could not be found for undo.");
            waiting.remove(action->id);
            break;
        case model::AdminAction::Kind::PatientDeleted:
            if (!action->patientBefore || !database.addPatient(std::move(action->patientBefore)))
                throw utils::ClinicException("The deleted patient could not be restored.");
            waiting.restore(action->waitingQueueBefore);
            break;
        case model::AdminAction::Kind::PatientEdited:
            if (!action->patientBefore || !database.replacePatient(std::move(action->patientBefore)))
                throw utils::ClinicException("The previous patient details could not be restored.");
            break;
        case model::AdminAction::Kind::AppointmentScheduled: {
            const auto* appointment = database.findAppointment(action->id);
            if (appointment) {
                if (auto* patient = database.findPatient(appointment->getPatientId()))
                    patient->cancelAppointment(action->id);
                database.removeAppointment(action->id);
            }
            break;
        }
        case model::AdminAction::Kind::AppointmentCancelled:
        case model::AdminAction::Kind::AppointmentUpdated:
            if (!action->appointmentBefore ||
                !database.restoreAppointment(*action->appointmentBefore))
                throw utils::ClinicException("The previous appointment could not be restored.");
            if (auto* patient = database.findPatient(action->appointmentBefore->getPatientId()))
                patient->scheduleAppointment(action->id);
            break;
        case model::AdminAction::Kind::QueueJoined:
        case model::AdminAction::Kind::QueueCalledNext:
            waiting.restore(action->waitingQueueBefore);
            break;
        }
    }
};

} // namespace scapms::controller

#endif
