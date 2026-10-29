#ifndef SCAPMS_MODEL_APPOINTMENT_H
#define SCAPMS_MODEL_APPOINTMENT_H

#include <string>
#include <utility>

namespace scapms::model {

enum class AppointmentStatus { Pending, Completed, Cancelled };

inline std::string appointmentStatusName(AppointmentStatus status) {
    switch (status) {
    case AppointmentStatus::Pending: return "Pending";
    case AppointmentStatus::Completed: return "Completed";
    case AppointmentStatus::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

// An appointment links a patient to a date, time, purpose, and lifecycle state.
class Appointment {
private:
    std::string id;
    std::string patientId;
    std::string dateTime; // ISO format: YYYY-MM-DD HH:MM
    std::string reason;
    AppointmentStatus status = AppointmentStatus::Pending;

public:
    Appointment(std::string id, std::string patientId, std::string dateTime,
                std::string reason,
                AppointmentStatus status = AppointmentStatus::Pending)
        : id(std::move(id)), patientId(std::move(patientId)),
          dateTime(std::move(dateTime)), reason(std::move(reason)), status(status) {}

    const std::string& getId() const { return id; }
    const std::string& getPatientId() const { return patientId; }
    const std::string& getDateTime() const { return dateTime; }
    const std::string& getReason() const { return reason; }
    AppointmentStatus getStatus() const { return status; }
    void setStatus(AppointmentStatus value) { status = value; }
};

} // namespace scapms::model

#endif
