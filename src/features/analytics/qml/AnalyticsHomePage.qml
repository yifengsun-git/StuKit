import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel
    required property string startupError

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: root.width
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                Layout.topMargin: AppTheme.spacingLarge
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                text: qsTr("数据分析")
                color: AppTheme.textPrimaryColor
                font.pixelSize: AppTheme.fontSizeDisplay
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                text: qsTr("默认展示最近 12 个月，也可以调整月份范围并按账户、分类筛选。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                Layout.preferredHeight: controlsLayout.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor

                RowLayout {
                    id: controlsLayout

                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    spacing: AppTheme.spacingSmall

                    Button {
                        text: qsTr("上一月")
                        enabled: root.viewModel !== null
                        onClicked: root.viewModel.showPreviousMonth()
                    }

                    Label {
                        Layout.preferredWidth: 90
                        horizontalAlignment: Text.AlignHCenter
                        text: root.viewModel ? root.viewModel.monthLabel : "--"
                        color: AppTheme.textPrimaryColor
                        font.bold: true
                    }

                    Button {
                        text: qsTr("下一月")
                        enabled: root.viewModel !== null
                        onClicked: root.viewModel.showNextMonth()
                    }

                    Label {
                        Layout.leftMargin: AppTheme.spacingSmall
                        text: qsTr("统计范围")
                        color: AppTheme.textSecondaryColor
                    }

                    ComboBox {
                        id: rangeBox

                        Layout.preferredWidth: 110
                        enabled: root.viewModel !== null
                        textRole: "label"
                        valueRole: "value"
                        model: [
                            { label: qsTr("近 6 个月"), value: 6 },
                            { label: qsTr("近 12 个月"), value: 12 },
                            { label: qsTr("近 24 个月"), value: 24 }
                        ]
                        currentIndex: {
                            if (!root.viewModel)
                                return 1
                            if (root.viewModel.monthCount === 6)
                                return 0
                            if (root.viewModel.monthCount === 24)
                                return 2
                            return 1
                        }
                        onActivated: root.viewModel.monthCount = currentValue
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Button {
                        text: qsTr("恢复默认")
                        enabled: root.viewModel !== null
                        onClicked: root.viewModel.resetFilters()
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                visible: text.length > 0
                text: root.startupError.length > 0
                    ? root.startupError
                    : root.viewModel && root.viewModel.hasError
                        ? root.viewModel.errorMessage
                        : ""
                color: AppTheme.errorColor
                wrapMode: Text.WordWrap
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                columns: root.width >= 760 ? 3 : 1
                columnSpacing: AppTheme.spacingMedium
                rowSpacing: AppTheme.spacingMedium

                Repeater {
                    model: [
                        {
                            title: qsTr("本月收入"),
                            value: root.viewModel
                                ? root.viewModel.selectedIncomeText : "0.00",
                            change: root.viewModel
                                ? root.viewModel.incomeChangeText : "0.00"
                        },
                        {
                            title: qsTr("本月支出"),
                            value: root.viewModel
                                ? root.viewModel.selectedExpenseText : "0.00",
                            change: root.viewModel
                                ? root.viewModel.expenseChangeText : "0.00"
                        },
                        {
                            title: qsTr("本月结余"),
                            value: root.viewModel
                                ? root.viewModel.selectedBalanceText : "0.00",
                            change: root.viewModel
                                ? root.viewModel.balanceChangeText : "0.00"
                        }
                    ]

                    delegate: Rectangle {
                        id: summaryCard

                        required property var modelData

                        Layout.fillWidth: true
                        Layout.preferredHeight: summaryLayout.implicitHeight
                            + AppTheme.spacingLarge * 2
                        radius: AppTheme.radiusMedium
                        color: AppTheme.surfaceColor
                        border.color: AppTheme.borderColor

                        ColumnLayout {
                            id: summaryLayout

                            anchors.fill: parent
                            anchors.margins: AppTheme.spacingLarge

                            Label {
                                text: summaryCard.modelData.title
                                color: AppTheme.textSecondaryColor
                            }

                            Label {
                                text: summaryCard.modelData.value
                                color: AppTheme.textPrimaryColor
                                font.pixelSize: AppTheme.fontSizeHeading
                                font.bold: true
                            }

                            Label {
                                text: qsTr("较上月 %1")
                                    .arg(summaryCard.modelData.change)
                                color: AppTheme.primaryColor
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                Layout.preferredHeight: trendPreview.implicitHeight
                    + AppTheme.spacingLarge * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor

                ColumnLayout {
                    id: trendPreview

                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingLarge
                    spacing: AppTheme.spacingSmall

                    Label {
                        text: qsTr("月度趋势数据")
                        color: AppTheme.textPrimaryColor
                        font.pixelSize: AppTheme.fontSizeHeading
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("收入、支出和结余已经转换为通用图表系列；下一课在这里绘制柱状图和折线图。")
                        color: AppTheme.textSecondaryColor
                        wrapMode: Text.WordWrap
                    }

                    Repeater {
                        model: root.viewModel ? root.viewModel.months : null

                        delegate: RowLayout {
                            id: monthlyRow

                            required property string monthLabel
                            required property string incomeText
                            required property string expenseText
                            required property string balanceText

                            Layout.fillWidth: true

                            Label {
                                Layout.preferredWidth: 80
                                text: monthlyRow.monthLabel
                                color: AppTheme.textSecondaryColor
                            }
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("收入 %1").arg(monthlyRow.incomeText)
                                color: AppTheme.textPrimaryColor
                            }
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("支出 %1").arg(monthlyRow.expenseText)
                                color: AppTheme.textPrimaryColor
                            }
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("结余 %1").arg(monthlyRow.balanceText)
                                color: AppTheme.primaryColor
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: root.viewModel && !root.viewModel.hasData
                        text: qsTr("当前分析范围还没有账单数据。")
                        color: AppTheme.textSecondaryColor
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                text: root.viewModel
                    ? qsTr("账户筛选 %1 项 · 分类筛选 %2 项 · 记账时区 %3")
                        .arg(root.viewModel.selectedAccountIds.length)
                        .arg(root.viewModel.selectedCategoryIds.length)
                        .arg(root.viewModel.timeZoneId)
                    : ""
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                Layout.bottomMargin: AppTheme.spacingLarge
                text: qsTr("图表系列模型保持通用，未来饮食、学习等模块可以提供自己的数据系列。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }
        }
    }
}
