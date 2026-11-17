#include <QCoreApplication>
#include <QDebug>
#include <QNetworkRequest>
#include "fourchanrequestheaders.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    const QUrl mp4("https://i.4cdn.org/b/1794935740430078.mp4");
    QNetworkRequest request(mp4);
    FourchanRequestHeaders::apply(request, mp4, "Wget/1.12");
    if (request.rawHeader("User-Agent") != FourchanRequestHeaders::defaultUserAgent()) return 1;
    if (request.rawHeader("Referer") != "https://boards.4chan.org/b/") return 2;
    if (request.rawHeader("Range") != "bytes=0-") return 3;
    if (!request.rawHeader("Accept").contains("video/mp4")) return 4;

    const QUrl webm("https://i.4cdn.org/wsg/123456.webm");
    QNetworkRequest custom(webm);
    FourchanRequestHeaders::apply(custom, webm, "MyTestAgent/1.0");
    if (custom.rawHeader("User-Agent") != "MyTestAgent/1.0") return 5;
    if (custom.rawHeader("Referer") != "https://boards.4chan.org/wsg/") return 6;

    const QUrl image("https://i.4cdn.org/b/123456.jpg");
    QNetworkRequest jpg(image);
    FourchanRequestHeaders::apply(jpg, image, QByteArray());
    if (jpg.hasRawHeader("Range") || jpg.hasRawHeader("Referer")) return 7;

    const QUrl thread("https://a.4cdn.org/b/thread/123456.json");
    QNetworkRequest api(thread);
    FourchanRequestHeaders::apply(api, thread, QByteArray());
    if (api.hasRawHeader("Range") || api.hasRawHeader("Referer")) return 8;

    qInfo() << "PASS: video browser-style headers, full-range GET, UA migration, images/API unchanged";
    return 0;
}
