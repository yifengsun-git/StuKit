pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel
    required property var managementViewModel
    required property string startupError

    readonly property bool compactLayout: width <= 640
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
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: root.width
            spacing: AppTheme.spacingMedium

            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                Layout.topMargin: AppTheme.spacingLarge
                columns: root.compactLayout ? 1 : 3
                rowSpacing: AppTheme.spacingSmall
                columnSpacing: AppTheme.spacingSmall

                ComboBox {
                    id: termSelector

                    Layout.fillWidth: true
                    Layout.maximumWidth: 360
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
                        const index = indexOfValue(root.viewModel.selectedTermId)
                        currentIndex = index >= 0 ? index : -1
                    }

                    Component.onCompleted: syncSelection()

                    Connections {
                        target: root.viewModel
                        function onTermsChanged() { termSelector.syncSelection() }
                        function onSelectedTermChanged() { termSelector.syncSelection() }
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

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                AppButton {
                    objectName: "schedulePreviousWeekButton"
                    text: qsTr("上一周")
                    enabled: root.viewModel && root.viewModel.canShowPreviousWeek
                    onClicked: root.viewModel.showPreviousWeek()
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: AppTheme.spacingExtraSmall

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        color: AppTheme.textPrimaryColor
                        font.pixelSize: AppTheme.fontSizeHeading
                        font.bold: true
                        text: root.viewModel
                            ? root.viewModel.weekLabel
                            : qsTr("课表")
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        color: AppTheme.textSecondaryColor
                        text: root.viewModel ? root.viewModel.weekDateRange : ""
                    }
                }

                AppButton {
                    objectName: "scheduleNextWeekButton"
                    text: qsTr("下一周")
                    enabled: root.viewModel && root.viewModel.canShowNextWeek
                    onClicked: root.viewModel.showNextWeek()
                }
            }

            AppButton {
                objectName: "scheduleCurrentWeekButton"
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("返回当前周")
                visible: root.viewModel
                    && root.viewModel.currentWeekAvailable
                enabled: root.viewModel && !root.viewModel.showingCurrentWeek
                onClicked: root.viewModel.showCurrentWeek()
            }

            GridLayout {
                Layout.alignment: Qt.AlignHCenter
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
                    text: qsTr("− 减少一节")
                    enabled: root.schedulePeriodCount > 1
                    onClicked: root.schedulePeriodCount = Math.max(
                        1, root.schedulePeriodCount - 1)
                }

                AppButton {
                    objectName: "scheduleIncreasePeriodsButton"
                    text: qsTr("+ 增加一节")
                    enabled: root.schedulePeriodCount < 20
                    onClicked: root.schedulePeriodCount = Math.min(
                        20, root.schedulePeriodCount + 1)
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
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
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
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
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
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
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                Layout.bottomMargin: AppTheme.spacingLarge
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
            viewportWidth: root.width
            viewportHeight: root.height
            onRequestManageCourses: managementDialog.open()
        }
    }

    Dialog {
        id: managementDialog

        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        width: Math.min(root.width - AppTheme.spacingLarge * 2, 840)
        height: Math.min(root.height - AppTheme.spacingLarge * 2, 600)
        title: qsTr("学期与课程资料")
        standardButtons: Dialog.Close
        onClosed: {
            if (root.managementViewModel
                    && root.managementViewModel.selectedTermId.length > 0) {
                root.selectTerm(root.managementViewModel.selectedTermId)
            }
        }

        Loader {
            anchors.fill: parent
            active: managementDialog.visible
                && root.managementViewModel !== null
            sourceComponent: Component {
                ScheduleManagementPanel {
                    viewModel: root.managementViewModel
                }
            }
        }
    }

    Dialog {
        id: courseDetailDialog

        objectName: "courseDetailDialog"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        width: Math.min(root.width - AppTheme.spacingLarge * 2, 420)
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
