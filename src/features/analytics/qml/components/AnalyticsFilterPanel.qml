pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var viewModel
    property bool expanded: true
    property bool compactLayout: false
    readonly property int contentPadding: compactLayout
        ? AppTheme.spacingMedium : AppTheme.spacingLarge

    implicitHeight: panel.implicitHeight + contentPadding * 2
    radius: AppTheme.radiusLarge
    color: AppTheme.surfaceColor
    border.color: AppTheme.borderColor

    ColumnLayout {
        id: panel

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: root.contentPadding
        spacing: AppTheme.spacingMedium

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: AppTheme.spacingExtraSmall

                Label {
                    text: qsTr("分析条件")
                    color: AppTheme.textPrimaryColor
                    font.pixelSize: AppTheme.fontSizeSubtitle
                    font.bold: true
                }

                Label {
                    objectName: "analyticsFilterSummary"
                    Layout.fillWidth: true
                    text: root.viewModel ? qsTr("%1 · 账户 %2 项 · 分类 %3 项 · %4").arg(root.viewModel.monthLabel).arg(root.viewModel.selectedAccountIds.length === 0 ? qsTr("不限") : root.viewModel.selectedAccountIds.length).arg(root.viewModel.selectedCategoryIds.length === 0 ? qsTr("不限") : root.viewModel.selectedCategoryIds.length).arg(root.viewModel.timeZoneId) : ""
                    color: AppTheme.textSecondaryColor
                    wrapMode: Text.WordWrap
                }
            }

            AppButton {
                objectName: "analyticsFilterToggle"
                Layout.minimumHeight: AppTheme.controlHeight
                text: root.expanded ? qsTr("收起") : qsTr("展开")
                onClicked: root.expanded = !root.expanded
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            enabled: root.viewModel !== null
            visible: root.expanded
            spacing: AppTheme.spacingMedium

            Flow {
                Layout.fillWidth: true
                spacing: AppTheme.spacingSmall

                AppButton {
                    height: AppTheme.touchTargetHeight
                    text: qsTr("上一月")
                    onClicked: root.viewModel.showPreviousMonth()
                }

                Label {
                    width: 90
                    height: AppTheme.touchTargetHeight
                    text: root.viewModel ? root.viewModel.monthLabel : "--"
                    color: AppTheme.textPrimaryColor
                    font.bold: true
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }

                AppButton {
                    height: AppTheme.touchTargetHeight
                    text: qsTr("下一月")
                    onClicked: root.viewModel.showNextMonth()
                }

                AppComboBox {
                    objectName: "analyticsRangeBox"
                    width: root.compactLayout ? 150 : 160
                    textRole: "label"
                    valueRole: "value"
                    model: [
                        {
                            label: qsTr("近 6 个月"),
                            value: 6
                        },
                        {
                            label: qsTr("近 12 个月"),
                            value: 12
                        },
                        {
                            label: qsTr("近 24 个月"),
                            value: 24
                        }
                    ]
                    currentIndex: {
                        if (!root.viewModel || root.viewModel.monthCount === 12)
                            return 1;
                        return root.viewModel.monthCount === 6 ? 0 : 2;
                    }
                    onActivated: root.viewModel.monthCount = currentValue
                }

                AppButton {
                    height: AppTheme.touchTargetHeight
                    text: qsTr("恢复默认")
                    onClicked: root.viewModel.resetFilters()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: AppTheme.borderColor
            }

            RowLayout {
                Layout.fillWidth: true

                Label {
                    Layout.fillWidth: true
                    text: qsTr("账户")
                    color: AppTheme.textPrimaryColor
                    font.bold: true
                }

                AppButton {
                    Layout.minimumHeight: AppTheme.controlHeight
                    text: qsTr("全选")
                    flat: true
                    onClicked: root.viewModel.selectAllAccounts()
                }

                AppButton {
                    Layout.minimumHeight: AppTheme.controlHeight
                    text: qsTr("不限")
                    flat: true
                    onClicked: root.viewModel.clearAccountSelection()
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: AppTheme.spacingSmall

                Repeater {
                    model: root.viewModel ? root.viewModel.accountFilterOptions : []

                    delegate: CheckBox {
                        id: accountOption

                        required property var modelData

                        height: AppTheme.touchTargetHeight
                        text: accountOption.modelData.archived ? qsTr("%1（已归档）").arg(accountOption.modelData.label) : accountOption.modelData.label
                        checked: accountOption.modelData.selected
                        onToggled: root.viewModel.setAccountSelected(accountOption.modelData.value, checked)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Label {
                    Layout.fillWidth: true
                    text: qsTr("分类")
                    color: AppTheme.textPrimaryColor
                    font.bold: true
                }

                AppButton {
                    Layout.minimumHeight: AppTheme.controlHeight
                    text: qsTr("全选")
                    flat: true
                    onClicked: root.viewModel.selectAllCategories()
                }

                AppButton {
                    Layout.minimumHeight: AppTheme.controlHeight
                    text: qsTr("不限")
                    flat: true
                    onClicked: root.viewModel.clearCategorySelection()
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: AppTheme.spacingSmall

                Repeater {
                    model: root.viewModel ? root.viewModel.categoryFilterOptions : []

                    delegate: CheckBox {
                        id: categoryOption

                        required property var modelData

                        height: AppTheme.touchTargetHeight
                        text: qsTr("%1 · %2%3").arg(categoryOption.modelData.isIncome ? qsTr("收入") : qsTr("支出")).arg(categoryOption.modelData.label).arg(categoryOption.modelData.archived ? qsTr("（已归档）") : "")
                        checked: categoryOption.modelData.selected
                        onToggled: root.viewModel.setCategorySelected(categoryOption.modelData.value, checked)
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("“不限”表示不按该项过滤；未勾选任何项目时仍统计全部数据。")
                color: AppTheme.textSecondaryColor
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
        }
    }
}
