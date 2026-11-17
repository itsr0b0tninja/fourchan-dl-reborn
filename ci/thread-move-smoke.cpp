#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QDebug>
#include "threadfilemover.h"

static bool writeFile(const QString &name, const QByteArray &contents) {
    if (!QDir().mkpath(QFileInfo(name).absolutePath())) return false;
    QFile f(name);
    return f.open(QIODevice::WriteOnly) && f.write(contents) == contents.size();
}
static QByteArray readFile(const QString &name) {
    QFile f(name);
    if (!f.open(QIODevice::ReadOnly)) return QByteArray();
    return f.readAll();
}
static ThreadMoveFile file(const QString &source, const QString &target) {
    ThreadMoveFile f;
    f.source = source;
    f.destination = target;
    return f;
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir temp;
    if (!temp.isValid()) return 1;

    const QString base = temp.filePath("old");
    const QString thread = base + "/b/956576597";
    const QString destinationRoot = temp.filePath("archive");
    QString error;
    const QString mapped = ThreadFileMover::mappedDirectory(
                base + "/%b/%n/", thread, destinationRoot, &error);
    if (!error.isEmpty() || mapped != QDir::cleanPath(destinationRoot + "/b/956576597"))
        return 2;

#ifdef Q_OS_WIN
    const QString oldWindows = QStringLiteral("D:\\zzzz\\/%b/%n/");
    const QString mappedWindows = ThreadFileMover::mappedDirectory(
                oldWindows, QStringLiteral("D:\\zzzz\\b\\956576597"),
                QStringLiteral("C:\\sgsgkjskjg\\"), &error);
    if (!error.isEmpty() ||
            QDir::fromNativeSeparators(mappedWindows) != QStringLiteral("C:/sgsgkjskjg/b/956576597"))
        return 3;
#endif

    const QString sourceImage = thread + "/image.png";
    const QString sourceVideo = thread + "/video.webm";
    const QString unrelated = thread + "/different-thread.png";
    const QString existingSource = thread + "/same.gif";
    const QString existingDestination = mapped + "/same.gif";
    if (!writeFile(sourceImage, QByteArray(500 * 1024, 'X'))
            || !writeFile(sourceVideo, QByteArray(900 * 1024, 'V'))
            || !writeFile(unrelated, "UNRELATED")
            || !writeFile(existingSource, "ALREADY")
            || !writeFile(existingDestination, "ALREADY"))
        return 4;

    QList<ThreadMoveFile> plan;
    plan << file(sourceImage, mapped + "/image.png")
         << file(sourceVideo, mapped + "/video.webm")
         << file(existingSource, existingDestination);
    ThreadFileMover worker(plan);
    worker.start();
    worker.wait();
    if (!worker.error().isEmpty()) {
        qWarning() << worker.error();
        return 5;
    }
    if (QFileInfo::exists(sourceImage) || QFileInfo::exists(sourceVideo)
            || QFileInfo::exists(existingSource) || !QFileInfo::exists(unrelated))
        return 6;
    if (readFile(mapped + "/image.png") != QByteArray(500 * 1024, 'X')
            || readFile(mapped + "/video.webm") != QByteArray(900 * 1024, 'V')
            || readFile(existingDestination) != "ALREADY")
        return 7;

    // Conflict must not overwrite the destination or remove the source.
    const QString conflictSource = thread + "/conflict.png";
    const QString conflictDestination = mapped + "/conflict.png";
    if (!writeFile(conflictSource, "ORIGINAL") || !writeFile(conflictDestination, "DIFFERENT"))
        return 8;
    QList<ThreadMoveFile> conflict;
    conflict << file(conflictSource, conflictDestination);
    ThreadFileMover conflicting(conflict);
    conflicting.start();
    conflicting.wait();
    if (conflicting.error().isEmpty() || readFile(conflictSource) != "ORIGINAL"
            || readFile(conflictDestination) != "DIFFERENT")
        return 9;

    // If the second copy cannot be created, roll back earlier new copies
    // without deleting either source.
    const QString rollbackA = thread + "/rollbackA.png";
    const QString rollbackB = thread + "/rollbackB.png";
    const QString blocked = temp.filePath("blocked");
    if (!writeFile(rollbackA, "A") || !writeFile(rollbackB, "B")
            || !writeFile(blocked, "NOT A DIRECTORY"))
        return 10;
    QList<ThreadMoveFile> rollback;
    rollback << file(rollbackA, mapped + "/rollbackA.png")
             << file(rollbackB, blocked + "/rollbackB.png");
    ThreadFileMover failing(rollback);
    failing.start();
    failing.wait();
    if (failing.error().isEmpty() || !QFileInfo::exists(rollbackA)
            || !QFileInfo::exists(rollbackB)
            || QFileInfo::exists(mapped + "/rollbackA.png"))
        return 11;

    qInfo() << "PASS: DLL folder mapping, copy+SHA256 verification, unchanged shared files,";
    qInfo() << "      existing identical destinations, conflicts and copy-stage rollback";
    return 0;
}
