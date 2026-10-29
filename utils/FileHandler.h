#ifndef SCAPMS_UTILS_FILE_HANDLER_H
#define SCAPMS_UTILS_FILE_HANDLER_H

#include <memory>
#include <cstddef>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QString>

#include "../model/ClinicDatabase.h"
#include "../model/EmergencyPatient.h"
#include "../model/FollowUpPatient.h"
#include "../model/GeneralPatient.h"
#include "../model/WaitingQueue.h"
#include "DateUtils.h"

namespace scapms::utils {

class FileHandler {
private:
    static std::string text(const QJsonValue& value) {
        const QByteArray bytes = value.toString().toUtf8();
        return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
    }
    static QString qstr(const std::string& value) { return QString::fromStdString(value); }

public:
    static bool save(const QString& path, const model::ClinicDatabase& database,
                     const model::WaitingQueue& queue, QString& error) {
        QJsonObject root; root.insert("version", 1);
        QJsonArray patients;
        for (const auto& patient : database.getPatients()) {
            QJsonObject item;
            item.insert("id", qstr(patient->getPatientId()));
            item.insert("name", qstr(patient->getName()));
            item.insert("age", patient->getAge());
            item.insert("contact", qstr(patient->getContact()));
            item.insert("category", qstr(patient->category()));
            QString detail = qstr(patient->categoryDetail());
            if (const auto* emergency = dynamic_cast<const model::EmergencyPatient*>(patient.get()))
                detail = QString::number(emergency->getPriorityLevel());
            item.insert("detail", detail);
            QJsonArray appointmentIds;
            for (const auto& id : patient->getAppointmentIds()) appointmentIds.append(qstr(id));
            item.insert("appointmentIds", appointmentIds);
            patients.append(item);
        }
        root.insert("patients", patients);

        QJsonArray appointments;
        for (const auto& appointment : database.getAppointments()) {
            QJsonObject item;
            item.insert("id", qstr(appointment.getId()));
            item.insert("patientId", qstr(appointment.getPatientId()));
            item.insert("dateTime", qstr(appointment.getDateTime()));
            item.insert("reason", qstr(appointment.getReason()));
            item.insert("status", qstr(model::appointmentStatusName(appointment.getStatus())));
            appointments.append(item);
        }
        root.insert("appointments", appointments);
        QJsonArray waiting;
        for (const auto& id : queue.toVector()) waiting.append(qstr(id));
        root.insert("waitingQueue", waiting);

        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) { error = file.errorString(); return false; }
        const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
        if (file.write(bytes) != bytes.size() || !file.commit()) {
            error = file.errorString(); return false;
        }
        return true;
    }

    static bool load(const QString& path, model::ClinicDatabase& database,
                     model::WaitingQueue& queue, QString& error) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) { error = file.errorString(); return false; }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            error = "Invalid JSON: " + parseError.errorString(); return false;
        }
        const QJsonObject root = document.object();
        if (root.value("version").toInt() != 1 || !root.value("patients").isArray() ||
            !root.value("appointments").isArray() || !root.value("waitingQueue").isArray()) {
            error = "This file is not a supported clinic data file."; return false;
        }

        model::ClinicDatabase loaded;
        model::WaitingQueue loadedQueue;
        for (const auto& value : root.value("patients").toArray()) {
            if (!value.isObject()) { error = "Invalid patient entry."; return false; }
            const QJsonObject item = value.toObject();
            const std::string id = text(item.value("id"));
            const std::string name = text(item.value("name"));
            const std::string contact = text(item.value("contact"));
            const std::string category = text(item.value("category"));
            const std::string detail = text(item.value("detail"));
            const int age = item.value("age").toInt(-1);
            if (id.empty() || name.empty() || contact.empty() || age < 0 || age > 120) {
                error = "Patient entry has missing or invalid identity details."; return false;
            }
            std::unique_ptr<model::PatientRecord> patient;
            if (category == "General") patient = std::make_unique<model::GeneralPatient>(id,name,age,contact);
            else if (category == "Emergency") {
                QString priority = QString::fromStdString(detail).trimmed();
                // Earlier saved files stored the display label, such as "Priority 1".
                if (priority.startsWith("Priority", Qt::CaseInsensitive))
                    priority = priority.mid(QString("Priority").size()).trimmed();
                bool ok = false; const int level = priority.toInt(&ok);
                if (!ok || level < 1 || level > 5) { error = "Invalid emergency priority."; return false; }
                patient = std::make_unique<model::EmergencyPatient>(id,name,age,contact,level);
            } else if (category == "Follow-up") patient = std::make_unique<model::FollowUpPatient>(id,name,age,contact,detail);
            else { error = "Unknown patient category."; return false; }
            if (!loaded.addPatient(std::move(patient))) { error = "Duplicate patient ID: " + QString::fromStdString(id); return false; }
        }

        for (const auto& value : root.value("appointments").toArray()) {
            if (!value.isObject()) { error = "Invalid appointment entry."; return false; }
            const QJsonObject item = value.toObject();
            const std::string id = text(item.value("id"));
            const std::string patientId = text(item.value("patientId"));
            const std::string dateTime = text(item.value("dateTime"));
            const std::string reason = text(item.value("reason"));
            const std::string status = text(item.value("status"));
            if (id.empty() || !loaded.findPatient(patientId) || !isValidDateTime(dateTime) || reason.empty()) {
                error = "Appointment entry contains invalid or missing data."; return false;
            }
            model::AppointmentStatus state;
            if (status == "Pending") state = model::AppointmentStatus::Pending;
            else if (status == "Completed") state = model::AppointmentStatus::Completed;
            else if (status == "Cancelled") state = model::AppointmentStatus::Cancelled;
            else { error = "Unknown appointment status."; return false; }
            if (!loaded.addAppointment(model::Appointment(id,patientId,dateTime,reason,state))) {
                error = "Duplicate appointment ID: " + QString::fromStdString(id); return false;
            }
        }
        // Appointment records are authoritative. Rebuild the patient-side links
        // so legacy saves with stale or misassigned IDs can still be recovered.
        for (const auto& appointment : loaded.getAppointments()) {
            auto* patient = loaded.findPatient(appointment.getPatientId());
            if (appointment.getStatus() != model::AppointmentStatus::Cancelled)
                patient->scheduleAppointment(appointment.getId());
        }
        std::unordered_set<std::string> queuedPatients;
        for (const auto& value : root.value("waitingQueue").toArray()) {
            if (!value.isString()) { error = "Waiting queue contains an invalid entry."; return false; }
            const std::string id = text(value);
            if (!loaded.findPatient(id)) { error = "Waiting queue refers to an unknown patient."; return false; }
            if (!queuedPatients.insert(id).second) { error = "Waiting queue contains a duplicate patient."; return false; }
            loadedQueue.enqueue(id);
        }
        database = std::move(loaded);
        queue = std::move(loadedQueue);
        return true;
    }
};

} // namespace scapms::utils

#endif
