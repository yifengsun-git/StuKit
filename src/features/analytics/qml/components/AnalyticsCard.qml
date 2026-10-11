import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    default property alias contentData: body.data
    property string title: ""
    property string description: ""

    implicitHeight: body.implicitHeight + AppTheme.spacingLarge * 2
    radius: AppTheme.radiusMedium
    color: AppTheme.surfaceColor
    border.color: AppTheme.borderColor

    ColumnLayout {
        id: body

        anchors.fill: parent
        anchors.margins: AppTheme.spacingLarge
        spacing: AppTheme.spacingSmall

        Label {
            Layout.fillWidth: true
            visible: root.title.length > 0
            text: root.title
            color: AppTheme.textPrimaryColor
            font.pixelSize: AppTheme.fontSizeSubtitle
            font.bold: true
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            visible: root.description.length > 0
            text: root.description
            color: AppTheme.textSecondaryColor
            wrapMode: Text.WordWrap
        }
    }
}
