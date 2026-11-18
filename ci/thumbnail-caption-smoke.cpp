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

    qInfo() << "PASS: mp4/webm identification; four caption modes;"
               " original filename preserved; official HTTPS previews only.";
    return 0;
}
