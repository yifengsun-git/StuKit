import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    signal accountingRequested()
    signal scheduleRequested()
    signal weatherRequested()
    signal tasksRequested()

    readonly property int featureColumns: width < 520 ? 1
        : width < 920 ? 2 : 4

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

        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            columns: root.featureColumns
            rowSpacing: AppTheme.spacingMedium
            columnSpacing: AppTheme.spacingMedium

            AppButton {
                Layout.preferredWidth: 160
                Layout.preferredHeight: AppTheme.controlHeight
                text: qsTr("记账本")
                onClicked: root.accountingRequested()
            }

            AppButton {
                Layout.preferredWidth: 160
                Layout.preferredHeight: AppTheme.controlHeight
                text: qsTr("课表")
                onClicked: root.scheduleRequested()
            }

            AppButton {
                Layout.preferredWidth: 160
                Layout.preferredHeight: AppTheme.controlHeight
                text: qsTr("天气")
                onClicked: root.weatherRequested()
            }

            AppButton {
                Layout.preferredWidth: 160
                Layout.preferredHeight: AppTheme.controlHeight
                text: qsTr("待办")
                onClicked: root.tasksRequested()
            }
        }
    }
}
