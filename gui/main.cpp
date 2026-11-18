#include <QApplication>
#include <QtDebug>

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QDateTime>
#include <QSslSocket>
#include <QThread>
#include "mainwindow.h"
#include "downloadmanager.h"
//#include "thumbnailthread.h"
#include "thumbnailcreator.h"
#include "foldershortcuts.h"
#include "pluginmanager.h"
#include "uiimageviewer.h"
#include "apptheme.h"
#include "legacyruntimecleanup.h"
#include "QsLog.h"
#include "QsLogDest.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#error "4chan Downloader Reborn requires Qt5 or newer."
#endif

DownloadManager* downloadManager;
ThumbnailCreator* tnt;
FolderShortcuts* folderShortcuts;
PluginManager* pluginManager;
QString updaterFileName;
UIImageViewer* imageViewer;
QFile* fLogFile;
QTextStream logOutput;
MainWindow* mainWindow;

void checkEnvironment();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // Portable mode: resolve settings.ini, thumbnails and updater paths beside the executable.
    QDir::setCurrent(a.applicationDirPath());
    a.setApplicationName("4chan Downloader Reborn");
    a.setApplicationVersion(PROGRAM_VERSION);
    QSettings settings("settings.ini", QSettings::IniFormat);
    int logLevel;

    // v0.4.2: both Light and Dark use the built-in Fusion style.
    // No styles/qwindowsvistastyle.dll is required.
    a.setStyle("fusion");

    // Apply the stored Light/Dark palette before creating any widgets.
    AppTheme::applyTheme(settings.value("options/theme", "light").toString());

    // init the logging mechanism
    QsLogging::Logger& logger = QsLogging::Logger::instance();
    logger.setLoggingLevel(QsLogging::TraceLevel);
    const QString sLogPath(QDir(a.applicationDirPath()).filePath("fourchan-dl.log"));
    QsLogging::DestinationPtr fileDestination(
       QsLogging::DestinationFactory::MakeFileDestination(sLogPath) );
    QsLogging::DestinationPtr debugDestination(
       QsLogging::DestinationFactory::MakeDebugOutputDestination() );
    logger.addDestination(debugDestination.get());
    logger.addDestination(fileDestination.get());
#ifdef __DEBUG__
    logger.setLoggingLevel(QsLogging::TraceLevel);
#else
    logger.setLoggingLevel(QsLogging::WarnLevel);
#endif

    logLevel = settings.value("options/log_level", -1).toInt();
    if (logLevel != -1) {
        logger.setLoggingLevel((QsLogging::Level)logLevel);
        QLOG_ALWAYS() << "APP :: Setting logging level to " << logLevel;
    }

    QLOG_INFO() << "APP :: Program started";
    QLOG_INFO() << "APP :: Built with Qt" << QT_VERSION_STR << "running on" << qVersion();
    QLOG_INFO() << "APP :: Qt SSL library build:" << QSslSocket::sslLibraryBuildVersionString();
    QLOG_INFO() << "APP :: SSL runtime:" << QSslSocket::sslLibraryVersionString();
    QLOG_INFO() << "APP :: TLS available:" << QSslSocket::supportsSsl();

    checkEnvironment();
    downloadManager = new DownloadManager();
    folderShortcuts = new FolderShortcuts();
    pluginManager = new PluginManager();

    downloadManager->pauseDownloads();  // Do not download anything until we are fully set

    QThread thumbnailThread;
    tnt = new ThumbnailCreator();
    tnt->moveToThread(&thumbnailThread);
//    thumbnailThread.setPriority(QThread::NormalPriority);

    a.connect(&thumbnailThread, SIGNAL(started()), tnt, SLOT(go()));
    a.connect(&thumbnailThread, SIGNAL(finished()), tnt, SLOT(deleteLater()));
    downloadManager->resumeDownloads();


    thumbnailThread.start();

    mainWindow = new MainWindow();
    imageViewer = new UIImageViewer(mainWindow);

    mainWindow->show();
    mainWindow->restoreTabs();

    a.connect(&a, SIGNAL(aboutToQuit()), mainWindow, SLOT(aboutToQuit()));
    a.connect(mainWindow, SIGNAL(quitAll()), downloadManager, SLOT(deleteLater()));

    const int exitCode = a.exec();

    // QThread::quit() cannot unblock ThumbnailCreator::go() by itself.
    // Ask the worker to stop, wake it and join the thread before destruction.
    tnt->requestStop();
    thumbnailThread.quit();
    thumbnailThread.wait();
    return exitCode;
}

void checkEnvironment() {
    // Reborn is a Qt5 portable application. Keep all runtime and user files
    // in the application directory; only the standalone updater is allowed
    // to have its own Qt/SSL dependencies.
    const QDir root(QCoreApplication::applicationDirPath());
    if (!root.exists(QStringLiteral("updater")))
        root.mkdir(QStringLiteral("updater"));
    if (!root.exists(QStringLiteral("plugins")))
        root.mkdir(QStringLiteral("plugins"));

#ifdef Q_OS_WIN
    // Bootstrap missing updater dependencies from existing root DLLs, but do
    // not try to copy upd4t3r.exe from the root: it is installed directly
    // inside updater/ in every Reborn Qt5 portable release.
    const QStringList updaterDependencies = QStringList()
            << QStringLiteral("Qt5Core.dll")
            << QStringLiteral("Qt5Network.dll")
            << QStringLiteral("libgcc_s_seh-1.dll")
            << QStringLiteral("libstdc++-6.dll")
            << QStringLiteral("libwinpthread-1.dll")
            << QStringLiteral("libssl-3-x64.dll")
            << QStringLiteral("libcrypto-3-x64.dll");
    for (const QString &file : updaterDependencies) {
        const QString source = root.filePath(file);
        const QString destination = root.filePath(QStringLiteral("updater/") + file);
        if (!QFileInfo::exists(source) || QFileInfo::exists(destination))
            continue;
        if (!QFile::copy(source, destination))
            QLOG_WARN() << "APP :: Could not copy updater dependency" << file;
    }
#endif

    const QString modernUpdater = root.filePath(QStringLiteral("updater/upd4t3r.exe"));
    const QString legacyUpdater = root.filePath(QStringLiteral("updater/au.exe"));
    updaterFileName.clear();
    if (QFileInfo(modernUpdater).isFile()) {
        updaterFileName = QStringLiteral("updater/upd4t3r.exe");
#ifdef Q_OS_WIN
        // Explicit names only. No recursive deletion or wildcard cleanup.
        // Never delete settings.ini, threads, thumbnails, logs or unknown files.
        const QStringList removed = LegacyRuntimeCleanup::removeObsoleteFiles(root.path());
        for (const QString &file : removed)
            QLOG_INFO() << "APP :: Removed obsolete pre-Qt5 file:" << file;
#endif
    } else if (QFileInfo(legacyUpdater).isFile()) {
        // Preserve the fallback for installations not yet fully migrated.
        updaterFileName = QStringLiteral("updater/au.exe");
    } else {
        QLOG_WARN() << "APP :: No updater found!";
    }
}
