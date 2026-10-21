#include "medialoader.hpp"

#include <QNetworkReply>
#include <QNetworkRequest>

TmarkMediaLoader::TmarkMediaLoader(QObject* parent) : QObject(parent), network_(this) {}

QNetworkReply* TmarkMediaLoader::fetch(const QUrl& url) {
    if (url.scheme() != "http" && url.scheme() != "https") return nullptr;
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    return network_.get(request);
}
