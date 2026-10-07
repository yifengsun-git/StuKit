import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property string heading
    required property string description

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: AppTheme.spacingSmall

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.heading
            color: AppTheme.textPrimaryColor
            font.pixelSize: AppTheme.fontSizeDisplay
            font.bold: true
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.description
            color: AppTheme.textSecondaryColor
            font.pixelSize: AppTheme.fontSizeBody
        }
    }
}
