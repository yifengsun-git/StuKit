import QtQuick
import QtQuick.Controls

ComboBox {
    id: control

    implicitHeight: AppTheme.touchTargetHeight
    leftPadding: AppTheme.spacingMedium
    rightPadding: AppTheme.spacingExtraLarge

    contentItem: Text {
        leftPadding: 0
        rightPadding: 0
        text: control.displayText
        font: control.font
        color: AppTheme.textPrimaryColor
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: AppTheme.radiusSmall
        color: AppTheme.surfaceColor
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus
            ? AppTheme.primaryColor : AppTheme.borderColor
    }
}
