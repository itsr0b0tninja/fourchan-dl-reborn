#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include "retryafterpolicy.h"
#include "downloadrequest.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    const QDateTime now(QDate(2026, 11, 17), QTime(19, 0, 0), Qt::UTC);
    if (RetryAfterPolicy::delaySeconds("3275", 60, now) != 3277) return 1;
    if (RetryAfterPolicy::delaySeconds("0", 60, now) != 2) return 2;
    if (RetryAfterPolicy::delaySeconds("999999", 60, now) != 86400) return 3;
    if (RetryAfterPolicy::delaySeconds("garbage", 60, now) != 60) return 4;
    const QByteArray date = now.addSecs(95).toString(Qt::RFC2822Date).toLatin1();
    if (RetryAfterPolicy::delaySeconds(date, 60, now) != 97) return 5;
    DownloadRequest request;
    request.download_error(0, 1);
    if (request.error_count() != 0 || !request.paused()) return 6;
    request.download_error(1, 1);
    if (request.error_count() != 1) return 7;
    qInfo() << "PASS: 429 Retry-After time, date, cap, retry budget";
    return 0;
}
