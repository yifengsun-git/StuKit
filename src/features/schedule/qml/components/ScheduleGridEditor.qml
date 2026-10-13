pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: root

    required property var viewModel
    required property var scheduleModel
    required property var weekDays
    property bool compactLayout: false
    property int periodCount: 12
    property var periodDefinitions: []
    property real viewportWidth: width
    property real viewportHeight: height

    signal requestManageCourses()

    property string localMessage: ""

    objectName: "scheduleGridEditor"
    implicitHeight: editorColumn.implicitHeight

    function syncCourseSelection() {
        if (!root.viewModel) {
            courseSelector.currentIndex = -1
            return
        }
        const index = courseSelector.indexOfValue(
            root.viewModel.selectedCourseId)
        courseSelector.currentIndex = index >= 0 ? index : -1
    }

    function openNewMeeting(weekday, startPeriod) {
        root.localMessage = ""
        root.viewModel.clearMessages()
        if (!root.viewModel.hasCourses
                || root.viewModel.selectedCourseId.length === 0) {
            root.localMessage = qsTr("请先创建并选择一门课程")
            return
        }
        if (root.viewModel.selectedTermArchived) {
            root.localMessage = qsTr("已归档学期不能修改排课")
            return
        }
        const selectedCourseId = root.viewModel.selectedCourseId
        if (!root.viewModel.selectCourse(selectedCourseId)) {
            root.localMessage = qsTr("当前课程无效，请重新选择")
            return
        }
        meetingDialog.meetingId = ""
        meetingDialog.courseId = selectedCourseId
        meetingDialog.courseName = courseSelector.currentText
        weekdayBox.currentIndex = Math.max(0, weekday - 1)
        startPeriodField.value = startPeriod
        endPeriodField.value = Math.min(20, startPeriod + 1)
        meetingDialog.selectWeeks("all")
        meetingDialog.open()
    }

    function openExistingMeeting(details) {
        root.localMessage = ""
        root.viewModel.clearMessages()
        if (!root.viewModel.selectCourse(details.courseId))
            return
        meetingDialog.meetingId = details.meetingId
        meetingDialog.courseId = details.courseId
        meetingDialog.courseName = details.courseName
        weekdayBox.currentIndex = Math.max(0, details.weekday - 1)
        startPeriodField.value = details.startPeriod
        endPeriodField.value = details.endPeriod
        meetingDialog.setSelectedWeeks(details.activeWeeks)
        meetingDialog.open()
    }

    ColumnLayout {
        id: editorColumn
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: AppTheme.spacingMedium

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: editorToolbar.implicitHeight
                + AppTheme.spacingMedium * 2
            radius: AppTheme.radiusMedium
            color: AppTheme.surfaceColor
            border.color: AppTheme.primaryColor

            ColumnLayout {
                id: editorToolbar
                anchors.fill: parent
                anchors.margins: AppTheme.spacingMedium
                spacing: AppTheme.spacingSmall

                Label {
                    Layout.fillWidth: true
                    text: qsTr("学期排课模板")
                    color: AppTheme.textPrimaryColor
                    font.bold: true
                    font.pixelSize: AppTheme.fontSizeSubtitle
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("选择课程后点击空白节次添加；点击已有课程块进行编辑。默认一次应用到整个学期。")
                    color: AppTheme.textSecondaryColor
                    wrapMode: Text.WordWrap
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.compactLayout ? 1 : 4
                    rowSpacing: AppTheme.spacingSmall
                    columnSpacing: AppTheme.spacingSmall

                    Label {
                        text: qsTr("要排入的课程")
                        color: AppTheme.textPrimaryColor
                    }

                    ComboBox {
                        id: courseSelector
                        objectName: "scheduleEditorCourseSelector"
                        Layout.fillWidth: true
                        model: root.viewModel ? root.viewModel.courses : null
                        textRole: "name"
                        valueRole: "courseId"
                        enabled: root.viewModel
                            && !root.viewModel.selectedTermArchived
                            && root.viewModel.hasCourses
                        onActivated: root.viewModel.selectCourse(currentValue)

                        Component.onCompleted: root.syncCourseSelection()

                        Connections {
                            target: root.viewModel
                            function onSelectionChanged() {
                                root.syncCourseSelection()
                            }
                            function onSetupChanged() {
                                root.syncCourseSelection()
                            }
                        }
                    }

                    AppButton {
                        Layout.fillWidth: root.compactLayout
                        text: qsTr("课程资料")
                        onClicked: root.requestManageCourses()
                    }

                    AppButton {
                        objectName: "scheduleImportCsvButton"
                        Layout.fillWidth: root.compactLayout
                        text: qsTr("导入 CSV")
                        enabled: root.viewModel
                            && !root.viewModel.selectedTermArchived
                        onClicked: importFileDialog.open()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.viewModel && !root.viewModel.hasCourses
                    text: qsTr("当前学期还没有课程。请先打开“课程资料”创建课程，或使用 CSV 一次导入。")
                    color: AppTheme.textSecondaryColor
                    wrapMode: Text.WordWrap
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Excel/WPS 可另存为 UTF-8 CSV。必填列：课程名称、星期、开始节次、结束节次、教学周。")
                    color: AppTheme.textSecondaryColor
                    wrapMode: Text.WordWrap
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.localMessage.length > 0
                        || (root.viewModel && (root.viewModel.hasError
                            || root.viewModel.importMessage.length > 0))
                    text: root.localMessage.length > 0
                        ? root.localMessage
                        : root.viewModel && root.viewModel.hasError
                            ? root.viewModel.errorMessage
                            : root.viewModel ? root.viewModel.importMessage : ""
                    color: root.localMessage.length > 0
                            || (root.viewModel && root.viewModel.hasError)
                        ? AppTheme.errorColor : AppTheme.primaryColor
                    wrapMode: Text.WordWrap
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: editorGrid.implicitHeight

            Flickable {
                id: gridFlickable
                anchors.fill: parent
                contentWidth: Math.max(width, editorGrid.implicitWidth)
                contentHeight: editorGrid.implicitHeight
                flickableDirection: Flickable.HorizontalFlick
                boundsBehavior: Flickable.StopAtBounds
                clip: true

                WeeklyGrid {
                    id: editorGrid
                    objectName: "semesterScheduleGrid"
                    width: gridFlickable.contentWidth
                    model: root.scheduleModel
                    weekDays: root.weekDays
                    periodDefinitions: root.periodDefinitions
                    editable: root.viewModel
                        && !root.viewModel.selectedTermArchived
                        && root.viewModel.hasCourses
                        && root.viewModel.selectedCourseId.length > 0
                    showDates: true
                    showWeekRanges: true
                    periodCount: root.periodCount
                    emptyText: qsTr("该教学周还没有排课")
                    onEmptyCellActivated: (weekday, startPeriod) =>
                        root.openNewMeeting(weekday, startPeriod)
                    onCourseActivated: details =>
                        root.openExistingMeeting(details)
                }

                ScrollBar.horizontal: ScrollBar {
                    policy: gridFlickable.contentWidth > gridFlickable.width
                        ? ScrollBar.AsNeeded
                        : ScrollBar.AlwaysOff
                }
            }
        }
    }

    Dialog {
        id: meetingDialog

        property string meetingId: ""
        property string courseId: ""
        property string courseName: ""
        property var selectedWeeks: []

        function setSelectedWeeks(weeks) {
            const normalized = []
            const maximum = root.viewModel
                ? root.viewModel.selectedTermTotalWeeks : 1
            for (const value of weeks ?? []) {
                const week = Number(value)
                if (week >= 1 && week <= maximum
                        && normalized.indexOf(week) < 0)
                    normalized.push(week)
            }
            normalized.sort((left, right) => left - right)
            selectedWeeks = normalized
        }

        function selectWeeks(rule) {
            const weeks = []
            const maximum = root.viewModel
                ? root.viewModel.selectedTermTotalWeeks : 1
            for (let week = 1; week <= maximum; ++week) {
                if (rule === "all"
                        || (rule === "odd" && week % 2 === 1)
                        || (rule === "even" && week % 2 === 0))
                    weeks.push(week)
            }
            selectedWeeks = weeks
        }

        function toggleWeek(week, checked) {
            const weeks = selectedWeeks.slice()
            const index = weeks.indexOf(week)
            if (checked && index < 0)
                weeks.push(week)
            else if (!checked && index >= 0)
                weeks.splice(index, 1)
            weeks.sort((left, right) => left - right)
            selectedWeeks = weeks
        }

        objectName: "scheduleGridMeetingDialog"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        width: Math.min(
            root.viewportWidth - AppTheme.spacingSmall * 2,
            parent.width - AppTheme.spacingSmall * 2,
            560)
        height: Math.min(
            root.viewportHeight - AppTheme.spacingMedium * 2,
            parent.height - AppTheme.spacingMedium * 2,
            680)
        title: meetingId.length > 0
            ? qsTr("编辑“%1”的课程安排").arg(courseName)
            : qsTr("为“%1”添加课程安排").arg(courseName)
        standardButtons: Dialog.NoButton

        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: AppTheme.spacingSmall

            Label {
                Layout.fillWidth: true
                text: meetingDialog.meetingId.length > 0
                    ? qsTr("修改会应用到这条安排设置的全部教学周。")
                    : qsTr("默认应用到整个学期，也可以自由选择任意教学周。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("当前课程：%1").arg(meetingDialog.courseName)
                color: AppTheme.textPrimaryColor
                font.bold: true
                wrapMode: Text.WordWrap
            }

            GridLayout {
                columns: 2
                rowSpacing: AppTheme.spacingSmall
                columnSpacing: AppTheme.spacingSmall

                Label { text: qsTr("星期") }
                ComboBox {
                    id: weekdayBox
                    model: [qsTr("周一"), qsTr("周二"), qsTr("周三"),
                            qsTr("周四"), qsTr("周五"), qsTr("周六"), qsTr("周日")]
                }
                Label { text: qsTr("起始节次") }
                SpinBox {
                    id: startPeriodField
                    objectName: "scheduleEditorStartPeriod"
                    from: 1
                    to: 20
                }
                Label { text: qsTr("结束节次") }
                SpinBox {
                    id: endPeriodField
                    objectName: "scheduleEditorEndPeriod"
                    from: 1
                    to: 20
                }
            }

            Label {
                text: qsTr("教学周（已选 %1 周）")
                    .arg(meetingDialog.selectedWeeks.length)
                color: AppTheme.textPrimaryColor
                font.bold: true
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.compactLayout ? 2 : 4
                rowSpacing: AppTheme.spacingSmall
                columnSpacing: AppTheme.spacingSmall

                AppButton {
                    objectName: "scheduleSelectAllWeeksButton"
                    Layout.fillWidth: root.compactLayout
                    text: qsTr("全选")
                    onClicked: meetingDialog.selectWeeks("all")
                }
                AppButton {
                    objectName: "scheduleClearWeeksButton"
                    Layout.fillWidth: root.compactLayout
                    text: qsTr("清空")
                    onClicked: meetingDialog.selectedWeeks = []
                }
                AppButton {
                    objectName: "scheduleSelectOddWeeksButton"
                    Layout.fillWidth: root.compactLayout
                    text: qsTr("单周")
                    onClicked: meetingDialog.selectWeeks("odd")
                }
                AppButton {
                    objectName: "scheduleSelectEvenWeeksButton"
                    Layout.fillWidth: root.compactLayout
                    text: qsTr("双周")
                    onClicked: meetingDialog.selectWeeks("even")
                }
            }

            GridLayout {
                columns: root.compactLayout ? 3 : 6
                rowSpacing: 2
                columnSpacing: AppTheme.spacingSmall

                Repeater {
                    model: root.viewModel
                        ? root.viewModel.selectedTermTotalWeeks : 0

                    CheckBox {
                        required property int index
                        text: qsTr("%1周").arg(index + 1)
                        implicitHeight: AppTheme.controlHeight
                        checked: meetingDialog.selectedWeeks
                            .indexOf(index + 1) >= 0
                        onToggled: meetingDialog.toggleWeek(
                            index + 1, checked)
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: root.viewModel && root.viewModel.hasError
                text: root.viewModel ? root.viewModel.errorMessage : ""
                color: AppTheme.errorColor
                wrapMode: Text.WordWrap
            }

            Repeater {
                model: root.viewModel ? root.viewModel.conflicts : []

                Label {
                    required property var modelData
                    Layout.fillWidth: true
                    text: qsTr("与“%1”冲突：第 %2 周，第 %3～%4 节")
                        .arg(modelData.courseName)
                        .arg(modelData.firstConflictingWeek)
                        .arg(modelData.startPeriod)
                        .arg(modelData.endPeriod)
                    color: AppTheme.errorColor
                    wrapMode: Text.WordWrap
                }
            }

            RowLayout {
                Layout.fillWidth: true

                AppButton {
                    visible: meetingDialog.meetingId.length > 0
                    text: qsTr("删除安排")
                    onClicked: {
                        deleteMeetingDialog.meetingId = meetingDialog.meetingId
                        meetingDialog.close()
                        deleteMeetingDialog.open()
                    }
                }

                Item { Layout.fillWidth: true }

                AppButton {
                    text: qsTr("取消")
                    onClicked: meetingDialog.close()
                }

                AppButton {
                    text: root.compactLayout
                        ? qsTr("保存") : qsTr("保存到所选周次")
                    highlighted: true
                    onClicked: {
                        if (root.viewModel.saveMeetingWeeks(
                                meetingDialog.meetingId,
                                meetingDialog.courseId,
                                weekdayBox.currentIndex + 1,
                                startPeriodField.value,
                                endPeriodField.value,
                                meetingDialog.selectedWeeks)) {
                            meetingDialog.close()
                        }
                    }
                }
            }
        }
        }

        onClosed: root.viewModel.clearMessages()
    }

    FileDialog {
        id: importFileDialog
        title: qsTr("选择要导入的课表 CSV")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("CSV 文件 (*.csv)")]
        onAccepted: root.viewModel.importScheduleCsv(selectedFile)
    }

    Dialog {
        id: deleteMeetingDialog

        property string meetingId: ""

        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        width: Math.min(
            parent.width - AppTheme.spacingSmall * 2, 420)
        title: qsTr("确认删除上课安排")
        standardButtons: Dialog.NoButton

        ColumnLayout {
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                text: qsTr("这只会删除当前这一条上课安排，课程资料会保留。此操作无法撤销。")
                color: AppTheme.textPrimaryColor
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                AppButton {
                    text: qsTr("取消")
                    onClicked: deleteMeetingDialog.close()
                }
                AppButton {
                    text: qsTr("确认删除")
                    highlighted: true
                    onClicked: {
                        if (root.viewModel.deleteMeeting(
                                deleteMeetingDialog.meetingId))
                            deleteMeetingDialog.close()
                    }
                }
            }
        }
    }
}
