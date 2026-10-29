#ifndef SCAPMS_UTILS_CLINIC_EXCEPTION_H
#define SCAPMS_UTILS_CLINIC_EXCEPTION_H

#include <stdexcept>
#include <string>

namespace scapms::utils {

class ClinicException : public std::runtime_error {
public:
    explicit ClinicException(const std::string& message) : std::runtime_error(message) {}
};

} // namespace scapms::utils

#endif
