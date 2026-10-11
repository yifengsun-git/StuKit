pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel
    required property var balancesViewModel
    required property var setupViewModel
    required property var transactionViewModel
    required property string startupError

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
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: root.width
            spacing: AppTheme.spacingMedium

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                Layout.topMargin: AppTheme.spacingLarge

                Button {
                    text: qsTr("上一月")
                    enabled: root.viewModel !== null
                    onClicked: root.viewModel.showPreviousMonth()
                }

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    color: AppTheme.textPrimaryColor
                    font.pixelSize: AppTheme.fontSizeHeading
                    font.bold: true
                    text: root.viewModel ? root.viewModel.monthLabel : qsTr("记账本")
                }

                Button {
                    text: qsTr("下一月")
                    enabled: root.viewModel !== null
                    onClicked: root.viewModel.showNextMonth()
                }
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                text: root.viewModel
                    ? qsTr("时区：%1  ·  更改").arg(root.viewModel.timeZoneId)
                    : ""
                visible: text.length > 0
                onClicked: root.openTimeZoneDialog()
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
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

                    Button {
                        text: qsTr("重试读取")
                        visible: root.setupViewModel !== null
                            && root.setupViewModel.hasError
                        onClicked: root.setupViewModel.load()
                    }
                }
            }

            Flow {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                spacing: AppTheme.spacingSmall
                visible: root.setupViewModel !== null

                Button {
                    text: qsTr("新增账户")
                    onClicked: root.openAccountDialog()
                }

                Button {
                    text: qsTr("新增支出分类")
                    onClicked: root.openCategoryDialog(false)
                }

                Button {
                    text: qsTr("新增收入分类")
                    onClicked: root.openCategoryDialog(true)
                }

                Button {
                    text: qsTr("管理账户与分类")
                    onClicked: setupManagementDialog.open()
                }
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("记一笔")
                enabled: root.transactionViewModel !== null
                    && root.transactionViewModel.ready
                    && root.activeError.length === 0
                highlighted: true
                onClicked: root.openTransactionDialog()
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                spacing: AppTheme.spacingMedium
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

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
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

                    Button {
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
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
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

                    Label {
                        text: qsTr("筛选本月流水")
                        color: AppTheme.textPrimaryColor
                        font.bold: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.width >= 760 ? 3 : 1
                        columnSpacing: AppTheme.spacingSmall
                        rowSpacing: AppTheme.spacingSmall

                        ComboBox {
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

                        ComboBox {
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

                        ComboBox {
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

                    TextField {
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

                        Button {
                            text: qsTr("清除筛选")
                            visible: root.viewModel
                                && root.viewModel.hasActiveTransactionFilters
                            onClicked: root.viewModel.clearTransactionFilters()
                        }
                    }
                }
            }

            ListView {
                id: transactionList

                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                Layout.preferredHeight: contentHeight
                Layout.maximumHeight: 480
                spacing: AppTheme.spacingSmall
                clip: true
                model: root.viewModel ? root.viewModel.transactions : null
                visible: root.activeError.length === 0
                    && root.viewModel !== null
                    && !root.viewModel.empty
                    && !root.viewModel.noMatchingTransactions

                delegate: Rectangle {
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
                    height: transactionDelegate.note.length > 0 ? 88 : 68
                    radius: AppTheme.radiusSmall
                    color: AppTheme.surfaceColor
                    border.color: AppTheme.borderColor

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: AppTheme.spacingMedium

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: AppTheme.spacingExtraSmall

                            Label {
                                text: transactionDelegate.categoryName
                                color: AppTheme.textPrimaryColor
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Label {
                                text: qsTr("%1 · %2")
                                    .arg(transactionDelegate.accountName)
                                    .arg(transactionDelegate.occurredAtText)
                                color: AppTheme.textSecondaryColor
                            }

                            Label {
                                Layout.fillWidth: true
                                text: transactionDelegate.note
                                color: AppTheme.textSecondaryColor
                                elide: Text.ElideRight
                                visible: transactionDelegate.note.length > 0
                            }
                        }

                        Label {
                            text: (transactionDelegate.isIncome ? "+" : "-")
                                + transactionDelegate.amountText
                            color: transactionDelegate.isIncome
                                ? AppTheme.primaryColor
                                : AppTheme.errorColor
                            font.bold: true
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.openTransactionDetails(
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

    Dialog {
        id: setupManagementDialog

        anchors.centerIn: parent
        modal: true
        title: qsTr("账户与分类管理")

        ColumnLayout {
            width: Math.min(root.width - AppTheme.spacingLarge * 2, 620)
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

                TabButton {
                    text: qsTr("账户")
                }

                TabButton {
                    text: qsTr("分类")
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 360
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

                            Button {
                                text: qsTr("重命名")
                                onClicked: root.openRenameSetupItemDialog(
                                    "account",
                                    accountManagementDelegate.accountId,
                                    accountManagementDelegate.name)
                            }

                            Button {
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

                            Button {
                                text: qsTr("重命名")
                                onClicked: root.openRenameSetupItemDialog(
                                    "category",
                                    categoryManagementDelegate.categoryId,
                                    categoryManagementDelegate.name)
                            }

                            Button {
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

            Button {
                Layout.alignment: Qt.AlignRight
                text: qsTr("关闭")
                onClicked: setupManagementDialog.close()
            }
        }
    }

    Dialog {
        id: renameSetupItemDialog

        property string itemKind: ""
        property string itemId: ""

        anchors.centerIn: parent
        modal: true
        title: itemKind === "account"
            ? qsTr("重命名账户")
            : qsTr("重命名分类")

        ColumnLayout {
            width: Math.min(root.width - AppTheme.spacingLarge * 4, 360)
            spacing: AppTheme.spacingMedium

            TextField {
                id: renameSetupItemField
                Layout.fillWidth: true
                placeholderText: qsTr("新名称")
                maximumLength: 40
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight

                Button {
                    text: qsTr("取消")
                    onClicked: renameSetupItemDialog.close()
                }

                Button {
                    text: qsTr("保存")
                    onClicked: {
                        const succeeded = renameSetupItemDialog.itemKind
                            === "account"
                            ? root.setupViewModel.renameAccount(
                                renameSetupItemDialog.itemId,
                                renameSetupItemField.text)
                            : root.setupViewModel.renameCategory(
                                renameSetupItemDialog.itemId,
                                renameSetupItemField.text)
                        if (succeeded)
                            renameSetupItemDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: archiveSetupItemDialog

        property string itemKind: ""
        property string itemId: ""
        property string itemName: ""

        anchors.centerIn: parent
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

        Label {
            width: Math.min(root.width - AppTheme.spacingLarge * 4, 380)
            text: qsTr("确定归档“%1”吗？它将不再出现在新增或编辑账单的选项中，历史账单不受影响。")
                .arg(archiveSetupItemDialog.itemName)
            color: AppTheme.textPrimaryColor
            wrapMode: Text.WordWrap
        }
    }

    Dialog {
        id: accountDialog

        anchors.centerIn: parent
        modal: true
        title: qsTr("创建资金账户")

        ColumnLayout {
            width: Math.min(root.width - AppTheme.spacingLarge * 4, 360)
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                text: qsTr("例如：现金、银行卡、支付宝")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            TextField {
                id: accountNameField
                Layout.fillWidth: true
                placeholderText: qsTr("账户名称")
                maximumLength: 40
            }

            TextField {
                id: openingBalanceField
                Layout.fillWidth: true
                placeholderText: qsTr("期初余额")
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                text: "0.00"
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight

                Button {
                    text: qsTr("取消")
                    onClicked: accountDialog.close()
                }

                Button {
                    text: qsTr("保存")
                    onClicked: {
                        if (root.setupViewModel.createAccount(
                                accountNameField.text,
                                openingBalanceField.text)) {
                            accountDialog.close()
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: categoryDialog

        property bool income: false

        anchors.centerIn: parent
        modal: true
        title: income ? qsTr("创建收入分类") : qsTr("创建支出分类")

        ColumnLayout {
            width: Math.min(root.width - AppTheme.spacingLarge * 4, 360)
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                text: categoryDialog.income
                    ? qsTr("例如：工资、奖金")
                    : qsTr("例如：餐饮、交通")
                color: AppTheme.textSecondaryColor
            }

            TextField {
                id: categoryNameField
                Layout.fillWidth: true
                placeholderText: qsTr("分类名称")
                maximumLength: 40
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight

                Button {
                    text: qsTr("取消")
                    onClicked: categoryDialog.close()
                }

                Button {
                    text: qsTr("保存")
                    onClicked: {
                        if (root.setupViewModel.createCategory(
                                categoryNameField.text,
                                categoryDialog.income)) {
                            categoryDialog.close()
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: transactionDialog

        anchors.centerIn: parent
        modal: true
        title: root.transactionViewModel && root.transactionViewModel.editing
            ? qsTr("编辑账单")
            : qsTr("新增账单")

        ColumnLayout {
            width: Math.min(root.width - AppTheme.spacingLarge * 4, 420)
            spacing: AppTheme.spacingMedium

            RowLayout {
                Layout.fillWidth: true

                ButtonGroup {
                    id: transactionTypeGroup
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("支出")
                    checkable: true
                    checked: root.transactionViewModel
                        && !root.transactionViewModel.income
                    ButtonGroup.group: transactionTypeGroup
                    onClicked: root.transactionViewModel.income = false
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("收入")
                    checkable: true
                    checked: root.transactionViewModel
                        && root.transactionViewModel.income
                    ButtonGroup.group: transactionTypeGroup
                    onClicked: root.transactionViewModel.income = true
                }
            }

            TextField {
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

            ComboBox {
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

            ComboBox {
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

                TextField {
                    id: dateField
                    Layout.fillWidth: true
                    placeholderText: qsTr("日期 YYYY-MM-DD")
                    inputMethodHints: Qt.ImhDate
                    onTextEdited: root.transactionViewModel.occurredDateText = text
                }

                TextField {
                    id: timeField
                    Layout.preferredWidth: 120
                    placeholderText: qsTr("时间 HH:mm")
                    inputMethodHints: Qt.ImhTime
                    onTextEdited: root.transactionViewModel.occurredTimeText = text
                }
            }

            TextArea {
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

            RowLayout {
                Layout.alignment: Qt.AlignRight

                Button {
                    text: qsTr("取消")
                    onClicked: transactionDialog.close()
                }

                Button {
                    text: root.transactionViewModel
                        ? root.transactionViewModel.submitButtonText
                        : qsTr("保存")
                    enabled: root.transactionViewModel
                        && root.transactionViewModel.canSubmit
                    onClicked: {
                        if (root.transactionViewModel.submit())
                            transactionDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: timeZoneDialog

        anchors.centerIn: parent
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
            width: Math.min(root.width - AppTheme.spacingLarge * 4, 460)
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

            ComboBox {
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

            ComboBox {
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

            ComboBox {
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

    Dialog {
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
        modal: true
        title: qsTr("账单详情")
        standardButtons: Dialog.Close

        GridLayout {
            width: Math.min(root.width - AppTheme.spacingLarge * 4, 420)
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

                Button {
                    text: qsTr("删除")
                    onClicked: deleteTransactionDialog.open()
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("编辑")
                    onClicked: root.openEditTransactionDialog(
                        transactionDetailDialog.transactionId)
                }
            }
        }
    }

    Dialog {
        id: deleteTransactionDialog

        anchors.centerIn: parent
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
            width: Math.min(root.width - AppTheme.spacingLarge * 4, 380)
            text: qsTr("确定永久删除这笔账单吗？删除后无法恢复。")
            color: AppTheme.textPrimaryColor
            wrapMode: Text.WordWrap
        }
    }
}
