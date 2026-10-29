#ifndef SCAPMS_MODEL_APPOINTABLE_H
#define SCAPMS_MODEL_APPOINTABLE_H

#include <string>

namespace scapms::model {

// Interface-like contract implemented by records that may hold appointments.
class Appointable {
public:
    virtual ~Appointable() = default;
    virtual bool scheduleAppointment(const std::string& appointmentId) = 0;
    virtual bool cancelAppointment(const std::string& appointmentId) = 0;
};

} // namespace scapms::model

#endif
