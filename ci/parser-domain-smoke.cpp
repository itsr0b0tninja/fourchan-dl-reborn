#include <QCoreApplication>
#include <QDebug>
#include <QUrl>
#include "parserdomainpolicy.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    const QString domain = QStringLiteral("4chan.org");

    if (!ParserDomainPolicy::supportsUrl(QUrl("https://boards.4chan.org/g/thread/1234"), domain)) return 1;
    if (!ParserDomainPolicy::supportsUrl(QUrl("https://boards.4channel.org/pol/thread/1234"), domain)) return 2;
    if (!ParserDomainPolicy::supportsUrl(QUrl("https://4chan.org/"), domain)) return 3;
    if (!ParserDomainPolicy::supportsUrl(QUrl::fromUserInput("boards.4channel.org/pol/thread/1234"), domain)) return 4;
    if (ParserDomainPolicy::supportsUrl(QUrl("https://fake4chan.org/g/thread/1234"), domain)) return 5;
    if (ParserDomainPolicy::supportsUrl(QUrl("https://boards.4chan.org.evil.example/g/thread/1234"), domain)) return 6;
    if (ParserDomainPolicy::supportsUrl(QUrl("https://other.example/?redirect=boards.4chan.org"), domain)) return 7;
    if (ParserDomainPolicy::supportsUrl(QUrl("ftp://boards.4chan.org/g/thread/1234"), domain)) return 8;
    if (ParserDomainPolicy::supportsUrl(QUrl("https://boards.4channel.org.evil.example"), domain)) return 9;
    if (!ParserDomainPolicy::supportsUrl(QUrl("https://docs.example.org/path"), QStringLiteral("example.org"))) return 10;

    qInfo() << "PASS: parser host matching, official alias and lookalike rejection";
    return 0;
}
