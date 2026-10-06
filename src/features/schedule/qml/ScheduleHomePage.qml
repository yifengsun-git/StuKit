import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page
{
    ColumnLayout
    {
        anchors.centerIn: parent
        spacing: 12

        Label
        {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("课表")
            font.pixelSize: 28
            font.bold: true
        }

        Label
        {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("课表功能将在后续课程中实现")
        }
    }
}
