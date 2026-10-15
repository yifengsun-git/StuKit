pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel
    required property string startupError

    signal backRequested()

    property int trendChartMode: 0
    property int expenseChartMode: 0
    property int incomeChartMode: 0
    readonly property bool compactLayout:
        width < AppTheme.breakpointCompact
    readonly property int pagePadding: AppTheme.pagePadding(width)
    readonly property int accountFlowColumns: compactLayout ? 1 : 4
    readonly property int chartHeight: compactLayout ? 280 : 340
    readonly property var trendData: root.viewModel && root.viewModel.trendSeries ? root.viewModel.trendSeries.items : []

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    ScrollView {
        id: pageScrollView

        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: Math.min(
                pageScrollView.availableWidth - root.pagePadding * 2,
                AppTheme.contentMaxWidth)
            x: Math.max(root.pagePadding,
                        (pageScrollView.availableWidth - width) / 2)
            spacing: AppTheme.spacingMedium

            Item {
                Layout.preferredHeight: root.pagePadding
            }

            AppSectionCard {
                objectName: "analyticsPrimaryCard"
                Layout.fillWidth: true
                title: qsTr("记账数据分析")
                description: qsTr("从默认概览开始，需要时再调整月份、账户和分类。图表组件可被其它模块复用。")
                accentColor: AppTheme.primaryColor

                AppButton {
                    objectName: "analyticsBackButton"
                    text: qsTr("返回记账本")
                    onClicked: root.backRequested()
                }
            }

            AnalyticsFilterPanel {
                objectName: "analyticsFilterPanel"
                Layout.fillWidth: true
                viewModel: root.viewModel
                compactLayout: root.compactLayout
                expanded: !root.compactLayout
            }

            Label {
                Layout.fillWidth: true
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
                objectName: "analyticsSummaryGrid"
                Layout.fillWidth: true
                columns: root.compactLayout ? 1 : 3
                columnSpacing: AppTheme.spacingMedium
                rowSpacing: AppTheme.spacingMedium

                Repeater {
                    model: [
                        {
                            title: qsTr("本月收入"),
                            value: root.viewModel ? root.viewModel.selectedIncomeText : "0.00",
                            change: root.viewModel ? root.viewModel.incomeChangeText : "0.00",
                            color: AppTheme.successColor
                        },
                        {
                            title: qsTr("本月支出"),
                            value: root.viewModel ? root.viewModel.selectedExpenseText : "0.00",
                            change: root.viewModel ? root.viewModel.expenseChangeText : "0.00",
                            color: AppTheme.errorColor
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
                        compactLayout: root.compactLayout
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
                objectName: "analyticsTrendGrid"
                Layout.fillWidth: true
                columns: root.width >= AppTheme.breakpointWide ? 2 : 1
                columnSpacing: AppTheme.spacingMedium
                rowSpacing: AppTheme.spacingMedium

                AnalyticsCard {
                    Layout.fillWidth: true
                    compactLayout: root.compactLayout
                    title: qsTr("月收入与月支出")
                    description: qsTr("观察长期收支规模，并可切换柱状图和折线图。")

                    AppComboBox {
                        objectName: "analyticsTrendChartSelector"
                        Layout.fillWidth: root.compactLayout
                        Layout.alignment: root.compactLayout
                            ? Qt.AlignLeft : Qt.AlignRight
                        Layout.preferredWidth: root.compactLayout ? -1 : 160
                        model: [qsTr("分组柱状图"), qsTr("双折线图")]
                        currentIndex: root.trendChartMode
                        onActivated: root.trendChartMode = currentIndex
                    }

                    StackLayout {
                        objectName: "trendChartStack"
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.chartHeight
                        currentIndex: root.trendChartMode

                        BarSeriesChart {
                            objectName: "monthlyBarChart"
                            seriesData: root.trendData
                            firstSeriesId: "income"
                            secondSeriesId: "expense"
                        }

                        LineSeriesChart {
                            objectName: "monthlyLineChart"
                            seriesData: root.trendData
                            seriesId: "income"
                            secondSeriesId: "expense"
                        }
                    }
                }

                AnalyticsCard {
                    Layout.fillWidth: true
                    compactLayout: root.compactLayout
                    title: qsTr("月结余趋势")
                    description: qsTr("结余低于零时折线会进入零线下方。")

                    LineSeriesChart {
                        objectName: "balanceLineChart"
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.chartHeight
                        seriesData: root.trendData
                        seriesId: "balance"
                    }
                }
            }

            GridLayout {
                objectName: "analyticsCategoryGrid"
                Layout.fillWidth: true
                columns: root.width >= AppTheme.breakpointWide ? 2 : 1
                columnSpacing: AppTheme.spacingMedium
                rowSpacing: AppTheme.spacingMedium

                AnalyticsCard {
                    Layout.fillWidth: true
                    compactLayout: root.compactLayout
                    title: qsTr("支出分类")
                    description: root.viewModel ? qsTr("%1 的支出去向").arg(root.viewModel.monthLabel) : ""

                    AppComboBox {
                        objectName: "analyticsExpenseChartSelector"
                        Layout.fillWidth: root.compactLayout
                        Layout.alignment: root.compactLayout
                            ? Qt.AlignLeft : Qt.AlignRight
                        Layout.preferredWidth: root.compactLayout ? -1 : 160
                        model: [qsTr("横向排行"), qsTr("环形图")]
                        currentIndex: root.expenseChartMode
                        onActivated: root.expenseChartMode = currentIndex
                    }

                    StackLayout {
                        objectName: "expenseChartStack"
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.expenseChartMode === 0
                            ? Math.max(150, expenseRanking.implicitHeight)
                            : expenseDonut.implicitHeight
                        currentIndex: root.expenseChartMode

                        RankingBars {
                            id: expenseRanking
                            objectName: "expenseRankingBars"

                            model: root.viewModel ? root.viewModel.expenseCategories : null
                            barColor: AppTheme.errorColor
                            compactLayout: root.compactLayout
                            emptyText: qsTr("本月暂无支出分类数据")
                        }

                        DonutBreakdown {
                            id: expenseDonut
                            objectName: "expenseDonutChart"
                            model: root.viewModel ? root.viewModel.expenseCategories : null
                            compactLayout: root.compactLayout
                            emptyText: qsTr("本月暂无支出分类数据")
                        }
                    }
                }

                AnalyticsCard {
                    Layout.fillWidth: true
                    compactLayout: root.compactLayout
                    title: qsTr("收入分类")
                    description: root.viewModel ? qsTr("%1 的收入来源").arg(root.viewModel.monthLabel) : ""

                    AppComboBox {
                        objectName: "analyticsIncomeChartSelector"
                        Layout.fillWidth: root.compactLayout
                        Layout.alignment: root.compactLayout
                            ? Qt.AlignLeft : Qt.AlignRight
                        Layout.preferredWidth: root.compactLayout ? -1 : 160
                        model: [qsTr("横向排行"), qsTr("环形图")]
                        currentIndex: root.incomeChartMode
                        onActivated: root.incomeChartMode = currentIndex
                    }

                    StackLayout {
                        objectName: "incomeChartStack"
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.incomeChartMode === 0
                            ? Math.max(150, incomeRanking.implicitHeight)
                            : incomeDonut.implicitHeight
                        currentIndex: root.incomeChartMode

                        RankingBars {
                            id: incomeRanking
                            objectName: "incomeRankingBars"

                            model: root.viewModel ? root.viewModel.incomeCategories : null
                            barColor: AppTheme.successColor
                            compactLayout: root.compactLayout
                            emptyText: qsTr("本月暂无收入分类数据")
                        }

                        DonutBreakdown {
                            id: incomeDonut
                            objectName: "incomeDonutChart"
                            model: root.viewModel ? root.viewModel.incomeCategories : null
                            compactLayout: root.compactLayout
                            emptyText: qsTr("本月暂无收入分类数据")
                        }
                    }
                }
            }

            AnalyticsCard {
                Layout.fillWidth: true
                compactLayout: root.compactLayout
                title: qsTr("账户资金流")
                description: qsTr("查看所选月份每个账户的收入、支出和净流入。")

                Repeater {
                    id: accountRows

                    model: root.viewModel ? root.viewModel.accountFlows : null

                    delegate: GridLayout {
                        id: accountRow
                        objectName: "accountFlowRow"

                        required property string name
                        required property string incomeText
                        required property string expenseText
                        required property string netFlowText
                        required property int transactionCount
                        required property bool archived

                        Layout.fillWidth: true
                        columns: root.accountFlowColumns
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
                            color: AppTheme.successColor
                        }

                        Label {
                            text: qsTr("支出 %1").arg(accountRow.expenseText)
                            color: AppTheme.errorColor
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
                Layout.bottomMargin: root.pagePadding
                text: qsTr("图表组件只依赖通用数据契约，未来饮食、学习模块也可以复用。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            Item {
                Layout.preferredHeight: root.pagePadding
            }
        }
    }
}
