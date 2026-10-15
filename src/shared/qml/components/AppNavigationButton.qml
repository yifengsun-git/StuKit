import QtQuick
import QtQuick.Controls

Button {
    id: control

    property bool selected: false
    property bool compact: false

    hoverEnabled: Qt.platform.os !== "android"
    flat: true
    implicitHeight: compact ? 52 : AppTheme.touchTargetHeight
    leftPadding: compact ? AppTheme.spacingExtraSmall : AppTheme.spacingMedium
    rightPadding: compact ? AppTheme.spacingExtraSmall : AppTheme.spacingMedium

    contentItem: Label {
        text: control.text
        color: control.selected
            ? AppTheme.primaryColor : AppTheme.textSecondaryColor
        font.pixelSize: control.compact
            ? AppTheme.fontSizeCaption : AppTheme.fontSizeBody
        font.bold: control.selected
        horizontalAlignment: control.compact
            ? Text.AlignHCenter : Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: AppTheme.radiusMedium
        color: {
            if (control.down || control.selected)
                return AppTheme.selectionColor
            if (control.hovered)
                return AppTheme.surfaceHoverColor
            return "transparent"
        }
        border.width: control.activeFocus ? 2 : 0
        border.color: AppTheme.primaryColor

        Behavior on color {
            ColorAnimation { duration: AppTheme.animationFast }
        }
    }
}
