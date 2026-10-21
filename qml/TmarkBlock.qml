import QtQuick
import QtQuick.Controls

Item {
    id: block
    property var node: ({ tag: "", fields: {}, fieldHtml: {}, children: [], html: "", text: "", raw: "" })
    property var theme
    property TmarkMediaLoader mediaLoader
    property var customWidgets: ({})
    property var scrollToAnchor
    implicitHeight: loader.item ? Math.max(loader.item.implicitHeight, loader.item.height) : 0
    height: implicitHeight

    Loader {
        id: loader
        width: parent.width
        sourceComponent: {
            if (block.customWidgets && block.customWidgets[block.node.tag])
                return block.customWidgets[block.node.tag]
            switch (block.node.tag) {
            case "p": return paragraph
            case "h": return heading
            case "pre": case "math-block": return code
            case "hr": return divider
            case "anchor": return anchorBlock
            case "img": case "video": case "audio": return mediaBlock
            case "collage": case "slideshow": return galleryBlock
            case "table": return tableBlock
            case "q": return quote
            case "as": return pullQuote
            case "details": return detailsBlock
            case "list": return listBlock
            case "li": case "document": return group
            default: return fallback
            }
        }
        onLoaded: {
            if (item && "node" in item) item.node = block.node
            if (item && "theme" in item) item.theme = block.theme
        }
    }

    Component {
        id: paragraph
        TmarkRichText {
            width: block.width
            sourceHtml: block.node.html
            scrollToAnchor: block.scrollToAnchor
            font: block.theme.font
            color: block.theme.textColor
            linkColor: block.theme.linkColor
        }
    }
    Component {
        id: heading
        TmarkRichText {
            width: block.width
            sourceHtml: block.node.html
            scrollToAnchor: block.scrollToAnchor
            font.family: block.theme.font.family
            font.pixelSize: block.theme.font.pixelSize * (1.8 - Math.min(6, Number(block.node.fields.s)) * 0.12)
            font.bold: true
            color: block.theme.textColor
            linkColor: block.theme.linkColor
        }
    }
    Component {
        id: code
        Rectangle {
            width: block.width
            implicitHeight: codeText.implicitHeight + (block.node.tag === "pre" ? 20 : 12)
            color: block.theme.panelColor
            radius: 8
            Item {
                width: 32; height: 32
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 6
                visible: block.node.tag === "pre"
                Accessible.role: Accessible.Button
                Accessible.name: "Copy code"
                Rectangle {
                    x: 10; y: 7; width: 14; height: 17
                    radius: 1; color: "transparent"
                    border.color: block.theme.mutedColor
                    border.width: 2
                }
                Rectangle {
                    x: 7; y: 10; width: 14; height: 17
                    radius: 1; color: block.theme.panelColor
                    border.color: block.theme.mutedColor
                    border.width: 2
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        codeText.selectAll()
                        codeText.copy()
                        codeText.select(0, 0)
                    }
                }
            }
            TextEdit {
                id: codeText
                x: 10; y: block.node.tag === "pre" ? 10 : 6
                width: parent.width - (block.node.tag === "pre" ? 54 : 20)
                text: block.node.tag === "pre" ? block.node.text.replace(/^\n/, "").replace(/\n$/, "") : block.node.text
                textFormat: Text.PlainText
                wrapMode: Text.WrapAnywhere
                readOnly: true
                selectByMouse: true
                font: block.theme.codeFont
                color: block.theme.textColor
            }
        }
    }
    Component {
        id: divider
        Rectangle { width: block.width; height: 1; color: block.theme.mutedColor }
    }
    Component {
        id: anchorBlock
        Item { objectName: "tmark-anchor-" + block.node.text; width: block.width; height: 0 }
    }
    Component {
        id: mediaBlock
        TmarkMedia { width: block.width; node: block.node; theme: block.theme; mediaLoader: block.mediaLoader }
    }
    Component {
        id: galleryBlock
        TmarkGallery { width: block.width; node: block.node; theme: block.theme; mediaLoader: block.mediaLoader }
    }
    Component {
        id: tableBlock
        TmarkTable { width: block.width; node: block.node; theme: block.theme; scrollToAnchor: block.scrollToAnchor }
    }
    Component {
        id: listBlock
        TmarkList {
            width: block.width
            node: block.node
            theme: block.theme
            mediaLoader: block.mediaLoader
            customWidgets: block.customWidgets
            scrollToAnchor: block.scrollToAnchor
        }
    }
    Component {
        id: pullQuote
        Column {
            width: block.width
            spacing: 6
            TmarkRichText {
                width: parent.width
                sourceHtml: block.node.html
                scrollToAnchor: block.scrollToAnchor
                horizontalAlignment: Text.AlignHCenter
                font: block.theme.font
                color: block.theme.mutedColor
            }
            TmarkCaption {
                width: parent.width
                caption: ({ fieldHtml: { credit: block.node.fieldHtml.credit } })
                theme: block.theme
            }
        }
    }
    Component {
        id: quote
        Row {
            width: block.width
            spacing: 10
            Rectangle { width: 3; height: content.implicitHeight; color: block.theme.mutedColor }
            Loader { id: content; width: block.width - 13; sourceComponent: group }
        }
    }
    Component {
        id: detailsBlock
        Column {
            id: details
            width: block.width
            property bool expanded: block.node.fields.open === "t"
            spacing: 6
            Row {
                width: parent.width
                spacing: 8
                Item {
                    id: toggle
                    width: 24; height: 24
                    Accessible.role: Accessible.Button
                    Accessible.name: details.expanded ? "Collapse details" : "Expand details"
                    Rectangle {
                        anchors.fill: parent
                        radius: 12
                        color: block.theme.panelColor
                        border.color: block.theme.mutedColor
                        border.width: 1
                    }
                    Canvas {
                        id: detailArrow
                        width: 12; height: 12
                        anchors.centerIn: parent
                        property bool expanded: details.expanded
                        property color strokeColor: block.theme.textColor
                        onExpandedChanged: requestPaint()
                        onStrokeColorChanged: requestPaint()
                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.strokeStyle = strokeColor.toString()
                            ctx.lineWidth = 2
                            ctx.lineCap = "round"
                            ctx.lineJoin = "round"
                            ctx.beginPath()
                            if (expanded) {
                                ctx.moveTo(2, 4); ctx.lineTo(6, 8); ctx.lineTo(10, 4)
                            } else {
                                ctx.moveTo(4, 2); ctx.lineTo(8, 6); ctx.lineTo(4, 10)
                            }
                            ctx.stroke()
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: details.expanded = !details.expanded
                    }
                }
                TmarkRichText {
                    width: parent.width - toggle.width - parent.spacing
                    anchors.verticalCenter: parent.verticalCenter
                    sourceHtml: block.node.fieldHtml.summary || "Details"
                    scrollToAnchor: block.scrollToAnchor
                    font: block.theme.font
                    color: block.theme.textColor
                }
            }
            Row {
                width: parent.width
                visible: details.expanded
                height: visible ? implicitHeight : 0
                spacing: 10
                Rectangle {
                    width: 1
                    height: detailContent.height
                    color: block.theme.mutedColor
                }
                Loader {
                    id: detailContent
                    width: details.width - 11
                    active: details.expanded
                    height: active ? implicitHeight : 0
                    sourceComponent: group
                }
            }
        }
    }
    Component {
        id: group
        Column {
            id: groupContent
            width: parent.width
            spacing: 8
            Repeater {
                model: block.node.children || []
                Loader {
                    required property var modelData
                    width: groupContent.width - (block.node.tag === "li" ? 16 : 0)
                    x: block.node.tag === "li" ? 16 : 0
                    Component.onCompleted: setSource("TmarkBlock.qml", {
                        node: modelData,
                        theme: block.theme,
                        mediaLoader: block.mediaLoader,
                        customWidgets: block.customWidgets,
                        scrollToAnchor: block.scrollToAnchor
                    })
                }
            }
            TmarkCaption {
                visible: block.node.tag === "q" && !!block.node.fieldHtml.credit
                width: parent.width
                caption: ({ fieldHtml: { credit: block.node.fieldHtml.credit } })
                theme: block.theme
            }
        }
    }
    Component {
        id: fallback
        Text {
            width: block.width
            text: block.node.raw
            textFormat: Text.PlainText
            wrapMode: Text.WrapAnywhere
            font: block.theme.codeFont
            color: block.theme.mutedColor
        }
    }
}
