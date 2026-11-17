#include "threadfilemover.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QSaveFile>
#include <QSet>
#include <QStringList>

ThreadFileMover::ThreadFileMover(const QList<ThreadMoveFile> &files, QObject *parent)
    : QThread(parent), transfers(files)
{
}

QString ThreadFileMover::mappedDirectory(const QString &pattern,
                                          const QString &resolvedSource,
                                          const QString &destinationRoot,
                                          QString *error)
{
    const QString original = QDir::fromNativeSeparators(pattern.trimmed());
    const QString source = QDir::cleanPath(QDir::fromNativeSeparators(resolvedSource.trimmed()));
    const QString targetRoot = QDir::cleanPath(QDir::fromNativeSeparators(destinationRoot.trimmed()));

    if (original.isEmpty() || !QDir::isAbsolutePath(source)
            || !QDir::isAbsolutePath(targetRoot)) {
        if (error) *error = QStringLiteral("Both the source and destination must be absolute folders.");
        return QString();
    }

    // The DLL expands %b, %n, %h (and %a). Locate the parent of the first
    // placeholder, not the last folder, so nested DLL paths are preserved.
    const int token = QRegExp("%[nbha]", Qt::CaseInsensitive).indexIn(original);
    QString sourceRoot;
    if (token >= 0) {
        const int slash = original.lastIndexOf('/', token);
        if (slash < 0) {
            if (error) *error = QStringLiteral("Cannot determine the root of this thread's folder template.");
            return QString();
        }
        sourceRoot = QDir::cleanPath(original.left(slash + 1));
    } else {
        // A literal thread folder retains its leaf name under the new root.
        sourceRoot = QFileInfo(source).absolutePath();
    }

    if (!QDir::isAbsolutePath(sourceRoot)) {
        if (error) *error = QStringLiteral("The thread's source folder is not absolute.");
        return QString();
    }

    const QString suffix = QDir(sourceRoot).relativeFilePath(source);
    const QString normalizedSuffix = QDir::cleanPath(suffix);
    if (normalizedSuffix.isEmpty() || normalizedSuffix == "."
            || normalizedSuffix == ".." || normalizedSuffix.startsWith("../")
            || QDir::isAbsolutePath(normalizedSuffix)) {
        if (error) *error = QStringLiteral("The DLL folder suffix could not be mapped safely.");
        return QString();
    }

    const QString target = QDir::cleanPath(QDir(targetRoot).filePath(normalizedSuffix));
#ifdef Q_OS_WIN
    const Qt::CaseSensitivity comparison = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity comparison = Qt::CaseSensitive;
#endif
    if (source.compare(target, comparison) == 0) {
        if (error) *error = QStringLiteral("The move destination is the current thread folder.");
        return QString();
    }
    if (target.startsWith(source + "/", comparison)) {
        if (error) *error = QStringLiteral("The destination cannot be inside the thread's source folder.");
        return QString();
    }
    if (error) error->clear();
    return target;
}

QByteArray ThreadFileMover::fileDigest(const QString &filename, QString *error)
{
    QFile input(filename);
    if (!input.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("Cannot read file: %1").arg(filename);
        return QByteArray();
    }
    QCryptographicHash sha(QCryptographicHash::Sha256);
    QByteArray buffer;
    while (!(buffer = input.read(256 * 1024)).isEmpty())
        sha.addData(buffer);
    if (input.error() != QFile::NoError) {
        if (error) *error = QStringLiteral("Read failed: %1").arg(filename);
        return QByteArray();
    }
    return sha.result();
}

void ThreadFileMover::run()
{
    failure.clear();
    QStringList created;
    QSet<QString> destinations;

    // Preflight every file before writing or deleting anything.
    for (const ThreadMoveFile &item : transfers) {
        const QFileInfo source(item.source);
        const QFileInfo destination(item.destination);
        if (item.source == item.destination || destinations.contains(item.destination)) {
            failure = QStringLiteral("Duplicate or identical move destination: %1").arg(item.destination);
            return;
        }
        destinations.insert(item.destination);
        if (!source.exists()) {
            // The user may have deliberately deleted this file earlier.
            continue;
        }
        if (!source.isFile() || source.isSymLink() || destination.isSymLink()
                || (destination.exists() && !destination.isFile())) {
            failure = QStringLiteral("Unsupported file or conflicting destination: %1").arg(item.source);
            return;
        }
        if (destination.exists()) {
            QString readError;
            if (source.size() != destination.size()
                    || fileDigest(item.source, &readError) != fileDigest(item.destination, &readError)
                    || !readError.isEmpty()) {
                failure = QStringLiteral("Destination already contains a different file: %1")
                              .arg(item.destination);
                return;
            }
        }
    }

    // Phase one: copy all files atomically and verify every copy. Originals
    // remain in place until every destination is known to be correct.
    for (const ThreadMoveFile &item : transfers) {
        if (!QFileInfo::exists(item.source) || QFileInfo::exists(item.destination))
            continue;
        const QString parentFolder = QFileInfo(item.destination).absolutePath();
        if (!QDir().mkpath(parentFolder)) {
            failure = QStringLiteral("Cannot create destination folder: %1").arg(parentFolder);
            break;
        }
        QFile input(item.source);
        QSaveFile output(item.destination);
        if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly)) {
            failure = QStringLiteral("Cannot copy %1 to %2").arg(item.source, item.destination);
            break;
        }
        QCryptographicHash sourceHash(QCryptographicHash::Sha256);
        QByteArray chunk;
        while (!(chunk = input.read(256 * 1024)).isEmpty()) {
            sourceHash.addData(chunk);
            if (output.write(chunk) != chunk.size()) {
                failure = QStringLiteral("Destination write failed: %1").arg(item.destination);
                break;
            }
        }
        if (failure.isEmpty() && input.error() != QFile::NoError)
            failure = QStringLiteral("Source read failed: %1").arg(item.source);
        input.close();

        if (!failure.isEmpty() || QFileInfo::exists(item.destination)) {
            output.cancelWriting();
            if (failure.isEmpty())
                failure = QStringLiteral("Destination appeared during copy: %1").arg(item.destination);
            break;
        }
        if (!output.commit()) {
            failure = QStringLiteral("Could not finish writing: %1").arg(item.destination);
            break;
        }
        created.append(item.destination);
        QString readError;
        if (fileDigest(item.destination, &readError) != sourceHash.result()
                || !readError.isEmpty()) {
            failure = QStringLiteral("Copied file failed SHA-256 verification: %1").arg(item.destination);
            break;
        }
    }

    if (!failure.isEmpty()) {
        // Copies from this attempt can safely be removed: no source was deleted.
        for (const QString &path : created) QFile::remove(path);
        return;
    }

    // Phase two: re-verify source against destination before deleting each
    // original. If removal fails, leave the tab open and destination copies
    // intact so a retry can finish without data loss.
    for (const ThreadMoveFile &item : transfers) {
        const QFileInfo source(item.source);
        if (!source.exists())
            continue;
        const QFileInfo destination(item.destination);
        QString readError;
        if (!destination.isFile() || source.size() != destination.size()
                || fileDigest(item.source, &readError) != fileDigest(item.destination, &readError)
                || !readError.isEmpty()) {
            failure = QStringLiteral("Final verification failed; original kept: %1").arg(item.source);
            return;
        }
        if (!QFile::remove(item.source)) {
            failure = QStringLiteral("Could not remove original after copying: %1").arg(item.source);
            return;
        }
    }
}
