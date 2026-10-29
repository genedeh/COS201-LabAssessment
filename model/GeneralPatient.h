#ifndef SCAPMS_MODEL_GENERAL_PATIENT_H
#define SCAPMS_MODEL_GENERAL_PATIENT_H

#include "PatientRecord.h"

namespace scapms::model {

class GeneralPatient final : public PatientRecord {
public:
    GeneralPatient(std::string id, std::string name, int age, std::string contact,
                   std::vector<std::string> appointmentIds = {})
        : PatientRecord(std::move(id), std::move(name), age, std::move(contact),
                        std::move(appointmentIds)) {}
    std::string category() const override { return "General"; }
    std::string categoryDetail() const override { return "Routine"; }
    std::unique_ptr<PatientRecord> clone() const override {
        return std::make_unique<GeneralPatient>(*this);
    }
};

} // namespace scapms::model

#endif
