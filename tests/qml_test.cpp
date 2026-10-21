#include <QGuiApplication>
#include <QClipboard>
#include <QEventLoop>
#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTimer>

#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QQmlEngine engine;
    engine.addImportPath(TMARK_QML_BUILD_ROOT);
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import io.github.waus.tmark
TmarkView {
    width: 300
    height: 300
    text: "{document;#url{x}#title{Test}{p;Hello {b;world}}}"
    softParser: true
    customWidgets: ({ "p": paragraph })
    Component { id: paragraph; Text { property var node; text: node ? node.text : "" } }
}
)", QUrl());
    if (!component.isReady()) {
        std::cerr << component.errorString().toStdString();
        return 1;
    }
    std::unique_ptr<QObject> view(component.create());
    if (!view) {
        std::cerr << component.errorString().toStdString();
        return 1;
    }
    if (!view->property("error").toString().isEmpty()) return 1;
    if (view->property("parsedDocument").toMap().value("children").toList().size() != 1) return 1;
    view->setProperty("text", QStringLiteral("{document;#url{x}#title{Test}{p;First line\nSecond line &nbsp; <b> {spoiler;secret}}{table;{tr;{td;Cell first\nCell second}}}}"));
    app.processEvents();
    const auto blocks = view->property("parsedDocument").toMap().value("children").toList();
    const QString paragraphHtml = blocks[0].toMap().value("html").toString();
    const QString cellHtml = blocks[1].toMap().value("children").toList()[0].toMap().value("children").toList()[0].toMap().value("html").toString();
    if (!paragraphHtml.contains("First line<br>Second line &amp;nbsp; &lt;b&gt;") ||
        !cellHtml.contains("Cell first<br>Cell second")) return 1;
    QTextDocument rendered;
    rendered.setHtml(paragraphHtml);
    const QString visible = rendered.toPlainText();
    if (!visible.contains("&nbsp; <b> secret") || !paragraphHtml.contains("color:#777;background-color:#777") ||
        (!visible.contains("First line\nSecond line") && !visible.contains("First line\u2028Second line"))) return 1;
    rendered.setHtml(cellHtml);
    if (!rendered.toPlainText().contains("Cell first\nCell second") &&
        !rendered.toPlainText().contains("Cell first\u2028Cell second")) return 1;
    QQmlComponent richText(&engine);
    richText.setData("import io.github.waus.tmark; TmarkRichText {}", QUrl());
    std::unique_ptr<QObject> spoiler(richText.create());
    if (!spoiler) return 1;
    spoiler->setProperty("sourceHtml", paragraphHtml);
    const QString link = QRegularExpression("tmark-spoiler:[^\"]+").match(paragraphHtml).captured();
    if (link.isEmpty() || !QMetaObject::invokeMethod(spoiler.get(), "linkActivated", Q_ARG(QString, link))) return 1;
    rendered.setHtml(spoiler->property("text").toString());
    if (!rendered.toPlainText().contains("secret")) return 1;
    if (spoiler->property("text").toString().contains("background-color:#777")) return 1;
    view->setProperty("text", QStringLiteral("{document;#url{x}#title{Inline}{p;{code;copy me} {datetime;#timezone{UTC}0}}}"));
    app.processEvents();
    const QString inlineHtml = view->property("parsedDocument").toMap().value("children").toList()[0].toMap().value("html").toString();
    if (!inlineHtml.contains("href=\"tmark-copy:copy%20me\"") || inlineHtml.contains(">0<")) return 1;
    spoiler->setProperty("sourceHtml", inlineHtml);
    if (!QMetaObject::invokeMethod(spoiler.get(), "linkActivated", Q_ARG(QString, QStringLiteral("tmark-copy:copy%20me"))) ||
        app.clipboard()->text() != QStringLiteral("copy me")) return 1;
    QQmlComponent galleryComponent(&engine);
    galleryComponent.setData("import io.github.waus.tmark; TmarkGallery { width: 420; theme: TmarkViewTheme {} }", QUrl());
    std::unique_ptr<QObject> gallery(galleryComponent.create());
    if (!gallery) { std::cerr << galleryComponent.errorString().toStdString(); return 1; }
    for (int count = 1; count <= 12; ++count) {
        QVariantList images;
        for (int i = 0; i < count; ++i)
            images.append(QVariantMap{{"tag", "img"}, {"fields", QVariantMap{}}, {"text", ""}});
        gallery->setProperty("node", QVariantMap{{"tag", "collage"}, {"fields", QVariantMap{}}, {"children", images}});
        for (int width : {120, 420}) {
            gallery->setProperty("width", width);
            app.processEvents();
            const auto layout = gallery->property("collageLayout").toMap();
            const auto tiles = layout.value("tiles").toList();
            if (tiles.size() != count || layout.value("height").toDouble() <= 0) return 1;
            for (const auto& value : tiles) {
                const auto tile = value.toMap();
                if (tile.value("width").toDouble() <= 0 || tile.value("height").toDouble() <= 0 ||
                    tile.value("x").toDouble() < 0 || tile.value("y").toDouble() < 0 ||
                    tile.value("x").toDouble() + tile.value("width").toDouble() > width + 1 ||
                    tile.value("y").toDouble() + tile.value("height").toDouble() > layout.value("height").toDouble()) return 1;
            }
        }
    }
    QVariantList slidesData;
    for (int i = 0; i < 3; ++i)
        slidesData.append(QVariantMap{{"tag", "img"}, {"fields", QVariantMap{}}, {"text", ""}});
    gallery->setProperty("node", QVariantMap{{"tag", "slideshow"}, {"fields", QVariantMap{}}, {"children", slidesData}});
    app.processEvents();
    auto* slides = gallery->findChild<QObject*>("tmark-slides");
    if (!slides || slides->property("count").toInt() != 3) return 1;
    slides->setProperty("currentIndex", 2);
    gallery->setProperty("node", QVariantMap{{"tag", "slideshow"}, {"fields", QVariantMap{}}, {"children", slidesData.mid(0, 1)}});
    app.processEvents();
    if (slides->property("count").toInt() != 1 || slides->property("currentIndex").toInt() != 0) return 1;
    for (const char* name : {"media_gallery", "quotes_and_details", "tables", "lists_and_tasks", "references_and_anchors", "inline_formatting"}) {
        QFile file(QStringLiteral(TMARK_FIXTURES "/valid/example_") + QString::fromLatin1(name) + ".tmark");
        if (!file.open(QIODevice::ReadOnly)) return 1;
        view->setProperty("text", QString::fromUtf8(file.readAll()));
        app.processEvents();
        if (!view->property("error").toString().isEmpty()) {
            std::cerr << name << ": " << view->property("error").toString().toStdString() << '\n';
            return 1;
        }
    }
    QString anchored = "{document;#url{x}#title{Anchors}";
    for (int i = 0; i < 30; ++i) anchored += "{p;Before the anchor.}";
    anchored += "{anchor;target}{p;After the anchor.}}";
    view->setProperty("text", anchored);
    app.processEvents();
    if (!QMetaObject::invokeMethod(view.get(), "scrollToAnchor", Q_ARG(QVariant, QVariant("target")))) return 1;
    if (view->property("contentY").toReal() <= 0) return 1;
    QQuickWindow window;
    window.resize(300, 300);
    qobject_cast<QQuickItem*>(view.get())->setParentItem(window.contentItem());
    window.show();
    const auto settle = [] {
        QEventLoop loop;
        QTimer::singleShot(50, &loop, &QEventLoop::quit);
        loop.exec();
    };
    view->setProperty("text", QStringLiteral("{document;#url{x}#title{Details}{details;#summary{More}#open{t}{p;First line}{p;Second line}}{p;After}}"));
    settle();
    const auto findDetails = [](auto&& self, QQuickItem* item) -> QQuickItem* {
        if (item->property("expanded") == true) return item;
        for (auto* child : item->childItems()) if (auto* found = self(self, child)) return found;
        return nullptr;
    };
    auto* details = findDetails(findDetails, qobject_cast<QQuickItem*>(view.get()));
    if (!details) return 1;
    const auto openHeight = view->property("contentHeight").toReal();
    details->setProperty("expanded", false);
    settle();
    const auto closedHeight = view->property("contentHeight").toReal();
    if (closedHeight >= openHeight) { std::cerr << "open=" << openHeight << " closed=" << closedHeight << '\n'; return 1; }
    details->setProperty("expanded", true);
    settle();
    if (view->property("contentHeight").toReal() <= closedHeight) { std::cerr << "did not reopen\n"; return 1; }
    return 0;
}
