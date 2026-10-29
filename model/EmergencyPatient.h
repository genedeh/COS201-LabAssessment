#ifndef SCAPMS_MODEL_EMERGENCY_PATIENT_H
#define SCAPMS_MODEL_EMERGENCY_PATIENT_H

#include "PatientRecord.h"

namespace scapms::model {

class EmergencyPatient final : public PatientRecord {
private:
    int priorityLevel;

public:
    EmergencyPatient(std::string id, std::string name, int age, std::string contact,
                     int priorityLevel = 1,
                     std::vector<std::string> appointmentIds = {})
        : PatientRecord(std::move(id), std::move(name), age, std::move(contact),
                        std::move(appointmentIds)), priorityLevel(priorityLevel) {}
    int getPriorityLevel() const { return priorityLevel; }
    std::string category() const override { return "Emergency"; }
    std::string categoryDetail() const override {
        return "Priority " + std::to_string(priorityLevel);
    }
    std::unique_ptr<PatientRecord> clone() const override {
        return std::make_unique<EmergencyPatient>(*this);
    }
};

} // namespace scapms::model

#endif
