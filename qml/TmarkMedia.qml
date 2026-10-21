import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Effects
import QtMultimedia
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
    property bool requested: false
    onNodeChanged: {
        revealed = node.fields.has_spoiler !== "t"
        requested = false
        player.stop()
    }
    readonly property bool video: node.tag === "video"
    readonly property bool audio: node.tag === "audio"
    width: parent ? parent.width : implicitWidth
    spacing: 6

    function clock(milliseconds) {
        const seconds = Math.floor(milliseconds / 1000)
        return Math.floor(seconds / 60) + ":" + String(seconds % 60).padStart(2, "0")
    }

    function togglePlayback() {
        if (player.playbackState === MediaPlayer.PlayingState) player.pause()
        else {
            requested = true
            if (!download.playableSource.toString().length) return
            player.play()
        }
    }

    TmarkMediaSource {
        id: download
        loader: media.mediaLoader
        kind: media.audio ? "audio" : "video"
        source: media.revealed && media.requested ? media.node.text : ""
        onPlayableSourceChanged: {
            if (media.requested && playableSource.toString().length) Qt.callLater(() => player.play())
        }
    }

    TmarkMediaSource {
        id: imageDownload
        loader: media.mediaLoader
        kind: "image"
        source: media.audio ? "" : media.video ? media.node.fields.preview : media.node.text
    }

    MediaPlayer {
        id: player
        source: download.playableSource
        videoOutput: videoOutput
        audioOutput: AudioOutput {}
        loops: media.node.fields.loop === "t" ? MediaPlayer.Infinite : 1
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
            visible: !media.revealed && !media.audio && !media.lottie
            blurEnabled: true
            blur: 1
            blurMax: 64
        }
        VideoOutput {
            id: videoOutput
            anchors.fill: parent
            visible: media.video && player.playbackState === MediaPlayer.PlayingState
            fillMode: media.cropGalleryImage ? VideoOutput.PreserveAspectCrop : VideoOutput.PreserveAspectFit
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
        ToolButton {
            anchors.centerIn: parent
            visible: media.video && media.revealed
            text: player.playbackState === MediaPlayer.PlayingState ? "⏸" : "▶"
            font.pixelSize: 28
            Accessible.name: player.playbackState === MediaPlayer.PlayingState ? "Pause video" : "Play video"
            onClicked: media.togglePlayback()
        }
        Text {
            anchors.centerIn: parent
            visible: media.revealed && ((picture.status === Image.Error && !media.lottie) || animation.status === LottieAnimation.Error || imageDownload.error.length > 0) && player.playbackState !== MediaPlayer.PlayingState
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
        Row {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8
            ToolButton {
                id: audioButton
                width: 40
                height: 40
                text: player.playbackState === MediaPlayer.PlayingState ? "⏸" : "▶"
                font.pixelSize: 20
                Accessible.name: player.playbackState === MediaPlayer.PlayingState ? "Pause audio" : "Play audio"
                onClicked: media.togglePlayback()
            }
            Basic.Slider {
                id: audioProgress
                width: Math.max(40, parent.width - audioButton.width - timeText.width - parent.spacing * 2)
                anchors.verticalCenter: parent.verticalCenter
                from: 0
                to: Math.max(1, player.duration)
                value: player.position
                onMoved: player.setPosition(value)
                Accessible.name: "Audio position"
                background: Rectangle {
                    x: audioProgress.leftPadding
                    y: audioProgress.topPadding + (audioProgress.availableHeight - height) / 2
                    width: audioProgress.availableWidth
                    height: 6
                    radius: 3
                    color: Qt.rgba(media.theme.textColor.r, media.theme.textColor.g, media.theme.textColor.b, 0.16)
                    Rectangle {
                        width: audioProgress.visualPosition * parent.width
                        height: parent.height
                        radius: parent.radius
                        color: Qt.rgba(media.theme.textColor.r, media.theme.textColor.g, media.theme.textColor.b, 0.4)
                    }
                }
                handle: Rectangle {
                    x: audioProgress.leftPadding + audioProgress.visualPosition * (audioProgress.availableWidth - width)
                    y: audioProgress.topPadding + (audioProgress.availableHeight - height) / 2
                    width: 14
                    height: 14
                    radius: 7
                    color: media.theme.mutedColor
                }
            }
            Text {
                id: timeText
                width: 90
                anchors.verticalCenter: parent.verticalCenter
                text: media.clock(player.position) + " / " + media.clock(player.duration)
                color: media.theme.textColor
                font.pixelSize: media.theme.font.pixelSize * 0.8
                horizontalAlignment: Text.AlignRight
            }
        }
    }
    Text {
        visible: player.error !== MediaPlayer.NoError || download.error.length > 0
        text: download.error || player.errorString
        color: media.theme.errorColor
        wrapMode: Text.Wrap
        width: parent.width
    }
    TmarkCaption {
        visible: !media.galleryFrame && !!media.node.fields.caption
        caption: media.node.fields.caption || ({})
        theme: media.theme
        width: parent.width
    }
}
