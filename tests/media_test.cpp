#include "mediaformat.hpp"
#include "medialoader.hpp"

#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkReply>
#include <QTemporaryFile>
#include <QTimer>

#include <iostream>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    TmarkMediaLoader loader;
    if (loader.fetch(QUrl("file:///tmp/image.png")) || loader.fetch(QUrl("custom:asset"))) return 1;
    const auto check = [&loader](const char* name, TmarkMediaKind kind, bool allowed) {
        auto* reply = loader.fetch(QUrl(QStringLiteral("https://tmark.waus.app/static/media-formats/") + QString::fromLatin1(name)));
        if (!reply) return false;
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(15000, &loop, &QEventLoop::quit);
        if (!reply->isFinished()) loop.exec();
        if (!reply->isFinished()) reply->abort();
        const QByteArray data = reply->readAll();
        const auto networkError = reply->error();
        delete reply;
        QTemporaryFile renamed;
        if (networkError != QNetworkReply::NoError || data.isEmpty() || !renamed.open()) {
            std::cerr << name << ": download failed\n";
            return false;
        }
        if (renamed.write(data) != data.size()) return false;
        renamed.flush();
        QSize lottieSize;
        const QString error = validateMediaFile(renamed.fileName(), kind, &lottieSize);
        if (error.isEmpty() != allowed) {
            std::cerr << name << ": " << error.toStdString() << '\n';
            return false;
        }
        if ((QString::fromLatin1(name) == "animation.json") != !lottieSize.isEmpty()) return false;
        return true;
    };
    for (const char* name : {"image.jpg", "image.png", "image.webp", "image.avif", "animation.json"})
        if (!check(name, TmarkMediaKind::Image, true)) return 1;
    for (const char* name : {"image.gif"})
        if (!check(name, TmarkMediaKind::Image, false)) return 1;
    QTemporaryFile externalAsset;
    if (!externalAsset.open() || externalAsset.write(R"({"v":"5.0","fr":30,"ip":0,"op":30,"w":100,"h":100,"layers":[],"assets":[{"p":"https://example.org/image.png"}]})") < 0) return 1;
    externalAsset.flush();
    if (validateMediaFile(externalAsset.fileName(), TmarkMediaKind::Image).isEmpty()) return 1;
    for (const char* name : {"tone.mp3", "tone.flac", "tone-opus.ogg", "tone-aac-lc.m4a"})
        if (!check(name, TmarkMediaKind::Audio, true)) return 1;
    for (const char* name : {"tone-vorbis.ogg", "tone-aac-he.m4a"})
        if (!check(name, TmarkMediaKind::Audio, false)) return 1;
    return 0;
}
