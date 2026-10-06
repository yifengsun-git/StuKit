import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page
{
    id: root

    signal accountingRequested()
    signal scheduleRequested()

    ColumnLayout
    {
        anchors.centerIn: parent
        spacing: 20

        Label
        {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("欢迎使用 StuKit")
            font.pixelSize: 28
            font.bold: true
        }

        Label
        {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("选择一个功能开始使用")
        }

        RowLayout
        {
            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            Button
            {
                Layout.preferredWidth: 160
                text: qsTr("记账本")
                onClicked: root.accountingRequested()
            }

            Button
            {
                Layout.preferredWidth: 160
                text: qsTr("课表")
                onClicked: root.scheduleRequested()
            }
        }
    }
}
