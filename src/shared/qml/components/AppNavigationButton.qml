import QtQuick
import QtQuick.Controls

Button {
    id: control

    property bool selected: false
    property bool compact: false
    property bool iconOnly: false
    property url iconSource: ""
    property string fallbackIcon: "•"

    hoverEnabled: Qt.platform.os !== "android"
    flat: true
    implicitHeight: compact ? 52 : AppTheme.touchTargetHeight
    leftPadding: compact || iconOnly
        ? AppTheme.spacingExtraSmall : AppTheme.spacingMedium
    rightPadding: compact || iconOnly
        ? AppTheme.spacingExtraSmall : AppTheme.spacingMedium

    ToolTip.visible: iconOnly && hovered
    ToolTip.text: text
    ToolTip.delay: 350
    ToolTip.timeout: 2500

    contentItem: Item {
        implicitWidth: control.iconOnly ? 32
            : navigationLabel.implicitWidth
            + (navigationVisual.visible ? navigationVisual.width
                + AppTheme.spacingSmall : 0)
        implicitHeight: control.compact && navigationVisual.visible
            ? 38 : navigationLabel.implicitHeight

        Label {
            id: fallbackGlyph

            width: control.compact ? 18 : 22
            height: width
            x: control.compact || control.iconOnly
                ? (parent.width - width) / 2 : 0
            y: control.compact ? 1 : (parent.height - height) / 2
            visible: navigationIcon.source.toString().length === 0
            text: control.fallbackIcon
            color: control.selected
                ? AppTheme.primaryColor : AppTheme.textSecondaryColor
            font.pixelSize: control.compact ? 17 : 20
            font.bold: control.selected
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Item {
            id: navigationVisual
            width: navigationIcon.visible
                ? navigationIcon.width : fallbackGlyph.width
            height: navigationIcon.visible
                ? navigationIcon.height : fallbackGlyph.height
            visible: navigationIcon.visible || fallbackGlyph.visible
        }

        Image {
            id: navigationIcon

            width: control.compact ? 18 : 20
            height: width
            x: control.compact || control.iconOnly
                ? (parent.width - width) / 2 : 0
            y: control.compact ? 1 : (parent.height - height) / 2
            visible: source.toString().length > 0
            source: control.iconSource
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: true
        }

        Label {
            id: navigationLabel

            visible: !control.iconOnly
            x: control.compact ? 0
                : (navigationVisual.visible
                    ? navigationVisual.width + AppTheme.spacingSmall : 0)
            y: control.compact && navigationVisual.visible ? 23 : 0
            width: parent.width - x
            height: control.compact && navigationVisual.visible
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
