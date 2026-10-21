#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSize>
#include <QString>

enum class TmarkMediaKind { Image, Audio, Video };

// Returns an error for unsupported or undecodable media, or an empty string.
QString validateMediaFile(const QString& path, TmarkMediaKind kind, QSize* lottieSize = nullptr);

inline QSize selfContainedLottieSize(const QByteArray& bytes) {
    const QJsonObject json = QJsonDocument::fromJson(bytes).object();
    if (!json.value("v").isString() || !json.value("fr").isDouble() ||
        !json.value("ip").isDouble() || !json.value("op").isDouble() ||
        !json.value("w").isDouble() || !json.value("h").isDouble() ||
        !json.value("layers").isArray()) return {};
    const int width = json.value("w").toInt();
    const int height = json.value("h").toInt();
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192) return {};
    const QJsonValue assets = json.value("assets");
    if (!assets.isUndefined() && !assets.isArray()) return {};
    for (const QJsonValue asset : assets.toArray()) {
        if (!asset.isObject()) return {};
        const QJsonValue image = asset.toObject().value("p");
        if (image.isString() && !image.toString().isEmpty() && !image.toString().startsWith("data:image/")) return {};
    }
    return {width, height};
}
