#include "viewmodel.hpp"
#include "tmark/codec.hpp"

#include <QSet>
#include <QUrl>
#include <QDateTime>
#include <QLocale>
#include <QRegularExpression>
#include <QTimeZone>

#include <cmath>
#include <limits>

namespace {

QString escaped(const QString& input) { return input.toHtmlEscaped(); }

QString contentText(const std::vector<tmark::Part>& content) {
    QString out;
    for (const auto& part : content) if (const auto* s = std::get_if<std::string>(&part)) out += QString::fromStdString(*s);
    return out;
}

QString html(const std::vector<tmark::Part>& content, const QHash<QString, QUrl>& icons, QSet<QString>& missing);

QString inlineNode(const tmark::Node& node, const QHash<QString, QUrl>& icons, QSet<QString>& missing) {
    const auto inner = html(node.content, icons, missing);
    const auto tag = QString::fromStdString(node.tag);
    const auto namedText = [&node](const char* name) {
        for (const auto& field : node.fields) if (field.name == name)
            return contentText(std::get<std::shared_ptr<const tmark::Node>>(field.value.front())->content);
        return QString{};
    };
    if (tag == "b") return "<b>" + inner + "</b>";
    if (tag == "i") return "<i>" + inner + "</i>";
    if (tag == "u") return "<u>" + inner + "</u>";
    if (tag == "s") return "<s>" + inner + "</s>";
    if (tag == "sub") return "<sub>" + inner + "</sub>";
    if (tag == "sup") return "<sup>" + inner + "</sup>";
    if (tag == "code") {
        const QString value = contentText(node.content);
        return "<a href=\"tmark-copy:" + QString::fromLatin1(QUrl::toPercentEncoding(value)) + "\"><code style=\"font-family:monospace\">" + escaped(value) + "</code></a>";
    }
    if (tag == "a") {
        const auto href = namedText("href");
        const QUrl url(href);
        if (url.scheme() == "http" || url.scheme() == "https") return "<a href=\"" + escaped(href) + "\">" + inner + "</a>";
    }
    if (tag == "anchor-link") return "<a href=\"#" + escaped(namedText("name")) + "\">" + inner + "</a>";
    if (tag == "m") return "<span style=\"background-color:#fff2ae;color:#242424\">" + inner + "</span>";
    if (tag == "spoiler") {
        const QString encoded = QString::fromLatin1(QUrl::toPercentEncoding(inner));
        return "<a href=\"tmark-spoiler:" + encoded + "\" style=\"color:#777;background-color:#777;text-decoration:none\">" + inner + "</a>";
    }
    if (tag == "icon") {
        const auto src = contentText(node.content);
        if (icons.contains(src))
            return "<img src=\"" + escaped(icons.value(src).toString()) + "\" width=\"16\" height=\"16\" alt=\"" + escaped(namedText("alt")) + "\">";
        if (!src.isEmpty()) missing.insert(src);
        return escaped(namedText("alt"));
    }
    if (tag == "datetime") {
        const auto timezone = namedText("timezone");
        const auto id = timezone.isEmpty() ? QByteArray("UTC") : timezone.toUtf8();
        bool ok = false;
        const double seconds = contentText(node.content).toDouble(&ok);
        static const QRegularExpression offsetPattern(QStringLiteral(
            "\\A(?:UTC([+-])([0-9]{1,2})(?::([0-9]{2}))?|([+-])([0-9]{2}):([0-9]{2}))\\z"));
        const auto match = offsetPattern.match(QString::fromUtf8(id));
        const bool prefixed = !match.captured(1).isEmpty();
        const int hours = match.captured(prefixed ? 2 : 5).toInt();
        const int minutes = match.captured(prefixed ? 3 : 6).toInt();
        const bool fixed = id == "UTC" || id == "GMT" || id == "Z" ||
            (match.hasMatch() && hours < 24 && minutes < 60);
        static const QRegularExpression namePattern(QStringLiteral("\\A[A-Za-z0-9_+-]+(?:/[A-Za-z0-9_+-]+)*\\z"));
        static const QRegularExpression abbreviationPattern(QStringLiteral("\\A[A-Za-z]{2,4}\\z"));
        static const QSet<QByteArray> shortNames = {"CET", "Cuba", "EET", "EST", "Eire", "GB", "GMT", "HST", "Iran",
            "MET", "MST", "NZ", "PRC", "ROC", "ROK", "UCT", "UTC", "WET", "Zulu"};
        const auto upper = id.toUpper();
        const bool named = !match.hasMatch() && !upper.startsWith("UTC+") && !upper.startsWith("UTC-") &&
            !upper.startsWith("GMT+") && !upper.startsWith("GMT-") &&
            id != "Local" && id != "localtime" && id != "posixrules" &&
            !id.startsWith("posix/") && !id.startsWith("right/") &&
            namePattern.match(QString::fromUtf8(id)).hasMatch() &&
            (!abbreviationPattern.match(QString::fromUtf8(id)).hasMatch() || shortNames.contains(id)) &&
            QTimeZone::isTimeZoneIdAvailable(id);
        if ((fixed || named) && ok && std::isfinite(seconds) &&
            seconds > double(std::numeric_limits<qint64>::min()) / 1000 &&
            seconds < double(std::numeric_limits<qint64>::max()) / 1000) {
            auto date = QDateTime::fromMSecsSinceEpoch(qint64(seconds * 1000), QTimeZone::utc());
            if (fixed) date = date.addSecs((hours * 3600 + minutes * 60) * (match.captured(prefixed ? 1 : 4) == "-" ? -1 : 1));
            else date = date.toTimeZone(QTimeZone(id));
            if (date.isValid()) return escaped(QLocale().toString(date, QLocale::ShortFormat) + " " + QString::fromUtf8(id));
        }
        return escaped(contentText(node.content) + " " + QString::fromUtf8(id));
    }
    return inner.isEmpty() ? escaped(QString::fromStdString(node.raw)) : inner;
}

QString html(const std::vector<tmark::Part>& content, const QHash<QString, QUrl>& icons, QSet<QString>& missing) {
    QString out;
    for (const auto& part : content) {
        if (const auto* s = std::get_if<std::string>(&part))
            out += escaped(QString::fromStdString(*s)).replace('\n', QStringLiteral("<br>"));
        else out += inlineNode(**std::get_if<std::shared_ptr<const tmark::Node>>(&part), icons, missing);
    }
    return out;
}

QVariantMap variant(const tmark::Node& node, const QHash<QString, QUrl>& icons, QSet<QString>& missing, const QString& implied = {});

QVariantList children(const std::vector<tmark::Part>& content, const QHash<QString, QUrl>& icons, QSet<QString>& missing, const QString& implied = {}) {
    QVariantList out;
    for (const auto& part : content) {
        if (const auto* s = std::get_if<std::string>(&part)) {
            if (QString::fromStdString(*s).trimmed().isEmpty() && s->find('\n') != std::string::npos) continue;
            out.append(QVariantMap{{"tag", "text"}, {"text", QString::fromStdString(*s)}});
        } else out.append(variant(**std::get_if<std::shared_ptr<const tmark::Node>>(&part), icons, missing, implied));
    }
    return out;
}

QVariantMap variant(const tmark::Node& node, const QHash<QString, QUrl>& icons, QSet<QString>& missing, const QString& implied) {
    const QString tag = node.tag.empty() ? implied : QString::fromStdString(node.tag);
    QVariantMap fields;
    QVariantMap fieldHtml;
    for (const auto& field : node.fields) {
        const auto& wrapper = *std::get<std::shared_ptr<const tmark::Node>>(field.value.front());
        const QString name = QString::fromStdString(field.name);
        fieldHtml.insert(name, html(wrapper.content, icons, missing));
        if (name == "caption" && tag != "table") fields.insert(name, variant(wrapper, icons, missing, "caption"));
        else if (name == "attached_media") fields.insert(name, children(wrapper.content, icons, missing, "attached"));
        else fields.insert(name, contentText(wrapper.content));
    }
    QString childTag;
    if (tag == "list") childTag = "li";
    else if (tag == "table") childTag = "tr";
    else if (tag == "tr") childTag = "td";
    const auto content = children(node.content, icons, missing, childTag);
    return {{"tag", tag}, {"fields", fields}, {"fieldHtml", fieldHtml}, {"children", content},
            {"text", contentText(node.content)}, {"html", html(node.content, icons, missing)},
            {"raw", QString::fromStdString(node.raw)}};
}

} // namespace

