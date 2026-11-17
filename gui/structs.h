#ifndef STRUCTS_H
#define STRUCTS_H

#include <QtGlobal>

struct _IMAGE
{
    QString largeURI;
    QString thumbURI;
    QString originalFilename;
    QString savedAs;
    bool downloaded;
    bool requested;
    qint64 expectedSize; // Expected attachment bytes from 4chan's fsize, if known.
};

struct ParsingStatus
{
    bool hasImages;
    bool hasTitle;
    bool isFrontpage;
    bool hasErrors;
    bool hasRedirect;
    bool threadFragmented;
};

struct component_information {
    QString componentName;
    QString type;
    QString filename;
    QString version;
    QString remote_version;
    QString src;
    QString target;
};

#endif // STRUCTS_H
