pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel
    required property var managementViewModel
    required property string startupError

    readonly property bool compactLayout:
        width < AppTheme.breakpointCompact
    readonly property int pagePadding: AppTheme.pagePadding(width)
    readonly property string layoutMode: editingSchedule
        ? "editor"
        : compactLayout ? "daily" : "grid"
    property bool editingSchedule: false
    property int schedulePeriodCount: 12
    property string detailCourseName: ""
    property string detailTeacher: ""
    property string detailLocation: ""
    property string detailWeekday: ""
    property string detailPeriod: ""
    property color detailColor: AppTheme.primaryColor

    readonly property string activeError: startupError.length > 0
        ? startupError
        : viewModel && viewModel.hasError
            ? viewModel.errorMessage
            : ""

    objectName: "scheduleHomePage"

    function showCourseDetails(courseName, teacher, location,
                               weekdayText, periodText, courseColor) {
        detailCourseName = courseName
        detailTeacher = teacher
        detailLocation = location
        detailWeekday = weekdayText
        detailPeriod = periodText
        detailColor = courseColor
        courseDetailDialog.open()
    }

    function selectTerm(termId) {
        if (root.viewModel)
            root.viewModel.selectTerm(termId)
        if (root.managementViewModel)
            root.managementViewModel.selectTerm(termId)
    }

    function toggleScheduleEditing() {
        if (root.editingSchedule) {
            root.editingSchedule = false
            return
        }
        if (!root.managementViewModel || !root.viewModel)
            return
        root.managementViewModel.selectTerm(root.viewModel.selectedTermId)
        if (root.managementViewModel.selectedTermArchived)
            return
        root.editingSchedule = true
    }

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    ScrollView {
        id: pageScrollView

        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            x: Math.max(0, (pageScrollView.availableWidth - width) / 2)
            width: Math.min(
                pageScrollView.availableWidth,
                AppTheme.contentMaxWidth)
            spacing: AppTheme.spacingMedium

            AppSectionCard {
                objectName: "schedulePrimaryCard"
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                Layout.topMargin: root.pagePadding
                title: qsTr("课程表")
                description: root.editingSchedule
                    ? qsTr("正在编辑学期排课，完成后请退出编辑模式。")
                    : qsTr("选择学期查看课表，或进入编辑模式调整排课。")
                accentColor: root.editingSchedule
                    ? AppTheme.warningColor : AppTheme.primaryColor

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.compactLayout ? 1 : 3
                    rowSpacing: AppTheme.spacingSmall
                    columnSpacing: AppTheme.spacingSmall

                    AppComboBox {
                        id: termSelector

                        Layout.fillWidth: true
                        Layout.maximumWidth: root.compactLayout ? 1000 : 360
                        model: root.viewModel ? root.viewModel.termOptions : []
                        textRole: "label"
                        valueRole: "id"
                        enabled: root.viewModel && root.viewModel.hasTerms
                        onActivated: root.selectTerm(currentValue)

                        function syncSelection() {
                            if (!root.viewModel) {
                                currentIndex = -1
                                return
                            }
                            const index = indexOfValue(
                                root.viewModel.selectedTermId)
                            currentIndex = index >= 0 ? index : -1
                        }

                        Component.onCompleted: syncSelection()

                        Connections {
                            target: root.viewModel
                            function onTermsChanged() {
                                termSelector.syncSelection()
                            }
                            function onSelectedTermChanged() {
                                termSelector.syncSelection()
                            }
                        }
                    }

                    AppButton {
                        Layout.fillWidth: root.compactLayout
                        text: qsTr("学期与课程资料")
                        enabled: root.managementViewModel !== null
                        onClicked: managementDialog.open()
                    }

                    AppButton {
                        objectName: "scheduleEditModeButton"
                        Layout.fillWidth: root.compactLayout
                        text: root.editingSchedule
                            ? qsTr("完成编辑")
                            : qsTr("编辑课表")
                        highlighted: root.editingSchedule
                        enabled: root.viewModel
                            && root.viewModel.hasTerms
                            && root.managementViewModel !== null
                            && (!root.managementViewModel.selectedTermArchived
                                || root.editingSchedule)
                        onClicked: root.toggleScheduleEditing()
                    }
                }
            }

            AppSectionCard {
                objectName: "scheduleWeekNavigationCard"
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                title: root.viewModel
                    ? root.viewModel.weekLabel : qsTr("课表")
                description: root.viewModel
                    ? root.viewModel.weekDateRange : ""

                RowLayout {
                    Layout.fillWidth: true

                    AppButton {
                        objectName: "schedulePreviousWeekButton"
                        Layout.fillWidth: true
                        text: qsTr("上一周")
                        enabled: root.viewModel
                            && root.viewModel.canShowPreviousWeek
                        onClicked: root.viewModel.showPreviousWeek()
                    }

                    AppButton {
                        objectName: "scheduleCurrentWeekButton"
                        Layout.fillWidth: true
                        text: qsTr("当前周")
                        visible: root.viewModel
                            && root.viewModel.currentWeekAvailable
                        enabled: root.viewModel
                            && !root.viewModel.showingCurrentWeek
                        onClicked: root.viewModel.showCurrentWeek()
                    }

                    AppButton {
                        objectName: "scheduleNextWeekButton"
                        Layout.fillWidth: true
                        text: qsTr("下一周")
                        enabled: root.viewModel
                            && root.viewModel.canShowNextWeek
                        onClicked: root.viewModel.showNextWeek()
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    visible: root.editingSchedule
                        && root.viewModel
                        && root.viewModel.hasTerms
                    rowSpacing: AppTheme.spacingSmall
                    columnSpacing: AppTheme.spacingSmall
                    columns: root.compactLayout ? 2 : 3

                    Label {
                        Layout.columnSpan: root.compactLayout ? 2 : 1
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("当前显示 %1 节").arg(
                            root.schedulePeriodCount)
                        color: AppTheme.textSecondaryColor
                    }

                    AppButton {
                        objectName: "scheduleDecreasePeriodsButton"
                        Layout.fillWidth: root.compactLayout
                        text: qsTr("− 减少一节")
                        enabled: root.schedulePeriodCount > 1
                        onClicked: root.schedulePeriodCount = Math.max(
                            1, root.schedulePeriodCount - 1)
                    }

                    AppButton {
                        objectName: "scheduleIncreasePeriodsButton"
                        Layout.fillWidth: root.compactLayout
                        text: qsTr("+ 增加一节")
                        enabled: root.schedulePeriodCount < 20
                        onClicked: root.schedulePeriodCount = Math.min(
                            20, root.schedulePeriodCount + 1)
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                Layout.preferredHeight: errorLabel.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.errorColor
                visible: root.activeError.length > 0

                Label {
                    id: errorLabel
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    color: AppTheme.errorColor
                    text: root.activeError
                    wrapMode: Text.WordWrap
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                Layout.preferredHeight: noTermContent.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor
                visible: root.viewModel
                    && !root.viewModel.hasTerms
                    && root.activeError.length === 0

                ColumnLayout {
                    id: noTermContent
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    spacing: AppTheme.spacingExtraSmall

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("还没有学期")
                        color: AppTheme.textPrimaryColor
                        font.bold: true
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("点击“管理学期与课程”开始设置")
                        color: AppTheme.textSecondaryColor
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                Layout.preferredHeight: weekStatusLabel.implicitHeight
                    + AppTheme.spacingSmall * 2
                radius: AppTheme.radiusSmall
                color: AppTheme.surfaceVariantColor
                border.color: AppTheme.borderColor
                visible: root.viewModel
                    && root.viewModel.hasTerms
                    && root.activeError.length === 0
                    && (root.viewModel.empty
                        || !root.viewModel.showingCurrentWeek)

                Label {
                    id: weekStatusLabel
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingSmall
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: AppTheme.textSecondaryColor
                    wrapMode: Text.WordWrap
                    text: root.viewModel && root.viewModel.empty
                        ? qsTr("这一周还没有课程")
                        : root.viewModel && root.viewModel.currentWeekAvailable
                            ? qsTr("你正在查看非当前教学周")
                            : qsTr("当前日期不在所选学期范围内")
                }
            }

            Loader {
                id: scheduleLayoutLoader

                property Item loadedLayout: item as Item

                objectName: "scheduleLayoutLoader"
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                Layout.bottomMargin: root.pagePadding
                Layout.preferredHeight: loadedLayout
                    ? loadedLayout.implicitHeight : 0
                visible: root.viewModel
                    && root.viewModel.hasTerms
                    && root.activeError.length === 0
                sourceComponent: root.editingSchedule
                    ? scheduleEditorComponent
                    : root.compactLayout
                        ? dailyScheduleComponent
                        : weeklyGridComponent
            }
        }
    }

    Component {
        id: weeklyGridComponent

        Item {
            implicitHeight: weeklyGrid.implicitHeight

            Flickable {
                id: gridFlickable
                anchors.fill: parent
                contentWidth: Math.max(width, weeklyGrid.implicitWidth)
                contentHeight: weeklyGrid.implicitHeight
                flickableDirection: Flickable.HorizontalFlick
                boundsBehavior: Flickable.StopAtBounds
                clip: true

                WeeklyGrid {
                    id: weeklyGrid
                    width: gridFlickable.contentWidth
                    model: root.viewModel ? root.viewModel.meetings : null
                    weekDays: root.viewModel ? root.viewModel.weekDays : []
                    periodCount: root.schedulePeriodCount
                    periodDefinitions: root.managementViewModel
                        ? root.managementViewModel.periodDefinitions : []
                    onCourseActivated: details =>
                        root.showCourseDetails(
                            details.courseName,
                            details.teacher,
                            details.location,
                            details.weekdayText,
                            details.periodText,
                            details.courseColor)
                }

                ScrollBar.horizontal: ScrollBar {
                    policy: gridFlickable.contentWidth > gridFlickable.width
                        ? ScrollBar.AsNeeded
                        : ScrollBar.AlwaysOff
                }
            }
        }
    }

    Component {
        id: dailyScheduleComponent

        DayScheduleList {
            model: root.viewModel ? root.viewModel.meetings : null
            weekDays: root.viewModel ? root.viewModel.weekDays : []
            onCourseActivated: details =>
                root.showCourseDetails(
                    details.courseName,
                    details.teacher,
                    details.location,
                    details.weekdayText,
                    details.periodText,
                    details.courseColor)
        }
    }

    Component {
        id: scheduleEditorComponent

        ScheduleGridEditor {
            viewModel: root.managementViewModel
            scheduleModel: root.viewModel ? root.viewModel.meetings : null
            weekDays: root.viewModel ? root.viewModel.weekDays : []
            compactLayout: root.compactLayout
            periodCount: root.schedulePeriodCount
            periodDefinitions: root.managementViewModel
                ? root.managementViewModel.periodDefinitions : []
            viewportWidth: scheduleLayoutLoader.width
            viewportHeight: root.height
            onRequestManageCourses: managementDialog.open()
        }
    }

    AppDialog {
        id: managementDialog
        objectName: "scheduleManagementDialog"

        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        width: Math.min(root.width - AppTheme.spacingMedium * 2, 840)
        height: Math.min(root.height - AppTheme.spacingMedium * 2, 600)
        title: qsTr("学期与课程资料")
        standardButtons: Dialog.Close
        onClosed: {
            if (root.managementViewModel
                    && root.managementViewModel.selectedTermId.length > 0) {
                root.selectTerm(root.managementViewModel.selectedTermId)
            }
        }

        Loader {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(
                120, managementDialog.height - 150)
            active: managementDialog.visible
                && root.managementViewModel !== null
            sourceComponent: Component {
                ScheduleManagementPanel {
                    viewModel: root.managementViewModel
                }
            }
        }
    }

    AppDialog {
        id: courseDetailDialog

        objectName: "courseDetailDialog"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        width: Math.min(root.width - AppTheme.spacingMedium * 2, 420)
        title: root.detailCourseName
        standardButtons: Dialog.Close

        ColumnLayout {
            width: parent.width
            spacing: AppTheme.spacingMedium

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 8
                radius: 4
                color: root.detailColor
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("%1 · %2")
                    .arg(root.detailWeekday)
                    .arg(root.detailPeriod)
                color: AppTheme.textPrimaryColor
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                visible: root.detailTeacher.length > 0
                text: qsTr("教师：%1").arg(root.detailTeacher)
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                visible: root.detailLocation.length > 0
                text: qsTr("地点：%1").arg(root.detailLocation)
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }
        }
    }
}
