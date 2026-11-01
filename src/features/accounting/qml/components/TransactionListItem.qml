pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    objectName: "accountingTransactionListItem"

    required property string transactionId
    required property bool isIncome
    required property string amountText
    required property string accountName
    required property string categoryName
    required property string typeText
    required property string occurredAtText
    required property string fullOccurredAtText
    required property string note

    signal activated()

    implicitHeight: note.length > 0 ? 88 : 68
    height: implicitHeight
    radius: AppTheme.radiusSmall
    color: transactionMouseArea.containsMouse
        ? AppTheme.surfaceHoverColor : AppTheme.surfaceColor
    border.color: AppTheme.borderColor

    RowLayout {
        anchors.fill: parent
        anchors.margins: AppTheme.spacingMedium

        ColumnLayout {
            Layout.fillWidth: true
            spacing: AppTheme.spacingExtraSmall

            Label {
                text: root.categoryName
                color: AppTheme.textPrimaryColor
                font.bold: true
                elide: Text.ElideRight
            }

            Label {
                text: qsTr("%1 · %2")
                    .arg(root.accountName)
                    .arg(root.occurredAtText)
                color: AppTheme.textSecondaryColor
            }

            Label {
                Layout.fillWidth: true
                text: root.note
                color: AppTheme.textSecondaryColor
                elide: Text.ElideRight
                visible: root.note.length > 0
            }
        }

        Label {
            text: (root.isIncome ? "+" : "-") + root.amountText
            color: root.isIncome
                ? AppTheme.primaryColor : AppTheme.errorColor
            font.bold: true
        }
    }

    MouseArea {
        id: transactionMouseArea
        anchors.fill: parent
        hoverEnabled: Qt.platform.os !== "android"
        cursorShape: Qt.PointingHandCursor
        onClicked: root.activated()
    }

    Behavior on color {
        ColorAnimation { duration: AppTheme.animationFast }
    }
}
