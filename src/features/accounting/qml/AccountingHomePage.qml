import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel
    required property var setupViewModel
    required property var transactionViewModel
    required property string startupError

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

    function openTransactionDialog() {
        transactionViewModel.resetForNewTransaction()
        if (!transactionViewModel.ready)
            return

        amountField.text = transactionViewModel.amountText
        dateField.text = transactionViewModel.occurredDateText
        timeField.text = transactionViewModel.occurredTimeText
        noteField.text = transactionViewModel.note
        transactionDialog.open()
        amountField.forceActiveFocus()
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

            Label {
                Layout.alignment: Qt.AlignHCenter
                color: AppTheme.textSecondaryColor
                text: root.viewModel
                    ? qsTr("时区：%1").arg(root.viewModel.timeZoneId)
                    : ""
                visible: text.length > 0
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
                                text: modelData.title
                                color: AppTheme.textSecondaryColor
                            }

                            Label {
                                text: modelData.value
                                color: modelData.color
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

                delegate: Rectangle {
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
                    height: note.length > 0 ? 88 : 68
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
                                text: categoryName
                                color: AppTheme.textPrimaryColor
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Label {
                                text: qsTr("%1 · %2").arg(accountName).arg(occurredAtText)
                                color: AppTheme.textSecondaryColor
                            }

                            Label {
                                Layout.fillWidth: true
                                text: note
                                color: AppTheme.textSecondaryColor
                                elide: Text.ElideRight
                                visible: note.length > 0
                            }
                        }

                        Label {
                            text: (isIncome ? "+" : "-") + amountText
                            color: isIncome
                                ? AppTheme.primaryColor
                                : AppTheme.errorColor
                            font.bold: true
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.openTransactionDetails(
                            transactionId,
                            typeText,
                            isIncome,
                            amountText,
                            accountName,
                            categoryName,
                            fullOccurredAtText,
                            note)
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
        title: qsTr("新增账单")

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
                    text: qsTr("保存")
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
        }
    }
}
