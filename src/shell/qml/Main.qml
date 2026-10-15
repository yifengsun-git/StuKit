import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
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
    readonly property bool weatherFeatureEnabled: false
    readonly property bool shortLayout: height < 520
    readonly property bool compactLayout:
        width < AppTheme.breakpointCompact || shortLayout
    readonly property string currentPageTitle: {
        switch (currentPage) {
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

    width: 1180
    height: 760
    minimumWidth: Qt.platform.os === "android" ? 0 : 360
    minimumHeight: Qt.platform.os === "android" ? 0 : 520
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

    function navigateTo(pageIndex) {
        if (pageIndex === 1)
            accountingContainer.analyticsVisible = false
        currentPage = pageIndex
    }

    function handleBackRequest() {
        if (Qt.inputMethod.visible) {
            Qt.inputMethod.hide()
            return true
        }
        if (accountingContainer.analyticsVisible) {
            accountingContainer.analyticsVisible = false
            return true
        }
        if (currentPage !== 0) {
            navigateTo(0)
            return true
        }
        return false
    }

    header: ToolBar {
        topPadding: Math.max(
            AppTheme.spacingSmall,
            window.SafeArea.margins.top)
        leftPadding: Math.max(
            AppTheme.spacingMedium,
            window.SafeArea.margins.left + AppTheme.spacingSmall)
        rightPadding: Math.max(
            AppTheme.spacingMedium,
            window.SafeArea.margins.right + AppTheme.spacingSmall)
        bottomPadding: AppTheme.spacingSmall

        background: Rectangle {
            color: AppTheme.surfaceColor
            border.color: AppTheme.borderColor
            border.width: 1
        }

        contentItem: RowLayout {
            spacing: AppTheme.spacingSmall

            Label {
                visible: window.compactLayout
                text: qsTr("StuKit")
                color: AppTheme.primaryColor
                font.pixelSize: AppTheme.fontSizeSubtitle
                font.bold: true
            }

            Rectangle {
                visible: window.compactLayout
                Layout.preferredWidth: 1
                Layout.preferredHeight: 20
                color: AppTheme.borderColor
            }

            Label {
                Layout.fillWidth: true
                text: window.currentPageTitle
                color: AppTheme.textPrimaryColor
                font.pixelSize: AppTheme.fontSizeTitle
                font.bold: true
                elide: Text.ElideRight
            }

            Label {
                visible: !window.compactLayout
                text: qsTr("个人工具箱")
                color: AppTheme.textSecondaryColor
                font.pixelSize: AppTheme.fontSizeBody
            }
        }
    }

    footer: ToolBar {
        visible: window.compactLayout
        leftPadding: Math.max(
            AppTheme.spacingExtraSmall,
            window.SafeArea.margins.left)
        rightPadding: Math.max(
            AppTheme.spacingExtraSmall,
            window.SafeArea.margins.right)
        topPadding: AppTheme.spacingExtraSmall
        bottomPadding: Math.max(
            AppTheme.spacingExtraSmall,
            window.SafeArea.margins.bottom)

        background: Rectangle {
            color: AppTheme.surfaceColor
            border.color: AppTheme.borderColor
            border.width: 1
        }

        contentItem: RowLayout {
            spacing: AppTheme.spacingExtraSmall

            AppNavigationButton {
                objectName: "homeBottomNavigation"
                Layout.fillWidth: true
                compact: true
                selected: window.currentPage === 0
                text: qsTr("首页")
                onClicked: window.navigateTo(0)
            }

            AppNavigationButton {
                objectName: "accountingBottomNavigation"
                Layout.fillWidth: true
                compact: true
                selected: window.currentPage === 1
                text: qsTr("记账")
                onClicked: window.navigateTo(1)
            }

            AppNavigationButton {
                objectName: "scheduleBottomNavigation"
                Layout.fillWidth: true
                compact: true
                selected: window.currentPage === 2
                text: qsTr("课表")
                onClicked: window.navigateTo(2)
            }

            AppNavigationButton {
                objectName: "tasksBottomNavigation"
                Layout.fillWidth: true
                compact: true
                selected: window.currentPage === 4
                text: qsTr("待办")
                onClicked: window.navigateTo(4)
            }

            AppNavigationButton {
                objectName: "weatherBottomNavigation"
                visible: window.weatherFeatureEnabled
                Layout.fillWidth: true
                compact: true
                selected: window.currentPage === 3
                text: qsTr("天气")
                onClicked: window.navigateTo(3)
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Pane {
            id: navigationRail

            visible: !window.compactLayout
            Layout.preferredWidth: AppTheme.navigationRailWidth
            Layout.fillHeight: true
            topPadding: AppTheme.spacingLarge
            bottomPadding: AppTheme.spacingLarge
            leftPadding: Math.max(
                AppTheme.spacingMedium,
                window.SafeArea.margins.left + AppTheme.spacingMedium)
            rightPadding: AppTheme.spacingMedium

            background: Rectangle {
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor
                border.width: 1
            }

            contentItem: ColumnLayout {
                spacing: AppTheme.spacingSmall

                Label {
                    Layout.fillWidth: true
                    text: qsTr("StuKit")
                    color: AppTheme.primaryColor
                    font.pixelSize: AppTheme.fontSizeHeading
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    Layout.bottomMargin: AppTheme.spacingMedium
                    text: qsTr("学习与生活工具箱")
                    color: AppTheme.textSecondaryColor
                    font.pixelSize: AppTheme.fontSizeCaption
                    wrapMode: Text.WordWrap
                }

                AppNavigationButton {
                    Layout.fillWidth: true
                    selected: window.currentPage === 0
                    text: qsTr("首页")
                    onClicked: window.navigateTo(0)
                }

                AppNavigationButton {
                    Layout.fillWidth: true
                    selected: window.currentPage === 1
                    text: qsTr("记账本")
                    onClicked: window.navigateTo(1)
                }

                AppNavigationButton {
                    Layout.fillWidth: true
                    selected: window.currentPage === 2
                    text: qsTr("课表")
                    onClicked: window.navigateTo(2)
                }

                AppNavigationButton {
                    Layout.fillWidth: true
                    selected: window.currentPage === 4
                    text: qsTr("待办")
                    onClicked: window.navigateTo(4)
                }

                AppNavigationButton {
                    visible: window.weatherFeatureEnabled
                    Layout.fillWidth: true
                    selected: window.currentPage === 3
                    text: qsTr("天气")
                    onClicked: window.navigateTo(3)
                }

                Item {
                    Layout.fillHeight: true
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("数据仅保存在本机")
                    color: AppTheme.textSecondaryColor
                    font.pixelSize: AppTheme.fontSizeCaption
                    wrapMode: Text.WordWrap
                }
            }
        }

        StackLayout {
            id: pageStack

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: window.compactLayout
                ? window.SafeArea.margins.left : 0
            Layout.rightMargin: window.SafeArea.margins.right
            currentIndex: window.currentPage
            focus: true

            Keys.onBackPressed: event => {
                event.accepted = window.handleBackRequest()
            }

            HomePage {
                weatherFeatureVisible: window.weatherFeatureEnabled
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
                    onAnalyticsRequested:
                        accountingContainer.analyticsVisible = true
                }

                AnalyticsHomePage {
                    viewModel: window.accountingAnalytics
                    startupError: window.accountingStartupError
                    onBackRequested:
                        accountingContainer.analyticsVisible = false
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
}
