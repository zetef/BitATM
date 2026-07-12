#pragma once
#include <QDateTime>
#include <QString>
#include <QTimeZone>

/**
 * @brief Wire timestamp normalization helpers.
 *
 * Live packets carry client-generated ISO strings ("...T...Z") while server
 * replays carry PostgreSQL text ("YYYY-MM-DD HH:MM:SS.f", naive UTC, trailing
 * zeros trimmed). Message dedup and read-receipt matching compare timestamps
 * as strings, so every ingress must canonicalize first.
 */
namespace TimestampUtil {

/**
 * @brief Normalize a wire timestamp to canonical Qt::ISODateWithMs UTC.
 *
 * Accepts ISO 8601 (with or without zone designator) and PostgreSQL text
 * format. Naive timestamps are treated as UTC. Unparseable input is returned
 * unchanged.
 */
inline QString canonical(const QString& raw) {
    QString s = raw.trimmed();
    if (s.size() > 10 && s.at(10) == QLatin1Char(' ')) s[10] = QLatin1Char('T');
    QDateTime dt = QDateTime::fromString(s, Qt::ISODateWithMs);
    if (!dt.isValid()) return raw;
    // PostgreSQL text has no zone suffix; those instants are UTC already
    if (dt.timeSpec() == Qt::LocalTime) dt = QDateTime(dt.date(), dt.time(), QTimeZone::utc());
    return dt.toUTC().toString(Qt::ISODateWithMs);
}

}  // namespace TimestampUtil
