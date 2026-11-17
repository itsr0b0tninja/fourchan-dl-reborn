#ifndef UIIMAGEOVERVIEW_H
#define UIIMAGEOVERVIEW_H

#include <QWidget>
#include <QFileDialog>
#include <QMenu>
#include <QClipboard>
#include <QDesktopServices>
#include <QStyle>
#include <QSettings>
#include <QSet>
#include <QHash>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSaveFile>
#include <QFileInfo>
#include <QImage>
#include <QCloseEvent>
#include <QThread>
#include "threadfilemover.h"
#include "defines.h"
#include "mainwindow.h"
#include "thumbnailcreator.h"
#include "blacklist.h"
#include "downloadmanager.h"
#include "foldershortcuts.h"
#include "pluginmanager.h"
#include "requesthandler.h"
#include "uiimageviewer.h"
#include "HtmlEntities.h"

class ThumbnailCreator;
class MainWindow;

extern ThumbnailCreator* tnt;
extern FolderShortcuts* folderShortcuts;
extern MainWindow* mainWindow;
extern PluginManager* pluginManager;
extern UIImageViewer* imageViewer;

namespace Ui {
    class UIImageOverview;
}

class UIImageOverview : public QWidget
{
    Q_OBJECT

public:
    explicit UIImageOverview(QWidget *parent = 0);
    ~UIImageOverview();
    QString getURI();
    QString getTitle();
    QString getStatus();
    bool hasNewImages();
    void threadViewed();

    void setDirectory(QString);
    QString getValues(void);
    void setValues(QString);
    bool setThumbnailSize(QSize s);
    void useOriginalFilenames(bool);
    void updateSettings(void);
    void setBlackList(BlackList* bl);
    int getTotalImagesCount() {return images.count();}
    int getDownloadedImagesCount();
    bool prepareMoveThread(const QString &destinationRoot, QList<ThreadMoveFile> *moves,
                           QString *error);
    void setMoveInProgress(bool moving);
    void showMoveError(const QString &detail);
    QSet<QString> knownDownloadedFilePaths() const;

private:
    Ui::UIImageOverview *ui;
    QTimer* timer;
    QTimer* thumbnailCheckTimer;
    QList<int> timeoutValues;
    QList<_IMAGE> images;
    QHash<QString, int> imageIndexByUrl;
    QSet<QString> originalFilenameSet;
    QHash<QString, int> nextFilenameNumber;
    int nextImageSearchIndex;
    QSet<QString> failedImageUrls;
    QSet<QString> missingImageUrls;
    QPushButton* retryFailedButton;
    QComboBox* videoOverrideCombo;
    QLabel* attachmentInfoLabel;
    QSize iconSize;
    QAction* deleteFileAction;
    QAction* reloadFileAction;
    QAction* openFileAction;
    QSettings* settings;
    QStringList pendingThumbnails;
    QMap<QString,QString> missingThumbs;

    BlackList* blackList;
    QString _status;
    RequestHandler* requestHandler;
    ParserPluginInterface* iParser;
    QObject* oParser;
    QThread* parserThread;

    bool thumbnailsizeLocked;
    bool running;
    bool closeWhenFinished;
    bool downloading;
    bool moveInProgress;
    int expectedThumbnailCount;
    int thumbnailCount;
    bool _cachedResult;
    bool _threadBlocked;
    bool fresh_thread;
    bool follow_redirects;
    int thumbnailCountLastViewed;

    void loadSettings(void);
    void setStatus(QString s);
    void createSupervisedDownload(QUrl);
    void removeSupervisedDownload(QUrl);
    bool getUrlOfFilename(QString filename, QString * url);
    bool isDownloadFinished();
    bool isVideoAttachment(const QString &uri) const;
    bool videosAllowed() const;
    void updateAttachmentInfo();
    int pendingEnabledCount() const;
    void deleteAllThumbnails();
    bool getNextImage(QString* s);
    bool selectParser(QUrl url=QUrl());
    void mergeImageList(QList<_IMAGE> list);
    bool addImage(_IMAGE img);
    QString getSavepath();
    void updateExpectedThumbnailCount();
    void updateMoveThreadButton();

private slots:
    void on_listWidget_customContextMenuRequested(QPoint pos);
    void chooseLocation(void);
    void triggerRescan(void);
    void createThumbnail(QString);
    void deleteFile(void);
    void deleteAllFiles(void);
    void reloadFile(void);
    void openFile(void);
    void errorHandler(QUrl, int);
    void messageHandler(QString);
    void setTabTitle(QString);
    void debugButton(void);
    void processCloseRequest();
    void openURI();
    void openDownloadFolder();
    void selectShortcutIndex(int idx);
    void selectShortcut(QString name);
    void fillShortcutComboBox();
    void checkForFolderShortcut(QString s);
    void addShortcut();
    void rebuildThumbnails();
    void reloadThread();
    void retryFailedDownloads();
    void videoOverrideChanged(int index);

    void startDownload(void);
    void stopDownload(void);
    void download(bool b);
    void processRequestResponse(QUrl url, QByteArray ba, bool cached);
    void processDownloadedFile(QUrl url, QString filename);
    void setCompleted(QString uri, QString filename);
    bool isImage(QUrl);
    void updateDownloadStatus();
    void showImagePreview();
    void requestMoveThread();
    void blockThread(bool);

    void checkForMissingThumbnails();

public slots:
    void addThumbnail(QString, QString);
    void addThumbnails(QString);
    void start(void);
    void stop(void);
    void videoSettingChanged();
    void refreshMoveThreadButton();
    bool checkForExistingThread(QString s="");

signals:
    void finished(void);
    void errorMessage(QString);
    void tabTitleChanged(UIImageOverview*,QString);
    void directoryChanged(QString);
    void closeRequest(UIImageOverview*, int);
    void createTabRequest(QString);
    void removeFiles(QStringList);
    void changed();
    void moveThreadRequested(UIImageOverview*);
protected:
    void closeEvent(QCloseEvent *);
};

#endif // UI4CHAN_H
