#ifndef LEGACYRUNTIMECLEANUP_H
#define LEGACYRUNTIMECLEANUP_H

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

// v0.5.0: explicitly listed, previously shipped Qt4 / pre-UCRT libraries.
// Never scan directories for arbitrary DLLs, and never remove user data.
// Only run once a modern standalone updater is installed.
namespace LegacyRuntimeCleanup {

inline QStringList obsoleteRelativeFiles() {
    return QStringList()
        << QStringLiteral("QtCore4.dll")
        << QStringLiteral("QtGui4.dll")
        << QStringLiteral("QtNetwork4.dll")
        << QStringLiteral("QtXml4.dll")
        << QStringLiteral("libeay32.dll")
        << QStringLiteral("ssleay32.dll")
        << QStringLiteral("libgcc_s_dw2-1.dll")
        << QStringLiteral("mingwm10.dll")
        << QStringLiteral("imageformats/qgif4.dll")
        << QStringLiteral("imageformats/qico4.dll")
        << QStringLiteral("imageformats/qjpeg4.dll")
        << QStringLiteral("imageformats/qmng4.dll")
        << QStringLiteral("imageformats/qsvg4.dll")
        << QStringLiteral("imageformats/qtiff4.dll")
        << QStringLiteral("updater/QtCore4.dll")
        << QStringLiteral("updater/QtGui4.dll")
        << QStringLiteral("updater/QtNetwork4.dll")
        << QStringLiteral("updater/QtXml4.dll")
        << QStringLiteral("updater/libeay32.dll")
        << QStringLiteral("updater/ssleay32.dll")
        << QStringLiteral("updater/libgcc_s_dw2-1.dll")
        << QStringLiteral("updater/mingwm10.dll")
        << QStringLiteral("styles/qwindowsvistastyle.dll")
        << QStringLiteral("au.exe")
        << QStringLiteral("updater/au.exe");
}

inline QStringList removeObsoleteFiles(const QString &applicationDirectory) {
    const QDir root(applicationDirectory);
    // A legacy updater may still require Qt4 or OpenSSL 1.x. Leave all files
    // in place unless the supported Qt5 updater is already installed.
    const QFileInfo modernUpdater(root.filePath(QStringLiteral("updater/upd4t3r.exe")));
    if (!modernUpdater.isFile() || modernUpdater.isSymLink())
        return QStringList();

    QStringList removed;
    const QStringList names = obsoleteRelativeFiles();
    for (const QString &relative : names) {
        // The manifest is made of literal, installation-relative paths.
        // Do not follow links or remove directories, even at listed paths.
        const QFileInfo info(root.filePath(relative));
        if (!info.isFile() || info.isSymLink())
            continue;
        if (QFile::remove(info.absoluteFilePath()))
            removed << relative;
    }

    // Only remove a now-empty legacy styles folder. Unknown files are kept.
    root.rmdir(QStringLiteral("styles"));
    return removed;
}

} // namespace LegacyRuntimeCleanup

#endif // LEGACYRUNTIMECLEANUP_H
