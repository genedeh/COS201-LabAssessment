#ifndef SCAPMS_MODEL_FOLLOW_UP_PATIENT_H
#define SCAPMS_MODEL_FOLLOW_UP_PATIENT_H

#include "PatientRecord.h"

namespace scapms::model {

class FollowUpPatient final : public PatientRecord {
private:
    std::string followUpNote;

public:
    FollowUpPatient(std::string id, std::string name, int age, std::string contact,
                    std::string followUpNote = "Follow-up visit",
                    std::vector<std::string> appointmentIds = {})
        : PatientRecord(std::move(id), std::move(name), age, std::move(contact),
                        std::move(appointmentIds)), followUpNote(std::move(followUpNote)) {}
    const std::string& getFollowUpNote() const { return followUpNote; }
    std::string category() const override { return "Follow-up"; }
    std::string categoryDetail() const override { return followUpNote; }
    std::unique_ptr<PatientRecord> clone() const override {
        return std::make_unique<FollowUpPatient>(*this);
    }
};

} // namespace scapms::model

#endif
