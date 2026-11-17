#include "blacklist.h"
#include <QSaveFile>
#include <QTextStream>

BlackList::BlackList(QObject *parent) :
    QObject(parent)
{
    timer = new QTimer(this);
    cleanupBatchTimer = new QTimer(this);
    manager = new QNetworkAccessManager(this);
    settings = new QSettings("settings.ini", QSettings::IniFormat, this);
    useBlackList = true;
    activeCleanupRequests = 0;

    cleanupBatchTimer->setInterval(1000);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));
    connect(timer, SIGNAL(timeout()), this, SLOT(cleanUp()));
    connect(cleanupBatchTimer, SIGNAL(timeout()), this, SLOT(processCleanupBatch()));

    loadBlackList();
    loadSettings();
}

BlackList::~BlackList() {
    saveBlackList();
}

void BlackList::loadBlackList() {
    QFile file("black.list");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while (!file.atEnd()) {
        const QString url = QString::fromUtf8(file.readLine()).trimmed();
        if (!url.isEmpty())
            entries.insert(url.toCaseFolded(), url);
    }
}

void BlackList::saveBlackList() {
    // Atomic replacement avoids losing the list if the application closes
    // during a write.
    QSaveFile file("black.list");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QLOG_WARN() << "BlackList :: Could not save black.list:" << file.errorString();
        return;
    }
    QStringList urls = entries.values();
    urls.sort(Qt::CaseInsensitive);
    foreach (const QString &url, urls) {
        if (file.write(url.toUtf8() + "\n") < 0) {
            file.cancelWriting();
            return;
        }
    }
    if (!file.commit())
        QLOG_WARN() << "BlackList :: Could not commit black.list:" << file.errorString();
}

void BlackList::add(QString uri) {
    const QString key = uri.toCaseFolded();
    if (!entries.contains(key)) {
        entries.insert(key, uri);
        QLOG_INFO() << "BlackList :: Blacklisted image" << uri;
        saveBlackList();
    }
}

void BlackList::remove(QString uri) {
    if (entries.remove(uri.toCaseFolded()) > 0)
        saveBlackList();
}

bool BlackList::contains(QString uri) {
    return useBlackList && entries.contains(uri.toCaseFolded());
}

void BlackList::cleanUp() {
    // Never send one HEAD request per entry at application startup, and
    // do not start a second cleanup while one is already running.
    if (!useBlackList || !pendingCleanup.isEmpty() || activeCleanupRequests > 0)
        return;
    pendingCleanup = entries.values();
    if (!pendingCleanup.isEmpty())
        cleanupBatchTimer->start();
}

void BlackList::processCleanupBatch() {
    // At most 2 outstanding requests, and at most 2 new requests per second.
    int issued = 0;
    while (activeCleanupRequests < 2 && issued < 2 && !pendingCleanup.isEmpty()) {
        const QString uri = pendingCleanup.takeFirst();
        if (!entries.contains(uri.toCaseFolded()))
            continue;
        manager->head(QNetworkRequest(QUrl(uri)));
        ++activeCleanupRequests;
        ++issued;
    }
    if (pendingCleanup.isEmpty() && activeCleanupRequests == 0)
        cleanupBatchTimer->stop();
}

void BlackList::replyFinished(QNetworkReply* reply) {
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404
            || reply->error() == QNetworkReply::ContentNotFoundError)
        remove(reply->url().toString());
    activeCleanupRequests = qMax(0, activeCleanupRequests - 1);
    reply->deleteLater();
    if (pendingCleanup.isEmpty() && activeCleanupRequests == 0)
        cleanupBatchTimer->stop();
}

void BlackList::loadSettings() {
    useBlackList = settings->value("blacklist/use_blacklist", true).toBool();
    const int seconds = qMax(60, settings->value(
        "blacklist/blacklist_check_interval", 600).toInt());
    timer->stop();
    timer->setInterval(seconds * 1000);
    timer->start();
    if (!useBlackList) {
        pendingCleanup.clear();
        cleanupBatchTimer->stop();
    }
}
