import QtQuick

QtObject {
    property SystemPalette systemPalette: SystemPalette {}
    property font font: Qt.font({ pixelSize: 16 })
    property font codeFont: font
    property color backgroundColor: systemPalette.base
    readonly property bool dark: backgroundColor.r * 0.2126 + backgroundColor.g * 0.7152 + backgroundColor.b * 0.0722 < 0.5
    property color textColor: systemPalette.text
    property color mutedColor: Qt.rgba(textColor.r, textColor.g, textColor.b, 0.7)
    property color linkColor: dark ? "#7ab8ff" : "#1769aa"
    property color panelColor: dark ? "#292929" : "#f4f4f4"
    property color errorColor: dark ? "#ff8080" : "#b00020"
}
