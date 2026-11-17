#include "parser4chan.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

Parser4chan::Parser4chan()
{
    _html = "";
    _statusCode.hasErrors = false;
    _statusCode.hasImages = false;
    _statusCode.hasTitle = false;
    _statusCode.isFrontpage = false;
    _statusCode.hasRedirect = false;
    _statusCode.threadFragmented = false;
    _errorCode = 0;
    _redirect = QUrl();
    _images.clear();
}

QString Parser4chan::getAuthor() {
    return "Mickey Fenton";
}

QString Parser4chan::getPluginName() {
    return "4chan.org Parser";
}

QString Parser4chan::getVersion() {
    return _LIB_VERSION;
}

QString Parser4chan::getDomain() {
    return "4chan.org";
}

ParsingStatus Parser4chan::parseHTML(QString html) {
    _html = html;
    _images.clear();
    _urlList.clear();
    _redirect.clear();
    _threadTitle.clear();
    _errorCode = 0;
    _statusCode.hasErrors = false;
    _statusCode.hasImages = false;
    _statusCode.hasTitle = false;
    _statusCode.isFrontpage = false;
    _statusCode.hasRedirect = false;
    _statusCode.threadFragmented = false;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(html.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        _statusCode.hasErrors = true;
        _errorCode = (html.contains("4chan - Banned")) ? 999 : 1;
        return _statusCode;
    }

    const QJsonObject root = doc.object();
    if (root.value("posts").isArray()) {
        const QJsonArray posts = root.value("posts").toArray();
        for (int i = 0; i < posts.size(); ++i) {
            const QJsonObject post = posts.at(i).toObject();
            if (i == 0) {
                _threadTitle = post.value("sub").toString();
                if (_threadTitle.isEmpty()) {
                    _threadTitle = post.value("com").toString();
                    _threadTitle.remove(QRegExp("<[^>]*>"));
                    _threadTitle.replace("&gt;", ">").replace("&lt;", "<")
                        .replace("&amp;", "&").replace("&quot;", "\"");
                    _threadTitle.replace(QRegExp("\\s+"), " ");
                    _threadTitle = _threadTitle.left(80).trimmed();
                }
            }

            if (post.contains("tim") && post.value("ext").isString()) {
                const QString tim = QString::number(static_cast<qint64>(post.value("tim").toDouble()));
                const QString ext = post.value("ext").toString();
                if (tim == "0" || ext.isEmpty()) continue;
                _IMAGE img;
                img.largeURI = QString("https://i.4cdn.org/%1/%2%3")
                    .arg(boardName, tim, ext);
                img.thumbURI = QString("https://i.4cdn.org/%1/%2s.jpg")
                    .arg(boardName, tim);
                img.originalFilename = post.value("filename").toString() + ext;
                img.downloaded = false;
                img.requested = false;
                img.expectedSize = qMax(qint64(0),
                    static_cast<qint64>(post.value("fsize").toDouble()));
                _images.append(img);
            }
        }
        _statusCode.hasImages = !_images.isEmpty();
        if (_threadTitle.isEmpty())
            _threadTitle = QString("Thread# %1").arg(threadNumber);
        _statusCode.hasTitle = true;
    }
    else if (root.value("threads").isArray()) {
        const QJsonArray threads = root.value("threads").toArray();
        foreach (const QJsonValue &threadValue, threads) {
            const QJsonArray posts = threadValue.toObject().value("posts").toArray();
            if (posts.isEmpty()) continue;
            const qint64 id = static_cast<qint64>(
                posts.at(0).toObject().value("no").toDouble());
            if (id <= 0) continue;
            _urlList << QUrl(QString("https://boards.4chan.org/%1/thread/%2")
                .arg(boardName).arg(id));
        }
        _threadTitle = _url.toString();
        _statusCode.hasTitle = true;
        _statusCode.isFrontpage = true;
    }
    else {
        _statusCode.hasErrors = true;
        _errorCode = 1;
    }
    return _statusCode;
}

QString Parser4chan::getThreadTitle() {
    return _threadTitle;
}

QList<_IMAGE> Parser4chan::getImageList() {
    return _images;
}

QUrl Parser4chan::getRedirectURL() {
    return _redirect;
}

int Parser4chan::getErrorCode() {
    return _errorCode;
}

ParsingStatus Parser4chan::getStatusCode() {
    return _statusCode;
}

QList<QUrl> Parser4chan::getUrlList() {
    return _urlList;
}

void Parser4chan::setURL(QUrl url) {
    QString s;
    QString s_url;

    s_url = url.toString();

    //if (url.toString().endsWith(".json")) {
    if (s_url.endsWith(".json")) {
        s = url.toString();
        s.replace(".json", "");
        _url = QUrl(s);
    }
    else {
        _url = url;
    }

    boardName = _url.path().section("/",1,1);
    //if (_url.path().contains("res")) {
    if (_url.path().contains("res") || _url.path().contains("thread/")) {
        threadNumber = _url.path().section("/",3,3);
    }
    else {
        threadNumber = "";
    }
}

