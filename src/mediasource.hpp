#pragma once

#include "mediaformat.hpp"
#include "medialoader.hpp"

#include <QPointer>
#include <QQmlEngine>
#include <QTemporaryFile>
#include <QUrl>

#include <memory>

class QNetworkReply;

class TmarkMediaSource : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(TmarkMediaSource)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QUrl playableSource READ playableSource NOTIFY playableSourceChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(TmarkMediaLoader* loader READ loader WRITE setLoader NOTIFY loaderChanged)
    Q_PROPERTY(QString kind READ kind WRITE setKind NOTIFY kindChanged)
    Q_PROPERTY(QSize lottieSize READ lottieSize NOTIFY lottieSizeChanged)

public:
    explicit TmarkMediaSource(QObject* parent = nullptr);

    QUrl source() const { return source_; }
    void setSource(const QUrl& source);
    QUrl playableSource() const { return playableSource_; }
    QString error() const { return error_; }
    TmarkMediaLoader* loader() const { return loader_; }
    void setLoader(TmarkMediaLoader* loader);
    QString kind() const { return kind_; }
    void setKind(const QString& kind);
    QSize lottieSize() const { return lottieSize_; }

signals:
    void sourceChanged();
    void playableSourceChanged();
    void errorChanged();
    void loaderChanged();
    void kindChanged();
    void lottieSizeChanged();

private:
    void setPlayableSource(const QUrl& source);
    void setError(const QString& error);

    TmarkMediaLoader defaultLoader_;
    QPointer<TmarkMediaLoader> loader_;
    QPointer<QNetworkReply> reply_;
    std::unique_ptr<QTemporaryFile> file_;
    QUrl source_;
    QUrl playableSource_;
    QString error_;
    QString kind_ = QStringLiteral("image");
    QSize lottieSize_;
};
