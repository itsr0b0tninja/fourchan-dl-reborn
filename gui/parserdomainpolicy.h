#ifndef PARSERDOMAINPOLICY_H
#define PARSERDOMAINPOLICY_H

#include <QString>
#include <QUrl>

// Match official hosts rather than strings containing a site's name.
namespace ParserDomainPolicy {
inline bool matchesHost(const QString &actualHost, const QString &domain) {
    const QString host = actualHost.trimmed().toLower();
    const QString expected = domain.trimmed().toLower();
    return !host.isEmpty() && !expected.isEmpty()
            && (host == expected || host.endsWith(QStringLiteral(".") + expected));
}

inline bool supportsUrl(const QUrl &url, const QString &domain) {
    if (!url.isValid())
        return false;
    const QString scheme = url.scheme().toLower();
    if (scheme != QStringLiteral("https") && scheme != QStringLiteral("http"))
        return false;
    if (matchesHost(url.host(), domain))
        return true;

    // Official 4channel.org aliases are used by some 4chan boards.
    return domain.compare(QStringLiteral("4chan.org"), Qt::CaseInsensitive) == 0
            && matchesHost(url.host(), QStringLiteral("4channel.org"));
}
} // namespace ParserDomainPolicy

#endif // PARSERDOMAINPOLICY_H
