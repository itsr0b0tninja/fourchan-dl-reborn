#ifndef DOWNLOADREQUEST_H
#define DOWNLOADREQUEST_H

#include <QObject>
#include <QSaveFile>
#include "requesthandler.h"

class RequestHandler;

class DownloadRequest : public QObject
{
    Q_OBJECT
public:
    explicit DownloadRequest(QObject *parent = 0);

    void setRequestHandler(RequestHandler* rh) {_rh = rh;}
    RequestHandler* requestHandler() {return _rh;}

    void setUrl(QUrl url) {_url = url;}
    QUrl url() {return _url;}

    void setPriority(int prio) {_prio = prio;}
    int priority() {return _prio;}

    QString savePath() const {return _savePath;}
    void setSavePath(const QString &path) {_savePath = path;}
    bool hasFileDestination() const {return !_savePath.isEmpty();}
    bool beginStreaming();
    bool appendStreaming(const QByteArray &bytes);
    bool commitStreaming();
    void abortStreaming();
    bool streamFailed() const {return _streamFailed;}
    qint64 streamedBytes() const {return _streamedBytes;}

    QByteArray response(void) {return _response;}
    void setResponse(QByteArray ba);

    bool processing() {return _processing;}
    void setProcessing(bool b) {_processing = b;}
    void reset();
    bool finished() {return _finished;}
    bool cached() {return _cached_reply;}
    void setCached(bool b) {_cached_reply = b;}

    bool paused() {return _paused;}
    void pause(int s);

    qint64 expectedSize() const {return _expected_size;}
    void setExpectedSize(qint64 bytes) {_expected_size = qMax(qint64(0), bytes);}

    int error_count() {return _error_count;}
    void download_error(int increment=1, int delaySeconds=10);

private:
    RequestHandler* _rh;
    QUrl _url;
    int _prio;
    QByteArray _response;
    QString _savePath;
    QSaveFile* _streamFile;
    qint64 _streamedBytes;
    bool _streamFailed;
    QTimer* pauseTimer;
    bool _processing;
    bool _finished;
    bool _cached_reply;
    bool _paused;
    int _error_count;
    qint64 _expected_size;


private slots:
    void pauseTimerTriggered();

signals:
    void requestUnpaused();
};

#endif // DOWNLOADREQUEST_H
