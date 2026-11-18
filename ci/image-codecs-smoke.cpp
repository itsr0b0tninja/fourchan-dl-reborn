#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QDebug>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);

    // Run this test from the final portable folder on Windows with only
    // that folder and the Windows system directories on PATH. This verifies
    // the intended image plugin selection AND its transitive DLLs load.
    const QList<QByteArray> available = QImageReader::supportedImageFormats();
    const QList<QByteArray> required = QList<QByteArray>()
        << "gif" << "ico" << "jpeg" << "png" << "svg" << "tiff" << "webp";
    for (const QByteArray &name : required) {
        if (!available.contains(name)) {
            qCritical() << "Missing usable portable image codec" << name
                        << "Available formats:" << available;
            return 2;
        }
    }
    const QString imageDir = QCoreApplication::applicationDirPath() + "/imageformats";
    const QStringList permitted = QStringList()
        << "qgif.dll" << "qico.dll" << "qjpeg.dll"
        << "qsvg.dll" << "qtiff.dll" << "qwebp.dll";
    const QStringList installed = QDir(imageDir).entryList(
                QStringList() << "*.dll", QDir::Files, QDir::Name);
    if (installed != permitted) {
        qCritical() << "Unexpected image plugins included" << installed;
        return 3;
    }
    if (QDir(QCoreApplication::applicationDirPath() + "/styles").exists()) {
        qCritical() << "Legacy Qt style directory included";
        return 4;
    }
    qInfo() << "PASS: curated portable Qt5 image plugins load" << available;
    return 0;
}
