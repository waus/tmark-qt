import QtQuick
import QtQuick.Layouts

Column {
    id: table
    property var node: ({ children: [], fields: {}, fieldHtml: {} })
    property var theme
    property var scrollToAnchor
    width: parent ? parent.width : implicitWidth
    spacing: 6

    readonly property var layout: {
        const occupied = ({})
        const cells = []
        let columns = 0
        for (let row = 0; row < node.children.length; ++row) {
            let column = 0
            for (const cell of node.children[row].children) {
                while (occupied[row + ":" + column]) ++column
                const colspan = Math.max(1, Number(cell.fields.colspan) || 1)
                const rowspan = Math.max(1, Number(cell.fields.rowspan) || 1)
                for (let r = row; r < row + rowspan; ++r)
                    for (let c = column; c < column + colspan; ++c)
                        occupied[r + ":" + c] = true
                cells.push({ node: cell, row: row, column: column, colspan: colspan, rowspan: rowspan })
                column += colspan
                columns = Math.max(columns, column)
            }
        }
        return { cells: cells, columns: Math.max(1, columns) }
    }

    GridLayout {
        width: parent.width
        columns: table.layout.columns
        columnSpacing: 0
        rowSpacing: 0
        Repeater {
            model: table.layout.cells
            Rectangle {
                required property var modelData
                Layout.row: modelData.row
                Layout.column: modelData.column
                Layout.rowSpan: modelData.rowspan
                Layout.columnSpan: modelData.colspan
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 100 * modelData.colspan
                implicitHeight: cellText.implicitHeight + 12
                color: table.node.fields.striped === "t" && modelData.row % 2 ? table.theme.panelColor : table.theme.backgroundColor
                border.width: table.node.fields.bordered === "t" ? 1 : 0
                border.color: table.theme.mutedColor
                TmarkRichText {
                    id: cellText
                    anchors.fill: parent
                    anchors.margins: 6
                    sourceHtml: modelData.node.html
                    scrollToAnchor: table.scrollToAnchor
                    font.family: table.theme.font.family
                    font.pixelSize: table.theme.font.pixelSize
                    font.bold: modelData.node.fields.header === "t"
                    color: table.theme.textColor
                    linkColor: table.theme.linkColor
                    horizontalAlignment: modelData.node.fields.align === "right" ? Text.AlignRight
                                         : modelData.node.fields.align === "center" ? Text.AlignHCenter : Text.AlignLeft
                    verticalAlignment: modelData.node.fields.valign === "bottom" ? Text.AlignBottom
                                       : modelData.node.fields.valign === "middle" ? Text.AlignVCenter : Text.AlignTop
                }
            }
        }
    }

    TmarkCaption {
        visible: !!table.node.fieldHtml.caption
        width: parent.width
        caption: ({ html: table.node.fieldHtml.caption })
        theme: table.theme
    }
}
