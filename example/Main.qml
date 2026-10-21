import QtQuick
import QtQuick.Controls
import io.github.waus.tmark

ApplicationWindow {
    id: window
    width: 760
    height: 640
    visible: true
    title: "tmark QML examples"

    readonly property var examples: [
        { title: "All widgets", file: "all_widgets.tmark" },
        { title: "Other edge cases", file: "edge_cases.tmark" },
        { title: "Empty elements", file: "edge_empties.tmark" },
        { title: "Inline formatting", file: "inline_formatting.tmark" },
        { title: "Lists and tasks", file: "lists_and_tasks.tmark" },
        { title: "Maps and events", file: "maps_and_events.tmark" },
        { title: "Math and code", file: "math_and_code.tmark" },
        { title: "Media formats", file: "media_formats.tmark" },
        { title: "Media gallery", file: "media_gallery.tmark" },
        { title: "Quotes and details", file: "quotes_and_details.tmark" },
        { title: "References and anchors", file: "references_and_anchors.tmark" },
        { title: "Tables", file: "tables.tmark" }
    ]
    property string documentText: ""
    property string loadError: ""
    property bool loading: false

    function selectExample(index) {
        const file = examples[index].file
        loadError = ""
        documentText = ""
        loading = true
        const request = new XMLHttpRequest()
        request.open("GET", "https://tmark.waus.app/examples/" + file)
        request.onreadystatechange = function() {
            if (request.readyState !== 4)
                return
            if (picker.currentIndex !== index)
                return
            loading = false
            if (request.status === 200)
                documentText = request.responseText
            else
                loadError = "Could not load " + file + " (HTTP " + request.status + ")"
        }
        request.send()
    }

    TmarkViewTheme { id: documentTheme }
    color: documentTheme.backgroundColor

    Rectangle {
        id: header
        width: parent.width
        height: 68
        color: documentTheme.panelColor

        ComboBox {
            id: picker
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 16
            model: window.examples
            textRole: "title"
            onActivated: window.selectExample(currentIndex)
        }
    }

    TmarkView {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        text: window.documentText
        softParser: true
        theme: documentTheme
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: window.loading
        visible: running
    }

    Label {
        anchors.centerIn: parent
        visible: window.loadError.length > 0
        text: window.loadError
        color: documentTheme.errorColor
    }

    Component.onCompleted: selectExample(0)
}
