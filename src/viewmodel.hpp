#pragma once

#include <QObject>
#include <QHash>
#include <QVariantMap>
#include <QtQmlIntegration>

#include "medialoader.hpp"
#include "mediasource.hpp"

class TmarkDocument : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(bool softParser READ softParser WRITE setSoftParser NOTIFY softParserChanged)
    Q_PROPERTY(QVariantMap document READ document NOTIFY documentChanged)
    Q_PROPERTY(QString error READ error NOTIFY documentChanged)
    Q_PROPERTY(TmarkMediaLoader* mediaLoader READ mediaLoader WRITE setMediaLoader NOTIFY mediaLoaderChanged)
public:
    explicit TmarkDocument(QObject* parent = nullptr) : QObject(parent) {}
    QString text() const { return text_; }
    void setText(const QString& text);
    bool softParser() const { return softParser_; }
    void setSoftParser(bool soft);
    QVariantMap document() const { return document_; }
    QString error() const { return error_; }
    TmarkMediaLoader* mediaLoader() const { return mediaLoader_; }
    void setMediaLoader(TmarkMediaLoader* loader);
signals:
    void textChanged();
    void softParserChanged();
    void documentChanged();
    void mediaLoaderChanged();
private:
    void update();
    void clearIcons();
    QString text_;
    bool softParser_ = false;
    QVariantMap document_;
    QString error_;
    QPointer<TmarkMediaLoader> mediaLoader_;
    QHash<QString, TmarkMediaSource*> iconSources_;
    QHash<QString, QUrl> iconUrls_;
};
