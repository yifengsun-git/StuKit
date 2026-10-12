pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var viewModel

    function openNewTerm() {
        termDialog.termId = ""
        termDialog.archived = false
        termNameField.text = ""
        termStartDateField.text = Qt.formatDate(new Date(), "yyyy-MM-dd")
        termWeeksField.value = 18
        termDialog.open()
    }

    function openTermEditor(termId, name, startDateText, totalWeeks, archived) {
        termDialog.termId = termId
        termDialog.archived = archived
        termNameField.text = name
        termStartDateField.text = startDateText
        termWeeksField.value = totalWeeks
        termDialog.open()
    }

    function openNewCourse() {
        courseDialog.courseId = ""
        courseNameField.text = ""
        courseTeacherField.text = ""
        courseLocationField.text = ""
        courseColorField.text = "#3B82F6"
        courseDialog.open()
    }

    function openCourseEditor(courseId, name, teacher, location, color) {
        courseDialog.courseId = courseId
        courseNameField.text = name
        courseTeacherField.text = teacher
        courseLocationField.text = location
        courseColorField.text = color
        courseDialog.open()
    }

    function openNewMeeting() {
        meetingDialog.meetingId = ""
        weekdayBox.currentIndex = 0
        startPeriodField.value = 1
        endPeriodField.value = 2
        startWeekField.value = 1
        endWeekField.value = Math.max(1, root.viewModel.selectedTermTotalWeeks)
        weekPatternBox.currentIndex = 0
        meetingDialog.open()
    }

    function openMeetingEditor(meetingId, weekday, startPeriod, endPeriod,
                               startWeek, endWeek, weekPattern) {
        meetingDialog.meetingId = meetingId
        weekdayBox.currentIndex = Math.max(0, weekday - 1)
        startPeriodField.value = startPeriod
        endPeriodField.value = endPeriod
        startWeekField.value = startWeek
        endWeekField.value = endWeek
        weekPatternBox.currentIndex = weekPattern
        meetingDialog.open()
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: root.width
            spacing: AppTheme.spacingMedium

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: termSection.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor

                ColumnLayout {
                    id: termSection
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    spacing: AppTheme.spacingSmall

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("学期")
                            color: AppTheme.textPrimaryColor
                            font.bold: true
                            font.pixelSize: AppTheme.fontSizeSubtitle
                        }

                        Button {
                            text: qsTr("新增学期")
                            onClicked: root.openNewTerm()
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("先选择一个学期，再维护它的课程和上课安排。")
                        color: AppTheme.textSecondaryColor
                        wrapMode: Text.WordWrap
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(contentHeight, 180)
                        model: root.viewModel ? root.viewModel.terms : null
                        spacing: AppTheme.spacingExtraSmall
                        clip: true

                        delegate: Rectangle {
                            id: termDelegate
                            required property string termId
                            required property string name
                            required property string startDateText
                            required property int totalWeeks
                            required property bool archived

                            width: ListView.view.width
                            height: 54
                            radius: AppTheme.radiusSmall
                            color: root.viewModel.selectedTermId === termId
                                ? AppTheme.surfaceVariantColor
                                : "transparent"
                            border.color: root.viewModel.selectedTermId === termId
                                ? AppTheme.primaryColor
                                : AppTheme.borderColor

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: AppTheme.spacingSmall

                                Button {
                                    Layout.fillWidth: true
                                    flat: true
                                    text: termDelegate.archived
                                        ? qsTr("%1（已归档） · %2 周").arg(
                                            termDelegate.name,
                                            termDelegate.totalWeeks)
                                        : qsTr("%1 · %2 周").arg(
                                            termDelegate.name,
                                            termDelegate.totalWeeks)
                                    onClicked: root.viewModel.selectTerm(
                                        termDelegate.termId)
                                }

                                Button {
                                    text: qsTr("编辑")
                                    onClicked: root.openTermEditor(
                                        termDelegate.termId,
                                        termDelegate.name,
                                        termDelegate.startDateText,
                                        termDelegate.totalWeeks,
                                        termDelegate.archived)
                                }

                                Button {
                                    text: termDelegate.archived
                                        ? qsTr("恢复") : qsTr("归档")
                                    onClicked: root.viewModel.setTermArchived(
                                        termDelegate.termId,
                                        !termDelegate.archived)
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: courseSection.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor
                visible: root.viewModel && root.viewModel.hasTerms

                ColumnLayout {
                    id: courseSection
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    spacing: AppTheme.spacingSmall

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("课程")
                            color: AppTheme.textPrimaryColor
                            font.bold: true
                            font.pixelSize: AppTheme.fontSizeSubtitle
                        }

                        Button {
                            text: qsTr("新增课程")
                            enabled: !root.viewModel.selectedTermArchived
                            onClicked: root.openNewCourse()
                        }
                    }

                    Label {
                        text: root.viewModel.selectedTermArchived
                            ? qsTr("已归档学期只允许查看历史课程。")
                            : qsTr("选择课程后可以维护它的多个上课安排。")
                        color: AppTheme.textSecondaryColor
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(contentHeight, 180)
                        model: root.viewModel ? root.viewModel.courses : null
                        spacing: AppTheme.spacingExtraSmall
                        clip: true

                        delegate: Rectangle {
                            id: courseDelegate
                            required property string courseId
                            required property string name
                            required property string teacher
                            required property string location
                            required property string courseColor

                            width: ListView.view.width
                            height: 54
                            radius: AppTheme.radiusSmall
                            color: root.viewModel.selectedCourseId === courseId
                                ? AppTheme.surfaceVariantColor
                                : "transparent"
                            border.color: root.viewModel.selectedCourseId === courseId
                                ? AppTheme.primaryColor
                                : AppTheme.borderColor

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: AppTheme.spacingSmall

                                Rectangle {
                                    Layout.preferredWidth: 8
                                    Layout.fillHeight: true
                                    radius: 4
                                    color: courseDelegate.courseColor
                                }

                                Button {
                                    Layout.fillWidth: true
                                    flat: true
                                    text: [courseDelegate.name,
                                           courseDelegate.teacher,
                                           courseDelegate.location]
                                        .filter(value => value.length > 0)
                                        .join(" · ")
                                    onClicked: root.viewModel.selectCourse(
                                        courseDelegate.courseId)
                                }

                                Button {
                                    text: qsTr("编辑")
                                    enabled: !root.viewModel.selectedTermArchived
                                    onClicked: root.openCourseEditor(
                                        courseDelegate.courseId,
                                        courseDelegate.name,
                                        courseDelegate.teacher,
                                        courseDelegate.location,
                                        courseDelegate.courseColor)
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: meetingSection.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor
                visible: root.viewModel && root.viewModel.hasCourses

                ColumnLayout {
                    id: meetingSection
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    spacing: AppTheme.spacingSmall

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("上课安排")
                            color: AppTheme.textPrimaryColor
                            font.bold: true
                            font.pixelSize: AppTheme.fontSizeSubtitle
                        }

                        Button {
                            text: qsTr("新增安排")
                            enabled: !root.viewModel.selectedTermArchived
                            onClicked: root.openNewMeeting()
                        }
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(contentHeight, 180)
                        model: root.viewModel ? root.viewModel.meetings : null
                        spacing: AppTheme.spacingExtraSmall
                        clip: true

                        delegate: RowLayout {
                            id: meetingDelegate
                            required property string meetingId
                            required property int weekday
                            required property int startPeriod
                            required property int endPeriod
                            required property int startWeek
                            required property int endWeek
                            required property int weekPattern
                            required property string summary

                            width: ListView.view.width

                            Label {
                                Layout.fillWidth: true
                                text: meetingDelegate.summary
                                color: AppTheme.textPrimaryColor
                            }

                            Button {
                                text: qsTr("编辑")
                                enabled: !root.viewModel.selectedTermArchived
                                onClicked: root.openMeetingEditor(
                                    meetingDelegate.meetingId,
                                    meetingDelegate.weekday,
                                    meetingDelegate.startPeriod,
                                    meetingDelegate.endPeriod,
                                    meetingDelegate.startWeek,
                                    meetingDelegate.endWeek,
                                    meetingDelegate.weekPattern)
                            }
                        }
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
                    color: AppTheme.errorColor
                    text: qsTr("与“%1”冲突：第 %2 周，第 %3～%4 节")
                        .arg(modelData.courseName)
                        .arg(modelData.firstConflictingWeek)
                        .arg(modelData.startPeriod)
                        .arg(modelData.endPeriod)
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    Dialog {
        id: termDialog
        property string termId: ""
        property bool archived: false
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: termId.length > 0 ? qsTr("编辑学期") : qsTr("新增学期")
        standardButtons: Dialog.NoButton

        ColumnLayout {
            spacing: AppTheme.spacingSmall

            TextField {
                id: termNameField
                Layout.fillWidth: true
                placeholderText: qsTr("学期名称，例如 2026 秋季学期")
            }
            TextField {
                id: termStartDateField
                Layout.fillWidth: true
                placeholderText: qsTr("开始日期：yyyy-MM-dd")
            }
            RowLayout {
                Label { text: qsTr("总周数") }
                SpinBox {
                    id: termWeeksField
                    from: 1
                    to: 30
                    value: 18
                }
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { text: qsTr("取消"); onClicked: termDialog.close() }
                Button {
                    text: qsTr("保存")
                    highlighted: true
                    onClicked: {
                        if (root.viewModel.saveTerm(
                                termDialog.termId,
                                termNameField.text,
                                termStartDateField.text,
                                termWeeksField.value,
                                termDialog.archived))
                            termDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: courseDialog
        property string courseId: ""
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: courseId.length > 0 ? qsTr("编辑课程") : qsTr("新增课程")
        standardButtons: Dialog.NoButton

        ColumnLayout {
            spacing: AppTheme.spacingSmall
            TextField { id: courseNameField; Layout.fillWidth: true; placeholderText: qsTr("课程名称") }
            TextField { id: courseTeacherField; Layout.fillWidth: true; placeholderText: qsTr("教师（可选）") }
            TextField { id: courseLocationField; Layout.fillWidth: true; placeholderText: qsTr("教室（可选）") }
            TextField { id: courseColorField; Layout.fillWidth: true; placeholderText: qsTr("颜色，例如 #3B82F6") }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { text: qsTr("取消"); onClicked: courseDialog.close() }
                Button {
                    text: qsTr("保存")
                    highlighted: true
                    onClicked: {
                        if (root.viewModel.saveCourseDetails(
                                courseDialog.courseId,
                                root.viewModel.selectedTermId,
                                courseNameField.text,
                                courseTeacherField.text,
                                courseLocationField.text,
                                courseColorField.text))
                            courseDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: meetingDialog
        property string meetingId: ""
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: meetingId.length > 0 ? qsTr("编辑上课安排") : qsTr("新增上课安排")
        standardButtons: Dialog.NoButton

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
            SpinBox { id: startPeriodField; from: 1; to: 20; value: 1 }
            Label { text: qsTr("结束节次") }
            SpinBox { id: endPeriodField; from: 1; to: 20; value: 2 }
            Label { text: qsTr("起始周") }
            SpinBox { id: startWeekField; from: 1; to: 30; value: 1 }
            Label { text: qsTr("结束周") }
            SpinBox { id: endWeekField; from: 1; to: 30; value: 18 }
            Label { text: qsTr("重复规则") }
            ComboBox {
                id: weekPatternBox
                model: [qsTr("每周"), qsTr("单周"), qsTr("双周")]
            }
            RowLayout {
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignRight
                Button { text: qsTr("取消"); onClicked: meetingDialog.close() }
                Button {
                    text: qsTr("保存")
                    highlighted: true
                    onClicked: {
                        if (root.viewModel.saveMeetingDetails(
                                meetingDialog.meetingId,
                                root.viewModel.selectedCourseId,
                                weekdayBox.currentIndex + 1,
                                startPeriodField.value,
                                endPeriodField.value,
                                startWeekField.value,
                                endWeekField.value,
                                weekPatternBox.currentIndex))
                            meetingDialog.close()
                    }
                }
            }
        }
    }
}
