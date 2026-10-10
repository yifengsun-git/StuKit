import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel
    required property var setupViewModel
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
                            : qsTr("账户表示钱存放的位置，分类说明一笔钱的用途或来源。")
                        color: root.setupViewModel && root.setupViewModel.hasError
                            ? AppTheme.errorColor
                            : AppTheme.textSecondaryColor
                        wrapMode: Text.WordWrap
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: AppTheme.spacingSmall

                        Button {
                            text: qsTr("创建资金账户")
                            visible: root.setupViewModel !== null
                                && !root.setupViewModel.hasAccounts
                            onClicked: root.openAccountDialog()
                        }

                        Button {
                            text: qsTr("创建支出分类")
                            visible: root.setupViewModel !== null
                                && !root.setupViewModel.hasExpenseCategories
                            onClicked: root.openCategoryDialog(false)
                        }

                        Button {
                            text: qsTr("创建收入分类")
                            visible: root.setupViewModel !== null
                                && !root.setupViewModel.hasIncomeCategories
                            onClicked: root.openCategoryDialog(true)
                        }

                        Button {
                            text: qsTr("重试读取")
                            visible: root.setupViewModel !== null
                                && root.setupViewModel.hasError
                            onClicked: root.setupViewModel.load()
                        }
                    }
                }
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
                    required property bool isIncome
                    required property string amountText
                    required property string occurredAtText
                    required property string note

                    width: transactionList.width
                    height: 68
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
                                text: note.length > 0 ? note : qsTr("无备注")
                                color: AppTheme.textPrimaryColor
                                elide: Text.ElideRight
                            }

                            Label {
                                text: occurredAtText
                                color: AppTheme.textSecondaryColor
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
}
