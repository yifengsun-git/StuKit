pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var days: []
    property string selectedDate: ""
    property bool compactLayout: false

    signal dateSelected(string dateText)

    objectName: "accountingCalendar"
    implicitHeight: calendarColumn.implicitHeight

    ColumnLayout {
        id: calendarColumn
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: AppTheme.spacingExtraSmall

        GridLayout {
            Layout.fillWidth: true
            columns: 7
            columnSpacing: AppTheme.spacingExtraSmall

            Repeater {
                model: [qsTr("一"), qsTr("二"), qsTr("三"), qsTr("四"),
                        qsTr("五"), qsTr("六"), qsTr("日")]

                Label {
                    required property string modelData
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: modelData
                    color: AppTheme.textSecondaryColor
                    font.bold: true
                }
            }
        }

        GridLayout {
            id: dayGrid
            Layout.fillWidth: true
            columns: 7
            rowSpacing: AppTheme.spacingExtraSmall
            columnSpacing: AppTheme.spacingExtraSmall

            Repeater {
                model: root.days

                Rectangle {
                    id: dayCell
                    required property var modelData

                    readonly property bool selected:
                        root.selectedDate === modelData.dateText

                    Layout.fillWidth: true
                    Layout.preferredHeight: root.compactLayout ? 72 : 92
                    radius: AppTheme.radiusSmall
                    color: selected
                        ? AppTheme.surfaceVariantColor
                        : modelData.inCurrentMonth
                            ? AppTheme.surfaceColor : "transparent"
                    border.width: selected || modelData.isToday ? 2 : 1
                    border.color: selected
                        ? AppTheme.primaryColor
                        : modelData.isToday
                            ? AppTheme.warningColor : AppTheme.borderColor
                    opacity: modelData.inCurrentMonth ? 1.0 : 0.35

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.compactLayout ? 3 : 6
                        spacing: 1

                        Label {
                            Layout.fillWidth: true
                            text: dayCell.modelData.day
                            color: AppTheme.textPrimaryColor
                            font.bold: dayCell.selected
                                || dayCell.modelData.isToday
                            horizontalAlignment: Text.AlignRight
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: dayCell.modelData.transactionCount > 0
                            text: "+" + dayCell.modelData.incomeText
                            color: AppTheme.primaryColor
                            font.pixelSize: root.compactLayout
                                ? AppTheme.fontSizeCaption - 1
                                : AppTheme.fontSizeCaption
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: dayCell.modelData.transactionCount > 0
                            text: "-" + dayCell.modelData.expenseText
                            color: AppTheme.errorColor
                            font.pixelSize: root.compactLayout
                                ? AppTheme.fontSizeCaption - 1
                                : AppTheme.fontSizeCaption
                            elide: Text.ElideRight
                        }

                        Item { Layout.fillHeight: true }

                        Label {
                            Layout.fillWidth: true
                            visible: dayCell.modelData.transactionCount > 0
                                && !root.compactLayout
                            text: qsTr("%1 笔").arg(
                                dayCell.modelData.transactionCount)
                            color: AppTheme.textSecondaryColor
                            font.pixelSize: AppTheme.fontSizeCaption
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: dayCell.modelData.inCurrentMonth
                        cursorShape: enabled ? Qt.PointingHandCursor
                                             : Qt.ArrowCursor
                        onClicked: root.dateSelected(dayCell.modelData.dateText)
                    }
                }
            }
        }
    }
}
