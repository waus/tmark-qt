    import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import Qt.labs.lottieqt

Column {
    id: media
    property var node: ({ tag: "img", fields: {}, text: "" })
    property var theme
    property TmarkMediaLoader mediaLoader
    property bool galleryFrame: false
    property bool cropGalleryImage: false
    readonly property bool lottie: imageDownload.lottieSize.width > 0
    readonly property real aspectRatio: lottie ? imageDownload.lottieSize.width / imageDownload.lottieSize.height : picture.sourceSize.height > 0 ? picture.sourceSize.width / picture.sourceSize.height : 0
    property bool revealed: node.fields.has_spoiler !== "t"
    onNodeChanged: revealed = node.fields.has_spoiler !== "t"
    readonly property bool video: node.tag === "video"
    readonly property bool audio: node.tag === "audio"
    width: parent ? parent.width : implicitWidth
    spacing: 6

    TmarkMediaSource {
        id: imageDownload
        loader: media.mediaLoader
        kind: "image"
        source: media.audio ? "" : media.video ? media.node.fields.preview : media.node.text
    }

    Item {
        width: media.galleryFrame ? parent.width : media.lottie ? Math.min(parent.width, imageDownload.lottieSize.width) : picture.sourceSize.width > 0 ? Math.min(parent.width, picture.sourceSize.width) : parent.width
        height: media.galleryFrame ? media.height : media.lottie ? width * imageDownload.lottieSize.height / imageDownload.lottieSize.width : picture.sourceSize.width > 0 ? Math.min(picture.sourceSize.height, width * picture.sourceSize.height / picture.sourceSize.width) : 0
        anchors.horizontalCenter: parent.horizontalCenter
        visible: !media.audio
        clip: true
        Image {
            id: picture
            anchors.fill: parent
            source: media.lottie ? "" : imageDownload.playableSource
            fillMode: media.cropGalleryImage ? Image.PreserveAspectCrop : Image.PreserveAspectFit
            asynchronous: true
            opacity: media.revealed ? 1 : 0
        }
        LottieAnimation {
            id: animation
            anchors.centerIn: parent
            width: imageDownload.lottieSize.width
            height: imageDownload.lottieSize.height
            scale: width > 0 && height > 0 ? (media.cropGalleryImage ? Math.max(parent.width / width, parent.height / height) : Math.min(parent.width / width, parent.height / height)) : 1
            source: media.lottie && media.revealed ? imageDownload.playableSource : ""
            loops: LottieAnimation.Infinite
            visible: media.lottie && media.revealed
        }
        MultiEffect {
            anchors.fill: picture
            source: picture
            visible: !media.revealed && !media.lottie
            blurEnabled: true
            blur: 1
            blurMax: 64
        }
        MouseArea {
            anchors.fill: parent
            visible: !media.revealed
            cursorShape: Qt.PointingHandCursor
            Accessible.role: Accessible.Button
            Accessible.name: "Reveal media"
            onClicked: media.revealed = true
        }
        Text {
            anchors.centerIn: parent
            visible: !media.revealed
            text: "👁️"
            font.pixelSize: 30
        }
        Text {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            visible: media.video && media.revealed
            text: "Video playback unavailable in browser build"
            color: media.theme.textColor
        }
        Text {
            anchors.centerIn: parent
            visible: media.revealed && ((picture.status === Image.Error && !media.lottie) || animation.status === LottieAnimation.Error || imageDownload.error.length > 0)
            text: imageDownload.error || (media.lottie ? "Could not load animation" : "Could not load image")
            color: media.theme.errorColor
        }
    }

    Rectangle {
        width: parent.width
        height: 56
        visible: media.audio
        color: media.theme.panelColor
        radius: 8
        Text {
            anchors.centerIn: parent
            text: "Audio playback unavailable in browser build"
            color: media.theme.mutedColor
        }
    }
    TmarkCaption {
        visible: !media.galleryFrame && !!media.node.fields.caption
        caption: media.node.fields.caption || ({})
        theme: media.theme
        width: parent.width
    }
}
