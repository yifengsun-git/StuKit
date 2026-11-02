pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as PlatformDialogs
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel
    required property var balancesViewModel
    required property var setupViewModel
    required property var transactionViewModel
    required property var transferViewModel
    required property var budgetViewModel
    required property var maintenanceViewModel
    required property string startupError

    signal analyticsRequested()

    readonly property bool compactLayout:
        width < AppTheme.breakpointCompact
    readonly property int pagePadding: AppTheme.pagePadding(width)
    property bool setupExpanded: false
    property bool dataToolsExpanded: false
    property string transactionDisplayMode: "calendar"

    // Keep every accounting dialog inside the page on narrow windows. Dialog
    // content must use the dialog's content width instead of calculating a
    // second, potentially wider width from the page.
    readonly property real availableDialogWidth: Math.max(
        0, root.width - AppTheme.spacingMedium * 2)

    readonly property var fixedOffsetOptions: {
        const options = []
        for (let minutes = -14 * 60; minutes <= 14 * 60; minutes += 15) {
            const absoluteMinutes = Math.abs(minutes)
            const hours = Math.floor(absoluteMinutes / 60)
            const remainingMinutes = absoluteMinutes % 60
            const sign = minutes >= 0 ? "+" : "-"
            const label = "UTC" + sign
                + String(hours).padStart(2, "0") + ":"
                + String(remainingMinutes).padStart(2, "0")
            options.push({ "label": label, "minutes": minutes })
        }
        return options
    }

    function openTimeZoneDialog() {
        const currentId = viewModel.timeZoneId
        const fixedMatch = /^UTC([+-])(\d{2}):(\d{2})$/.exec(currentId)
        if (fixedMatch) {
            timeZoneModeCombo.currentIndex = 1
            const direction = fixedMatch[1] === "+" ? 1 : -1
            const minutes = direction
                * (Number(fixedMatch[2]) * 60 + Number(fixedMatch[3]))
            const fixedIndex = fixedOffsetCombo.indexOfValue(minutes)
            fixedOffsetCombo.currentIndex = fixedIndex >= 0 ? fixedIndex : 56
        } else {
            timeZoneModeCombo.currentIndex = 0
            const regionIndex = regionTimeZoneCombo.find(currentId)
            regionTimeZoneCombo.currentIndex = regionIndex >= 0 ? regionIndex : 0
        }
        timeZoneDialog.open()
    }

    function openAccountDialog() {
        accountNameField.text = ""
        openingBalanceField.text = "0.00"
        accountDialog.open()
        accountNameField.forceActiveFocus()
    }

    function openCategoryDialog(income) {
        categoryDialog.income = income
        categoryNameField.text = ""
        categoryDialog.open()
        categoryNameField.forceActiveFocus()
    }

    function openRenameSetupItemDialog(kind, itemId, currentName) {
        renameSetupItemDialog.itemKind = kind
        renameSetupItemDialog.itemId = itemId
        renameSetupItemField.text = currentName
        renameSetupItemDialog.open()
        renameSetupItemField.selectAll()
        renameSetupItemField.forceActiveFocus()
    }

    function requestArchiveSetupItem(kind, itemId, itemName) {
        archiveSetupItemDialog.itemKind = kind
        archiveSetupItemDialog.itemId = itemId
        archiveSetupItemDialog.itemName = itemName
        archiveSetupItemDialog.open()
    }

    function requestDeleteSetupItem(kind, itemId, itemName) {
        deleteSetupItemDialog.itemKind = kind
        deleteSetupItemDialog.itemId = itemId
        deleteSetupItemDialog.itemName = itemName
        deleteSetupItemDialog.open()
    }

    function openTransactionDialog() {
        transactionViewModel.resetForNewTransaction()
        if (!transactionViewModel.ready)
            return

        loadTransactionForm()
        transactionDialog.open()
        amountField.forceActiveFocus()
    }

    function openEditTransactionDialog(transactionId) {
        if (!transactionViewModel.beginEditing(transactionId))
            return

        loadTransactionForm()
        transactionDetailDialog.close()
        transactionDialog.open()
        amountField.forceActiveFocus()
    }

    function loadTransactionForm() {
        amountField.text = transactionViewModel.amountText
        dateField.text = transactionViewModel.occurredDateText
        timeField.text = transactionViewModel.occurredTimeText
        noteField.text = transactionViewModel.note
        const accountIndex = accountBox.indexOfValue(
            transactionViewModel.selectedAccountId)
        accountBox.currentIndex = accountIndex >= 0 ? accountIndex : 0
        const categoryIndex = categoryBox.indexOfValue(
            transactionViewModel.selectedCategoryId)
        categoryBox.currentIndex = categoryIndex >= 0 ? categoryIndex : 0
    }

    function openTransferDialog() {
        transferViewModel.prepare()
        transferAmountField.text = ""
        transferFeeField.text = "0.00"
        transferNoteField.text = ""
        transferDateField.text = transferViewModel.occurredDateText
        transferTimeField.text = transferViewModel.occurredTimeText
        transferSourceAccountBox.currentIndex = 0
        transferDestinationAccountBox.currentIndex =
            transferDestinationAccountBox.count > 1 ? 1 : 0
        transferOutgoingCategoryBox.currentIndex = 0
        transferIncomingCategoryBox.currentIndex = 0
        transferFeeCategoryBox.currentIndex = 0
        transferDialog.open()
        transferAmountField.forceActiveFocus()
    }

    function openTransactionDetails(transactionId, typeText, isIncome,
                                    amountText, accountName, categoryName,
                                    occurredAtText, note) {
        transactionDetailDialog.transactionId = transactionId
        transactionDetailDialog.typeText = typeText
        transactionDetailDialog.income = isIncome
        transactionDetailDialog.amountText = amountText
        transactionDetailDialog.accountName = accountName
        transactionDetailDialog.categoryName = categoryName
        transactionDetailDialog.occurredAtText = occurredAtText
        transactionDetailDialog.note = note
        transactionDetailDialog.open()
    }

    readonly property string activeError: startupError.length > 0
        ? startupError
        : viewModel && viewModel.hasError
            ? viewModel.errorMessage
            : balancesViewModel && balancesViewModel.hasError
                ? balancesViewModel.errorMessage
            : ""

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    ScrollView {
        id: pageScrollView

        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            x: Math.max(0, (pageScrollView.availableWidth - width) / 2)
            width: Math.min(
                pageScrollView.availableWidth,
                AppTheme.contentMaxWidth)
            spacing: AppTheme.spacingMedium

            AppSectionCard {
                objectName: "accountingPrimaryCard"
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                Layout.topMargin: root.pagePadding
                accentColor: AppTheme.primaryColor

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.compactLayout ? 1 : 2
                    columnSpacing: AppTheme.spacingMedium
                    rowSpacing: AppTheme.spacingSmall

                    RowLayout {
                        Layout.fillWidth: true

                        AppButton {
                            text: qsTr("上一月")
                            enabled: root.viewModel !== null
                            onClicked: root.viewModel.showPreviousMonth()
                        }

                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            color: AppTheme.textPrimaryColor
                            font.pixelSize: root.compactLayout
                                ? AppTheme.fontSizeTitle
                                : AppTheme.fontSizeHeading
                            font.bold: true
                            text: root.viewModel
                                ? root.viewModel.monthLabel : qsTr("记账本")
                        }

                        AppButton {
                            text: qsTr("下一月")
                            enabled: root.viewModel !== null
                            onClicked: root.viewModel.showNextMonth()
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        spacing: AppTheme.spacingSmall

                        AppButton {
                            objectName: "accountingNewTransactionButton"
                            text: qsTr("记一笔")
                            enabled: root.transactionViewModel !== null
                                && root.transactionViewModel.ready
                                && root.activeError.length === 0
                            highlighted: true
                            onClicked: root.openTransactionDialog()
                        }

                        AppButton {
                            objectName: "accountingTransferButton"
                            text: qsTr("账户转账")
                            enabled: root.transferViewModel !== null
                                && root.activeError.length === 0
                            onClicked: root.openTransferDialog()
                        }

                        AppButton {
                            text: qsTr("数据分析")
                            onClicked: root.analyticsRequested()
                        }

                        AppButton {
                            flat: true
                            text: root.viewModel
                                ? qsTr("时区：%1").arg(root.viewModel.timeZoneId)
                                : ""
                            visible: text.length > 0
                            onClicked: root.openTimeZoneDialog()
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                Layout.preferredHeight: setupLayout.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.primaryColor
                visible: root.setupViewModel !== null
                    && (root.setupViewModel.hasError
                        || !root.setupViewModel.hasAccounts
                        || !root.setupViewModel.hasIncomeCategories
                        || !root.setupViewModel.hasExpenseCategories)

                ColumnLayout {
                    id: setupLayout
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    spacing: AppTheme.spacingSmall

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("开始记账前，先准备账户和分类")
                        color: AppTheme.textPrimaryColor
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.setupViewModel && root.setupViewModel.hasError
                            ? root.setupViewModel.errorMessage
                            : qsTr("账户表示钱存放的位置，分类说明一笔钱的用途或来源。请使用下方入口完成准备。")
                        color: root.setupViewModel && root.setupViewModel.hasError
                            ? AppTheme.errorColor
                            : AppTheme.textSecondaryColor
                        wrapMode: Text.WordWrap
                    }

                    AppButton {
                        text: qsTr("重试读取")
                        visible: root.setupViewModel !== null
                            && root.setupViewModel.hasError
                        onClicked: root.setupViewModel.load()
                    }
                }
            }

            AppSectionCard {
                objectName: "accountingSetupCard"
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                title: qsTr("账户与分类")
                description: root.setupExpanded
                    ? qsTr("维护资金账户和收支分类。归档后的项目不会出现在新账单选项中。")
                    : qsTr("低频配置已收起，展开后可以新增或管理。")
                visible: root.setupViewModel !== null

                AppButton {
                    flat: true
                    text: root.setupExpanded
                        ? qsTr("收起账户与分类") : qsTr("展开账户与分类")
                    onClicked: root.setupExpanded = !root.setupExpanded
                }

                Flow {
                    objectName: "accountingSetupActions"
                    Layout.fillWidth: true
                    Layout.preferredHeight: implicitHeight
                    spacing: AppTheme.spacingSmall
                    visible: root.setupExpanded

                    AppButton {
                        text: qsTr("新增账户")
                        onClicked: root.openAccountDialog()
                    }

                    AppButton {
                        text: qsTr("新增支出分类")
                        onClicked: root.openCategoryDialog(false)
                    }

                    AppButton {
                        text: qsTr("新增收入分类")
                        onClicked: root.openCategoryDialog(true)
                    }

                    AppButton {
                        flat: true
                        text: qsTr("管理全部")
                        onClicked: setupManagementDialog.open()
                    }
                }
            }

            AppSectionCard {
                objectName: "accountingDataToolsCard"
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                title: qsTr("数据工具")
                description: root.dataToolsExpanded
                    ? qsTr("备份或恢复数据库，也可以导出 CSV 后使用表格软件分析。")
                    : qsTr("备份、恢复与导出工具默认收起。")
                visible: root.maintenanceViewModel !== null

                AppButton {
                    flat: true
                    text: root.dataToolsExpanded
                        ? qsTr("收起数据工具") : qsTr("展开数据工具")
                    onClicked: root.dataToolsExpanded
                        = !root.dataToolsExpanded
                }

                GridLayout {
                    objectName: "accountingDataToolsFlow"
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible
                        ? (root.compactLayout
                            ? AppTheme.touchTargetHeight * 3
                                + AppTheme.spacingSmall * 2
                            : AppTheme.touchTargetHeight)
                        : 0
                    Layout.minimumHeight: Layout.preferredHeight
                    Layout.maximumHeight: Layout.preferredHeight
                    columns: root.compactLayout ? 1 : 3
                    columnSpacing: AppTheme.spacingSmall
                    rowSpacing: AppTheme.spacingSmall
                    visible: root.dataToolsExpanded

                    AppButton {
                        Layout.fillWidth: true
                        text: qsTr("备份数据库")
                        enabled: !root.maintenanceViewModel.busy
                        onClicked: backupFileDialog.open()
                    }

                    AppButton {
                        Layout.fillWidth: true
                        text: qsTr("恢复数据库")
                        enabled: !root.maintenanceViewModel.busy
                        onClicked: restoreFileDialog.open()
                    }

                    AppButton {
                        Layout.fillWidth: true
                        text: qsTr("导出全部流水 CSV")
                        enabled: !root.maintenanceViewModel.busy
                        highlighted: true
                        onClicked: exportCsvFileDialog.open()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    color: root.maintenanceViewModel.hasError
                        ? AppTheme.errorColor
                        : AppTheme.textSecondaryColor
                    text: root.maintenanceViewModel.hasError
                        ? root.maintenanceViewModel.errorMessage
                        : root.maintenanceViewModel.statusMessage
                    wrapMode: Text.WrapAnywhere
                    visible: text.length > 0
                }
            }

            GridLayout {
                objectName: "accountingSummaryGrid"
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                columns: root.width >= 820 ? 4
                    : root.width < 480 ? 1 : 2
                columnSpacing: AppTheme.spacingMedium
                rowSpacing: AppTheme.spacingMedium
                visible: root.viewModel !== null

                Repeater {
                    model: [
                        {
                            title: qsTr("总资产"),
                            value: root.balancesViewModel
                                ? root.balancesViewModel.activeTotalText
                                : "0.00",
                            color: AppTheme.textPrimaryColor
                        },
                        {
                            title: qsTr("收入"),
                            value: root.viewModel ? root.viewModel.totalIncomeText : "0.00",
                            color: AppTheme.primaryColor
                        },
                        {
                            title: qsTr("支出"),
                            value: root.viewModel ? root.viewModel.totalExpenseText : "0.00",
                            color: AppTheme.errorColor
                        },
                        {
                            title: qsTr("结余"),
                            value: root.viewModel ? root.viewModel.balanceText : "0.00",
                            color: AppTheme.textPrimaryColor
                        }
                    ]

                    delegate: Rectangle {
                        id: summaryCard

                        required property var modelData

                        Layout.fillWidth: true
                        Layout.preferredHeight: 96
                        radius: AppTheme.radiusMedium
                        color: AppTheme.surfaceColor
                        border.color: AppTheme.borderColor

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: AppTheme.spacingMedium

                            Label {
                                text: summaryCard.modelData.title
                                color: AppTheme.textSecondaryColor
                            }

                            Label {
                                text: summaryCard.modelData.value
                                color: summaryCard.modelData.color
                                font.pixelSize: AppTheme.fontSizeHeading
                                font.bold: true
                            }
                        }
                    }
                }
            }

            AppSectionCard {
                id: monthlyBudgetCard
                objectName: "accountingMonthlyBudgetCard"
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                title: qsTr("本月分类预算")
                description: root.budgetViewModel
                    && root.budgetViewModel.hasBudgets
                    ? qsTr("按支出分类跟踪本月消费进度。")
                    : qsTr("为餐饮、交通等支出分类设置本月额度。")
                visible: root.budgetViewModel !== null

                AppButton {
                    objectName: "accountingManageBudgetButton"
                    text: qsTr("管理预算")
                    enabled: root.budgetViewModel
                        && root.budgetViewModel.hasCategories
                    onClicked: budgetManagementDialog.open()
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: AppTheme.spacingMedium
                    visible: root.budgetViewModel
                        && root.budgetViewModel.hasBudgets

                    Repeater {
                        id: monthlyBudgetRepeater
                        objectName: "accountingMonthlyBudgetRepeater"
                        model: root.budgetViewModel
                            ? root.budgetViewModel.categories : []

                        ColumnLayout {
                            required property var modelData

                            Layout.fillWidth: true
                            spacing: AppTheme.spacingSmall
                            visible: modelData.hasBudget

                            RowLayout {
                                Layout.fillWidth: true

                                Label {
                                    Layout.fillWidth: true
                                    text: parent.parent.modelData.categoryName
                                    color: AppTheme.textPrimaryColor
                                    font.bold: true
                                }

                                Label {
                                    text: qsTr("%1 / %2 元 · %3")
                                        .arg(parent.parent.modelData.spentText)
                                        .arg(parent.parent.modelData.limitText)
                                        .arg(parent.parent.modelData.percentageText)
                                    color: parent.parent.modelData.overBudget
                                        ? AppTheme.errorColor
                                        : AppTheme.textSecondaryColor
                                }
                            }

                            ProgressBar {
                                Layout.fillWidth: true
                                from: 0
                                to: 1
                                value: parent.modelData.progress
                            }

                            Label {
                                text: parent.modelData.overBudget
                                    ? qsTr("已超支 %1 元").arg(
                                          parent.modelData.remainingText)
                                    : qsTr("剩余 %1 元").arg(
                                          parent.modelData.remainingText)
                                color: parent.modelData.overBudget
                                    ? AppTheme.errorColor
                                    : AppTheme.textSecondaryColor
                            }
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.budgetViewModel
                        && !root.budgetViewModel.hasCategories
                    text: qsTr("请先创建至少一个支出分类。")
                    color: AppTheme.textSecondaryColor
                    wrapMode: Text.WordWrap
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.budgetViewModel
                        && root.budgetViewModel.hasError
                    text: root.budgetViewModel
                        ? root.budgetViewModel.errorMessage : ""
                    color: AppTheme.errorColor
                    wrapMode: Text.WordWrap
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                Layout.preferredHeight: errorLayout.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.errorColor
                visible: root.activeError.length > 0

                RowLayout {
                    id: errorLayout
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium

                    Label {
                        Layout.fillWidth: true
                        color: AppTheme.errorColor
                        wrapMode: Text.WordWrap
                        text: root.activeError
                    }

                    AppButton {
                        text: qsTr("重试")
                        visible: root.viewModel !== null
                            && root.startupError.length === 0
                        onClicked: root.viewModel.load()
                    }
                }
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: AppTheme.spacingLarge
                color: AppTheme.textSecondaryColor
                text: qsTr("这个月还没有账单")
                visible: root.activeError.length === 0
                    && root.viewModel !== null
                    && root.viewModel.empty
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                Layout.preferredHeight: transactionFilterLayout.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor
                visible: root.activeError.length === 0
                    && root.viewModel !== null
                    && !root.viewModel.empty

                ColumnLayout {
                    id: transactionFilterLayout

                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    spacing: AppTheme.spacingSmall

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("本月流水")
                            color: AppTheme.textPrimaryColor
                            font.bold: true
                        }

                        AppButton {
                            objectName: "accountingCalendarModeButton"
                            text: qsTr("日历")
                            highlighted: root.transactionDisplayMode === "calendar"
                            onClicked: root.transactionDisplayMode = "calendar"
                        }

                        AppButton {
                            objectName: "accountingListModeButton"
                            text: qsTr("列表")
                            highlighted: root.transactionDisplayMode === "list"
                            onClicked: {
                                root.transactionDisplayMode = "list"
                                root.viewModel.clearCalendarDate()
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.width >= 760 ? 3 : 1
                        columnSpacing: AppTheme.spacingSmall
                        rowSpacing: AppTheme.spacingSmall

                        AppComboBox {
                            id: transactionTypeFilterBox

                            Layout.fillWidth: true
                            model: [
                                { "label": qsTr("全部收支"), "value": 0 },
                                { "label": qsTr("只看收入"), "value": 1 },
                                { "label": qsTr("只看支出"), "value": 2 }
                            ]
                            textRole: "label"
                            valueRole: "value"
                            currentIndex: root.viewModel
                                ? root.viewModel.typeFilter
                                : 0
                            onActivated: root.viewModel.typeFilter = currentValue
                        }

                        AppComboBox {
                            id: transactionAccountFilterBox

                            Layout.fillWidth: true
                            model: root.viewModel
                                ? root.viewModel.accountFilterOptions
                                : []
                            textRole: "label"
                            valueRole: "value"
                            currentIndex: {
                                if (!root.viewModel)
                                    return 0
                                const selected = indexOfValue(
                                    root.viewModel.accountIdFilter)
                                return selected >= 0 ? selected : 0
                            }
                            onActivated: root.viewModel.accountIdFilter
                                = currentValue
                        }

                        AppComboBox {
                            id: transactionCategoryFilterBox

                            Layout.fillWidth: true
                            model: root.viewModel
                                ? root.viewModel.categoryFilterOptions
                                : []
                            textRole: "label"
                            valueRole: "value"
                            currentIndex: {
                                if (!root.viewModel)
                                    return 0
                                const selected = indexOfValue(
                                    root.viewModel.categoryIdFilter)
                                return selected >= 0 ? selected : 0
                            }
                            onActivated: root.viewModel.categoryIdFilter
                                = currentValue
                        }
                    }

                    AppTextField {
                        id: transactionSearchField

                        Layout.fillWidth: true
                        placeholderText: qsTr("搜索备注、账户名或分类名")
                        text: root.viewModel ? root.viewModel.searchText : ""
                        onTextEdited: root.viewModel.searchText = text
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            Layout.fillWidth: true
                            text: root.viewModel
                                && root.viewModel.noMatchingTransactions
                                ? qsTr("没有符合条件的账单")
                                : qsTr("显示 %1 条账单").arg(
                                      root.viewModel
                                          ? root.viewModel.filteredTransactionCount
                                          : 0)
                            color: root.viewModel
                                && root.viewModel.noMatchingTransactions
                                ? AppTheme.errorColor
                                : AppTheme.textSecondaryColor
                        }

                        AppButton {
                            text: qsTr("清除筛选")
                            visible: root.viewModel
                                && root.viewModel.hasActiveTransactionFilters
                            onClicked: root.viewModel.clearTransactionFilters()
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                Layout.preferredHeight: calendarContent.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor
                visible: root.activeError.length === 0
                    && root.viewModel !== null
                    && !root.viewModel.empty
                    && root.transactionDisplayMode === "calendar"

                ColumnLayout {
                    id: calendarContent
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    spacing: AppTheme.spacingSmall

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("点击日期查看当天账单；绿色为收入，红色为支出。")
                        color: AppTheme.textSecondaryColor
                        wrapMode: Text.WordWrap
                    }

                    AccountingCalendar {
                        Layout.fillWidth: true
                        days: root.viewModel ? root.viewModel.calendarDays : []
                        selectedDate: root.viewModel
                            ? root.viewModel.selectedCalendarDate : ""
                        compactLayout: root.compactLayout
                        onDateSelected: dateText => {
                            if (root.viewModel.selectCalendarDate(dateText))
                                dailyTransactionsDialog.open()
                        }
                    }
                }
            }

            ListView {
                id: transactionList
                objectName: "accountingTransactionList"

                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                Layout.preferredHeight: Math.min(
                    Math.max(contentHeight, 68), 480)
                Layout.maximumHeight: 480
                spacing: AppTheme.spacingSmall
                clip: true
                model: root.viewModel ? root.viewModel.transactions : null
                visible: root.activeError.length === 0
                    && root.viewModel !== null
                    && !root.viewModel.empty
                    && !root.viewModel.noMatchingTransactions
                    && root.transactionDisplayMode === "list"

                delegate: Item {
                    id: transactionDelegate

                    required property string transactionId
                    required property bool isIncome
                    required property string amountText
                    required property string accountName
                    required property string categoryName
                    required property string typeText
                    required property string occurredAtText
                    required property string fullOccurredAtText
                    required property string note

                    width: transactionList.width
                    height: transactionItem.implicitHeight

                    TransactionListItem {
                        id: transactionItem

                        anchors.fill: parent
                        transactionId: transactionDelegate.transactionId
                        isIncome: transactionDelegate.isIncome
                        amountText: transactionDelegate.amountText
                        accountName: transactionDelegate.accountName
                        categoryName: transactionDelegate.categoryName
                        typeText: transactionDelegate.typeText
                        occurredAtText: transactionDelegate.occurredAtText
                        fullOccurredAtText:
                            transactionDelegate.fullOccurredAtText
                        note: transactionDelegate.note
                        onActivated: root.openTransactionDetails(
                            transactionDelegate.transactionId,
                            transactionDelegate.typeText,
                            transactionDelegate.isIncome,
                            transactionDelegate.amountText,
                            transactionDelegate.accountName,
                            transactionDelegate.categoryName,
                            transactionDelegate.fullOccurredAtText,
                            transactionDelegate.note)
                    }
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.minimumHeight: AppTheme.spacingLarge
            }
        }
    }

    AppDialog {
        id: budgetManagementDialog
        objectName: "accountingBudgetManagementDialog"

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 620)
        modal: true
        title: qsTr("%1 分类预算").arg(
            root.budgetViewModel ? root.budgetViewModel.monthLabel : "")
        standardButtons: Dialog.Close

        ColumnLayout {
            width: parent.width
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                text: qsTr("预算按月独立保存。输入金额后点击保存；删除只会移除预算，不会删除分类或账单。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            Repeater {
                id: budgetManagementRepeater
                objectName: "accountingBudgetManagementRepeater"
                model: root.budgetViewModel
                    ? root.budgetViewModel.categories : []

                Rectangle {
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.preferredHeight: budgetEditorLayout.implicitHeight
                        + AppTheme.spacingMedium * 2
                    radius: AppTheme.radiusSmall
                    color: AppTheme.surfaceVariantColor
                    border.color: AppTheme.borderColor

                    ColumnLayout {
                        id: budgetEditorLayout
                        anchors.fill: parent
                        anchors.margins: AppTheme.spacingMedium
                        spacing: AppTheme.spacingSmall

                        Label {
                            text: parent.parent.modelData.categoryName
                            color: AppTheme.textPrimaryColor
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: AppTheme.spacingSmall

                            AppTextField {
                                id: budgetAmountField
                                Layout.fillWidth: true
                                text: parent.parent.parent.modelData.limitText
                                placeholderText: qsTr("本月预算金额")
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                            }

                            AppButton {
                                text: qsTr("保存")
                                highlighted: true
                                onClicked: root.budgetViewModel.saveBudget(
                                    parent.parent.parent.modelData.categoryId,
                                    budgetAmountField.text)
                            }

                            AppButton {
                                text: qsTr("删除")
                                flat: true
                                visible: parent.parent.parent.modelData.hasBudget
                                onClicked: root.budgetViewModel.removeBudget(
                                    parent.parent.parent.modelData.categoryId)
                            }
                        }

                        Label {
                            visible: parent.parent.modelData.hasBudget
                            text: qsTr("本月已支出 %1 元")
                                .arg(parent.parent.modelData.spentText)
                            color: AppTheme.textSecondaryColor
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: root.budgetViewModel
                    && root.budgetViewModel.hasError
                text: root.budgetViewModel
                    ? root.budgetViewModel.errorMessage : ""
                color: AppTheme.errorColor
                wrapMode: Text.WordWrap
            }
        }
    }

    AppDialog {
        id: dailyTransactionsDialog
        objectName: "accountingDailyTransactionsDialog"

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 620)
        modal: true
        title: qsTr("%1 的流水记录").arg(
            root.viewModel ? root.viewModel.selectedCalendarDate : "")
        standardButtons: Dialog.Close

        onClosed: {
            if (root.viewModel)
                root.viewModel.clearCalendarDate()
        }

        ColumnLayout {
            width: parent.width
            spacing: AppTheme.spacingSmall

            Label {
                Layout.fillWidth: true
                text: qsTr("共 %1 笔，点击任意流水可以查看详情。")
                    .arg(dailyTransactionList.count)
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            ListView {
                id: dailyTransactionList
                objectName: "accountingDailyTransactionList"

                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(
                    Math.max(contentHeight, 96), 420)
                spacing: AppTheme.spacingSmall
                clip: true
                model: root.viewModel
                    ? root.viewModel.dailyTransactions : null

                delegate: Item {
                    id: dailyTransactionDelegate

                    required property string transactionId
                    required property bool isIncome
                    required property string amountText
                    required property string accountName
                    required property string categoryName
                    required property string typeText
                    required property string occurredAtText
                    required property string fullOccurredAtText
                    required property string note

                    width: dailyTransactionList.width
                    height: dailyTransactionItem.implicitHeight

                    TransactionListItem {
                        id: dailyTransactionItem

                        anchors.fill: parent
                        transactionId:
                            dailyTransactionDelegate.transactionId
                        isIncome: dailyTransactionDelegate.isIncome
                        amountText: dailyTransactionDelegate.amountText
                        accountName: dailyTransactionDelegate.accountName
                        categoryName: dailyTransactionDelegate.categoryName
                        typeText: dailyTransactionDelegate.typeText
                        occurredAtText:
                            dailyTransactionDelegate.occurredAtText
                        fullOccurredAtText:
                            dailyTransactionDelegate.fullOccurredAtText
                        note: dailyTransactionDelegate.note
                        onActivated: root.openTransactionDetails(
                            dailyTransactionDelegate.transactionId,
                            dailyTransactionDelegate.typeText,
                            dailyTransactionDelegate.isIncome,
                            dailyTransactionDelegate.amountText,
                            dailyTransactionDelegate.accountName,
                            dailyTransactionDelegate.categoryName,
                            dailyTransactionDelegate.fullOccurredAtText,
                            dailyTransactionDelegate.note)
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: dailyTransactionList.count === 0
                    text: qsTr("当天没有流水记录")
                    color: AppTheme.textSecondaryColor
                }
            }
        }
    }

    AppDialog {
        id: setupManagementDialog
        objectName: "accountingSetupManagementDialog"

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 660)
        modal: true
        title: qsTr("账户与分类管理")

        footer: Item {
            implicitHeight: setupManagementCloseButton.implicitHeight
                + AppTheme.spacingMedium * 2

            AppButton {
                id: setupManagementCloseButton

                anchors.right: parent.right
                anchors.rightMargin: AppTheme.spacingLarge
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("关闭")
                onClicked: setupManagementDialog.close()
            }
        }

        ColumnLayout {
            width: parent.width
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                text: qsTr("归档项会从新增账单的选项中隐藏，但不会删除历史账单。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            TabBar {
                id: setupManagementTabs
                Layout.fillWidth: true
                spacing: AppTheme.spacingSmall
                background: Rectangle {
                    color: "transparent"
                }

                TabButton {
                    objectName: "managedAccountsTab"
                    text: qsTr("账户")
                    contentItem: Label {
                        text: parent.text
                        color: parent.checked
                            ? "#ffffff" : AppTheme.textPrimaryColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        objectName: "managedAccountsTabBackground"
                        radius: AppTheme.radiusSmall
                        color: parent.checked
                            ? "#171717"
                            : (parent.hovered
                                ? AppTheme.surfaceHoverColor
                                : AppTheme.surfaceVariantColor)
                        border.color: parent.checked
                            ? "#171717" : AppTheme.borderColor

                        Behavior on color {
                            ColorAnimation { duration: AppTheme.animationFast }
                        }
                    }
                }

                TabButton {
                    objectName: "managedCategoriesTab"
                    text: qsTr("分类")
                    contentItem: Label {
                        text: parent.text
                        color: parent.checked
                            ? "#ffffff" : AppTheme.textPrimaryColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        objectName: "managedCategoriesTabBackground"
                        radius: AppTheme.radiusSmall
                        color: parent.checked
                            ? "#171717"
                            : (parent.hovered
                                ? AppTheme.surfaceHoverColor
                                : AppTheme.surfaceVariantColor)
                        border.color: parent.checked
                            ? "#171717" : AppTheme.borderColor

                        Behavior on color {
                            ColorAnimation { duration: AppTheme.animationFast }
                        }
                    }
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(
                    180, Math.min(360, root.height - 280))
                currentIndex: setupManagementTabs.currentIndex

                ListView {
                    id: managedAccountList
                    clip: true
                    spacing: AppTheme.spacingSmall
                    model: root.setupViewModel
                        && root.balancesViewModel
                        ? root.balancesViewModel.accounts
                        : null

                    delegate: Rectangle {
                        id: accountManagementDelegate

                        required property string accountId
                        required property string name
                        required property string openingBalanceText
                        required property string currentBalanceText
                        required property bool archived

                        width: managedAccountList.width
                        height: accountManagementRow.implicitHeight
                            + AppTheme.spacingMedium * 2
                        radius: AppTheme.radiusSmall
                        color: accountManagementDelegate.archived
                            ? AppTheme.surfaceVariantColor
                            : AppTheme.surfaceColor
                        border.color: AppTheme.borderColor

                        RowLayout {
                            id: accountManagementRow
                            anchors.fill: parent
                            anchors.margins: AppTheme.spacingMedium
                            spacing: AppTheme.spacingSmall

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: AppTheme.spacingExtraSmall

                                Label {
                                    Layout.fillWidth: true
                                    text: accountManagementDelegate.name
                                    color: AppTheme.textPrimaryColor
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("当前 %1 · 期初 %2 · %3")
                                        .arg(accountManagementDelegate.currentBalanceText)
                                        .arg(accountManagementDelegate.openingBalanceText)
                                        .arg(accountManagementDelegate.archived
                                             ? qsTr("已归档")
                                             : qsTr("使用中"))
                                    color: AppTheme.textSecondaryColor
                                    elide: Text.ElideRight
                                }
                            }

                            AppButton {
                                text: qsTr("重命名")
                                onClicked: root.openRenameSetupItemDialog(
                                    "account",
                                    accountManagementDelegate.accountId,
                                    accountManagementDelegate.name)
                            }

                            AppButton {
                                text: accountManagementDelegate.archived
                                    ? qsTr("恢复")
                                    : qsTr("归档")
                                onClicked: {
                                    if (accountManagementDelegate.archived) {
                                        root.setupViewModel.setAccountArchived(
                                            accountManagementDelegate.accountId,
                                            false)
                                    } else {
                                        root.requestArchiveSetupItem(
                                            "account",
                                            accountManagementDelegate.accountId,
                                            accountManagementDelegate.name)
                                    }
                                }
                            }

                            AppButton {
                                visible: accountManagementDelegate.archived
                                text: qsTr("删除")
                                onClicked: root.requestDeleteSetupItem(
                                    "account",
                                    accountManagementDelegate.accountId,
                                    accountManagementDelegate.name)
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: managedAccountList.count === 0
                        text: qsTr("还没有账户")
                        color: AppTheme.textSecondaryColor
                    }
                }

                ListView {
                    id: managedCategoryList
                    clip: true
                    spacing: AppTheme.spacingSmall
                    model: root.setupViewModel
                        ? root.setupViewModel.categories
                        : null

                    delegate: Rectangle {
                        id: categoryManagementDelegate

                        required property string categoryId
                        required property string name
                        required property bool isIncome
                        required property bool archived

                        width: managedCategoryList.width
                        height: categoryManagementRow.implicitHeight
                            + AppTheme.spacingMedium * 2
                        radius: AppTheme.radiusSmall
                        color: categoryManagementDelegate.archived
                            ? AppTheme.surfaceVariantColor
                            : AppTheme.surfaceColor
                        border.color: AppTheme.borderColor

                        RowLayout {
                            id: categoryManagementRow
                            anchors.fill: parent
                            anchors.margins: AppTheme.spacingMedium
                            spacing: AppTheme.spacingSmall

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: AppTheme.spacingExtraSmall

                                Label {
                                    Layout.fillWidth: true
                                    text: categoryManagementDelegate.name
                                    color: AppTheme.textPrimaryColor
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("%1分类 · %2")
                                        .arg(categoryManagementDelegate.isIncome
                                             ? qsTr("收入")
                                             : qsTr("支出"))
                                        .arg(categoryManagementDelegate.archived
                                             ? qsTr("已归档")
                                             : qsTr("使用中"))
                                    color: AppTheme.textSecondaryColor
                                    elide: Text.ElideRight
                                }
                            }

                            AppButton {
                                text: qsTr("重命名")
                                onClicked: root.openRenameSetupItemDialog(
                                    "category",
                                    categoryManagementDelegate.categoryId,
                                    categoryManagementDelegate.name)
                            }

                            AppButton {
                                text: categoryManagementDelegate.archived
                                    ? qsTr("恢复")
                                    : qsTr("归档")
                                onClicked: {
                                    if (categoryManagementDelegate.archived) {
                                        root.setupViewModel.setCategoryArchived(
                                            categoryManagementDelegate.categoryId,
                                            false)
                                    } else {
                                        root.requestArchiveSetupItem(
                                            "category",
                                            categoryManagementDelegate.categoryId,
                                            categoryManagementDelegate.name)
                                    }
                                }
                            }

                            AppButton {
                                visible: categoryManagementDelegate.archived
                                text: qsTr("删除")
                                onClicked: root.requestDeleteSetupItem(
                                    "category",
                                    categoryManagementDelegate.categoryId,
                                    categoryManagementDelegate.name)
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: managedCategoryList.count === 0
                        text: qsTr("还没有分类")
                        color: AppTheme.textSecondaryColor
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: root.setupViewModel && root.setupViewModel.hasError
                text: root.setupViewModel
                    ? root.setupViewModel.errorMessage
                    : ""
                color: AppTheme.errorColor
                wrapMode: Text.WordWrap
            }

        }
    }

    AppDialog {
        id: renameSetupItemDialog

        property string itemKind: ""
        property string itemId: ""

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 400)
        modal: true
        title: itemKind === "account"
            ? qsTr("重命名账户")
            : qsTr("重命名分类")
        enterActionEnabled: true
        onEnterActionRequested: submitRename()

        function submitRename() {
            const succeeded = itemKind === "account"
                ? root.setupViewModel.renameAccount(
                    itemId, renameSetupItemField.text)
                : root.setupViewModel.renameCategory(
                    itemId, renameSetupItemField.text)
            if (succeeded)
                close()
        }

        ColumnLayout {
            width: parent.width
            spacing: AppTheme.spacingMedium

            AppTextField {
                id: renameSetupItemField
                Layout.fillWidth: true
                placeholderText: qsTr("新名称")
                maximumLength: 40
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight

                AppButton {
                    text: qsTr("取消")
                    onClicked: renameSetupItemDialog.close()
                }

                AppButton {
                    text: qsTr("保存")
                    onClicked: renameSetupItemDialog.submitRename()
                }
            }
        }
    }

    AppDialog {
        id: archiveSetupItemDialog

        property string itemKind: ""
        property string itemId: ""
        property string itemName: ""

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 420)
        modal: true
        title: itemKind === "account"
            ? qsTr("归档账户")
            : qsTr("归档分类")
        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: {
            if (itemKind === "account")
                root.setupViewModel.setAccountArchived(itemId, true)
            else
                root.setupViewModel.setCategoryArchived(itemId, true)
        }

        ColumnLayout {
            width: parent.width

            Label {
                Layout.fillWidth: true
                text: qsTr("确定归档“%1”吗？它将不再出现在新增或编辑账单的选项中，历史账单不受影响。")
                    .arg(archiveSetupItemDialog.itemName)
                color: AppTheme.textPrimaryColor
                wrapMode: Text.WordWrap
            }
        }
    }

    AppDialog {
        id: deleteSetupItemDialog

        property string itemKind: ""
        property string itemId: ""
        property string itemName: ""

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 440)
        modal: true
        title: itemKind === "account"
            ? qsTr("永久删除账户")
            : qsTr("永久删除分类")
        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: {
            if (itemKind === "account")
                root.setupViewModel.deleteAccount(itemId)
            else
                root.setupViewModel.deleteCategory(itemId)
        }

        ColumnLayout {
            width: parent.width

            Label {
                Layout.fillWidth: true
                text: qsTr("确定永久删除“%1”吗？此操作无法撤销。仍被历史账单使用的项目不会被删除。")
                    .arg(deleteSetupItemDialog.itemName)
                color: AppTheme.textPrimaryColor
                wrapMode: Text.WordWrap
            }
        }
    }

    AppDialog {
        id: accountDialog

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 400)
        modal: true
        title: qsTr("创建资金账户")
        enterActionEnabled: true
        onEnterActionRequested: submitAccount()

        function submitAccount() {
            if (root.setupViewModel.createAccount(
                    accountNameField.text, openingBalanceField.text)) {
                close()
            }
        }

        ColumnLayout {
            width: parent.width
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                text: qsTr("例如：现金、银行卡、支付宝")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            AppTextField {
                id: accountNameField
                Layout.fillWidth: true
                placeholderText: qsTr("账户名称")
                maximumLength: 40
            }

            AppTextField {
                id: openingBalanceField
                Layout.fillWidth: true
                placeholderText: qsTr("期初余额")
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                text: "0.00"
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight

                AppButton {
                    text: qsTr("取消")
                    onClicked: accountDialog.close()
                }

                AppButton {
                    text: qsTr("保存")
                    onClicked: accountDialog.submitAccount()
                }
            }
        }
    }

    AppDialog {
        id: categoryDialog

        property bool income: false

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 400)
        modal: true
        title: income ? qsTr("创建收入分类") : qsTr("创建支出分类")
        enterActionEnabled: true
        onEnterActionRequested: submitCategory()

        function submitCategory() {
            if (root.setupViewModel.createCategory(
                    categoryNameField.text, income)) {
                close()
            }
        }

        ColumnLayout {
            width: parent.width
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                text: categoryDialog.income
                    ? qsTr("例如：工资、奖金")
                    : qsTr("例如：餐饮、交通")
                color: AppTheme.textSecondaryColor
            }

            AppTextField {
                id: categoryNameField
                Layout.fillWidth: true
                placeholderText: qsTr("分类名称")
                maximumLength: 40
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight

                AppButton {
                    text: qsTr("取消")
                    onClicked: categoryDialog.close()
                }

                AppButton {
                    text: qsTr("保存")
                    onClicked: categoryDialog.submitCategory()
                }
            }
        }
    }

    AppDialog {
        id: transactionDialog
        objectName: "accountingTransactionDialog"

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 460)
        modal: true
        title: root.transactionViewModel && root.transactionViewModel.editing
            ? qsTr("编辑账单")
            : qsTr("新增账单")
        enterActionEnabled: !transactionDateTimePicker.opened
            && !accountBox.activeFocus && !categoryBox.activeFocus
            && !noteField.activeFocus && !templateNameField.activeFocus
        onEnterActionRequested: submitTransaction()

        function submitTransaction() {
            if (root.transactionViewModel.submit())
                close()
        }

        footer: Item {
            implicitHeight: transactionDialogFooter.implicitHeight
                + AppTheme.spacingMedium * 2

            RowLayout {
                id: transactionDialogFooter

                anchors.right: parent.right
                anchors.rightMargin: AppTheme.spacingLarge
                anchors.verticalCenter: parent.verticalCenter
                spacing: AppTheme.spacingSmall

                AppButton {
                    text: qsTr("取消")
                    onClicked: transactionDialog.close()
                }

                AppButton {
                    text: root.transactionViewModel
                        ? root.transactionViewModel.submitButtonText
                        : qsTr("保存")
                    enabled: root.transactionViewModel
                        && root.transactionViewModel.canSubmit
                    highlighted: true
                    onClicked: transactionDialog.submitTransaction()
                }
            }
        }

        ColumnLayout {
            width: parent.width
            spacing: AppTheme.spacingMedium

            ColumnLayout {
                Layout.fillWidth: true
                spacing: AppTheme.spacingSmall
                visible: root.transactionViewModel
                    && !root.transactionViewModel.editing
                    && root.transactionViewModel.templates.length > 0

                Label {
                    text: qsTr("快速记账")
                    color: AppTheme.textSecondaryColor
                }

                Repeater {
                    id: transactionTemplateRepeater
                    objectName: "accountingTransactionTemplateRepeater"
                    model: root.transactionViewModel
                        ? root.transactionViewModel.templates : []

                    RowLayout {
                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        spacing: AppTheme.spacingSmall

                        AppButton {
                            Layout.fillWidth: true
                            text: {
                                const amount = modelData.amountText || ""
                                return amount.length > 0
                                    ? qsTr("%1 · %2 元").arg(modelData.name)
                                          .arg(amount)
                                    : modelData.name
                            }
                            onClicked: {
                                if (root.transactionViewModel.applyTemplate(
                                        index)) {
                                    root.loadTransactionForm()
                                    amountField.forceActiveFocus()
                                }
                            }
                        }

                        AppButton {
                            text: qsTr("删除")
                            flat: true
                            onClicked: root.transactionViewModel
                                .removeTemplate(index)
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                ButtonGroup {
                    id: transactionTypeGroup
                }

                AppButton {
                    Layout.fillWidth: true
                    text: qsTr("支出")
                    checkable: true
                    checked: root.transactionViewModel
                        && !root.transactionViewModel.income
                    ButtonGroup.group: transactionTypeGroup
                    onClicked: root.transactionViewModel.income = false
                }

                AppButton {
                    Layout.fillWidth: true
                    text: qsTr("收入")
                    checkable: true
                    checked: root.transactionViewModel
                        && root.transactionViewModel.income
                    ButtonGroup.group: transactionTypeGroup
                    onClicked: root.transactionViewModel.income = true
                }
            }

            AppTextField {
                id: amountField
                Layout.fillWidth: true
                placeholderText: qsTr("金额，例如 12.34")
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onTextEdited: root.transactionViewModel.amountText = text
            }

            Label {
                text: qsTr("账户")
                color: AppTheme.textSecondaryColor
            }

            AppComboBox {
                id: accountBox
                Layout.fillWidth: true
                model: root.transactionViewModel
                    ? root.transactionViewModel.availableAccounts
                    : null
                textRole: "name"
                valueRole: "accountId"
                onActivated: root.transactionViewModel.selectedAccountId
                    = currentValue
            }

            Label {
                text: root.transactionViewModel && root.transactionViewModel.income
                    ? qsTr("收入分类")
                    : qsTr("支出分类")
                color: AppTheme.textSecondaryColor
            }

            AppComboBox {
                id: categoryBox
                Layout.fillWidth: true
                model: root.transactionViewModel
                    ? root.transactionViewModel.availableCategories
                    : null
                textRole: "name"
                valueRole: "categoryId"
                onActivated: root.transactionViewModel.selectedCategoryId
                    = currentValue
            }

            RowLayout {
                Layout.fillWidth: true

                AppTextField {
                    id: dateField
                    objectName: "accountingTransactionDateField"
                    Layout.fillWidth: true
                    placeholderText: qsTr("日期 YYYY-MM-DD")
                    inputMethodHints: Qt.ImhDate
                    onTextEdited: root.transactionViewModel.occurredDateText
                        = text
                }

                AppTextField {
                    id: timeField
                    objectName: "accountingTransactionTimeField"
                    Layout.preferredWidth: 120
                    placeholderText: qsTr("时间 HH:mm")
                    inputMethodHints: Qt.ImhTime
                    onTextEdited: root.transactionViewModel.occurredTimeText
                        = text
                }

                AppButton {
                    objectName: "accountingTransactionDatePickerButton"
                    text: qsTr("选择")
                    onClicked: transactionDateTimePicker.openFor(dateField.text)
                }
            }

            AppTextArea {
                id: noteField
                Layout.fillWidth: true
                Layout.preferredHeight: 72
                placeholderText: qsTr("备注（可选）")
                wrapMode: TextEdit.Wrap
                onTextChanged: {
                    if (activeFocus)
                        root.transactionViewModel.note = text
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: AppTheme.spacingSmall
                visible: root.transactionViewModel
                    && !root.transactionViewModel.editing

                AppTextField {
                    id: templateNameField
                    objectName: "accountingTransactionTemplateNameField"
                    Layout.fillWidth: true
                    placeholderText: qsTr("模板名称，例如午餐")
                    maximumLength: 30
                }

                AppButton {
                    objectName: "accountingSaveTransactionTemplateButton"
                    text: qsTr("存为模板")
                    onClicked: {
                        if (root.transactionViewModel.saveTemplate(
                                templateNameField.text)) {
                            templateNameField.clear()
                            amountField.forceActiveFocus()
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: root.transactionViewModel
                    && root.transactionViewModel.hasError
                text: root.transactionViewModel
                    ? root.transactionViewModel.errorMessage
                    : ""
                color: AppTheme.errorColor
                wrapMode: Text.WordWrap
            }

        }
    }

    DateTimePickerDialog {
        id: transactionDateTimePicker
        objectName: "accountingTransactionDateTimePicker"
        allowEmpty: false
        includeTime: false
        onDateTimeSelected: value => {
            dateField.text = value
            root.transactionViewModel.occurredDateText = value
        }
    }

    AppDialog {
        id: transferDialog
        objectName: "accountingTransferDialog"

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 560)
        modal: true
        title: qsTr("账户转账")
        enterActionEnabled: !transferDateTimePicker.opened
            && !transferSourceAccountBox.activeFocus
            && !transferDestinationAccountBox.activeFocus
            && !transferOutgoingCategoryBox.activeFocus
            && !transferIncomingCategoryBox.activeFocus
            && !transferFeeCategoryBox.activeFocus
            && !transferNoteField.activeFocus
        onEnterActionRequested: submitTransfer()

        function submitTransfer() {
            if (root.transferViewModel.submit(
                    transferAmountField.text,
                    transferFeeField.text,
                    transferSourceAccountBox.currentValue,
                    transferDestinationAccountBox.currentValue,
                    transferOutgoingCategoryBox.currentValue,
                    transferIncomingCategoryBox.currentValue,
                    transferFeeCategoryBox.currentValue,
                    transferDateField.text,
                    transferTimeField.text,
                    transferNoteField.text)) {
                close()
            }
        }

        footer: Item {
            implicitHeight: transferDialogFooter.implicitHeight
                + AppTheme.spacingMedium * 2

            RowLayout {
                id: transferDialogFooter
                anchors.right: parent.right
                anchors.rightMargin: AppTheme.spacingLarge
                anchors.verticalCenter: parent.verticalCenter
                spacing: AppTheme.spacingSmall

                AppButton {
                    text: qsTr("取消")
                    onClicked: transferDialog.close()
                }
                AppButton {
                    objectName: "accountingSubmitTransferButton"
                    text: qsTr("保存转账")
                    enabled: root.transferViewModel
                        && root.transferViewModel.ready
                    highlighted: true
                    onClicked: transferDialog.submitTransfer()
                }
            }
        }

        ColumnLayout {
            width: parent.width
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                text: qsTr("将生成一条转出和一条转入流水；手续费大于 0 时，再生成一条手续费支出。三条流水会一次性保存。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.compactLayout ? 1 : 2
                columnSpacing: AppTheme.spacingMedium
                rowSpacing: AppTheme.spacingSmall

                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("转出账户"); color: AppTheme.textSecondaryColor }
                    AppComboBox {
                        id: transferSourceAccountBox
                        Layout.fillWidth: true
                        model: root.transferViewModel
                            ? root.transferViewModel.availableAccounts : null
                        textRole: "name"
                        valueRole: "accountId"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("转入账户"); color: AppTheme.textSecondaryColor }
                    AppComboBox {
                        id: transferDestinationAccountBox
                        Layout.fillWidth: true
                        model: root.transferViewModel
                            ? root.transferViewModel.availableAccounts : null
                        textRole: "name"
                        valueRole: "accountId"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("转账金额"); color: AppTheme.textSecondaryColor }
                    AppTextField {
                        id: transferAmountField
                        Layout.fillWidth: true
                        placeholderText: qsTr("例如 1000.00")
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("手续费（可为 0）"); color: AppTheme.textSecondaryColor }
                    AppTextField {
                        id: transferFeeField
                        Layout.fillWidth: true
                        placeholderText: qsTr("例如 2.00")
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("转出分类"); color: AppTheme.textSecondaryColor }
                    AppComboBox {
                        id: transferOutgoingCategoryBox
                        Layout.fillWidth: true
                        model: root.transferViewModel
                            ? root.transferViewModel.expenseCategories : null
                        textRole: "name"
                        valueRole: "categoryId"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("转入分类"); color: AppTheme.textSecondaryColor }
                    AppComboBox {
                        id: transferIncomingCategoryBox
                        Layout.fillWidth: true
                        model: root.transferViewModel
                            ? root.transferViewModel.incomeCategories : null
                        textRole: "name"
                        valueRole: "categoryId"
                    }
                }
            }

            Label {
                text: qsTr("手续费分类")
                color: AppTheme.textSecondaryColor
            }
            AppComboBox {
                id: transferFeeCategoryBox
                Layout.fillWidth: true
                model: root.transferViewModel
                    ? root.transferViewModel.expenseCategories : null
                textRole: "name"
                valueRole: "categoryId"
            }

            RowLayout {
                Layout.fillWidth: true
                AppTextField {
                    id: transferDateField
                    Layout.fillWidth: true
                    placeholderText: qsTr("日期 YYYY-MM-DD")
                    inputMethodHints: Qt.ImhDate
                }
                AppTextField {
                    id: transferTimeField
                    Layout.preferredWidth: 120
                    placeholderText: qsTr("时间 HH:mm")
                    inputMethodHints: Qt.ImhTime
                }
                AppButton {
                    objectName: "accountingTransferDatePickerButton"
                    text: qsTr("选择")
                    onClicked: transferDateTimePicker.openFor(
                        transferDateField.text)
                }
            }

            AppTextArea {
                id: transferNoteField
                Layout.fillWidth: true
                Layout.preferredHeight: 72
                placeholderText: qsTr("备注（可选）")
                wrapMode: TextEdit.Wrap
            }

            Label {
                Layout.fillWidth: true
                visible: root.transferViewModel
                    && root.transferViewModel.hasError
                text: root.transferViewModel
                    ? root.transferViewModel.errorMessage : ""
                color: AppTheme.errorColor
                wrapMode: Text.WordWrap
            }
        }
    }

    DateTimePickerDialog {
        id: transferDateTimePicker
        objectName: "accountingTransferDateTimePicker"
        allowEmpty: false
        includeTime: false
        onDateTimeSelected: value => transferDateField.text = value
    }

    AppDialog {
        id: timeZoneDialog

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 500)
        modal: true
        title: qsTr("选择记账时区")
        standardButtons: Dialog.Save | Dialog.Cancel

        onAccepted: {
            let saved = false
            if (timeZoneModeCombo.currentIndex === 0) {
                saved = root.viewModel.selectTimeZone(
                    regionTimeZoneCombo.editText.trim())
            } else {
                saved = root.viewModel.selectFixedOffsetMinutes(
                    fixedOffsetCombo.currentValue)
            }

            if (!saved)
                Qt.callLater(function() { timeZoneDialog.open() })
        }

        ColumnLayout {
            width: parent.width
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                text: qsTr("时区会影响账单所属月份、列表时间和新增账单的时间解释。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            Label {
                text: qsTr("时区类型")
                color: AppTheme.textSecondaryColor
            }

            AppComboBox {
                id: timeZoneModeCombo
                Layout.fillWidth: true
                model: [qsTr("地区时区（自动处理夏令时）"),
                        qsTr("固定 UTC 偏移")]
            }

            Label {
                text: qsTr("地区时区")
                color: AppTheme.textSecondaryColor
                visible: timeZoneModeCombo.currentIndex === 0
            }

            AppComboBox {
                id: regionTimeZoneCombo
                Layout.fillWidth: true
                visible: timeZoneModeCombo.currentIndex === 0
                editable: true
                model: root.viewModel ? root.viewModel.availableTimeZoneIds : []
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("可输入或选择 IANA 时区 ID，例如 Asia/Shanghai。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
                visible: timeZoneModeCombo.currentIndex === 0
            }

            Label {
                text: qsTr("UTC 偏移")
                color: AppTheme.textSecondaryColor
                visible: timeZoneModeCombo.currentIndex === 1
            }

            AppComboBox {
                id: fixedOffsetCombo
                Layout.fillWidth: true
                visible: timeZoneModeCombo.currentIndex === 1
                model: root.fixedOffsetOptions
                textRole: "label"
                valueRole: "minutes"
            }

            Label {
                Layout.fillWidth: true
                text: root.viewModel && root.viewModel.hasError
                    ? root.viewModel.errorMessage
                    : ""
                color: AppTheme.errorColor
                wrapMode: Text.WordWrap
                visible: text.length > 0
            }
        }
    }

    PlatformDialogs.FileDialog {
        id: backupFileDialog

        title: qsTr("保存 StuKit 数据备份")
        fileMode: PlatformDialogs.FileDialog.SaveFile
        defaultSuffix: "sqlite"
        nameFilters: [qsTr("StuKit 数据库 (*.sqlite)")]
        onAccepted: root.maintenanceViewModel.backupTo(selectedFile)
    }

    PlatformDialogs.FileDialog {
        id: restoreFileDialog

        title: qsTr("选择 StuKit 数据备份")
        fileMode: PlatformDialogs.FileDialog.OpenFile
        nameFilters: [qsTr("StuKit 数据库 (*.sqlite *.sqlite3 *.db)")]
        onAccepted: {
            restoreConfirmationDialog.backupFile = selectedFile
            restoreConfirmationDialog.open()
        }
    }

    PlatformDialogs.FileDialog {
        id: exportCsvFileDialog

        title: qsTr("导出全部流水")
        fileMode: PlatformDialogs.FileDialog.SaveFile
        defaultSuffix: "csv"
        nameFilters: [qsTr("CSV 文件 (*.csv)")]
        onAccepted: root.maintenanceViewModel.exportTransactionsTo(selectedFile)
    }

    AppDialog {
        id: restoreConfirmationDialog

        property url backupFile

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 480)
        modal: true
        title: qsTr("恢复数据")
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: root.maintenanceViewModel.restoreFrom(backupFile)

        ColumnLayout {
            width: parent.width

            Label {
                Layout.fillWidth: true
                text: qsTr("恢复会用所选备份替换当前记账数据。StuKit 会先自动保存当前数据库，确定继续吗？")
                color: AppTheme.textPrimaryColor
                wrapMode: Text.WordWrap
            }
        }
    }

    AppDialog {
        id: transactionDetailDialog

        property string transactionId: ""
        property string typeText: ""
        property bool income: false
        property string amountText: ""
        property string accountName: ""
        property string categoryName: ""
        property string occurredAtText: ""
        property string note: ""

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 460)
        modal: true
        title: qsTr("账单详情")
        standardButtons: Dialog.Close

        GridLayout {
            width: parent.width
            columns: 2
            rowSpacing: AppTheme.spacingMedium
            columnSpacing: AppTheme.spacingLarge

            Label {
                text: qsTr("类型")
                color: AppTheme.textSecondaryColor
            }
            Label {
                text: transactionDetailDialog.typeText
                color: AppTheme.textPrimaryColor
            }

            Label {
                text: qsTr("金额")
                color: AppTheme.textSecondaryColor
            }
            Label {
                text: (transactionDetailDialog.income ? "+" : "-")
                    + transactionDetailDialog.amountText
                color: transactionDetailDialog.income
                    ? AppTheme.primaryColor
                    : AppTheme.errorColor
                font.bold: true
            }

            Label {
                text: qsTr("分类")
                color: AppTheme.textSecondaryColor
            }
            Label {
                text: transactionDetailDialog.categoryName
                color: AppTheme.textPrimaryColor
            }

            Label {
                text: qsTr("账户")
                color: AppTheme.textSecondaryColor
            }
            Label {
                text: transactionDetailDialog.accountName
                color: AppTheme.textPrimaryColor
            }

            Label {
                text: qsTr("发生时间")
                color: AppTheme.textSecondaryColor
            }
            Label {
                text: transactionDetailDialog.occurredAtText
                color: AppTheme.textPrimaryColor
            }

            Label {
                text: qsTr("备注")
                color: AppTheme.textSecondaryColor
            }
            Label {
                Layout.fillWidth: true
                text: transactionDetailDialog.note.length > 0
                    ? transactionDetailDialog.note
                    : qsTr("无")
                color: AppTheme.textPrimaryColor
                wrapMode: Text.WordWrap
            }

            Label {
                text: qsTr("账单 ID")
                color: AppTheme.textSecondaryColor
            }
            Label {
                Layout.fillWidth: true
                text: transactionDetailDialog.transactionId
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WrapAnywhere
            }

            RowLayout {
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.topMargin: AppTheme.spacingMedium

                AppButton {
                    text: qsTr("删除")
                    onClicked: deleteTransactionDialog.open()
                }

                Item {
                    Layout.fillWidth: true
                }

                AppButton {
                    text: qsTr("编辑")
                    onClicked: root.openEditTransactionDialog(
                        transactionDetailDialog.transactionId)
                }
            }
        }
    }

    AppDialog {
        id: deleteTransactionDialog

        anchors.centerIn: parent
        width: Math.min(root.availableDialogWidth, 420)
        modal: true
        title: qsTr("删除账单")
        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: {
            if (root.transactionViewModel.deleteTransaction(
                    transactionDetailDialog.transactionId)) {
                transactionDetailDialog.close()
            }
        }

        Label {
            width: parent.width
            text: qsTr("确定永久删除这笔账单吗？删除后无法恢复。")
            color: AppTheme.textPrimaryColor
            wrapMode: Text.WordWrap
        }
    }
}
