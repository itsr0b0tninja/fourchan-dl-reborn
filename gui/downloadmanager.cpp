#include "downloadmanager.h"
#include "fourchanrequestheaders.h"
#include "retryafterpolicy.h"

#define MAX_CONCURRENT_DOWNLOADS_PER_NAM 5

DownloadManager::DownloadManager(QObject *parent) :
    QObject(parent)
{
    downloadsPaused = true;
    cookies = new QNetworkCookieJar();
    setMaxPriority(0);
    nams.clear();
    nams.append(new NetworkAccessManager(this));    // Add at least one AccessManager
    nams.at(0)->setCookieJar(cookies);
    settings = new QSettings("settings.ini", QSettings::IniFormat);

    waitTimer = new QTimer(this);
    waitTimer->setSingleShot(true);
    waitTimer->setInterval(20000);
    videoCooldownTimer = new QTimer(this);
    videoCooldownTimer->setSingleShot(true);
    connect(videoCooldownTimer, SIGNAL(timeout()), this, SLOT(retryReady()));

    requestList.clear();
    lastid = 0;
    currentRequests = 0;

    totalRequests = 0;
    finishedRequests = 0;

    statistic_downloadedFiles  = 0;
    statistic_downloadedKBytes = 0;

    _highestPriority = 100;

    loadSettings();
    const qint64 remaining = QDateTime::currentDateTimeUtc().msecsTo(videoCooldownUntil);
    if (remaining > 0)
        videoCooldownTimer->start(static_cast<int>(qMin<qint64>(remaining, 86400000)));

//    _manager = new QNetworkAccessManager(this);
//    connect(_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

    connect(nams.at(0), SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    connect(waitTimer, SIGNAL(timeout()), this, SLOT(resumeDownloads()));
}

void DownloadManager::loadSettings() {
    settings->beginGroup("download_manager");
        maxRequests = qBound(1, settings->value("concurrent_downloads", 20).toInt(), 99);
        initialTimeout = settings->value("initial_timeout", 30).toInt()*1000;
        runningTimeout = settings->value("running_timeout", 20).toInt()*1000;
        _useThreadCache = settings->value("use_thread_cache", false).toBool();
        _threadCachePath = settings->value("thread_cache_path", "").toString();
        maxRetries = qBound(0, settings->value("max_retries", 4).toInt(), 20);
        retryDelaySeconds = qBound(1, settings->value("retry_delay_seconds", 5).toInt(), 120);
        videoCooldownUntil = QDateTime::fromString(
            settings->value("video_cooldown_utc").toString(), Qt::ISODate).toUTC();
    settings->endGroup();

    settings->beginGroup("statistics");
        statistic_downloadedFiles = settings->value("downloaded_files", 0).toFloat();
        statistic_downloadedKBytes = settings->value("downloaded_kbytes", 0).toFloat();
    settings->endGroup();

    setupNetworkAccessManagers((maxRequests + MAX_CONCURRENT_DOWNLOADS_PER_NAM - 1) / MAX_CONCURRENT_DOWNLOADS_PER_NAM);
}

void DownloadManager::streamReplyData() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;
    const qint64 uid = activeReplies.key(reply, -1);
    DownloadRequest* request = requestList.value(uid, 0);
    if (request && request->hasFileDestination())
        request->appendStreaming(reply->readAll());
}

