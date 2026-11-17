#ifndef RETRYAFTERPOLICY_H
#define RETRYAFTERPOLICY_H

#include <QByteArray>
#include <QDateTime>
#include <QtGlobal>

// Honour delta-seconds or HTTP-date Retry-After; add two seconds safety margin.
namespace RetryAfterPolicy {
static inline int delaySeconds(const QByteArray &raw, int fallbackSeconds,
                               const QDateTime &nowUtc = QDateTime::currentDateTimeUtc()) {
    const QByteArray value = raw.trimmed();
    bool valid = false;
    const qint64 seconds = value.toLongLong(&valid, 10);
    if (valid && seconds >= 0)
        return static_cast<int>(qBound<qint64>(2, qMin<qint64>(seconds, 86398) + 2, 86400));
    const QDateTime until = QDateTime::fromString(QString::fromLatin1(value), Qt::RFC2822Date);
    if (until.isValid()) {
        const qint64 ms = nowUtc.msecsTo(until.toUTC());
        const qint64 remaining = ms > 0 ? (ms + 999) / 1000 : 0;
        return static_cast<int>(qBound<qint64>(2, remaining + 2, 86400));
    }
    return qBound(60, fallbackSeconds, 3600);
}
}
#endif
