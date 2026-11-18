#include "thumbnailcreator.h"

ThumbnailCreator::ThumbnailCreator(QObject *parent) :
    QObject(parent)
{
    iconSize = new QSize(100,100);
    newImages = false;
    stopping = false;
    settings = new QSettings("settings.ini", QSettings::IniFormat);
    cacheFolder = settings->value("options/thumbnail_cache_folder",
                                 QCoreApplication::applicationDirPath() + "/tncache").toString();
    halted = false;
}

void ThumbnailCreator::go() {
    QImage original, tn;
    QString cacheFile;
    QString currentFilename;
    QDir dir;
    int iconWidth;
    int iconHeight;
    bool enlargeThumbnails;
    bool hqRendering;

    forever {
        mutex.lock();
            if (stopping) {
                mutex.unlock();
                break;
            }
            if (list.count()>0 && !halted) {
                currentFilename = list.front();
                list.pop_front();
                newImages = true;
                cacheFile = getCacheFile(currentFilename);
            }
        mutex.unlock();

        if (newImages) {
            bool useCachedThumbnail;
            bool image_loaded;

            iconWidth = iconSize->width();
            iconHeight = iconSize->height();
            enlargeThumbnails = settings->value("options/enlarge_thumbnails", false).toBool();
            hqRendering = settings->value("options/hq_thumbnails", false).toBool();
            cacheFolder = settings->value("options/thumbnail_cache_folder", QString("%1/%2").arg(QCoreApplication::applicationDirPath())
                                          .arg("tncache")).toString();

    //        QLOG_ALWAYS() << "ThumbnailCreator :: Using thumbnail folder " << cacheFolder;
            if (!dir.exists(cacheFolder)) {
                QLOG_TRACE() << "ThumbnailCreator :: Creating thumbnail cache folder " << cacheFolder;
                dir.mkpath(cacheFolder);
            }

            useCachedThumbnail = false;
            image_loaded = false;
            // Check if thumbnail exists
            if (QFile::exists(cacheFile)) {
                QLOG_TRACE() << "ThumbnailCreator :: Cached thumbnail available for " << currentFilename;
                tn.load(cacheFile);

                if (tn.width() == iconWidth || tn.height() == iconHeight) {
                    useCachedThumbnail = true;
                }
            }

            if (!useCachedThumbnail){
                QLOG_TRACE() << "ThumbnailCreator :: Creating new thumbnail for " << currentFilename;
                if (original.load(currentFilename)) {
                    image_loaded = true;
                }
                else {
                    if (QFile::exists(currentFilename)) {
                        QLOG_DEBUG() << Q_FUNC_INFO << ":: Image" << currentFilename << "cannot be processed. Using default thumbnail instead.";
                        if (original.load(":/icons/resources/image-missing.png")) {
                            image_loaded = true;

                            QLOG_TRACE() << Q_FUNC_INFO << ":: Loaded default image";
                        }
                    }
                    else {
                        QLOG_ERROR() << Q_FUNC_INFO << ":: Image" << currentFilename << " does not exist. Thumbnail not created.";
                        image_loaded = false;
                    }
                }

                if (image_loaded) {
                    QLOG_TRACE() << "ThumbnailCreator :: Loaded original file " << currentFilename;
                    if (original.width()<iconWidth
                        && original.height()<iconHeight
                        && !(enlargeThumbnails)) {
                        QLOG_TRACE() << "ThumbnailCreator :: Setting original as thumbnail";
                        tn = original;
                    } else {
                        QLOG_TRACE() << "ThumbnailCreator :: Rendering thumbnail";

                        if (hqRendering) {
                            tn = original.scaled(*iconSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
                        }
                        else {
                            tn = original.scaled(*iconSize,Qt::KeepAspectRatio,Qt::FastTransformation);
                        }
                    }

                    tn.save(cacheFile, "PNG");
                    QLOG_TRACE() << "ThumbnailCreator :: Saving thumbnail as " << cacheFile;
                }
            }

            // The GUI continues to discover completed thumbnails by cache polling.
            mutex.lock();
            emit pendingThumbnails(list.count());
            newImages = false;
            if (list.isEmpty())
                condition.wait(&mutex);
            mutex.unlock();
        }
        else {
            mutex.lock();
                condition.wait(&mutex);
            mutex.unlock();
        }
    }
}

void ThumbnailCreator::setIconSize(QSize s) {
    mutex.lock();
    iconSize->setWidth(s.width());
    iconSize->setHeight(s.height());
    mutex.unlock();
}

QString ThumbnailCreator::addToList(QString s) {
    QString ret;

    mutex.lock();
    // Even an already-queued thumbnail must return its expected cache path.
    ret = getCacheFile(s);
    if (!list.contains(s)) {
        list.append(s);
        newImages = true;
    }
    mutex.unlock();
    condition.wakeAll();

    return ret;
}

QString ThumbnailCreator::getCacheFile(QString filename) {
    QString tmp, ret;

    tmp = filename;
    tmp.replace( QRegExp( "[" + QRegExp::escape( "\\/:*?\"<>|" ) + "]" ), QString( "_" ) );
    ret= QString("%1/%2.tn").arg(cacheFolder).arg(tmp);

    return ret;
}

void ThumbnailCreator::halt() {
    mutex.lock();
    halted = true;
    mutex.unlock();
}

void ThumbnailCreator::resume() {
    mutex.lock();
    halted = false;
    mutex.unlock();
}
void ThumbnailCreator::requestStop() {
    mutex.lock();
    stopping = true;
    list.clear();
    condition.wakeAll();
    mutex.unlock();
}

void ThumbnailCreator::wakeup() {
    condition.wakeAll();
}
