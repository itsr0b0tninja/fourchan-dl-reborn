#include "thumbnailcreator.h"
#include "thumbnailmetadata.h"
#include "fourchanrequestheaders.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>
#include <QMutexLocker>

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
    QString currentVideoPreview;
    QSize currentSize;
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
                currentVideoPreview = videoPreviews.take(currentFilename);
                currentSize = *iconSize;
                newImages = true;
                cacheFile = getCacheFileForSize(currentFilename, currentSize);
            }
        mutex.unlock();

        if (newImages) {
            // The worker reuses these images; never carry a previous job's
            // image into a failed video-preview request.
            original = QImage();
            tn = QImage();
            bool useCachedThumbnail;
            bool image_loaded;

            // Use one size snapshot for the cache key and the actual image.
            // The settings UI may update the shared size while we work.
            iconWidth = currentSize.width();
            iconHeight = currentSize.height();
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
            // Track the requested dimensions/options inside the cached PNG.
            // Merely matching one side let v1.1.0 reuse undersized images
            // after users increased their thumbnail settings.
            const QString cacheSignature =
                    QStringLiteral("v1.1.1:%1x%2:enlarge=%3:hq=%4")
                        .arg(iconWidth).arg(iconHeight)
                        .arg(enlargeThumbnails ? 1 : 0).arg(hqRendering ? 1 : 0);
            // Check if thumbnail exists
            if (QFile::exists(cacheFile)) {
                QLOG_TRACE() << "ThumbnailCreator :: Cached thumbnail available for " << currentFilename;
                if (tn.load(cacheFile) &&
                        tn.text(QStringLiteral("RebornThumbnailOptions")) == cacheSignature)
                    useCachedThumbnail = true;
            }

            if (!useCachedThumbnail){
                QLOG_TRACE() << "ThumbnailCreator :: Creating new thumbnail for " << currentFilename;
                if (ThumbnailMetadata::isVideo(currentFilename) &&
                        !currentVideoPreview.isEmpty()) {
                    original = fetchVideoPreview(currentVideoPreview);
                    image_loaded = !original.isNull();
                }
                if (!image_loaded && !ThumbnailMetadata::isVideo(currentFilename)
                        && original.load(currentFilename)) {
                    image_loaded = true;
                }
                if (!image_loaded) {
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
                    // 4chan video previews are often smaller than the
                    // configured thumbnail size. Upscale them smoothly for
                    // consistent-size tiles, without changing the preference
                    // for whether ordinary still images are enlarged.
                    const bool videoPreview = ThumbnailMetadata::isVideo(currentFilename);
                    if (original.width()<iconWidth
                        && original.height()<iconHeight
                        && !enlargeThumbnails && !videoPreview) {
                        QLOG_TRACE() << "ThumbnailCreator :: Setting original as thumbnail";
                        tn = original;
                    } else {
                        QLOG_TRACE() << "ThumbnailCreator :: Rendering thumbnail";

                        if (hqRendering || videoPreview) {
                            tn = original.scaled(currentSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
                        }
                        else {
                            tn = original.scaled(currentSize,Qt::KeepAspectRatio,Qt::FastTransformation);
                        }
                    }

                    tn.setText(QStringLiteral("RebornThumbnailOptions"), cacheSignature);
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

QImage ThumbnailCreator::fetchVideoPreview(const QString &previewUri) const {
    const QUrl url = ThumbnailMetadata::videoPreviewUrl(previewUri);
    if (url.isEmpty())
        return QImage();

    // This runs inside the thumbnail worker, so it never blocks the GUI.
    // A short deadline prevents unavailable thumbnails from stalling the
    // queue indefinitely; all failures fall back to image-missing.png.
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", FourchanRequestHeaders::defaultUserAgent());
    request.setRawHeader("Accept", "image/jpeg,image/*;q=0.8");
    const QString board = url.path().section('/', 1, 1);
    request.setRawHeader("Referer", QString("https://boards.4chan.org/%1/")
                                      .arg(board).toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QObject::connect(&timeout, SIGNAL(timeout()), &loop, SLOT(quit()));
    timeout.start(8000);
    if (!reply->isFinished())
        loop.exec();

    QImage result;
    if (reply->isFinished() && reply->error() == QNetworkReply::NoError &&
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200 &&
            reply->bytesAvailable() <= 2 * 1024 * 1024) {
        const QByteArray bytes = reply->readAll();
        result.loadFromData(bytes, "JPG");
        if (result.width() > 4096 || result.height() > 4096)
            result = QImage();
    } else {
        if (!reply->isFinished())
            reply->abort();
        QLOG_DEBUG() << "ThumbnailCreator :: 4chan video preview unavailable:"
                     << previewUri;
    }
    delete reply;
    return result;
}

void ThumbnailCreator::setIconSize(QSize s) {
    mutex.lock();
    iconSize->setWidth(s.width());
    iconSize->setHeight(s.height());
    mutex.unlock();
}

QString ThumbnailCreator::addToList(QString s, QString videoPreviewUrl) {
    QString ret;

    mutex.lock();
    // An already queued file can gain its video-preview URL later.
    // Return its expected cache path regardless of whether it was queued.
    ret = getCacheFileForSize(s, *iconSize);
    if (ThumbnailMetadata::isVideo(s) && !videoPreviewUrl.isEmpty())
        videoPreviews.insert(s, videoPreviewUrl);
    if (!list.contains(s)) {
        list.append(s);
        newImages = true;
    }
    mutex.unlock();
    condition.wakeAll();

    return ret;
}

QString ThumbnailCreator::getCacheFile(QString filename) {
    QMutexLocker locker(&mutex);
    return getCacheFileForSize(filename, *iconSize);
}

QString ThumbnailCreator::getCacheFileForSize(const QString &filename,
                                              const QSize &size) const {
    QString safeName = filename;
    safeName.replace(QRegExp("[" + QRegExp::escape("\\/:*?\"<>|") + "]"),
                     QStringLiteral("_"));
    // Different thumbnail sizes get different files. This prevents the GUI
    // from displaying an old cached image before the worker can regenerate
    // it at the user's newly chosen dimensions.
    const QString suffix = ThumbnailMetadata::isVideo(filename)
                           ? QStringLiteral(".video-preview.tn")
                           : QStringLiteral(".tn");
    return QStringLiteral("%1/%2.%3x%4%5")
            .arg(cacheFolder, safeName).arg(size.width()).arg(size.height()).arg(suffix);
}

void ThumbnailCreator::halt() {
    mutex.lock();
    halted = true;
    mutex.unlock();
}

void ThumbnailCreator::resume() {
    mutex.lock();
    halted = false;
    condition.wakeAll();
    mutex.unlock();
}
void ThumbnailCreator::requestStop() {
    mutex.lock();
    stopping = true;
    list.clear();
    videoPreviews.clear();
    condition.wakeAll();
    mutex.unlock();
}

void ThumbnailCreator::wakeup() {
    condition.wakeAll();
}
