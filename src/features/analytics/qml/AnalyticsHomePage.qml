import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

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
                text: qsTr("集中查看默认洞察，也可以按时间、账户、分类和指标创建自定义分析。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                columns: root.width >= 760 ? 2 : 1
                columnSpacing: AppTheme.spacingMedium
                rowSpacing: AppTheme.spacingMedium

                Repeater {
                    model: [
                        {
                            title: qsTr("默认记账分析"),
                            description: qsTr("收支趋势、上月对比、分类排行和账户资金流。"),
                            detail: qsTr("柱状图 · 折线图 · 排行图")
                        },
                        {
                            title: qsTr("自定义分析"),
                            description: qsTr("选择月份范围、账户、分类、指标和图表形式。"),
                            detail: qsTr("将在后续课程接入可交互图表")
                        }
                    ]

                    delegate: Rectangle {
                        id: analysisCard

                        required property var modelData

                        Layout.fillWidth: true
                        Layout.preferredHeight: cardLayout.implicitHeight
                            + AppTheme.spacingLarge * 2
                        radius: AppTheme.radiusMedium
                        color: AppTheme.surfaceColor
                        border.color: AppTheme.borderColor

                        ColumnLayout {
                            id: cardLayout

                            anchors.fill: parent
                            anchors.margins: AppTheme.spacingLarge
                            spacing: AppTheme.spacingSmall

                            Label {
                                Layout.fillWidth: true
                                text: analysisCard.modelData.title
                                color: AppTheme.textPrimaryColor
                                font.pixelSize: AppTheme.fontSizeHeading
                                font.bold: true
                            }

                            Label {
                                Layout.fillWidth: true
                                text: analysisCard.modelData.description
                                color: AppTheme.textSecondaryColor
                                wrapMode: Text.WordWrap
                            }

                            Label {
                                Layout.fillWidth: true
                                text: analysisCard.modelData.detail
                                color: AppTheme.primaryColor
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.margins: AppTheme.spacingLarge
                text: qsTr("分析外壳和图表组件将保持通用，未来饮食、学习等模块可以在这里增加自己的数据源。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }
        }
    }
}
