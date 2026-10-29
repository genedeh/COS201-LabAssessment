#ifndef SCAPMS_UTILS_ID_GENERATOR_H
#define SCAPMS_UTILS_ID_GENERATOR_H

#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>

namespace scapms::utils {

class IDGenerator {
private:
    std::unordered_map<std::string, unsigned int> nextValues;
public:
    std::string next(const std::string& prefix) {
        unsigned int& value = nextValues[prefix];
        if (value == 0) value = 1;
        std::ostringstream out;
        out << prefix << std::setw(4) << std::setfill('0') << value++;
        return out.str();
    }
};

} // namespace scapms::utils

#endif
