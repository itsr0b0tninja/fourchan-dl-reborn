#ifndef THREADFILEMOVER_H
#define THREADFILEMOVER_H

#include <QThread>
#include <QList>
#include <QString>

// Plain file paths: no parser, Qt widget, or mutable thread state crosses threads.
struct ThreadMoveFile {
    QString source;
    QString destination;
};

class ThreadFileMover : public QThread
{
public:
    explicit ThreadFileMover(const QList<ThreadMoveFile> &files, QObject *parent = 0);

    QString error() const { return failure; }

    // Replace only the template root. The parser-expanded suffix stays intact.
    // E.g. D:/root/%b/%n -> C:/archive/b/123.
    static QString mappedDirectory(const QString &pattern, const QString &resolvedSource,
                                   const QString &destinationRoot, QString *error);

protected:
    void run() override;

private:
    QList<ThreadMoveFile> transfers;
    QString failure;
    static QByteArray fileDigest(const QString &filename, QString *error);
};

#endif // THREADFILEMOVER_H