QString Parser4chan::parseSavepath(QString s) {
    s.replace("%n", threadNumber);
    s.replace("%b", boardName);
    s.replace("%h", _url.host());
    s.replace("%a", "");

    s.replace("//", "/");

    return s;
}

QMap<QString, QString> Parser4chan::getSupportedReplaceCharacters() {
    QMap<QString, QString> ret;

    ret.insert("%n", "Threadnumber");
    ret.insert("%b", "Board");
    ret.insert("%h", "Host");

    return ret;
}

void Parser4chan::initPlugin() {

}

QList<QUrl> Parser4chan::initialRequests() {
    QList<QUrl> uris;

//    uris.append(QUrl("http://boards.4chan.org/i/#cfg=%7B%22settings%22%3A%22%7B%5C%22quotePreview%5C%22%3Atrue%2C%5C%22backlinks%5C%22%3Atrue%2C%5C%22quickReply%5C%22%3Atrue%2C%5C%22threadUpdater%5C%22%3Afalse%2C%5C%22threadHiding%5C%22%3Afalse%2C%5C%22pageTitle%5C%22%3Afalse%2C%5C%22hideGlobalMsg%5C%22%3Atrue%2C%5C%22alwaysAutoUpdate%5C%22%3Afalse%2C%5C%22topPageNav%5C%22%3Afalse%2C%5C%22threadWatcher%5C%22%3Afalse%2C%5C%22imageExpansion%5C%22%3Afalse%2C%5C%22fitToScreenExpansion%5C%22%3Afalse%2C%5C%22threadExpansion%5C%22%3Afalse%2C%5C%22alwaysDepage%5C%22%3Afalse%2C%5C%22imageSearch%5C%22%3Afalse%2C%5C%22reportButton%5C%22%3Atrue%2C%5C%22localTime%5C%22%3Atrue%2C%5C%22stickyNav%5C%22%3Atrue%2C%5C%22keyBinds%5C%22%3Atrue%2C%5C%22inlineQuotes%5C%22%3Atrue%2C%5C%22filter%5C%22%3Afalse%2C%5C%22revealSpoilers%5C%22%3Atrue%2C%5C%22replyHiding%5C%22%3Afalse%2C%5C%22imageHover%5C%22%3Afalse%2C%5C%22threadStats%5C%22%3Afalse%2C%5C%22IDColor%5C%22%3Atrue%2C%5C%22downloadFile%5C%22%3Atrue%2C%5C%22inlineReport%5C%22%3Atrue%2C%5C%22noPictures%5C%22%3Afalse%2C%5C%22embedYouTube%5C%22%3Afalse%2C%5C%22embedSoundCloud%5C%22%3Afalse%2C%5C%22updaterSound%5C%22%3Afalse%2C%5C%22customCSS%5C%22%3Afalse%2C%5C%22autoScroll%5C%22%3Afalse%2C%5C%22hideStubs%5C%22%3Afalse%2C%5C%22compactThreads%5C%22%3Afalse%2C%5C%22dropDownNav%5C%22%3Afalse%2C%5C%22fixedThreadWatcher%5C%22%3Afalse%2C%5C%22persistentQR%5C%22%3Afalse%2C%5C%22disableAll%5C%22%3Afalse%2C%5C%22TW-position%5C%22%3A%5C%22left%3A%2019px%3B%20top%3A%20387px%3B%5C%22%2C%5C%22customMenu%5C%22%3Afalse%2C%5C%22embedVocaroo%5C%22%3Afalse%7D%22%7D"));

    return uris;
}

QUrl Parser4chan::alterUrl(QUrl u) {
    const QString host = u.host().toLower();
    // Never append .json to CDN images, including MP4 and WebM files.
    if (host == "i.4cdn.org" || host == "a.4cdn.org")
        return u;
    if (host != "boards.4chan.org" && host != "boards.4channel.org")
        return u;

    const QStringList parts = u.path().split("/", QString::SkipEmptyParts);
    if (parts.isEmpty())
        return u;

    const QString board = parts.at(0);
    if (!QRegExp("^[a-zA-Z0-9]+$").exactMatch(board))
        return u;

    QString path;
    if (parts.size() >= 3 && (parts.at(1) == "res" || parts.at(1) == "thread")) {
        QString id = parts.at(2);
        if (id.endsWith(".json")) id.chop(5);
        if (!QRegExp("^\\d+$").exactMatch(id)) return u;
        path = QString("/%1/thread/%2.json").arg(board, id);
    } else {
        QString page = parts.size() > 1 ? parts.at(1) : "1";
        if (page.endsWith(".json")) page.chop(5);
        if (!QRegExp("^\\d+$").exactMatch(page)) return u;
        path = QString("/%1/%2.json").arg(board, page);
    }

    QUrl apiUrl;
    apiUrl.setScheme("https");
    apiUrl.setHost("a.4cdn.org");
    apiUrl.setPath(path);
    return apiUrl;
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(pParser4chan, Parser4chan)
#endif
