#include "filehandler.h"

FileHandler::FileHandler(QObject *parent) :
    QObject(parent)
{
}

void FileHandler::startExchange(QList<FileUpdate> updates) {
    // Stage all replacements. Never delete an installed application file
    // before we have both a verified temporary file and a recoverable backup.
    updateList = updates;
    bool errors = false;
    QList<QString> installedTargets;
    QMap<QString,QString> backups;

    for (int i = 0; i < updateList.count(); ++i) {
        const FileUpdate& item = updateList.at(i);
        const QFileInfo downloaded(item.tmpFilename);
        if (item.tmpFilename.isEmpty() || !downloaded.isFile() || downloaded.size() == 0) {
            p("Missing or empty update file for " + item.filename);
            emit error("Downloaded update is missing: " + item.filename);
            errors = true;
            break;
        }
        if (QFileInfo(item.filename).isDir()) {
            emit error("Update destination is a directory: " + item.filename);
            errors = true;
            break;
        }
        const QString parent = QFileInfo(item.filename).absolutePath();
        if (!QDir(parent).exists()) {
            emit error("Update destination directory is missing: " + parent);
            errors = true;
            break;
        }
    }

    if (!errors) {
        for (int i = 0; i < updateList.count(); ++i) {
            const FileUpdate& item = updateList.at(i);
            const QString target = item.filename;
            const QString backup = target + ".reborn-backup-" +
                QUuid::createUuid().toString().remove("{").remove("}");
            if (QFile::exists(target)) {
                if (!QFile::rename(target, backup)) {
                    p("Unable to back up " + target);
                    emit error("Could not back up " + target);
                    errors = true;
                    break;
                }
                backups.insert(target, backup);
            }
            if (!QFile::rename(item.tmpFilename, target)) {
                p("Unable to install " + target);
                emit error("Could not install " + target);
                errors = true;
                break;
            }
            installedTargets.append(target);
        }
    }

    if (errors) {
        // Roll back every replacement already applied, including any target
        // whose previous file was moved before its new file could be installed.
        for (int i = installedTargets.count()-1; i >= 0; --i)
            QFile::remove(installedTargets.at(i));
        QMap<QString,QString>::const_iterator it = backups.constBegin();
        for (; it != backups.constEnd(); ++it) {
            if (!QFile::rename(it.value(), it.key())) {
                p("RESTORE FAILED: " + it.key() + " (backup at " + it.value() + ")");
                emit error("Restore failed. Backup retained: " + it.value());
            }
        }
    } else {
        QMap<QString,QString>::const_iterator it = backups.constBegin();
        for (; it != backups.constEnd(); ++it)
            QFile::remove(it.value());
    }

    p(errors ? "Update failed; rollback attempted" : "Update installed");
    emit exchangingFinished(errors);
}

void FileHandler::p(QString msg) {
    *output << "FileHandler: " << msg << endl;
    output->flush();
    *foutput << "FileHandler: " << msg << endl;
    foutput->flush();
}
