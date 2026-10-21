#include "mediasource.hpp"

#include <QNetworkReply>

namespace {
constexpr qint64 maxMediaBytes = 128 * 1024 * 1024;
}

TmarkMediaSource::TmarkMediaSource(QObject* parent) : QObject(parent), defaultLoader_(this) {}

void TmarkMediaSource::setLoader(TmarkMediaLoader* loader) {
    if (loader_ == loader) return;
    loader_ = loader;
    emit loaderChanged();
    const QUrl current = source_;
    setSource({});
    setSource(current);
}

void TmarkMediaSource::setKind(const QString& kind) {
    if (kind_ == kind) return;
    kind_ = kind;
    emit kindChanged();
    const QUrl current = source_;
    setSource({});
    setSource(current);
}

void TmarkMediaSource::setPlayableSource(const QUrl& source) {
    if (playableSource_ == source) return;
    playableSource_ = source;
    emit playableSourceChanged();
}

void TmarkMediaSource::setError(const QString& error) {
    if (error_ == error) return;
    error_ = error;
    emit errorChanged();
}

void TmarkMediaSource::setSource(const QUrl& source) {
    if (source_ == source) return;
    if (reply_) {
        reply_->disconnect(this);
        reply_->abort();
        reply_->deleteLater();
        reply_ = nullptr;
    }
    setPlayableSource({});
    if (!lottieSize_.isEmpty()) {
        lottieSize_ = {};
        emit lottieSizeChanged();
    }
    file_.reset();
    setError({});
    source_ = source;
    emit sourceChanged();
    if (source.isEmpty()) return;
    file_ = std::make_unique<QTemporaryFile>();
    if (!file_->open()) {
        setError(QStringLiteral("Could not create temporary media file"));
        return;
    }
    auto* reply = (loader_ ? loader_.data() : &defaultLoader_)->fetch(source);
    if (!reply) {
        setError(QStringLiteral("Unsupported media URL"));
        return;
    }
    reply_ = reply;
    connect(reply, &QIODevice::readyRead, this, [this, reply] {
        if (reply_ != reply || !error_.isEmpty()) return;
        const QByteArray chunk = reply->readAll();
        if (file_->size() + chunk.size() > maxMediaBytes || file_->write(chunk) != chunk.size()) {
            setError(QStringLiteral("Media download failed or exceeded 128 MiB"));
            reply->abort();
        }
    });
    const auto finish = [this, reply] {
        if (reply_ != reply) return;
        reply_ = nullptr;
        reply->deleteLater();
        if (!error_.isEmpty()) return;
        if (reply->error() != QNetworkReply::NoError) {
            setError(reply->errorString());
            return;
        }
        const QByteArray tail = reply->readAll();
        if (file_->size() + tail.size() > maxMediaBytes || file_->write(tail) != tail.size()) {
            setError(QStringLiteral("Media download failed or exceeded 128 MiB"));
            return;
        }
        file_->flush();
        file_->close();
        const auto kind = kind_ == "audio" ? TmarkMediaKind::Audio : kind_ == "video" ? TmarkMediaKind::Video : TmarkMediaKind::Image;
        QSize lottieSize;
        const QString error = validateMediaFile(file_->fileName(), kind, &lottieSize);
        if (!error.isEmpty()) {
            setError(error);
            return;
        }
        if (lottieSize_ != lottieSize) {
            lottieSize_ = lottieSize;
            emit lottieSizeChanged();
        }
        setPlayableSource(QUrl::fromLocalFile(file_->fileName()));
    };
    connect(reply, &QNetworkReply::finished, this, finish);
    if (reply->isFinished()) finish();
}
