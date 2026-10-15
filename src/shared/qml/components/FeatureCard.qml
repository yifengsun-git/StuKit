import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AbstractButton {
    id: control

    property string title: ""
    property string description: ""
    property string actionText: qsTr("打开")
    property color accentColor: AppTheme.primaryColor

    hoverEnabled: Qt.platform.os !== "android"
    implicitHeight: 148
    leftPadding: AppTheme.spacingLarge
    rightPadding: AppTheme.spacingLarge
    topPadding: AppTheme.spacingLarge
    bottomPadding: AppTheme.spacingLarge

    contentItem: ColumnLayout {
        spacing: AppTheme.spacingSmall

        Rectangle {
            Layout.preferredWidth: 36
            Layout.preferredHeight: 5
            radius: height / 2
            color: control.accentColor
        }

        Label {
            Layout.fillWidth: true
            text: control.title
            color: AppTheme.textPrimaryColor
            font.pixelSize: AppTheme.fontSizeTitle
            font.bold: true
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: control.description
            color: AppTheme.textSecondaryColor
            font.pixelSize: AppTheme.fontSizeBody
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        Label {
            text: control.actionText + "  ›"
            color: control.accentColor
            font.pixelSize: AppTheme.fontSizeBody
            font.bold: true
        }
    }

    background: Rectangle {
        radius: AppTheme.radiusLarge
        color: control.down
            ? AppTheme.selectionColor
            : control.hovered
                ? AppTheme.surfaceHoverColor
                : AppTheme.surfaceRaisedColor
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus
            ? control.accentColor : AppTheme.borderColor

        Behavior on color {
            ColorAnimation { duration: AppTheme.animationFast }
        }
    }
}
