import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    signal accountingRequested()
    signal scheduleRequested()
    signal weatherRequested()
    signal tasksRequested()

    property bool weatherFeatureVisible: false

    readonly property bool compactLayout: width < AppTheme.breakpointCompact
    readonly property int featureColumns: width < 560 ? 1 : 2
    readonly property int pagePadding: AppTheme.pagePadding(width)

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    ScrollView {
        id: scrollView

        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            id: pageContent

            x: Math.max(root.pagePadding,
                (scrollView.availableWidth - width) / 2)
            width: Math.max(0, Math.min(
                scrollView.availableWidth - root.pagePadding * 2,
                AppTheme.contentMaxWidth))
            spacing: AppTheme.spacingLarge

            Item {
                Layout.preferredHeight: root.compactLayout
                    ? AppTheme.spacingSmall : AppTheme.spacingLarge
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("欢迎使用 StuKit")
                color: AppTheme.textPrimaryColor
                font.pixelSize: root.compactLayout
                    ? AppTheme.fontSizeHeading : AppTheme.fontSizeDisplay
                font.bold: true
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("把学习、生活和个人数据整理在同一个地方。")
                color: AppTheme.textSecondaryColor
                font.pixelSize: AppTheme.fontSizeSubtitle
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: AppTheme.borderColor
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("常用工具")
                color: AppTheme.textPrimaryColor
                font.pixelSize: AppTheme.fontSizeTitle
                font.bold: true
            }

            GridLayout {
                objectName: "homeFeatureGrid"
                Layout.fillWidth: true
                columns: root.featureColumns
                rowSpacing: AppTheme.spacingMedium
                columnSpacing: AppTheme.spacingMedium

                FeatureCard {
                    objectName: "accountingFeatureCard"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    title: qsTr("记账本")
                    description: qsTr("记录收支、管理账户，并查看消费趋势。")
                    accentColor: AppTheme.primaryColor
                    onClicked: root.accountingRequested()
                }

                FeatureCard {
                    objectName: "scheduleFeatureCard"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    title: qsTr("课表")
                    description: qsTr("按周查看课程，管理学期与课程资料。")
                    accentColor: AppTheme.successColor
                    onClicked: root.scheduleRequested()
                }

                FeatureCard {
                    objectName: "tasksFeatureCard"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    title: qsTr("待办")
                    description: qsTr("安排作业、考试和日常任务，不遗漏截止时间。")
                    accentColor: AppTheme.warningColor
                    onClicked: root.tasksRequested()
                }

                FeatureCard {
                    objectName: "weatherFeatureCard"
                    visible: root.weatherFeatureVisible
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    title: qsTr("天气")
                    description: qsTr("查看当前天气与近期预报，为出行做准备。")
                    accentColor: "#0891B2"
                    onClicked: root.weatherRequested()
                }
            }

            Item {
                Layout.preferredHeight: AppTheme.spacingLarge
            }
        }
    }
}