void DownloadManager::replyFinished(QNetworkReply* reply) {
    const qint64 uid = activeReplies.key(reply, -1);
    if (uid == -1) {
        QLOG_WARN() << "DownloadManager :: Finished an unknown or cancelled reply"
                    << reply->url();
        reply->deleteLater();
        return;
    }

    DownloadRequest* dr = requestList.value(uid, 0);
    activeReplies.remove(uid);
    SupervisedNetworkReply* supervisor = supervisors.take(uid);
    if (supervisor) {
        supervisor->removeNetworkReply();
        supervisor->deleteLater();
    }
    currentRequests = qMax(0, currentRequests - 1);
    priorities.remove(priorities.key(uid), uid);

    if (!dr) {
        // A cancelled request can finish after its owner has been removed.
        reply->deleteLater();
        processRequests();
        return;
    }

    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    // Drain the last network bytes before checking the byte count.
    if (dr->hasFileDestination())
        dr->appendStreaming(reply->readAll());
    if (reply->error() != QNetworkReply::NoError || httpCode >= 400) {
        handleError(uid, reply);
    }
    else {
        const QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!redirect.isEmpty()) {
            dr->abortStreaming();
            dr->setUrl(reply->url().resolved(redirect));
            dr->setProcessing(false);
            priorities.insertMulti(dr->priority(), uid);
        } else if (dr->hasFileDestination()) {
            const qint64 streamed = dr->streamedBytes();
            const QVariant header = reply->header(QNetworkRequest::ContentLengthHeader);
            if (dr->streamFailed()) {
                retryOrFail(uid, "Could not write attachment to disk", false);
            } else if (header.isValid() && header.toLongLong() >= 0
                       && streamed != header.toLongLong()) {
                retryOrFail(uid, "Incomplete attachment body", false);
            } else if (dr->expectedSize() > 0 && streamed != dr->expectedSize()) {
                retryOrFail(uid, "Attachment size mismatch", false);
            } else if (streamed == 0) {
                retryOrFail(uid, "Empty attachment response", false);
            } else if (!dr->commitStreaming()) {
                retryOrFail(uid, "Could not commit completed attachment", false);
            } else {
                statistic_downloadedFiles++;
                statistic_downloadedKBytes += float(streamed) / 1024.0f;
                dr->requestHandler()->requestFileFinished(uid);
                emit finishedRequestsChanged(++finishedRequests);
            }
        } else {
            const QByteArray body = reply->readAll();
            const QVariant sizeHeader = reply->header(QNetworkRequest::ContentLengthHeader);
            if (sizeHeader.isValid() && sizeHeader.toLongLong() >= 0
                    && body.size() != sizeHeader.toLongLong()) {
                retryOrFail(uid, "Incomplete HTTP body", false);
            } else if (dr->expectedSize() > 0 && body.size() != dr->expectedSize()) {
                QLOG_WARN() << "DownloadManager :: 4chan attachment size mismatch"
                            << reply->url() << "expected" << dr->expectedSize()
                            << "actual" << body.size();
                retryOrFail(uid, "Attachment size mismatch", false);
            } else if (body.isEmpty() && reply->url().path().contains("/thread/")) {
                retryOrFail(uid, "Empty thread response", false);
            } else {
                const bool isMedia = reply->url().path().contains(
                    QRegExp(__IMAGE_REGEXP__ + QString("$"), Qt::CaseInsensitive));
                // Preserve the response for RequestHandler callbacks.
                dr->setResponse(body);
                dr->requestHandler()->requestFinished(uid);
                emit finishedRequestsChanged(++finishedRequests);

                if (isMedia) {
                    statistic_downloadedFiles++;
                    statistic_downloadedKBytes += float(body.size()) / 1024.0f;
                } else if (_useThreadCache) {
                    const QString filename = getFilenameForURL(reply->url());
                    if (filename != ".") {
                        const QString path = settings->value(
                            "download_manager/thread_cache_path",
                            QCoreApplication::applicationDirPath() + "/thread-cache").toString();
                        QDir().mkpath(path);
                        QFile cache(filename);
                        if (cache.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                            cache.write(settings->value(
                                "download_manager/compress_cache_file", true).toBool()
                                ? qCompress(body) : body);
                            cache.close();
                        }
                    }
                }
            }
        }
    }

    reply->deleteLater();
    if (activeReplies.count() > maxRequests + 10) {
        QLOG_WARN() << "DownloadManager :: Too many active requests. Pausing.";
        pauseDownloads();
        foreach (QNetworkReply* active, activeReplies)
            active->abort();
        waitTimer->start();
    }
    processRequests();
}

