import QtQuick
import QtQuick.Controls

Button {
    id: control

    property bool selected: false
    property bool compact: false
    property url iconSource: ""

    hoverEnabled: Qt.platform.os !== "android"
    flat: true
    implicitHeight: compact ? 52 : AppTheme.touchTargetHeight
    leftPadding: compact ? AppTheme.spacingExtraSmall : AppTheme.spacingMedium
    rightPadding: compact ? AppTheme.spacingExtraSmall : AppTheme.spacingMedium

    contentItem: Item {
        implicitWidth: navigationLabel.implicitWidth
            + (navigationIcon.visible ? navigationIcon.width
                + AppTheme.spacingSmall : 0)
        implicitHeight: control.compact && navigationIcon.visible
            ? 38 : navigationLabel.implicitHeight

        Image {
            id: navigationIcon

            width: control.compact ? 18 : 20
            height: width
            x: control.compact ? (parent.width - width) / 2 : 0
            y: control.compact ? 1 : (parent.height - height) / 2
            visible: source.toString().length > 0
            source: control.iconSource
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: true
        }

        Label {
            id: navigationLabel

            x: control.compact ? 0
                : (navigationIcon.visible
                    ? navigationIcon.width + AppTheme.spacingSmall : 0)
            y: control.compact && navigationIcon.visible ? 23 : 0
            width: parent.width - x
            height: control.compact && navigationIcon.visible
                ? parent.height - y : parent.height
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
