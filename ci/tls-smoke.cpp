#include <QCoreApplication>
#include <QSslSocket>
#include <QTextStream>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);

    out << "Qt SSL build: " << QSslSocket::sslLibraryBuildVersionString() << "\n";
    out << "SSL runtime: " << QSslSocket::sslLibraryVersionString() << "\n";
    out << "QSslSocket::supportsSsl: " << (QSslSocket::supportsSsl() ? "yes" : "no") << "\n";
    out.flush();

    if (!QSslSocket::supportsSsl()) {
        err << "FAIL: Qt cannot initialise the bundled TLS runtime.\n";
        return 2;
    }

    QSslSocket socket;
    socket.connectToHostEncrypted("github.com", 443);
    if (!socket.waitForEncrypted(15000)) {
        err << "FAIL: GitHub TLS handshake: " << socket.errorString() << "\n";
        return 3;
    }

    out << "PASS: TLS handshake with github.com succeeded.\n";
    return 0;
}
