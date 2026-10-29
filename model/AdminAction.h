#ifndef SCAPMS_MODEL_ADMIN_ACTION_H
#define SCAPMS_MODEL_ADMIN_ACTION_H

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Appointment.h"
#include "PatientRecord.h"

namespace scapms::model {

struct AdminAction {
    enum class Kind { PatientAdded, PatientDeleted, PatientEdited,
                      AppointmentScheduled, AppointmentCancelled, AppointmentUpdated,
                      QueueJoined, QueueCalledNext };
    Kind kind;
    std::string id;
    std::unique_ptr<PatientRecord> patientBefore;
    std::optional<Appointment> appointmentBefore;
    std::vector<std::string> waitingQueueBefore;

    AdminAction(Kind kind, std::string id,
                std::unique_ptr<PatientRecord> patientBefore = nullptr,
                std::optional<Appointment> appointmentBefore = std::nullopt,
                std::vector<std::string> waitingQueueBefore = {})
        : kind(kind), id(std::move(id)), patientBefore(std::move(patientBefore)),
          appointmentBefore(std::move(appointmentBefore)),
          waitingQueueBefore(std::move(waitingQueueBefore)) {}
    AdminAction(AdminAction&&) noexcept = default;
    AdminAction& operator=(AdminAction&&) noexcept = default;
    AdminAction(const AdminAction&) = delete;
    AdminAction& operator=(const AdminAction&) = delete;
};

} // namespace scapms::model

#endif
