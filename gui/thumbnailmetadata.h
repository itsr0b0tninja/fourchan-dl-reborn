#ifndef THUMBNAILMETADATA_H
#define THUMBNAILMETADATA_H

#include <QFileInfo>
#include <QListWidgetItem>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QRegExp>

// Keep the actual disk path separate from its optional visual caption.
// Delete, reload, open and thumbnail-cache actions must never receive
// a caption containing the extension label.
namespace ThumbnailMetadata {

inline bool isVideo(const QString &path) {
    const QString extension = QFileInfo(path).suffix().toLower();
    return extension == QLatin1String("webm") ||
           extension == QLatin1String("mp4");
}

// Only fetch the tiny JPEG preview supplied by 4chan, never an arbitrary URL.
inline QUrl videoPreviewUrl(const QString &raw) {
    const QUrl url(raw);
    if (!url.isValid() || url.scheme() != QLatin1String("https") ||
            url.host().compare(QLatin1String("i.4cdn.org"), Qt::CaseInsensitive) != 0 ||
            url.port(-1) != -1 || url.hasQuery() || url.hasFragment() ||
            !QRegExp("^/[a-zA-Z0-9]+/[0-9]+s\\.jpg$").exactMatch(url.path()))
        return QUrl();
    return url;
}

inline QString label(const QString &path, bool showExtension, bool showPath) {
    QStringList lines;
    const QString extension = QFileInfo(path).suffix().toLower();
    if (showExtension && !extension.isEmpty())
        lines << QStringLiteral(".%1").arg(extension);
    if (showPath)
        lines << path;
    return lines.join(QLatin1String("\n"));
}

inline QString filePath(const QListWidgetItem *item) {
    if (!item)
        return QString();
    const QString stored = item->data(Qt::UserRole).toString();
    return stored.isEmpty() ? item->text() : stored;
}

} // namespace ThumbnailMetadata

#endif // THUMBNAILMETADATA_H
