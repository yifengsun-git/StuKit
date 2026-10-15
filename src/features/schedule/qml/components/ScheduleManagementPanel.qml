pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var viewModel
    readonly property bool compactLayout:
        width < AppTheme.breakpointCompact

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
        courseAssessmentField.text = ""
        courseTeachingPlanField.text = ""
        courseColorField.text = "#3B82F6"
        courseDialog.open()
    }

    function openCourseEditor(courseId, name, teacher, location, color,
                              assessmentStandard, teachingPlan) {
        courseDialog.courseId = courseId
        courseNameField.text = name
        courseTeacherField.text = teacher
        courseLocationField.text = location
        courseAssessmentField.text = assessmentStandard
        courseTeachingPlanField.text = teachingPlan
        courseColorField.text = color
        courseDialog.open()
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

                        AppButton {
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

                                AppButton {
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

                                AppButton {
                                    text: qsTr("编辑")
                                    onClicked: root.openTermEditor(
                                        termDelegate.termId,
                                        termDelegate.name,
                                        termDelegate.startDateText,
                                        termDelegate.totalWeeks,
                                        termDelegate.archived)
                                }

                                AppButton {
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

                        AppButton {
                            text: qsTr("新增课程")
                            enabled: !root.viewModel.selectedTermArchived
                            onClicked: root.openNewCourse()
                        }
                    }

                    Label {
                        text: root.viewModel.selectedTermArchived
                            ? qsTr("已归档学期只允许查看历史课程。")
                            : qsTr("这里只维护课程资料；返回课表后点击“编辑课表”进行排课。")
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
                            required property string assessmentStandard
                            required property string teachingPlan
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

                                AppButton {
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

                                AppButton {
                                    text: qsTr("编辑")
                                    enabled: !root.viewModel.selectedTermArchived
                                    onClicked: root.openCourseEditor(
                                        courseDelegate.courseId,
                                        courseDelegate.name,
                                        courseDelegate.teacher,
                                        courseDelegate.location,
                                        courseDelegate.courseColor,
                                        courseDelegate.assessmentStandard,
                                        courseDelegate.teachingPlan)
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: periodSection.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor

                ColumnLayout {
                    id: periodSection
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    spacing: AppTheme.spacingSmall

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("节次时间")
                        color: AppTheme.textPrimaryColor
                        font.bold: true
                        font.pixelSize: AppTheme.fontSizeSubtitle
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("自定义名称和起止时间只影响显示；课程冲突仍按节次编号判断。")
                        color: AppTheme.textSecondaryColor
                        wrapMode: Text.WordWrap
                    }

                    Repeater {
                        model: root.viewModel
                            ? root.viewModel.periodDefinitions : []

                        Rectangle {
                            id: periodDelegate
                            required property var modelData

                            Layout.fillWidth: true
                            Layout.preferredHeight: periodEditor.implicitHeight
                                + AppTheme.spacingSmall * 2
                            radius: AppTheme.radiusSmall
                            color: AppTheme.surfaceVariantColor

                            GridLayout {
                                id: periodEditor
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: AppTheme.spacingSmall
                                columns: root.compactLayout ? 1 : 2
                                rowSpacing: AppTheme.spacingExtraSmall
                                columnSpacing: AppTheme.spacingSmall

                                Label {
                                    text: qsTr("第 %1 节名称").arg(
                                        periodDelegate.modelData.periodNumber)
                                    color: AppTheme.textSecondaryColor
                                }
                                AppTextField {
                                    id: periodNameField
                                    Layout.fillWidth: true
                                    text: periodDelegate.modelData.name
                                    placeholderText: qsTr("例如 第 1 节")
                                }

                                Label {
                                    text: qsTr("起止时间")
                                    color: AppTheme.textSecondaryColor
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    AppTextField {
                                        id: periodStartField
                                        Layout.fillWidth: true
                                        text: periodDelegate.modelData.startTime
                                        placeholderText: qsTr("08:00")
                                        inputMethodHints: Qt.ImhTime
                                    }
                                    Label { text: "—" }
                                    AppTextField {
                                        id: periodEndField
                                        Layout.fillWidth: true
                                        text: periodDelegate.modelData.endTime
                                        placeholderText: qsTr("08:45")
                                        inputMethodHints: Qt.ImhTime
                                    }
                                    AppButton {
                                        objectName: "scheduleSavePeriodButton"
                                        text: qsTr("保存")
                                        onClicked: root.viewModel.savePeriodDefinition(
                                            periodDelegate.modelData.periodNumber,
                                            periodNameField.text,
                                            periodStartField.text,
                                            periodEndField.text)
                                    }
                                }
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

    AppDialog {
        id: termDialog
        objectName: "scheduleTermDialog"
        property string termId: ""
        property bool archived: false
        parent: Overlay.overlay
        preferredWidth: 440
        title: termId.length > 0 ? qsTr("编辑学期") : qsTr("新增学期")
        standardButtons: Dialog.NoButton

        ColumnLayout {
            spacing: AppTheme.spacingSmall

            Label {
                text: qsTr("学期名称")
                color: AppTheme.textPrimaryColor
                font.bold: true
            }
            AppTextField {
                id: termNameField
                Layout.fillWidth: true
                placeholderText: qsTr("学期名称，例如 2026 秋季学期")
            }
            Label {
                text: qsTr("学期开始日期（第 1 周的周一）")
                color: AppTheme.textPrimaryColor
                font.bold: true
            }
            AppTextField {
                id: termStartDateField
                Layout.fillWidth: true
                placeholderText: qsTr("例如 2026-09-07")
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("该日期用于计算每个教学周的实际日期，格式为 yyyy-MM-dd。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Label {
                    text: qsTr("学期总周数")
                    color: AppTheme.textPrimaryColor
                    font.bold: true
                }
                SpinBox {
                    id: termWeeksField
                    from: 1
                    to: 30
                    value: 18
                }
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                AppButton { text: qsTr("取消"); onClicked: termDialog.close() }
                AppButton {
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

    AppDialog {
        id: courseDialog
        objectName: "scheduleCourseDialog"
        property string courseId: ""
        parent: Overlay.overlay
        preferredWidth: 560
        title: courseId.length > 0 ? qsTr("编辑课程") : qsTr("新增课程")
        standardButtons: Dialog.NoButton

        ColumnLayout {
            Layout.fillWidth: true
            spacing: AppTheme.spacingSmall
            AppTextField { id: courseNameField; Layout.fillWidth: true; placeholderText: qsTr("课程名称") }
            AppTextField { id: courseTeacherField; Layout.fillWidth: true; placeholderText: qsTr("教师（可选）") }
            AppTextField { id: courseLocationField; Layout.fillWidth: true; placeholderText: qsTr("教室（可选）") }
            AppTextField { id: courseColorField; Layout.fillWidth: true; placeholderText: qsTr("颜色，例如 #3B82F6") }

            Label {
                text: qsTr("期末考核标准")
                color: AppTheme.textPrimaryColor
                font.bold: true
            }
            AppTextArea {
                id: courseAssessmentField
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                placeholderText: qsTr("例如：平时成绩 30%，期末考试 70%")
                wrapMode: TextEdit.Wrap
            }

            Label {
                text: qsTr("课程教学安排")
                color: AppTheme.textPrimaryColor
                font.bold: true
            }
            AppTextArea {
                id: courseTeachingPlanField
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                placeholderText: qsTr("记录章节进度、实验安排、作业节点等")
                wrapMode: TextEdit.Wrap
            }

            RowLayout {
                Layout.fillWidth: true
                AppButton {
                    visible: courseDialog.courseId.length > 0
                    text: qsTr("删除课程")
                    onClicked: {
                        deleteCourseDialog.courseId = courseDialog.courseId
                        deleteCourseDialog.courseName = courseNameField.text
                        courseDialog.close()
                        deleteCourseDialog.open()
                    }
                }
                Item { Layout.fillWidth: true }
                AppButton { text: qsTr("取消"); onClicked: courseDialog.close() }
                AppButton {
                    text: qsTr("保存")
                    highlighted: true
                    onClicked: {
                        if (root.viewModel.saveCourseDetails(
                                courseDialog.courseId,
                                root.viewModel.selectedTermId,
                                courseNameField.text,
                                courseTeacherField.text,
                                courseLocationField.text,
                                courseColorField.text,
                                courseAssessmentField.text,
                                courseTeachingPlanField.text))
                            courseDialog.close()
                    }
                }
            }
        }
    }

    AppDialog {
        id: deleteCourseDialog
        objectName: "scheduleDeleteCourseDialog"
        property string courseId: ""
        property string courseName: ""
        parent: Overlay.overlay
        preferredWidth: 460
        title: qsTr("确认删除课程")
        standardButtons: Dialog.NoButton

        ColumnLayout {
            spacing: AppTheme.spacingMedium
            Label {
                Layout.fillWidth: true
                text: qsTr("删除“%1”后，它的全部上课安排也会一起删除。此操作无法撤销。")
                    .arg(deleteCourseDialog.courseName)
                color: AppTheme.textPrimaryColor
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                AppButton {
                    text: qsTr("取消")
                    onClicked: deleteCourseDialog.close()
                }
                AppButton {
                    text: qsTr("确认删除")
                    highlighted: true
                    onClicked: {
                        if (root.viewModel.deleteCourse(
                                deleteCourseDialog.courseId))
                            deleteCourseDialog.close()
                    }
                }
            }
        }
    }

}
