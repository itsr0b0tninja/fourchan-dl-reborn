#include "downloadrequest.h"

DownloadRequest::DownloadRequest(QObject *parent) :
    QObject(parent)
{
    pauseTimer = new QTimer(this);
    pauseTimer->setSingleShot(true);

    connect(pauseTimer, SIGNAL(timeout()), this, SLOT(pauseTimerTriggered()));

    _paused = false;
    _error_count = 0;
    _expected_size = 0;
    _streamFile = 0;
    _streamedBytes = 0;
    _streamFailed = false;
    reset();
    _url = QUrl("");
    _prio = 0;
    _cached_reply = false;
}

void DownloadRequest::reset() {
    abortStreaming();
    _response.clear();
    _finished = false;
    _processing = false;
    // Retry counts survive re-queues. Only a new request starts at zero.
}

bool DownloadRequest::beginStreaming() {
    abortStreaming();
    if (_savePath.isEmpty())
        return true;
    _streamFile = new QSaveFile(_savePath, this);
    if (!_streamFile->open(QIODevice::WriteOnly)) {
        delete _streamFile;
        _streamFile = 0;
        return false;
    }
    return true;
}

bool DownloadRequest::appendStreaming(const QByteArray &bytes) {
    if (!_streamFile || _streamFailed)
        return false;
    if (bytes.isEmpty())
        return true;
    const qint64 written = _streamFile->write(bytes);
    if (written != bytes.size()) {
        _streamFailed = true;
        return false;
    }
    _streamedBytes += written;
    return true;
}

bool DownloadRequest::commitStreaming() {
    if (!_streamFile || _streamFailed)
        return false;
    const bool ok = _streamFile->commit();
    delete _streamFile;
    _streamFile = 0;
    return ok;
}

void DownloadRequest::abortStreaming() {
    if (_streamFile) {
        _streamFile->cancelWriting();
        delete _streamFile;
        _streamFile = 0;
    }
    _streamedBytes = 0;
    _streamFailed = false;
}

void DownloadRequest::setResponse(QByteArray ba) {
     _response = ba;
     _finished = true;
}

void DownloadRequest::pause(int s) {
    if (s <= 0 ) {
        s = 10;
    }

    _paused = true;
    _retryDeadline = QDateTime::currentDateTimeUtc().addSecs(s);
    pauseTimer->setInterval(s*1000);
    pauseTimer->start();
}

void DownloadRequest::pauseTimerTriggered() {
    _paused = false;
    _retryDeadline = QDateTime();

    emit requestUnpaused();
}

void DownloadRequest::download_error(int increment, int delaySeconds) {
    _error_count += increment;
    pause(delaySeconds);
}
