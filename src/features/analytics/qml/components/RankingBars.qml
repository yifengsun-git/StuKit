pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var model: null
    property color barColor: AppTheme.primaryColor
    property int maximumItems: 8
    property string emptyText: qsTr("本月暂无分类数据")

    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content

        anchors.left: parent.left
        anchors.right: parent.right
        spacing: AppTheme.spacingMedium

        Repeater {
            id: rows

            model: root.model

            delegate: ColumnLayout {
                id: rankingRow

                required property int index
                required property string name
                required property string amountText
                required property double percentage
                required property string percentageText
                required property int transactionCount
                required property bool archived

                Layout.fillWidth: true
                visible: rankingRow.index < root.maximumItems
                spacing: AppTheme.spacingExtraSmall

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        text: rankingRow.archived ? qsTr("%1（已归档）").arg(rankingRow.name) : rankingRow.name
                        color: AppTheme.textPrimaryColor
                        elide: Text.ElideRight
                    }

                    Label {
                        text: qsTr("%1 · %2 · %3 笔").arg(rankingRow.amountText).arg(rankingRow.percentageText).arg(rankingRow.transactionCount)
                        color: AppTheme.textSecondaryColor
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 9
                    radius: height / 2
                    color: AppTheme.surfaceVariantColor

                    Rectangle {
                        width: parent.width * Math.min(1, Math.max(0, rankingRow.percentage / 100))
                        height: parent.height
                        radius: height / 2
                        color: root.barColor
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: rows.count === 0
            text: root.emptyText
            color: AppTheme.textSecondaryColor
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
