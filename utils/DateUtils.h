#ifndef SCAPMS_UTILS_DATE_UTILS_H
#define SCAPMS_UTILS_DATE_UTILS_H

#include <QDateTime>
#include <QString>

#include <string>

namespace scapms::utils {

inline bool isValidDateTime(const std::string& value) {
    const QString text = QString::fromStdString(value);
    const QDateTime parsed = QDateTime::fromString(text, "yyyy-MM-dd HH:mm");
    return parsed.isValid() && parsed.toString("yyyy-MM-dd HH:mm") == text;
}

} // namespace scapms::utils

#endif
