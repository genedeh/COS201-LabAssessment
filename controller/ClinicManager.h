#ifndef SCAPMS_CONTROLLER_CLINIC_MANAGER_H
#define SCAPMS_CONTROLLER_CLINIC_MANAGER_H

#include "AdminController.h"
#include "AppointmentController.h"
#include "PatientController.h"
#include "ReportController.h"
#include "SearchEngine.h"
#include "../model/ClinicDatabase.h"
#include "../model/WaitingQueue.h"
#include "../utils/IDGenerator.h"

namespace scapms::controller {

// Coordinates app-level objects; controllers remain independently testable.
class ClinicManager {
private:
    model::ClinicDatabase clinicDatabase;
    model::WaitingQueue consultationQueue;
    model::PatientAccessCache patientAccessCache;
    model::AdminActionHistory adminHistory;
    utils::IDGenerator idGenerator;
    PatientController patientsController{adminHistory};
    AppointmentController appointmentController{adminHistory, idGenerator};
    AdminController adminController;
    SearchEngine searchEngine;
    ReportController reportController;

public:
    model::ClinicDatabase& database() { return clinicDatabase; }
    const model::ClinicDatabase& database() const { return clinicDatabase; }
    model::WaitingQueue& waitingQueue() { return consultationQueue; }
    const model::WaitingQueue& waitingQueue() const { return consultationQueue; }
    model::PatientAccessCache& accessCache() { return patientAccessCache; }
    const model::PatientAccessCache& accessCache() const { return patientAccessCache; }
    model::AdminActionHistory& history() { return adminHistory; }
    PatientController& patients() { return patientsController; }
    AppointmentController& appointments() { return appointmentController; }
    AdminController& administration() { return adminController; }
    SearchEngine& search() { return searchEngine; }
    ReportController& reports() { return reportController; }
    utils::IDGenerator& ids() { return idGenerator; }
    void clearHistory() { adminHistory = model::AdminActionHistory{}; }
};

} // namespace scapms::controller

#endif
