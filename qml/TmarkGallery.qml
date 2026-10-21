import QtQuick
import QtQuick.Controls

Column {
    id: gallery
    property var node: ({ tag: "collage", children: [], fields: {} })
    property var theme
    property TmarkMediaLoader mediaLoader
    property int ratioRevision: 0
    readonly property var childrenNodes: node.children || []
    readonly property var collageLayout: layout(width, childrenNodes, ratioRevision)
    width: parent ? parent.width : implicitWidth
    spacing: 8

    component NavigationArrow: Canvas {
        property bool forward: false
        onForwardChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = "white"
            ctx.lineWidth = 2.5
            ctx.lineCap = "round"
            ctx.lineJoin = "round"
            ctx.beginPath()
            const centerX = width / 2
            const centerY = height / 2
            const direction = forward ? 1 : -1
            ctx.moveTo(centerX - direction * 3, centerY - 6)
            ctx.lineTo(centerX + direction * 3, centerY)
            ctx.lineTo(centerX - direction * 3, centerY + 6)
            ctx.stroke()
        }
    }

    onNodeChanged: {
        ratioRevision++
        Qt.callLater(() => slides.currentIndex = 0)
    }

    function layout(width, nodes, revision) {
        const count = nodes.length
        if (!count || width <= 0) return { tiles: [], height: 0 }
        const gap = 2
        const minWidth = Math.min(96, width / 3)
        const ratios = nodes.map((node, index) => {
            const item = collageRepeater.itemAt(index)
            const loaded = item ? item.aspectRatio : 0
            const fallback = count === 3 && index === 0 ? 1.6 : count === 4 && index === 0 ? 1.4 : node.tag === "video" ? 16 / 9 : 1
            return loaded > 0 ? Math.max(0.2, Math.min(5, loaded)) : fallback
        })
        const average = ratios.reduce((sum, ratio) => sum + ratio, 0) / count
        const shape = ratios.map(ratio => ratio > 1.2 ? "w" : ratio < 0.8 ? "n" : "q").join("")
        const tile = (x, y, w, h) => ({ x: Math.round(x), y: Math.round(y), width: Math.max(0, Math.round(w)), height: Math.max(0, Math.round(h)) })
        let tiles

        if (count === 1) {
            tiles = [tile(0, 0, width, width / ratios[0])]
        } else if (count === 2) {
            if (shape === "ww" && average > 1.4 && Math.abs(ratios[1] - ratios[0]) < 0.2) {
                const h = Math.min(width / ratios[0], width / ratios[1], (width - gap) / 2)
                tiles = [tile(0, 0, width, h), tile(0, h + gap, width, h)]
            } else if (shape === "ww" || shape === "qq") {
                const w = (width - gap) / 2
                const h = Math.min(w / ratios[0], w / ratios[1], width)
                tiles = [tile(0, 0, w, h), tile(w + gap, 0, w, h)]
            } else {
                const second = Math.min(Math.max(0.4 * (width - gap), (width - gap) / ratios[0] / (1 / ratios[0] + 1 / ratios[1])), width - gap - minWidth * 1.5)
                const first = width - second - gap
                const h = Math.min(width, first / ratios[0], second / ratios[1])
                tiles = [tile(0, 0, first, h), tile(first + gap, 0, second, h)]
            }
        } else if (count === 3) {
            if (shape[0] === "n") {
                const thirdH = Math.min((width - gap) / 2, ratios[1] * (width - gap) / (ratios[1] + ratios[2]))
                const secondH = width - thirdH - gap
                const rightW = Math.max(minWidth, Math.min((width - gap) / 2, thirdH * ratios[2], secondH * ratios[1]))
                const leftW = Math.min(width * ratios[0], width - gap - rightW)
                tiles = [tile(0, 0, leftW, width), tile(leftW + gap, 0, rightW, secondH), tile(leftW + gap, secondH + gap, rightW, thirdH)]
            } else {
                const firstH = Math.min(width / ratios[0], (width - gap) * 0.66)
                const secondW = (width - gap) / 2
                const secondH = Math.min(width - firstH - gap, secondW / ratios[1], secondW / ratios[2])
                tiles = [tile(0, 0, width, firstH), tile(0, firstH + gap, secondW, secondH), tile(secondW + gap, firstH + gap, width - secondW - gap, secondH)]
            }
        } else if (count === 4) {
            if (shape[0] === "w") {
                const firstH = Math.min(width / ratios[0], (width - gap) * 0.66)
                const rowH = Math.min(width - firstH - gap, (width - 2 * gap) / (ratios[1] + ratios[2] + ratios[3]))
                const firstW = Math.max(minWidth, Math.min((width - 2 * gap) * 0.4, rowH * ratios[1]))
                const thirdW = Math.max(minWidth, (width - 2 * gap) * 0.33, rowH * ratios[3])
                const secondW = width - firstW - thirdW - 2 * gap
                tiles = [tile(0, 0, width, firstH), tile(0, firstH + gap, firstW, rowH), tile(firstW + gap, firstH + gap, secondW, rowH), tile(firstW + secondW + 2 * gap, firstH + gap, thirdW, rowH)]
            } else {
                const leftW = Math.min(width * ratios[0], (width - gap) * 0.6)
                const rightW = Math.max(minWidth, Math.min(width - leftW - gap, (width - 2 * gap) / (1 / ratios[1] + 1 / ratios[2] + 1 / ratios[3])))
                const firstH = rightW / ratios[1]
                const secondH = rightW / ratios[2]
                tiles = [tile(0, 0, leftW, width), tile(leftW + gap, 0, rightW, firstH), tile(leftW + gap, firstH + gap, rightW, secondH), tile(leftW + gap, firstH + secondH + 2 * gap, rightW, width - firstH - secondH - 2 * gap)]
            }
        } else {
            const cropped = ratios.map(ratio => average > 1.1 ? Math.min(Math.max(ratio, 1), 2.75) : Math.min(Math.max(ratio, 0.6667), 1))
            let best = null
            function tryRows(rows) {
                let offset = 0
                const heights = rows.map(columns => {
                    const sum = cropped.slice(offset, offset + columns).reduce((a, b) => a + b, 0)
                    offset += columns
                    return (width - (columns - 1) * gap) / sum
                })
                const total = heights.reduce((a, b) => a + b, 0) + (rows.length - 1) * gap
                const badHeight = Math.min(...heights) < minWidth ? 1.5 : 1
                const badOrder = rows.some((columns, index) => index > 0 && rows[index - 1] > columns) ? 1.5 : 1
                const score = Math.abs(total - width * 4 / 3) * badHeight * badOrder
                if (!best || score < best.score) best = { rows, heights, score }
            }
            function candidates(rows, left) {
                if (left === 0) { if (rows.length >= 2) tryRows(rows); return }
                if (rows.length === 4) return
                for (let columns = 1; columns <= Math.min(left, rows.length === 1 && average < 0.85 ? 4 : 3); columns++)
                    candidates([...rows, columns], left - columns)
            }
            if (count <= 12) candidates([], count)
            if (!best) {
                const rows = []
                for (let left = count; left > 0; left -= 3) rows.push(Math.min(3, left))
                tryRows(rows)
            }
            tiles = []
            let index = 0
            let y = 0
            best.rows.forEach((columns, row) => {
                const h = best.heights[row]
                let x = 0
                for (let column = 0; column < columns; column++) {
                    const w = column === columns - 1 ? width - x : cropped[index] * h
                    tiles.push(tile(x, y, w, h))
                    x += w + gap
                    index++
                }
                y += h + gap
            })
        }
        return { tiles, height: tiles.reduce((height, rect) => Math.max(height, rect.y + rect.height), 0) }
    }

    Item {
        width: parent.width
        height: gallery.collageLayout.height
        visible: gallery.node.tag === "collage" && gallery.childrenNodes.length > 0
        Repeater {
            id: collageRepeater
            model: gallery.node.tag === "collage" ? gallery.node.children : []
            Item {
                required property int index
                required property var modelData
                readonly property real aspectRatio: mediaTile.aspectRatio
                readonly property var tile: gallery.collageLayout.tiles[index] || ({ x: 0, y: 0, width: 0, height: 0 })
                x: tile.x; y: tile.y
                width: tile.width; height: tile.height
                clip: true
                Rectangle { anchors.fill: parent; color: gallery.theme.panelColor }
                TmarkMedia {
                    id: mediaTile
                    anchors.fill: parent
                    node: modelData
                    theme: gallery.theme
                    mediaLoader: gallery.mediaLoader
                    galleryFrame: true
                    cropGalleryImage: true
                    onAspectRatioChanged: gallery.ratioRevision++
                }
            }
        }
    }

    Item {
        width: parent.width
        height: width * 9 / 16
        visible: gallery.node.tag === "slideshow" && gallery.childrenNodes.length > 0
        clip: true
        Rectangle {
            anchors.fill: parent
            color: gallery.theme.panelColor
            border.color: Qt.rgba(gallery.theme.textColor.r, gallery.theme.textColor.g, gallery.theme.textColor.b, 0.15)
        }
        SwipeView {
            id: slides
            objectName: "tmark-slides"
            anchors.fill: parent
            interactive: count > 1
            clip: true
            Repeater {
                model: gallery.node.tag === "slideshow" ? gallery.childrenNodes : []
                Item {
                    required property var modelData
                    TmarkMedia {
                        anchors.fill: parent
                        node: modelData
                        theme: gallery.theme
                        mediaLoader: gallery.mediaLoader
                        galleryFrame: true
                    }
                }
            }
        }
        ToolButton {
            visible: slides.count > 1
            width: 40; height: 40
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            Accessible.name: "Previous slide"
            background: Rectangle { radius: 20; color: "#99000000" }
            contentItem: NavigationArrow {}
            onClicked: slides.currentIndex = (slides.currentIndex - 1 + slides.count) % slides.count
        }
        ToolButton {
            visible: slides.count > 1
            width: 40; height: 40
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            Accessible.name: "Next slide"
            background: Rectangle { radius: 20; color: "#99000000" }
            contentItem: NavigationArrow { forward: true }
            onClicked: slides.currentIndex = (slides.currentIndex + 1) % slides.count
        }
        Rectangle {
            visible: slides.count > 1
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            width: counter.implicitWidth + 20
            height: counter.implicitHeight + 8
            radius: height / 2
            color: "#99000000"
            Text {
                id: counter
                anchors.centerIn: parent
                text: (slides.currentIndex + 1) + " / " + slides.count
                color: "white"
                font.pixelSize: 12
            }
        }
    }

    TmarkCaption {
        visible: !!gallery.node.fields.caption
        width: parent.width
        caption: gallery.node.fields.caption || ({})
        theme: gallery.theme
    }
}
