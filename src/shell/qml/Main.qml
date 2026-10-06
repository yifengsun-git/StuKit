import QtQuick
import QtQuick.Controls

ApplicationWindow
{
    width: 960
    height: 640
    minimumWidth: 640
    minimumHeight: 480
    visible: true
    title: qsTr("StuKit")

    Label
    {
        anchors.centerIn: parent
        text: qsTr("StuKit 工程已成功启动")
        font.pixelSize: 24
    }
}