#ifndef BLACKLIST_H
#define BLACKLIST_H

#include <QObject>
#include <QtNetwork>
#include <QTimer>
#include <QFile>
#include <QHash>
#include <QStringList>
#include "QsLog.h"

class BlackList : public QObject
{
    Q_OBJECT
public:
    explicit BlackList(QObject *parent = 0);
    ~BlackList();
    void add(QString);
    void remove(QString);
    bool contains(QString);

private:
    // Case-insensitive membership is O(1), even with many monitored threads.
    // Keep the original URL for persistence and HEAD cleanup.
    QHash<QString, QString> entries;
    QNetworkAccessManager* manager;
    QTimer* timer;
    QTimer* cleanupBatchTimer;
    QSettings* settings;
    bool useBlackList;
    int activeCleanupRequests;
    QStringList pendingCleanup;

    void loadBlackList();
    void saveBlackList();

private slots:
    void replyFinished(QNetworkReply*);
    void cleanUp();
    void processCleanupBatch();

public slots:
    void loadSettings();
};

#endif // BLACKLIST_H
