#ifndef OVERVIEWBOARD_H
#define OVERVIEWBOARD_H

#include <QString>
#include <QRegExp>
#include <QUrl>

// A board filter is an exact board ID, not a substring search of the URL.
// Keep the empty key for "All boards", including tabs without a URL.
namespace OverviewBoard {

inline QString normalizeFilter(const QString &value) {
    const QString board = value.trimmed().toLower();
    return QRegExp("^[a-z0-9]+$").exactMatch(board) ? board : QString();
}

inline QString boardForUrl(const QString &rawUrl) {
    if (rawUrl.trimmed().isEmpty())
        return QString();

    const QUrl url = QUrl::fromUserInput(rawUrl.trimmed());
    const QString scheme = url.scheme().toLower();
    const QString host = url.host().toLower();
    if (!url.isValid() || (scheme != QLatin1String("https") &&
                           scheme != QLatin1String("http")))
        return QString();
    const bool officialHost =
        host == QLatin1String("4chan.org") ||
        host.endsWith(QLatin1String(".4chan.org")) ||
        host == QLatin1String("4channel.org") ||
        host.endsWith(QLatin1String(".4channel.org"));
    if (!officialHost)
        return QString();

    // First segment identifies the board: /b/thread/123 or /r9k/res/123.
    return normalizeFilter(url.path().section('/', 1, 1));
}

inline bool matchesFilter(const QString &rawUrl, const QString &boardFilter) {
    return boardFilter.isEmpty() || boardForUrl(rawUrl) == boardFilter;
}

inline QString labelFor(const QString &board) {
    return QStringLiteral("/%1/").arg(board);
}

} // namespace OverviewBoard

#endif // OVERVIEWBOARD_H