QString DownloadManager::getSavePath(qint64 uid) const {
    DownloadRequest* request = requestList.value(uid, 0);
    return request ? request->savePath() : QString();
}

QByteArray DownloadManager::getByteArray(qint64 uid) {
    QByteArray ret;
    DownloadRequest* dr;

    dr = requestList.value(uid,0);

    if (dr != 0) {
        ret = dr->response();
    }

    return ret;
}

bool DownloadManager::cached(qint64 uid) {
    DownloadRequest* dr;
    bool ret;

    dr = requestList.value(uid,0);

    ret = false;
    if (dr != 0) {
        ret = dr->cached();
    }

    return ret;
}

void DownloadManager::freeRequest(qint64 uid) {
    DownloadRequest* dr;
    dr = requestList.value(uid, 0);
    QLOG_TRACE() << "DownloadManager :: " << "freeRequest(" << uid << ")";

    if (dr != 0) {
        dr->deleteLater();
        requestList.remove(uid);
        priorities.remove(priorities.key(uid), uid);
        activeReplies.remove(uid);
    }
//    QLOG_TRACE() << "DownloadManager :: " << "open requests" << requestList.keys();
//    QLOG_TRACE() << "DownloadManager :: " << "priorities" << priorities;
}

qint64 DownloadManager::requestDownload(RequestHandler* caller, QUrl url, int prio, qint64 expectedSize, QString savePath) {
    DownloadRequest* dr;
    qint64 uid;

    dr = new DownloadRequest();
    dr->setRequestHandler(caller);
    dr->setUrl(url);
    dr->setPriority(prio);
    dr->setExpectedSize(expectedSize);
    dr->setSavePath(savePath);

    connect(dr, SIGNAL(requestUnpaused()), this, SLOT(retryReady()));

    uid = getUID();

//    addRequest(uid, dr);
    requestList.insert(uid, dr);
    priorities.insertMulti(prio, uid);

    // Defer queue processing until the caller has stored the request ID.
    // Also batch bursts of requests when many restored tabs start together.
    QTimer::singleShot(0, this, SLOT(retryReady()));

    emit totalRequestsChanged(++totalRequests);

    _highestPriority = qMax(_highestPriority, prio);

    return uid;
}

/**
 * Add Request to internal list and start Thread/Download immediately if max. number of downloads is not met.
 */
void DownloadManager::addRequest(qint64 uid, DownloadRequest* dr) {
    requestList.insert(uid, dr);

    processRequests();
    emit totalRequestsChanged(++totalRequests);
}

void DownloadManager::downloadTimeout(qint64 uid) {
    QNetworkReply* r;

    r = activeReplies.value(uid);

    if (r != 0) {
//        QLOG_TRACE() << "DownloadManager :: " << uid << QString(r->readAll());
        r->abort();
    }
}

void DownloadManager::processRequests() {
    QList<qint64> uids;

    if (!downloadsPaused) {
        uids = priorities.values();

        if (uids.count() > 0) {
            foreach (qint64 uid, uids) {
                if (activeReplies.count() >= maxRequests) {
                    break;
                }
                else {
                    if (requestList.count(uid) > 0) {
                        if (!(requestList.value(uid)->finished()) &&
                            !(requestList.value(uid)->processing()) &&
                            !(requestList.value(uid)->paused())) {
                            // A CDN video cooldown does not block image or thread updates.
                            if (FourchanRequestHeaders::isFourchanVideo(requestList.value(uid)->url())
                                    && QDateTime::currentDateTimeUtc() < videoCooldownUntil)
                                continue;
                            startRequest(uid);
                            //                            QLOG_TRACE() << "DownloadManager :: " << "addRequest (" << uid << ")";
                        }
                    }
                }
            }
        }
        else {
            if (requestList.count() > 0) {
                // This should not happen but if there are no (more) priorities set for some downloads, reset them
                uids = requestList.keys();
                foreach(qint64 uid, uids) {
                    priorities.insertMulti(uid, 0);
                }

                processRequests();
            }
            else {
                // We are finished
                totalRequests = 0;
                finishedRequests = 0;
                _highestPriority = 100;
                emit totalRequestsChanged(totalRequests);
            }
        }
    }
}