void TmarkDocument::setText(const QString& text) {
    if (text_ == text) return;
    text_ = text;
    clearIcons();
    emit textChanged();
    update();
}

void TmarkDocument::setMediaLoader(TmarkMediaLoader* loader) {
    if (mediaLoader_ == loader) return;
    mediaLoader_ = loader;
    clearIcons();
    emit mediaLoaderChanged();
    update();
}

void TmarkDocument::clearIcons() {
    for (auto* source : std::as_const(iconSources_)) {
        source->disconnect(this);
        source->deleteLater();
    }
    iconSources_.clear();
    iconUrls_.clear();
}

void TmarkDocument::setSoftParser(bool soft) {
    if (softParser_ == soft) return;
    softParser_ = soft;
    emit softParserChanged();
    update();
}

void TmarkDocument::update() {
    try {
        QSet<QString> missing;
        document_ = variant(tmark::parse(text_.toStdString(), softParser_), iconUrls_, missing);
        error_.clear();
        for (const auto& src : missing) {
            if (iconSources_.contains(src)) continue;
            auto* source = new TmarkMediaSource(this);
            iconSources_.insert(src, source);
            source->setLoader(mediaLoader_);
            connect(source, &TmarkMediaSource::playableSourceChanged, this, [this, source, src] {
                if (source->playableSource().isEmpty()) return;
                iconUrls_.insert(src, source->playableSource());
                update();
            });
            source->setSource(QUrl(src));
        }
    } catch (const tmark::Error& error) {
        document_.clear();
        error_ = QString::fromUtf8(error.what());
    }
    emit documentChanged();
}
