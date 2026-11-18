#ifndef DEFINES_H
#define DEFINES_H

#ifdef Q_OS_WIN
#define APP_NAME        "4chan-dl-reborn.exe"
#define UPDATE_TREE     "win32-qt5"
#define USE_UPDATER
#else
#ifdef Q_OS_LINUX
#define APP_NAME        "fourchan-dl"
#define UPDATE_TREE     "linux"
//#define UPDATER_NAME    "au"
//#define USE_UPDATER
#else
#define APP_NAME        ""
#define UPDATE_TREE     "source"
#endif
#endif

#define debug_out(STRING, LEVEL)    qDebug() << STRING;

#define __IMAGE_REGEXP__ "(\\.jpg|\\.gif|\\.png|\\.jpeg|\\.webm|\\.mp4)+"
#define __IMAGEFILE_REGEXP__ QString("%1%2").arg("\\/(\\w+)").arg(__IMAGE_REGEXP__)

#endif // DEFINES_H
