import QtQuick
import QtQuick.Controls

Button {
    id: control

    hoverEnabled: true
    implicitHeight: AppTheme.controlHeight
    leftPadding: AppTheme.spacingMedium
    rightPadding: AppTheme.spacingMedium
    topPadding: AppTheme.spacingSmall
    bottomPadding: AppTheme.spacingSmall

    readonly property color visualBackgroundColor: {
        if (control.down)
            return AppTheme.primaryPressedColor
        if (control.highlighted || control.checked)
            return AppTheme.primaryColor
        if (control.hovered)
            return AppTheme.surfaceColor
        return control.flat ? "transparent" : AppTheme.surfaceVariantColor
    }

    contentItem: Label {
        text: control.text
        font: control.font
        color: control.down || control.highlighted || control.checked
            ? AppTheme.textOnPrimaryColor
            : AppTheme.textPrimaryColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: AppTheme.radiusSmall
        color: control.visualBackgroundColor
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus || control.highlighted
            ? AppTheme.primaryColor
            : AppTheme.borderColor
        opacity: control.enabled ? 1 : 0.45
    }
}
