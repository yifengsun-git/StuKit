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
    required property var weatherViewModel
    required property var tasksViewModel
    required property string accountingStartupError
    required property string scheduleStartupError

    property int currentPage: 0
    width: 960
    height: 640
    minimumWidth: Qt.platform.os === "android" ? 0 : 360
    minimumHeight: Qt.platform.os === "android" ? 0 : 480
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
        if (pageIndex === 1)
            accountingContainer.analyticsVisible = false
        currentPage = pageIndex
        navigationDrawer.close()
    }

    header: ToolBar
    {
        topPadding: Math.max(
            AppTheme.spacingExtraSmall,
            window.SafeArea.margins.top)
        leftPadding: Math.max(
            AppTheme.spacingExtraSmall,
            window.SafeArea.margins.left)
        rightPadding: Math.max(
            AppTheme.spacingExtraSmall,
            window.SafeArea.margins.right)
        bottomPadding: AppTheme.spacingExtraSmall

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
                        return accountingContainer.analyticsVisible
                            ? qsTr("记账数据分析") : qsTr("记账本")
                    case 2:
                        return qsTr("课表")
                    case 3:
                        return qsTr("天气")
                    case 4:
                        return qsTr("待办")
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
            topPadding: Math.max(
                AppTheme.spacingMedium,
                window.SafeArea.margins.top + AppTheme.spacingSmall)
            leftPadding: Math.max(
                AppTheme.spacingMedium,
                window.SafeArea.margins.left + AppTheme.spacingSmall)
            rightPadding: Math.max(
                AppTheme.spacingMedium,
                window.SafeArea.margins.right + AppTheme.spacingSmall)
            bottomPadding: Math.max(
                AppTheme.spacingMedium,
                window.SafeArea.margins.bottom + AppTheme.spacingSmall)
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

                Button
                {
                    Layout.fillWidth: true
                    text: qsTr("天气")
                    onClicked: window.navigateTo(3)
                }

                Button
                {
                    Layout.fillWidth: true
                    text: qsTr("待办")
                    onClicked: window.navigateTo(4)
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
        anchors.leftMargin: window.SafeArea.margins.left
        anchors.rightMargin: window.SafeArea.margins.right
        anchors.bottomMargin: window.SafeArea.margins.bottom
        currentIndex: window.currentPage
        focus: true

        Keys.onBackPressed: event => {
            if (accountingContainer.analyticsVisible) {
                accountingContainer.analyticsVisible = false
                event.accepted = true
            } else if (window.currentPage !== 0) {
                window.navigateTo(0)
                event.accepted = true
            } else {
                event.accepted = false
            }
        }

        HomePage
        {
            onAccountingRequested: window.navigateTo(1)
            onScheduleRequested: window.navigateTo(2)
            onWeatherRequested: window.navigateTo(3)
            onTasksRequested: window.navigateTo(4)
        }

        StackLayout {
            id: accountingContainer
            property bool analyticsVisible: false
            currentIndex: analyticsVisible ? 1 : 0

            AccountingHomePage {
                viewModel: window.accountingOverview
                balancesViewModel: window.accountingBalances
                setupViewModel: window.accountingSetup
                transactionViewModel: window.accountingTransactionEditor
                maintenanceViewModel: window.dataMaintenance
                startupError: window.accountingStartupError
                onAnalyticsRequested: accountingContainer.analyticsVisible = true
            }
            AnalyticsHomePage {
                viewModel: window.accountingAnalytics
                startupError: window.accountingStartupError
                onBackRequested: accountingContainer.analyticsVisible = false
            }
        }
        ScheduleHomePage {
            viewModel: window.scheduleOverview
            managementViewModel: window.scheduleManagement
            startupError: window.scheduleStartupError
        }
        WeatherHomePage {
            viewModel: window.weatherViewModel
        }
        TasksHomePage {
            viewModel: window.tasksViewModel
        }
    }
}