void DownloadManager::startRequest(qint64 uid) {
    QNetworkRequest req;
    QNetworkReply* rep;
    NetworkAccessManager* nam;
    DownloadRequest* dr;
    SupervisedNetworkReply* sup;

    dr = requestList.value(uid, 0);

    if (dr != 0 && !downloadsPaused) {
        if (dr->hasFileDestination() && !dr->beginStreaming()) {
            QLOG_ERROR() << "DownloadManager :: Cannot create temporary attachment file"
                         << dr->savePath();
            priorities.remove(priorities.key(uid), uid);
            dr->requestHandler()->error(uid, 503);
            return;
        }
        dr->setProcessing(true);
        sup = new SupervisedNetworkReply();
        sup->setTimeouts(initialTimeout, runningTimeout);
        connect(sup, SIGNAL(timeout(qint64)), this, SLOT(downloadTimeout(qint64)));

        req = QNetworkRequest(dr->url());
        req.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Automatic);
        const QByteArray configuredAgent = settings->value("download_manager/user-agent",
            QString::fromLatin1(FourchanRequestHeaders::defaultUserAgent())).toString().toLatin1();
        FourchanRequestHeaders::apply(req, dr->url(), configuredAgent);
        if (FourchanRequestHeaders::isFourchanVideo(dr->url())) {
            QLOG_DEBUG() << "DownloadManager :: Browser-style 4chan video request"
                         << dr->url() << "Range:" << req.rawHeader("Range")
                         << "Referer:" << req.rawHeader("Referer");
        }
        currentRequests++;
        nam = getFreeNAM();
        rep = nam->get(req);
        if (dr->hasFileDestination()) {
            rep->setReadBufferSize(256 * 1024);
            connect(rep, SIGNAL(readyRead()), this, SLOT(streamReplyData()));
        }
//        rep = _manager->get(req);

        sup->setNetworkReply(rep, uid);
        supervisors.insert(uid, sup);

//        QLOG_TRACE() << "DownloadManager :: " << "Requesting" << uid << ":" << dr->url();// << "with reply" << (qint64)rep;
        activeReplies.insert(uid, rep);
    }
    else {
        QLOG_WARN() << "DownloadManager :: " << "Requested start of uid which is non-existent";
    }
}

void DownloadManager::handleError(qint64 uid, QNetworkReply* reply) {
    DownloadRequest* dr = requestList.value(uid, 0);
    if (!dr)
        return;

    dr->abortStreaming();
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QLOG_WARN() << "DownloadManager :: Request failed" << reply->url()
                << "HTTP" << httpCode << "network error" << reply->error()
                << reply->errorString();

    if (httpCode == 404 || reply->error() == QNetworkReply::ContentNotFoundError) {
        if (_useThreadCache && cacheAvailable(dr->url())) {
            dr->setResponse(getCachedReply(dr->url()));
            dr->setCached(true);
            dr->requestHandler()->requestFinished(uid);
        } else {
            dr->requestHandler()->error(uid, 404);
        }
        return;
    }

    // A 429 is a temporary throttle. Keep its retry budget unchanged.
    const bool limited = (httpCode == 429);
    int retryAfterSeconds = 0;
    if (limited) {
        retryAfterSeconds = RetryAfterPolicy::delaySeconds(
            reply->rawHeader("Retry-After"), qMax(60, retryDelaySeconds * 4));
        if (FourchanRequestHeaders::isFourchanVideo(reply->url())) {
            const QDateTime deadline = QDateTime::currentDateTimeUtc().addSecs(retryAfterSeconds);
            if (!videoCooldownUntil.isValid() || deadline > videoCooldownUntil) {
                videoCooldownUntil = deadline;
                settings->setValue("download_manager/video_cooldown_utc",
                                   deadline.toString(Qt::ISODate));
                videoCooldownTimer->start(retryAfterSeconds * 1000);
            }
            QLOG_WARN() << "DownloadManager :: Video CDN HTTP 429"
                        << "Retry-After:" << reply->rawHeader("Retry-After")
                        << "Content-Type:"
                        << reply->header(QNetworkRequest::ContentTypeHeader).toString();
            emit error(QString("4chan video rate limited; retry at %1 (local time)")
                .arg(videoCooldownUntil.toLocalTime().toString("HH:mm:ss")));
        }
    }
    retryOrFail(uid, reply->errorString(), limited, retryAfterSeconds);
}

