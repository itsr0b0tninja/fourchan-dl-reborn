#ifndef THUMBNAILCREATOR_H
#define THUMBNAILCREATOR_H

#include <QObject>
#include <QImage>
#include <QList>
#include <QStringList>
#include <QSettings>
#include <QMutex>
#include <QWaitCondition>
#include <QDebug>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QUrl>
#include "QsLog.h"

class ThumbnailCreator : public QObject
{
    Q_OBJECT
public:
    explicit ThumbnailCreator(QObject *parent = 0);
    void setIconSize(QSize s);
    QString addToList(QString s, QString videoPreviewUrl = QString());
    QString getCacheFile(QString);
    void halt();
    void resume();
    void wakeup();
    // May be called from the GUI thread; synchronized using mutex.
    void requestStop();

private:
    QStringList list;
    QHash<QString, QString> videoPreviews;
    QSize* iconSize;
    volatile bool halted;
    QSettings* settings;
    bool newImages;
    bool stopping;
    QString cacheFolder;
    QMutex mutex;
    QWaitCondition condition;
    QImage fetchVideoPreview(const QString &previewUri) const;


signals:
    void pendingThumbnails(int);
    
public slots:
    void go();
    
};

#endif // THUMBNAILCREATOR_H
