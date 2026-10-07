import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    signal accountingRequested()
    signal scheduleRequested()

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: AppTheme.spacingLarge

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("欢迎使用 StuKit")
            color: AppTheme.textPrimaryColor
            font.pixelSize: AppTheme.fontSizeDisplay
            font.bold: true
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("选择一个功能开始使用")
            color: AppTheme.textSecondaryColor
            font.pixelSize: AppTheme.fontSizeBody
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: AppTheme.spacingMedium

            Button {
                Layout.preferredWidth: 160
                Layout.preferredHeight: AppTheme.controlHeight
                text: qsTr("记账本")
                onClicked: root.accountingRequested()
            }

            Button {
                Layout.preferredWidth: 160
                Layout.preferredHeight: AppTheme.controlHeight
                text: qsTr("课表")
                onClicked: root.scheduleRequested()
            }
        }
    }
}