void DownloadManager::retryOrFail(qint64 uid, const QString &reason,
                                  bool rateLimited, int retryAfterSeconds) {
    DownloadRequest* dr = requestList.value(uid, 0);
    if (!dr)
        return;
    dr->abortStreaming();

    if (rateLimited) {
        const int delay = qBound(1, retryAfterSeconds, 86400);
        QLOG_INFO() << "DownloadManager :: HTTP 429 - retry scheduled in"
                    << delay << "seconds; normal retry budget unchanged:" << dr->url();
        dr->download_error(0, delay);
        reschedule(uid);
        return;
    }

    if (dr->error_count() >= maxRetries) {
        QLOG_WARN() << "DownloadManager :: Giving up after"
                    << dr->error_count() << "retries:" << dr->url() << reason;
        dr->requestHandler()->error(uid, 503);
        return;
    }

    const int attempt = dr->error_count() + 1;
    const int delay = qMin(120, retryDelaySeconds * (1 << qMin(attempt-1, 5)));
    QLOG_INFO() << "DownloadManager :: Retry" << attempt << "of"
                << maxRetries << "in" << delay << "seconds:" << dr->url();
    dr->download_error(1, delay);
    reschedule(uid);
}

void DownloadManager::reschedule(qint64 uid) {
    DownloadRequest* dr;
    int prio;

    QLOG_INFO() << "DownloadManager :: " << "rescheduling" << uid;
    dr = requestList.value(uid,0);
    if (dr != 0) {
        prio = dr->priority();
        dr->setProcessing(false);

        // "lower priority for rescheduled downloads"
        priorities.remove(prio, uid);
        if (dr->url().toString().indexOf(QRegExp(__IMAGE_REGEXP__, Qt::CaseInsensitive)) != -1) {
            prio = _highestPriority + 1;
            _highestPriority++;
        }
        else
        {
            prio += 1;
        }
        QLOG_INFO() << "DownloadManager :: " << uid << ":" << "setting new priority" << prio;
        dr->setPriority(prio);
        priorities.insertMulti(prio,uid);

        dr->reset();
        // Active-request accounting is owned solely by replyFinished().
        processRequests();
    }
}

void DownloadManager::retryReady() {
    if (videoCooldownUntil.isValid()
            && QDateTime::currentDateTimeUtc() >= videoCooldownUntil) {
        videoCooldownUntil = QDateTime();
        settings->remove("download_manager/video_cooldown_utc");
    }
    // Do not unpause a user-paused download queue merely because a retry is ready.
    processRequests();
}

void DownloadManager::resumeDownloads() {
    QLOG_INFO() << "DownloadManager :: " << "resuming downloads";
    downloadsPaused = false;

    processRequests();
}

void DownloadManager::removeRequest(qint64 uid) {
    int prio;

    if (requestList.count(uid)>0) {
        prio = requestList.value(uid)->priority();
        priorities.remove(prio, uid);
        DownloadRequest* removed = requestList.take(uid);
        if (removed)
            removed->deleteLater();

        // See if this id is currently downloading
        if (activeReplies.count(uid) > 0) {
            supervisors.remove(uid);
            activeReplies.value(uid)->abort();
        }
    }
}

void DownloadManager::getStatistics(int *files, float *kbytes) {
    *files = statistic_downloadedFiles;
    *kbytes = statistic_downloadedKBytes;
}

