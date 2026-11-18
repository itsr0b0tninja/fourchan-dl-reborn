#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QDebug>
#include "legacyruntimecleanup.h"

static bool touch(const QString &path) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write("test", 4) == 4;
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir temp;
    if (!temp.isValid()) return 1;
    const QDir root(temp.path());
    const QStringList obsolete = LegacyRuntimeCleanup::obsoleteRelativeFiles();
    if (obsolete.size() < 24 || obsolete.contains("settings.ini") ||
            obsolete.contains("Qt5Network.dll") ||
            obsolete.contains("updater/upd4t3r.exe"))
        return 2;

    // Old installations without the modern standalone updater must remain
    // fully untouched, including any old Qt libraries and au.exe.
    if (!touch(root.filePath("QtCore4.dll")) ||
            !touch(root.filePath("updater/au.exe")))
        return 3;
    if (!LegacyRuntimeCleanup::removeObsoleteFiles(root.path()).isEmpty() ||
            !QFile::exists(root.filePath("QtCore4.dll")) ||
            !QFile::exists(root.filePath("updater/au.exe")))
        return 4;

    if (!touch(root.filePath("updater/upd4t3r.exe")) ||
            !touch(root.filePath("libeay32.dll")) ||
            !touch(root.filePath("imageformats/qjpeg4.dll")) ||
            !touch(root.filePath("updater/QtNetwork4.dll")) ||
            !touch(root.filePath("styles/qwindowsvistastyle.dll")) ||
            !touch(root.filePath("styles/custom-style.dll")) ||
            !touch(root.filePath("settings.ini")) ||
            !touch(root.filePath("black.list")) ||
            !touch(root.filePath("fourchan-dl.log")) ||
            !touch(root.filePath("update.log")) ||
            !touch(root.filePath("Qt5Network.dll")) ||
            !touch(root.filePath("libssl-3-x64.dll")) ||
            !touch(root.filePath("imageformats/qjpeg.dll")) ||
            !touch(root.filePath("plugins/parser4chan.dll")) ||
            !touch(root.filePath("downloads/image.png")))
        return 5;

    const QStringList removed = LegacyRuntimeCleanup::removeObsoleteFiles(root.path());
    const QStringList shouldRemove = QStringList()
        << "QtCore4.dll" << "updater/au.exe" << "libeay32.dll"
        << "imageformats/qjpeg4.dll" << "updater/QtNetwork4.dll"
        << "styles/qwindowsvistastyle.dll";
    for (const QString &name : shouldRemove) {
        if (!removed.contains(name) || QFile::exists(root.filePath(name))) {
            qCritical() << "Legacy file not removed:" << name;
            return 6;
        }
    }
    const QStringList protectedFiles = QStringList()
        << "updater/upd4t3r.exe" << "styles/custom-style.dll"
        << "settings.ini" << "black.list" << "fourchan-dl.log" << "update.log"
        << "Qt5Network.dll" << "libssl-3-x64.dll"
        << "imageformats/qjpeg.dll" << "plugins/parser4chan.dll"
        << "downloads/image.png";
    for (const QString &name : protectedFiles) {
        if (!QFile::exists(root.filePath(name))) {
            qCritical() << "Unexpected file deleted:" << name;
            return 7;
        }
    }
    if (!LegacyRuntimeCleanup::removeObsoleteFiles(root.path()).isEmpty())
        return 8;
    // Unknown files keep the styles directory in place.
    if (!QDir(root.filePath("styles")).exists())
        return 9;
    QFile::remove(root.filePath("styles/custom-style.dll"));
    LegacyRuntimeCleanup::removeObsoleteFiles(root.path());
    if (QDir(root.filePath("styles")).exists())
        return 10;

    qInfo() << "PASS: strictly named legacy cleanup, modern updater required,"
               " idempotence and preservation of all user/runtime files";
    return 0;
}
