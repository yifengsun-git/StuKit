import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow
{
    id: window

    property int currentPage: 0

    width: 960
    height: 640
    minimumWidth: 640
    minimumHeight: 480
    visible: true
    title: qsTr("StuKit")

    function navigateTo(pageIndex)
    {
        currentPage = pageIndex
        navigationDrawer.close()
    }

    header: ToolBar
    {
        RowLayout
        {
            anchors.fill: parent
            anchors.margins: 4

            ToolButton
            {
                text: qsTr("菜单")
                onClicked: navigationDrawer.open()
            }

            Label
            {
                Layout.fillWidth: true
                text:
                {
                    switch (window.currentPage)
                    {
                    case 1:
                        return qsTr("记账本")
                    case 2:
                        return qsTr("课表")
                    default:
                        return qsTr("首页")
                    }
                }
                font.bold: true
            }
        }
    }

    Drawer
    {
        id: navigationDrawer

        width: Math.min(window.width * 0.75, 280)
        height: window.height

        Pane
        {
            anchors.fill: parent

            ColumnLayout
            {
                anchors.fill: parent
                spacing: 8

                Label
                {
                    text: qsTr("StuKit")
                    font.pixelSize: 24
                    font.bold: true
                }

                Button
                {
                    Layout.fillWidth: true
                    text: qsTr("首页")
                    onClicked: window.navigateTo(0)
                }

                Button
                {
                    Layout.fillWidth: true
                    text: qsTr("记账本")
                    onClicked: window.navigateTo(1)
                }

                Button
                {
                    Layout.fillWidth: true
                    text: qsTr("课表")
                    onClicked: window.navigateTo(2)
                }

                Item
                {
                    Layout.fillHeight: true
                }
            }
        }
    }

    StackLayout
    {
        anchors.fill: parent
        currentIndex: window.currentPage

        HomePage
        {
            onAccountingRequested: window.navigateTo(1)
            onScheduleRequested: window.navigateTo(2)
        }

        AccountingHomePage {}
        ScheduleHomePage {}
    }
}
