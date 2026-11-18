#include <QApplication>
#include <QListWidgetItem>
#include <QDebug>
#include <QUrl>
#include "thumbnailmetadata.h"

static bool check(const bool good, const char *message) {
    if (!good)
        qCritical() << "FAIL:" << message;
    return good;
}

int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    const QString path = QStringLiteral("C:/thread files/example.WEBM");

    if (!check(ThumbnailMetadata::isVideo(path), "webm video detection") ||
        !check(ThumbnailMetadata::isVideo(QStringLiteral("C:/files/example.Mp4")),
               "case-insensitive mp4 detection") ||
        !check(!ThumbnailMetadata::isVideo(QStringLiteral("C:/files/example.jpeg")),
               "jpeg is not video"))
        return 1;

    if (!check(ThumbnailMetadata::label(path, true, true) ==
                   QStringLiteral(".webm\nC:/thread files/example.WEBM"),
               "both captions") ||
        !check(ThumbnailMetadata::label(path, false, true) == path,
               "path-only caption") ||
        !check(ThumbnailMetadata::label(path, true, false) == QStringLiteral(".webm"),
               "extension-only caption") ||
        !check(ThumbnailMetadata::label(path, false, false).isEmpty(),
               "thumbnail-only caption"))
        return 2;

    QListWidgetItem item;
    item.setData(Qt::UserRole, path);
    const QStringList labels = QStringList()
            << ThumbnailMetadata::label(path, true, true)
            << ThumbnailMetadata::label(path, true, false)
            << ThumbnailMetadata::label(path, false, true)
            << ThumbnailMetadata::label(path, false, false);
    for (const QString &caption : labels) {
        item.setText(caption);
        if (!check(ThumbnailMetadata::filePath(&item) == path,
                   "file actions must never use caption"))
            return 3;
    }
    if (!check(ThumbnailMetadata::filePath(nullptr).isEmpty(), "null selection"))
        return 4;

    if (!check(ThumbnailMetadata::videoPreviewUrl(QStringLiteral(
                    "https://i.4cdn.org/r9k/123456s.jpg")).isValid(),
               "genuine JPEG video preview") ||
        !check(ThumbnailMetadata::videoPreviewUrl(QStringLiteral(
                    "https://i.4cdn.org.evil.example/b/123s.jpg")).isEmpty(),
               "reject lookalike host") ||
        !check(ThumbnailMetadata::videoPreviewUrl(QStringLiteral(
                    "http://i.4cdn.org/b/123s.jpg")).isEmpty(),
               "reject insecure preview") ||
        !check(ThumbnailMetadata::videoPreviewUrl(QStringLiteral(
                    "https://other.example/b/123s.jpg")).isEmpty(),
               "reject unrelated host") ||
        !check(ThumbnailMetadata::videoPreviewUrl(QStringLiteral(
                    "https://i.4cdn.org/b/123.webm")).isEmpty(),
               "never download an actual video as a preview") ||
        !check(ThumbnailMetadata::videoPreviewUrl(QStringLiteral(
                    "https://i.4cdn.org:8443/b/123s.jpg")).isEmpty(),
               "reject nonstandard ports") ||
        !check(ThumbnailMetadata::videoPreviewUrl(QStringLiteral(
                    "https://i.4cdn.org/b/123s.jpg?redirect=https://other.example")).isEmpty(),
               "reject preview query parameters"))
        return 5;

    const QSize icon(200, 160);
    const QSize thumbnailsOnly = ThumbnailMetadata::gridCellSize(icon, false, false, 19);
    const QSize extensionOnly = ThumbnailMetadata::gridCellSize(icon, true, false, 19);
    const QSize pathOnly = ThumbnailMetadata::gridCellSize(icon, false, true, 19);
    const QSize bothCaptions = ThumbnailMetadata::gridCellSize(icon, true, true, 19);
    if (!check(thumbnailsOnly == QSize(216, 168), "compact thumbnail-only tiles") ||
        !check(extensionOnly == QSize(216, 191), "extension adds one line") ||
        !check(pathOnly == QSize(216, 191), "path adds one line") ||
        !check(bothCaptions == QSize(216, 210), "both captions add two lines") ||
        !check(bothCaptions.height() < 160 + 65, "no excessive blank space") ||
        !check(ThumbnailMetadata::gridCellSize(QSize(350, 280), false, false, 19)
                 == QSize(366, 288), "changed icon dimensions affect cell dimensions"))
        return 6;

    qInfo() << "PASS: video/extension types, four caption modes, compact"
               " grid sizes, original file paths, official HTTPS previews.";
    return 0;
}
