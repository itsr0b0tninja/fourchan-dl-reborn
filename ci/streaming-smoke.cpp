#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDebug>
#include <QByteArray>
#include "downloadrequest.h"

// Exercise the actual DownloadRequest temporary-file implementation. The final
// filename must never be visible before commit or after an aborted transfer.
int main(int argc, char* argv[]) {
    QCoreApplication application(argc, argv);
    QTemporaryDir dir;
    if (!dir.isValid()) return 1;

    const QString completedPath = dir.filePath("complete.webm");
    DownloadRequest request;
    request.setSavePath(completedPath);
    if (!request.beginStreaming()) return 2;
    const QByteArray chunk(64 * 1024, 'X');
    for (int n=0; n<32; ++n) {
        if (!request.appendStreaming(chunk)) return 3;
    }
    if (request.streamedBytes() != qint64(chunk.size()) * 32) return 4;
    if (QFile::exists(completedPath)) return 5;
    if (!request.commitStreaming()) return 6;

    QFile completed(completedPath);
    if (!completed.open(QIODevice::ReadOnly)) return 7;
    if (completed.size() != qint64(chunk.size()) * 32) return 8;
    if (completed.read(8) != QByteArray(8, 'X')) return 9;
    completed.close();

    const QString interruptedPath = dir.filePath("interrupted.mp4");
    request.setSavePath(interruptedPath);
    if (!request.beginStreaming()) return 10;
    if (!request.appendStreaming(chunk)) return 11;
    request.abortStreaming();
    if (QFile::exists(interruptedPath)) return 12;

    request.download_error(1, 1);
    request.reset();
    if (request.error_count() != 1) return 13;
    if (request.streamedBytes() != 0) return 14;
    qInfo() << "PASS: progressive writing, atomic completion, interrupted cleanup, retry count";
    return 0;
}
