import QtQuick
import QtQuick.Controls
import io.github.waus.tmark

Flickable {
    id: view
    property string text: ""
    property bool softParser: false
    property TmarkViewTheme theme: TmarkViewTheme {}
    property TmarkMediaLoader mediaLoader: TmarkMediaLoader {}
    // Map of wire tags to Components. Each custom item can declare `property var node`.
    property var customWidgets: ({})
    readonly property string error: document.error
    readonly property var parsedDocument: document.document

    contentWidth: width
    contentHeight: body.implicitHeight + 32
    clip: true

    function findAnchor(item, name) {
        if (item.objectName === "tmark-anchor-" + name)
            return item
        for (const child of item.children || []) {
            const found = findAnchor(child, name)
            if (found) return found
        }
        return null
    }

    function scrollToAnchor(name) {
        const anchor = findAnchor(body, name)
        if (anchor) contentY = Math.max(0, anchor.mapToItem(body, 0, 0).y + body.y)
    }

    TmarkDocument {
        id: document
        text: view.text
        softParser: view.softParser
        mediaLoader: view.mediaLoader
    }

    Column {
        id: body
        x: 16
        y: 16
        width: Math.max(0, view.width - 32)
        spacing: 12

        Label {
            visible: document.error.length > 0
            width: parent.width
            wrapMode: Text.Wrap
            text: document.error
            color: view.theme.errorColor
        }

        Repeater {
            model: document.document.children || []
            TmarkBlock {
                required property var modelData
                width: body.width
                node: modelData
                theme: view.theme
                mediaLoader: view.mediaLoader
                customWidgets: view.customWidgets
                scrollToAnchor: view.scrollToAnchor
            }
        }
    }
}
