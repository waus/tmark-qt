import QtQuick

Column {
    id: captionView
    property var caption: ({})
    property var theme
    width: parent ? parent.width : implicitWidth
    spacing: 2

    TmarkRichText {
        width: parent.width
        visible: !!captionView.caption.html
        sourceHtml: captionView.caption.html || ""
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: captionView.theme.font.pixelSize * 0.85
        color: captionView.theme.mutedColor
        linkColor: captionView.theme.linkColor
    }
    TmarkRichText {
        width: parent.width
        visible: !!(captionView.caption.fieldHtml && captionView.caption.fieldHtml.credit)
        sourceHtml: captionView.caption.fieldHtml ? captionView.caption.fieldHtml.credit || "" : ""
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: captionView.theme.font.pixelSize * 0.75
        color: captionView.theme.mutedColor
        linkColor: captionView.theme.linkColor
    }
}
