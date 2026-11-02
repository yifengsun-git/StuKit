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
    property var accountingViewModel: null
    property var scheduleViewModel: null
    property var tasksViewModel: null

    readonly property bool compactLayout: width < AppTheme.breakpointCompact
    readonly property int featureColumns: width < 560 ? 1 : 2
    readonly property int pagePadding: AppTheme.pagePadding(width)
    readonly property var todayAccounting: {
        if (!accountingViewModel || !accountingViewModel.calendarDays)
            return null
        const days = accountingViewModel.calendarDays
        for (let index = 0; index < days.length; ++index) {
            if (days[index].isToday)
                return days[index]
        }
        return null
    }
    readonly property var nextTask: {
        if (!tasksViewModel || !tasksViewModel.tasks)
            return null
        const tasks = tasksViewModel.tasks
        for (let index = 0; index < tasks.length; ++index) {
            if (!tasks[index].completed && !tasks[index].archived)
                return tasks[index]
        }
        return null
    }
    readonly property int todayCourseCount: {
        if (!scheduleViewModel || !scheduleViewModel.meetings
                || !scheduleViewModel.meetings.weekdayCounts)
            return 0
        const day = new Date().getDay()
        const index = day === 0 ? 6 : day - 1
        return Number(scheduleViewModel.meetings.weekdayCounts[index] || 0)
    }

    function activateSnapshot(action: string) {
        switch (action) {
        case "schedule":
            root.scheduleRequested()
            break
        case "tasks":
            root.tasksRequested()
            break
        case "accounting":
            root.accountingRequested()
            break
        }
    }

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
            spacing: AppTheme.spacingMedium

            Item {
                Layout.preferredHeight: AppTheme.spacingSmall
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
                text: qsTr("今天也把学习和生活安排得井井有条。")
                color: AppTheme.textSecondaryColor
                font.pixelSize: AppTheme.fontSizeSubtitle
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                visible: !root.compactLayout
                text: qsTr("今日概览")
                color: AppTheme.textPrimaryColor
                font.pixelSize: AppTheme.fontSizeTitle
                font.bold: true
            }

            Rectangle {
                objectName: "homeTodaySnapshot"
                visible: !root.compactLayout
                Layout.fillWidth: true
                Layout.preferredHeight: snapshotGrid.implicitHeight
                    + AppTheme.spacingLarge * 2
                radius: AppTheme.radiusLarge
                color: AppTheme.surfaceRaisedColor
                border.color: AppTheme.borderColor

                GridLayout {
                    id: snapshotGrid
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingLarge
                    columns: root.width >= 900 ? 3 : 1
                    columnSpacing: AppTheme.spacingMedium
                    rowSpacing: AppTheme.spacingMedium

                    Repeater {
                        model: [
                            {
                                action: "schedule",
                                eyebrow: qsTr("今日课程"),
                                value: root.todayCourseCount > 0
                                    ? qsTr("%1 节").arg(root.todayCourseCount)
                                    : qsTr("暂无课程"),
                                detail: root.scheduleViewModel
                                    && root.scheduleViewModel.hasTerms
                                    ? root.scheduleViewModel.weekLabel
                                    : qsTr("还没有设置学期"),
                                accent: AppTheme.successColor
                            },
                            {
                                action: "tasks",
                                eyebrow: qsTr("今日待办"),
                                value: root.tasksViewModel
                                    ? qsTr("%1 项待完成").arg(
                                        root.tasksViewModel.pendingCount)
                                    : qsTr("暂无任务"),
                                detail: root.nextTask
                                    ? root.nextTask.title
                                    : qsTr("今天可以轻松一点"),
                                accent: AppTheme.warningColor
                            },
                            {
                                action: "accounting",
                                eyebrow: qsTr("今日收支"),
                                value: root.todayAccounting
                                    ? qsTr("支出 %1").arg(
                                        root.todayAccounting.expenseText)
                                    : qsTr("暂无流水"),
                                detail: root.todayAccounting
                                    ? qsTr("收入 %1 · %2 笔").arg(
                                        root.todayAccounting.incomeText).arg(
                                        root.todayAccounting.transactionCount)
                                    : qsTr("记下今天的第一笔"),
                                accent: AppTheme.primaryColor
                            }
                        ]

                        delegate: Rectangle {
                            id: snapshotCard
                            required property var modelData
                            required property int index

                            function activate() {
                                root.activateSnapshot(modelData.action)
                            }

                            objectName: "homeSnapshotCard_"
                                + snapshotCard.modelData.action
                            Layout.fillWidth: true
                            Layout.preferredHeight: 112
                            radius: AppTheme.radiusMedium
                            color: snapshotHover.hovered
                                ? AppTheme.surfaceHoverColor
                                : AppTheme.surfaceColor
                            border.color: snapshotHover.hovered
                                ? snapshotCard.modelData.accent
                                : AppTheme.borderColor

                            Behavior on color {
                                ColorAnimation { duration: 100 }
                            }

                            HoverHandler {
                                id: snapshotHover
                                enabled: Qt.platform.os !== "android"
                                cursorShape: Qt.PointingHandCursor
                            }

                            TapHandler {
                                onTapped: snapshotCard.activate()
                            }

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: AppTheme.spacingMedium
                                spacing: AppTheme.spacingExtraSmall

                                Rectangle {
                                    Layout.preferredWidth: 32
                                    Layout.preferredHeight: 4
                                    radius: 2
                                    color: snapshotCard.modelData.accent
                                }

                                Label {
                                    text: snapshotCard.modelData.eyebrow
                                    color: AppTheme.textSecondaryColor
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: snapshotCard.modelData.value
                                    color: AppTheme.textPrimaryColor
                                    font.pixelSize: AppTheme.fontSizeTitle
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: snapshotCard.modelData.detail
                                    color: AppTheme.textSecondaryColor
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                text: root.compactLayout ? qsTr("常用工具") : qsTr("我的工具")
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
                    Layout.columnSpan: root.compactLayout ? 1 : 2
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: root.compactLayout ? 148 : 136
                    title: qsTr("记账本")
                    description: qsTr("记录收支、管理账户，并查看消费趋势。")
                    accentColor: AppTheme.primaryColor
                    onClicked: root.accountingRequested()
                }

                FeatureCard {
                    objectName: "scheduleFeatureCard"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: root.compactLayout ? 148 : 176
                    title: qsTr("课表")
                    description: qsTr("按周查看课程，管理学期与课程资料。")
                    accentColor: AppTheme.successColor
                    onClicked: root.scheduleRequested()
                }

                FeatureCard {
                    objectName: "tasksFeatureCard"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: root.compactLayout ? 148 : 176
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
