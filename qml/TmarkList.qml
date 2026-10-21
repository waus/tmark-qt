import QtQuick

Column {
    id: list
    property var node: ({ children: [] })
    property var theme
    property TmarkMediaLoader mediaLoader
    property var customWidgets: ({})
    property var scrollToAnchor
    width: parent ? parent.width : implicitWidth
    spacing: 6

    function letters(value, upper) {
        let result = ""
        for (let n = value; n > 0; n = Math.floor((n - 1) / 26))
            result = String.fromCharCode(65 + (n - 1) % 26) + result
        return upper ? result : result.toLowerCase()
    }

    function roman(value) {
        if (value < 1 || value > 3999) return String(value)
        const symbols = [[1000, "M"], [900, "CM"], [500, "D"], [400, "CD"],
                         [100, "C"], [90, "XC"], [50, "L"], [40, "XL"],
                         [10, "X"], [9, "IX"], [5, "V"], [4, "IV"], [1, "I"]]
        let result = ""
        for (const pair of symbols) {
            while (value >= pair[0]) { result += pair[1]; value -= pair[0] }
        }
        return result
    }

    function marker(item, index) {
        const kind = item.fields.type || (item.fields.order ? "1" : "")
        const order = Number(item.fields.order) || index + 1
        if (kind === "1") return order + "."
        if (kind === "a" || kind === "A") return letters(order, kind === "A") + "."
        if (kind === "i" || kind === "I") {
            const value = roman(order)
            return (kind === "i" ? value.toLowerCase() : value) + "."
        }
        return "•"
    }

    Repeater {
        model: list.node.children || []
        Row {
            required property var modelData
            required property int index
            width: list.width
            spacing: 8

            Item {
                width: 34
                height: Math.max(markerText.implicitHeight, taskBox.implicitHeight)
                Text {
                    id: markerText
                    width: parent.width
                    visible: modelData.fields.type !== "checkbox"
                    text: list.marker(modelData, index)
                    horizontalAlignment: Text.AlignRight
                    font: list.theme.font
                    color: list.theme.textColor
                }
                Rectangle {
                    id: taskBox
                    readonly property bool checked: modelData.fields.checked === "t"
                    visible: modelData.fields.type === "checkbox"
                    width: 18
                    height: 18
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 4
                    border.width: 1
                    border.color: checked ? list.theme.linkColor : list.theme.mutedColor
                    color: checked ? list.theme.linkColor : "transparent"
                    Accessible.role: Accessible.StaticText
                    Accessible.name: checked ? "Completed task" : "Incomplete task"
                    Text {
                        anchors.centerIn: parent
                        visible: taskBox.checked
                        text: "✓"
                        color: "white"
                        font.pixelSize: 14
                    }
                }
            }

            Column {
                id: content
                width: Math.max(0, list.width - 42)
                spacing: 6
                Repeater {
                    model: modelData.children || []
                    Loader {
                        required property var modelData
                        width: content.width
                        Component.onCompleted: setSource("TmarkBlock.qml", {
                            node: modelData,
                            theme: list.theme,
                            mediaLoader: list.mediaLoader,
                            customWidgets: list.customWidgets,
                            scrollToAnchor: list.scrollToAnchor
                        })
                    }
                }
            }
        }
    }
}
