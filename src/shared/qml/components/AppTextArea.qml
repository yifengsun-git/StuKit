import QtQuick
import QtQuick.Controls

TextArea {
    id: control

    leftPadding: AppTheme.spacingMedium
    rightPadding: AppTheme.spacingMedium
    topPadding: AppTheme.spacingSmall
    bottomPadding: AppTheme.spacingSmall
    color: AppTheme.textPrimaryColor
    placeholderTextColor: AppTheme.textSecondaryColor
    selectionColor: AppTheme.primaryColor
    selectedTextColor: AppTheme.textOnPrimaryColor
    selectByMouse: true

    background: Rectangle {
        radius: AppTheme.radiusSmall
        color: AppTheme.surfaceColor
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus
            ? AppTheme.primaryColor : AppTheme.borderColor
    }
}
