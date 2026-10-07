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
    color: AppTheme.backgroundColor

    palette.window: AppTheme.backgroundColor
    palette.windowText: AppTheme.textPrimaryColor
    palette.base: AppTheme.surfaceColor
    palette.text: AppTheme.textPrimaryColor
    palette.button: AppTheme.surfaceVariantColor
    palette.buttonText: AppTheme.textPrimaryColor
    palette.highlight: AppTheme.primaryColor
    palette.highlightedText: AppTheme.textOnPrimaryColor

    function navigateTo(pageIndex)
    {
        currentPage = pageIndex
        navigationDrawer.close()
    }

    header: ToolBar
    {
        background: Rectangle {
            color: AppTheme.surfaceColor
            border.color: AppTheme.borderColor
            border.width: 1
        }

        RowLayout
        {
            anchors.fill: parent
            anchors.margins: AppTheme.spacingExtraSmall

            ToolButton
            {
                text: qsTr("菜单")
                onClicked: navigationDrawer.open()
            }

            Label
            {
                Layout.fillWidth: true
                color: AppTheme.textPrimaryColor
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
        background: Rectangle {
            color: AppTheme.surfaceColor
        }

        Pane
        {
            anchors.fill: parent
            background: Rectangle {
                color: "transparent"
            }

            ColumnLayout
            {
                anchors.fill: parent
                spacing: AppTheme.spacingSmall

                Label
                {
                    text: qsTr("StuKit")
                    color: AppTheme.textPrimaryColor
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
