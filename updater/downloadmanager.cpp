#include "downloadmanager.h"

DownloadManager::DownloadManager(QObject *parent) :
    QObject(parent)
{
    manager = new QNetworkAccessManager();
    cookies = new QNetworkCookieJar();
    manager->setCookieJar(cookies);

    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

}

void DownloadManager::replyFinished(QNetworkReply* r) {
    const QString requestURI = r->request().url().toString();
    const QString location = r->header(QNetworkRequest::LocationHeader).toString();

    if (!location.isEmpty()) {
        // GitHub Releases redirects through its asset CDN.
        const QString resolved = r->url().resolved(QUrl(location)).toString();
        for (int i = 0; i < updateList.count(); ++i) {
            if (updateList.at(i).uri == requestURI) {
                FileUpdate next = updateList.at(i);
                next.uri = resolved;
                updateList.replace(i, next);
                break;
            }
        }
        r->deleteLater();
        initiateDownload();
        return;
    }

    if (r->error() != QNetworkReply::NoError) {
        emit error("GitHub update download failed: " + requestURI + " (" + r->errorString() + ")");
        r->deleteLater();
        return;
    }

    const QByteArray body = r->readAll();
    const QVariant sizeHeader = r->header(QNetworkRequest::ContentLengthHeader);
    if (sizeHeader.isValid() && sizeHeader.toLongLong() >= 0 &&
        body.size() != sizeHeader.toLongLong()) {
        emit error("Incomplete GitHub update download: " + requestURI);
        r->deleteLater();
        return;
    }
    if (body.isEmpty() ||
        r->header(QNetworkRequest::ContentTypeHeader).toString().startsWith("text/html")) {
        emit error("Invalid GitHub release file: " + requestURI);
        r->deleteLater();
        return;
    }

    const QString temporary = QUuid::createUuid().toString() + ".file";
    QFile file(temporary);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit error("Cannot save updater temporary file: " + temporary);
        r->deleteLater();
        return;
    }
    const qint64 written = file.write(body);
    const bool flushed = file.flush();
    file.close();
    if (written != body.size() || !flushed) {
        QFile::remove(temporary);
        emit error("Cannot finish writing updater temporary file: " + temporary);
        r->deleteLater();
        return;
    }

    bool found = false;
    for (int i = 0; i < updateList.count(); ++i) {
        if (updateList.at(i).uri == requestURI) {
            FileUpdate updated = updateList.at(i);
            updated.tmpFilename = temporary;
            updateList.replace(i, updated);
            found = true;
            break;
        }
    }
    if (!found) {
        QFile::remove(temporary);
        emit error("Update request no longer matches the manifest: " + requestURI);
        r->deleteLater();
        return;
    }
    r->deleteLater();
    initiateDownload();
}

void DownloadManager::startDownload(QList<FileUpdate> l) {
    updateList = l;
    initiateDownload();
}

void DownloadManager::initiateDownload() {
    QNetworkReply* nr;
    QString uri;
    bool downloadsAvailable;

    downloadsAvailable = false;

    for (int i=0; i<updateList.count(); i++) {
        if (updateList.at(i).tmpFilename == "") {
            uri = updateList.at(i).uri;
            downloadsAvailable = true;
            break;
        }
    }

    if (downloadsAvailable) {
        QNetworkRequest req;

        req.setUrl(QUrl(uri));
        req.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Automatic);
        req.setRawHeader("User-Agent", "Wget/1.12");
        nr = manager->get(req);

        connect(nr,SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(replyError(QNetworkReply::NetworkError)));

        p("Downloading "+uri);
    }
    else {
        emit downloadsFinished(updateList);
    }
}

void DownloadManager::replyError(QNetworkReply::NetworkError e) {
    p("Reply error "+QString::number(e));
}

void DownloadManager::p(QString msg) {
    *output << "DownloadManager: " << msg << endl;
    output->flush();
    *foutput << "DownloadManager: " << msg << endl;
    foutput->flush();
}
