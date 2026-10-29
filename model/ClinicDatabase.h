#ifndef SCAPMS_MODEL_CLINIC_DATABASE_H
#define SCAPMS_MODEL_CLINIC_DATABASE_H

#include <memory>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "Appointment.h"
#include "PatientRecord.h"

namespace scapms::model {

// Owns patient and appointment values. Other components refer to them by ID.
class ClinicDatabase {
private:
    std::vector<std::unique_ptr<PatientRecord>> patients;
    std::vector<Appointment> appointments;

public:
    bool addPatient(std::unique_ptr<PatientRecord> patient) {
        if (!patient || findPatient(patient->getPatientId())) return false;
        patients.push_back(std::move(patient));
        return true;
    }
    PatientRecord* findPatient(const std::string& id) {
        for (auto& patient : patients)
            if (patient->getPatientId() == id) return patient.get();
        return nullptr;
    }
    const PatientRecord* findPatient(const std::string& id) const {
        for (const auto& patient : patients)
            if (patient->getPatientId() == id) return patient.get();
        return nullptr;
    }
    std::unique_ptr<PatientRecord> removePatient(const std::string& id) {
        for (auto it = patients.begin(); it != patients.end(); ++it) {
            if ((*it)->getPatientId() == id) {
                auto removed = std::move(*it);
                patients.erase(it);
                return removed;
            }
        }
        return nullptr;
    }
    bool replacePatient(std::unique_ptr<PatientRecord> patient) {
        if (!patient) return false;
        for (auto& existing : patients) {
            if (existing->getPatientId() == patient->getPatientId()) {
                existing = std::move(patient);
                return true;
            }
        }
        return false;
    }
    const std::vector<std::unique_ptr<PatientRecord>>& getPatients() const { return patients; }
    std::size_t patientCount() const { return patients.size(); }

    bool addAppointment(const Appointment& appointment) {
        if (findAppointment(appointment.getId())) return false;
        appointments.push_back(appointment);
        return true;
    }
    Appointment* findAppointment(const std::string& id) {
        for (auto& appointment : appointments)
            if (appointment.getId() == id) return &appointment;
        return nullptr;
    }
    const Appointment* findAppointment(const std::string& id) const {
        for (const auto& appointment : appointments)
            if (appointment.getId() == id) return &appointment;
        return nullptr;
    }
    bool removeAppointment(const std::string& id) {
        for (auto it = appointments.begin(); it != appointments.end(); ++it) {
            if (it->getId() == id) {
                appointments.erase(it);
                return true;
            }
        }
        return false;
    }
    bool restoreAppointment(const Appointment& appointment) {
        for (auto& current : appointments) {
            if (current.getId() == appointment.getId()) {
                current = appointment;
                return true;
            }
        }
        return addAppointment(appointment);
    }
    const std::vector<Appointment>& getAppointments() const { return appointments; }
    std::vector<Appointment>& getAppointments() { return appointments; }
    void clear() { patients.clear(); appointments.clear(); }
};

} // namespace scapms::model

#endif
