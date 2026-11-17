#ifndef FOURCHANREQUESTHEADERS_H
#define FOURCHANREQUESTHEADERS_H

#include <QNetworkRequest>
#include <QUrl>
#include <QRegExp>
#include <QStringList>

// The historical Wget/1.12 default receives 429 responses for 4chan
// videos on some CDN paths, even when the URLs play in a normal browser.
// This stays small and testable so other HTTP requests remain unchanged.
namespace FourchanRequestHeaders {

static inline QByteArray defaultUserAgent() {
    return QByteArray("Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:140.0) "
                      "Gecko/20100101 Firefox/140.0");
}

static inline bool isFourchanVideo(const QUrl &url) {
    if (url.host().compare(QStringLiteral("i.4cdn.org"), Qt::CaseInsensitive) != 0)
        return false;
    const QString path = url.path().toLower();
    return path.endsWith(QStringLiteral(".mp4")) || path.endsWith(QStringLiteral(".webm"));
}

static inline void apply(QNetworkRequest &request, const QUrl &url,
                         const QByteArray &configuredUserAgent) {
    QByteArray agent = configuredUserAgent.trimmed();
    // Preserve intentional custom agents. Migrate the obsolete app default.
    if (agent.isEmpty() || agent == "Wget/1.12")
        agent = defaultUserAgent();
    request.setRawHeader("User-Agent", agent);

    if (!isFourchanVideo(url))
        return;

    // A browser requesting an HTML5 video normally sends a Range request
    // from the start of the file. 0- asks for the *whole* remaining file.
    // DownloadManager still validates against the JSON API's original fsize.
    request.setRawHeader("Accept", "video/webm,video/mp4,*/*;q=0.8");
    request.setRawHeader("Range", "bytes=0-");

    const QStringList pathParts = url.path().split('/', QString::SkipEmptyParts);
    if (!pathParts.isEmpty() && QRegExp("^[a-zA-Z0-9]+$").exactMatch(pathParts.first())) {
        const QByteArray referrer = QString("https://boards.4chan.org/%1/")
                .arg(pathParts.first()).toUtf8();
        request.setRawHeader("Referer", referrer);
    }
}
}
#endif
