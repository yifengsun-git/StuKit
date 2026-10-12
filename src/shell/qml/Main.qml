import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow
{
    id: window

    required property var accountingOverview
    required property var accountingBalances
    required property var accountingAnalytics
    required property var accountingSetup
    required property var accountingTransactionEditor
    required property var dataMaintenance
    required property var scheduleOverview
    required property var scheduleManagement
    required property string accountingStartupError
    required property string scheduleStartupError

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
                        return qsTr("数据分析")
                    case 3:
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
                    text: qsTr("数据分析")
                    onClicked: window.navigateTo(2)
                }

                Button
                {
                    Layout.fillWidth: true
                    text: qsTr("课表")
                    onClicked: window.navigateTo(3)
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
            onAnalyticsRequested: window.navigateTo(2)
            onScheduleRequested: window.navigateTo(3)
        }

        AccountingHomePage {
            viewModel: window.accountingOverview
            balancesViewModel: window.accountingBalances
            setupViewModel: window.accountingSetup
            transactionViewModel: window.accountingTransactionEditor
            maintenanceViewModel: window.dataMaintenance
            startupError: window.accountingStartupError
        }
        AnalyticsHomePage {
            viewModel: window.accountingAnalytics
            startupError: window.accountingStartupError
        }
        ScheduleHomePage {
            viewModel: window.scheduleOverview
            managementViewModel: window.scheduleManagement
            startupError: window.scheduleStartupError
        }
    }
}
