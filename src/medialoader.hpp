#pragma once

#include <QNetworkAccessManager>
#include <QQmlEngine>
#include <QUrl>

class QNetworkReply;

class TmarkMediaLoader : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit TmarkMediaLoader(QObject* parent = nullptr);
    // Override to support additional URL schemes. The caller owns the returned reply.
    virtual QNetworkReply* fetch(const QUrl& url);

protected:
    QNetworkAccessManager& networkManager() { return network_; }

private:
    QNetworkAccessManager network_;
};
