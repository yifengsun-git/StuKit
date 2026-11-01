import QtQuick
import QtQuick.Controls

ComboBox {
    id: control

    implicitHeight: AppTheme.touchTargetHeight
    leftPadding: AppTheme.spacingMedium
    rightPadding: AppTheme.spacingExtraLarge

    delegate: ItemDelegate {
        id: optionDelegate

        required property int index

        width: control.width
        hoverEnabled: Qt.platform.os !== "android"
        highlighted: control.highlightedIndex === index

        contentItem: Text {
            text: control.textAt(optionDelegate.index)
            font: control.font
            color: optionDelegate.highlighted
                ? AppTheme.textOnPrimaryColor
                : AppTheme.textPrimaryColor
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: AppTheme.radiusSmall
            color: optionDelegate.highlighted
                ? "#171717"
                : optionDelegate.hovered
                    ? AppTheme.surfaceHoverColor : "transparent"
            border.width: optionDelegate.highlighted ? 1 : 0
            border.color: "#000000"

            Behavior on color {
                ColorAnimation { duration: AppTheme.animationFast }
            }
        }
    }

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

    popup: Popup {
        y: control.height + AppTheme.spacingExtraSmall
        width: control.width
        implicitHeight: Math.min(contentList.implicitHeight
            + topPadding + bottomPadding, 320)
        padding: AppTheme.spacingExtraSmall

        contentItem: ListView {
            id: contentList
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            radius: AppTheme.radiusSmall
            color: AppTheme.surfaceRaisedColor
            border.width: 1
            border.color: AppTheme.borderStrongColor
        }
    }
}
