pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel
    required property string startupError

    property int trendChartMode: 0
    property int expenseChartMode: 0
    property int incomeChartMode: 0
    readonly property var trendData: root.viewModel && root.viewModel.trendSeries ? root.viewModel.trendSeries.items : []

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
                text: qsTr("从默认概览开始，需要时再调整月份、账户和分类。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            AnalyticsFilterPanel {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                viewModel: root.viewModel
                expanded: root.width >= 760
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                visible: text.length > 0
                text: root.startupError.length > 0 ? root.startupError : root.viewModel && root.viewModel.hasError ? root.viewModel.errorMessage : ""
                color: AppTheme.errorColor
                wrapMode: Text.WordWrap
            }

            BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
                visible: root.viewModel && root.viewModel.loading
                running: visible
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
                            value: root.viewModel ? root.viewModel.selectedIncomeText : "0.00",
                            change: root.viewModel ? root.viewModel.incomeChangeText : "0.00",
                            color: "#2E7D32"
                        },
                        {
                            title: qsTr("本月支出"),
                            value: root.viewModel ? root.viewModel.selectedExpenseText : "0.00",
                            change: root.viewModel ? root.viewModel.expenseChangeText : "0.00",
                            color: "#C62828"
                        },
                        {
                            title: qsTr("本月结余"),
                            value: root.viewModel ? root.viewModel.selectedBalanceText : "0.00",
                            change: root.viewModel ? root.viewModel.balanceChangeText : "0.00",
                            color: AppTheme.primaryColor
                        }
                    ]

                    delegate: AnalyticsCard {
                        id: summaryCard

                        required property var modelData

                        Layout.fillWidth: true
                        title: summaryCard.modelData.title

                        Label {
                            Layout.fillWidth: true
                            text: summaryCard.modelData.value
                            color: summaryCard.modelData.color
                            font.pixelSize: AppTheme.fontSizeHeading
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("较上月 %1").arg(summaryCard.modelData.change)
                            color: AppTheme.textSecondaryColor
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                columns: root.width >= 1120 ? 2 : 1
                columnSpacing: AppTheme.spacingMedium
                rowSpacing: AppTheme.spacingMedium

                AnalyticsCard {
                    Layout.fillWidth: true
                    title: qsTr("月收入与月支出")
                    description: qsTr("观察长期收支规模，并可切换柱状图和折线图。")

                    ComboBox {
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: 120
                        model: [qsTr("分组柱状图"), qsTr("双折线图")]
                        currentIndex: root.trendChartMode
                        onActivated: root.trendChartMode = currentIndex
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 340
                        currentIndex: root.trendChartMode

                        BarSeriesChart {
                            seriesData: root.trendData
                            firstSeriesId: "income"
                            secondSeriesId: "expense"
                        }

                        LineSeriesChart {
                            seriesData: root.trendData
                            seriesId: "income"
                            secondSeriesId: "expense"
                        }
                    }
                }

                AnalyticsCard {
                    Layout.fillWidth: true
                    title: qsTr("月结余趋势")
                    description: qsTr("结余低于零时折线会进入零线下方。")

                    LineSeriesChart {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 340
                        seriesData: root.trendData
                        seriesId: "balance"
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                columns: root.width >= 1040 ? 2 : 1
                columnSpacing: AppTheme.spacingMedium
                rowSpacing: AppTheme.spacingMedium

                AnalyticsCard {
                    Layout.fillWidth: true
                    title: qsTr("支出分类")
                    description: root.viewModel ? qsTr("%1 的支出去向").arg(root.viewModel.monthLabel) : ""

                    ComboBox {
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: 120
                        model: [qsTr("横向排行"), qsTr("环形图")]
                        currentIndex: root.expenseChartMode
                        onActivated: root.expenseChartMode = currentIndex
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.expenseChartMode === 0 ? Math.max(150, expenseRanking.implicitHeight) : 330
                        currentIndex: root.expenseChartMode

                        RankingBars {
                            id: expenseRanking

                            model: root.viewModel ? root.viewModel.expenseCategories : null
                            barColor: "#C62828"
                            emptyText: qsTr("本月暂无支出分类数据")
                        }

                        DonutBreakdown {
                            model: root.viewModel ? root.viewModel.expenseCategories : null
                            emptyText: qsTr("本月暂无支出分类数据")
                        }
                    }
                }

                AnalyticsCard {
                    Layout.fillWidth: true
                    title: qsTr("收入分类")
                    description: root.viewModel ? qsTr("%1 的收入来源").arg(root.viewModel.monthLabel) : ""

                    ComboBox {
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: 120
                        model: [qsTr("横向排行"), qsTr("环形图")]
                        currentIndex: root.incomeChartMode
                        onActivated: root.incomeChartMode = currentIndex
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.incomeChartMode === 0 ? Math.max(150, incomeRanking.implicitHeight) : 330
                        currentIndex: root.incomeChartMode

                        RankingBars {
                            id: incomeRanking

                            model: root.viewModel ? root.viewModel.incomeCategories : null
                            barColor: "#2E7D32"
                            emptyText: qsTr("本月暂无收入分类数据")
                        }

                        DonutBreakdown {
                            model: root.viewModel ? root.viewModel.incomeCategories : null
                            emptyText: qsTr("本月暂无收入分类数据")
                        }
                    }
                }
            }

            AnalyticsCard {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                title: qsTr("账户资金流")
                description: qsTr("查看所选月份每个账户的收入、支出和净流入。")

                Repeater {
                    id: accountRows

                    model: root.viewModel ? root.viewModel.accountFlows : null

                    delegate: GridLayout {
                        id: accountRow

                        required property string name
                        required property string incomeText
                        required property string expenseText
                        required property string netFlowText
                        required property int transactionCount
                        required property bool archived

                        Layout.fillWidth: true
                        columns: root.width >= 760 ? 4 : 1
                        columnSpacing: AppTheme.spacingMedium
                        rowSpacing: AppTheme.spacingSmall

                        Label {
                            Layout.fillWidth: true
                            text: accountRow.archived ? qsTr("%1（已归档）").arg(accountRow.name) : accountRow.name
                            color: AppTheme.textPrimaryColor
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Label {
                            text: qsTr("收入 %1").arg(accountRow.incomeText)
                            color: "#2E7D32"
                        }

                        Label {
                            text: qsTr("支出 %1").arg(accountRow.expenseText)
                            color: "#C62828"
                        }

                        Label {
                            text: qsTr("净额 %1 · %2 笔").arg(accountRow.netFlowText).arg(accountRow.transactionCount)
                            color: AppTheme.textSecondaryColor
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: accountRows.count === 0
                    text: qsTr("本月暂无账户资金流数据")
                    color: AppTheme.textSecondaryColor
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                Layout.bottomMargin: AppTheme.spacingLarge
                text: qsTr("图表组件只依赖通用数据契约，未来饮食、学习模块也可以复用。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }
        }
    }
}
