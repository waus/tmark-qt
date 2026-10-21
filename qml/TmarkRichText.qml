import QtQuick

Text {
    property string sourceHtml: ""
    property var revealedSpoilers: ({})
    property var scrollToAnchor
    readonly property color inlineCodeColor: color.r * 0.2126 + color.g * 0.7152 + color.b * 0.0722 < 0.5 ? "#a34732" : "#e5a579"
    textFormat: Text.RichText
    wrapMode: Text.Wrap
    text: {
        const revealed = sourceHtml.replace(/(<a href="tmark-spoiler:([^"]+)") style="color:#777;background-color:#777;text-decoration:none"/g,
            (whole, opening, encoded) => revealedSpoilers[encoded] ? opening + " style=\"color:" + color + ";text-decoration:none\"" : whole)
        return revealed.replace(/(<a href="tmark-copy:[^"]+")>/g,
            (whole, opening) => opening + " style=\"color:" + inlineCodeColor + ";text-decoration:none\">")
    }
    onLinkActivated: link => {
        if (link.startsWith("tmark-spoiler:")) {
            const encoded = link.substring("tmark-spoiler:".length)
            const next = Object.assign({}, revealedSpoilers)
            next[encoded] = true
            revealedSpoilers = next
        } else if (link.startsWith("tmark-copy:")) {
            clipboard.text = decodeURIComponent(link.substring("tmark-copy:".length))
            clipboard.selectAll()
            clipboard.copy()
        } else if (link.startsWith("#") && scrollToAnchor) {
            scrollToAnchor(link.substring(1))
        } else if (link.startsWith("https://") || link.startsWith("http://")) {
            Qt.openUrlExternally(link)
        }
    }
    TextEdit {
        id: clipboard
        visible: false
        readOnly: true
    }
}
