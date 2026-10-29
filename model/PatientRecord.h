#ifndef SCAPMS_MODEL_PATIENT_RECORD_H
#define SCAPMS_MODEL_PATIENT_RECORD_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Appointable.h"

namespace scapms::model {

// Shared patient identity and appointment links. Concrete patient categories
// provide their category name and a virtual clone for undo snapshots.
class PatientRecord : public Appointable {
private:
    std::string patientId;
    std::string name;
    int age;
    std::string contact;
    std::vector<std::string> appointmentIds;

protected:
    PatientRecord(std::string patientId, std::string name, int age,
                  std::string contact, std::vector<std::string> appointmentIds = {})
        : patientId(std::move(patientId)), name(std::move(name)), age(age),
          contact(std::move(contact)), appointmentIds(std::move(appointmentIds)) {}

public:
    virtual ~PatientRecord() = default;
    PatientRecord(const PatientRecord&) = default;
    PatientRecord& operator=(const PatientRecord&) = default;

    const std::string& getPatientId() const { return patientId; }
    const std::string& getName() const { return name; }
    int getAge() const { return age; }
    const std::string& getContact() const { return contact; }
    const std::vector<std::string>& getAppointmentIds() const { return appointmentIds; }
    void updateCommonDetails(std::string newName, int newAge, std::string newContact) {
        name = std::move(newName);
        age = newAge;
        contact = std::move(newContact);
    }

    bool scheduleAppointment(const std::string& appointmentId) override {
        for (const std::string& current : appointmentIds) {
            if (current == appointmentId) return false;
        }
        appointmentIds.push_back(appointmentId);
        return true;
    }

    bool cancelAppointment(const std::string& appointmentId) override {
        for (auto it = appointmentIds.begin(); it != appointmentIds.end(); ++it) {
            if (*it == appointmentId) {
                appointmentIds.erase(it);
                return true;
            }
        }
        return false;
    }

    virtual std::string category() const = 0;
    virtual std::string categoryDetail() const = 0;
    virtual std::unique_ptr<PatientRecord> clone() const = 0;
};

} // namespace scapms::model

#endif