NetworkAccessManager* DownloadManager::getFreeNAM() {
    NetworkAccessManager* ret;
    QMap<int, NetworkAccessManager*> load;

    for (int i=0; i<nams.count(); i++) {
        load.insertMulti(nams.at(i)->activeRequests(), nams.at(i));
    }

    ret = load.begin().value(); // Returns NetworkAccessManager with the least active requests

    return ret;
}

void DownloadManager::setupNetworkAccessManagers(int count) {
    for (int i=nams.count(); i<count; i++) {
        nams.append(new NetworkAccessManager(this));
        // QNetworkAccessManager owns its cookie jar; never share a single jar.
        nams.last()->setCookieJar(new QNetworkCookieJar(nams.last()));
        connect(nams.last(), SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    }
}

int DownloadManager::getPendingRequests() {
    return priorities.count();
}

int DownloadManager::getRunningRequests() {
    return activeReplies.count();
}

int DownloadManager::getTotalRequests() {
    return totalRequests;
}

int DownloadManager::getFinishedRequests() {
    return finishedRequests;
}

void DownloadManager::pauseDownloads() {
    downloadsPaused = true;
}

void DownloadManager::setMaxPriority(int mp) {
    _max_priority = mp;

    QLOG_INFO() << "DownloadManager :: Setting max priority to " << mp;
    if (!downloadsPaused) {
        processRequests();
    }
}

QMap<qint64, QString> DownloadManager::getPendingRequestsMap() {
    QMap<qint64, QString> ret;
    QHashIterator<qint64, DownloadRequest*>  requests(requestList);

    while (requests.hasNext()) {
        requests.next();

        ret.insertMulti(requests.value()->priority(), requests.value()->url().toString());
    }

    return ret;
}

QMap<qint64, QString> DownloadManager::getRunningRequestsMap() {
    QMap<qint64, QString> ret;
    QHashIterator<qint64, QNetworkReply*>  requests(activeReplies);
/*
    while (requests.hasNext()) {
        requests.next();

        ret.insertMulti(0, requests.value()->url().toString());
    }
*/
    return ret;
}

bool DownloadManager::cacheAvailable(QUrl url) {
    bool ret;
    QFile f;

    ret = false;

    if (_useThreadCache && !_threadCachePath.isEmpty()) {
        if (QFile::exists(getFilenameForURL(url))) {
            ret = true;
        }
    }

    return ret;
}

QByteArray DownloadManager::getCachedReply(QUrl url) {
    QByteArray ret;
    QFile f;

    if (cacheAvailable(url)) {
        f.setFileName(getFilenameForURL(url));
        f.open(QIODevice::ReadOnly);

        if (f.isOpen() && f.isReadable()) {
            if (settings->value("download_manager/compress_cache_file", true).toBool()) {
                ret = qUncompress(f.readAll());
            }
            else {
                ret = f.readAll();
            }
            QLOG_DEBUG() << "DownloadManager :: reading cache file for " << url.toString();
        }
        f.close();
    }

    return ret;
}

QString DownloadManager::encodeURL(QString in) {
    QString ret;

    ret = QUrl::toPercentEncoding(in);
    QLOG_DEBUG() << "DownloadManager :: encoding URL from " << in << " to " << ret;

    return ret;
}

QString DownloadManager::decodeURL(QString in) {
    QString ret;

    ret = QUrl::fromPercentEncoding(in.toUtf8());
    QLOG_DEBUG() << "DownloadManager :: decoding URL from " << in << " to " << ret;

    return ret;
}

QString DownloadManager::getFilenameForURL(QUrl url) {
    QString ret;

    if (!_threadCachePath.isEmpty()) {
        ret = QString("%1/%2.tcache").arg(_threadCachePath, encodeURL(url.toString()));
    }
    else {
        ret = ".";
    }

    QLOG_DEBUG() << "DownloadManager :: threadCache filename=" << ret;
    return ret;
}

int DownloadManager::getHighestPriority() {
    return _highestPriority;
}
