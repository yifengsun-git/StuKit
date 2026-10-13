pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root
    required property var viewModel
    readonly property bool compact: width < 680
    property var editingTask: null
    property string deletingId: ""
    property string dueValue: ""
    property string reminderValue: ""

    background: Rectangle { color: AppTheme.backgroundColor }

    function openEditor(task) {
        editingTask = task
        titleField.text = task ? task.title : ""
        descriptionField.text = task ? task.description : ""
        dueValue = task ? task.dueText : ""
        reminderValue = task ? task.reminderText : ""
        priorityBox.currentIndex = task ? task.priority : 1
        const categoryIndex = task
            ? categoryBox.find(task.categoryText) : 0
        categoryBox.currentIndex = categoryIndex >= 0 ? categoryIndex : 0
        courseBox.currentIndex = 0
        if (task && task.courseId.length > 0) {
            for (let i = 0; i < courseBox.count; ++i) {
                if (root.viewModel.courseOptions[i].id === task.courseId) {
                    courseBox.currentIndex = i
                    break
                }
            }
        }
        editorDialog.open()
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: root.width
            spacing: AppTheme.spacingMedium

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: AppTheme.spacingLarge
                Layout.bottomMargin: 0

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: AppTheme.spacingExtraSmall
                    Label {
                        text: qsTr("待办")
                        color: AppTheme.textPrimaryColor
                        font.pixelSize: AppTheme.fontSizeDisplay
                        font.bold: true
                    }
                    Label {
                        text: qsTr("管理作业、考试与竞赛任务")
                        color: AppTheme.textSecondaryColor
                    }
                }
                AppButton {
                    text: qsTr("新建任务")
                    highlighted: true
                    onClicked: root.openEditor(null)
                }
            }

            Repeater {
                objectName: "taskReminderRepeater"
                model: root.viewModel ? root.viewModel.reminders : []
                delegate: Rectangle {
                    id: reminderCard
                    required property var modelData
                    required property int index
                    Layout.fillWidth: true
                    Layout.leftMargin: AppTheme.spacingLarge
                    Layout.rightMargin: AppTheme.spacingLarge
                    Layout.preferredHeight: reminderRow.implicitHeight
                        + AppTheme.spacingMedium * 2
                    radius: AppTheme.radiusMedium
                    color: AppTheme.surfaceColor
                    border.color: AppTheme.primaryColor
                    RowLayout {
                        id: reminderRow
                        anchors.fill: parent
                        anchors.margins: AppTheme.spacingMedium
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label { text: qsTr("任务提醒：%1").arg(reminderCard.modelData.title); color: AppTheme.textPrimaryColor; font.bold: true }
                            Label { text: qsTr("截止 %1%2").arg(reminderCard.modelData.dueText).arg(reminderCard.modelData.courseName.length ? " · " + reminderCard.modelData.courseName : ""); color: AppTheme.textSecondaryColor; wrapMode: Text.WordWrap }
                        }
                        AppButton { text: qsTr("知道了"); onClicked: root.viewModel.dismissReminder(reminderCard.index) }
                    }
                }
            }

            GridLayout {
                objectName: "taskStatsGrid"
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                columns: root.compact ? 2 : 4
                columnSpacing: AppTheme.spacingSmall
                rowSpacing: AppTheme.spacingSmall

                Repeater {
                    model: [
                        { label: qsTr("待完成"), value: root.viewModel ? root.viewModel.pendingCount : 0 },
                        { label: qsTr("已完成"), value: root.viewModel ? root.viewModel.completedCount : 0 },
                        { label: qsTr("已逾期"), value: root.viewModel ? root.viewModel.overdueCount : 0 },
                        { label: qsTr("完成率"), value: root.viewModel ? root.viewModel.completionRateText : "0%" }
                    ]
                    delegate: Rectangle {
                        id: statCard
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 82
                        radius: AppTheme.radiusMedium
                        color: AppTheme.surfaceColor
                        border.color: AppTheme.borderColor
                        Column {
                            anchors.centerIn: parent
                            spacing: 4
                            Label { anchors.horizontalCenter: parent.horizontalCenter; text: statCard.modelData.value; color: AppTheme.textPrimaryColor; font.pixelSize: 22; font.bold: true }
                            Label { anchors.horizontalCenter: parent.horizontalCenter; text: statCard.modelData.label; color: AppTheme.textSecondaryColor }
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                columns: root.compact ? 1 : 3
                TextField {
                    Layout.fillWidth: true
                    placeholderText: qsTr("搜索标题、说明或课程")
                    onAccepted: root.viewModel.setKeyword(text)
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: [qsTr("全部分类")].concat(
                        root.viewModel ? root.viewModel.categoryOptions : [])
                    onActivated: root.viewModel.setCategoryFilter(currentIndex - 1)
                }
                CheckBox {
                    text: qsTr("显示已归档")
                    checked: root.viewModel ? root.viewModel.includeArchived : false
                    onToggled: root.viewModel.setIncludeArchived(checked)
                }
                AppButton {
                    Layout.fillWidth: root.compact
                    text: qsTr("新增分类")
                    onClicked: {
                        categoryNameField.text = ""
                        categoryDialog.open()
                        categoryNameField.forceActiveFocus()
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                visible: root.viewModel && root.viewModel.hasError
                text: root.viewModel ? root.viewModel.errorMessage : ""
                color: AppTheme.errorColor
                wrapMode: Text.WordWrap
            }

            Label {
                objectName: "taskEmptyState"
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: AppTheme.spacingLarge
                visible: root.viewModel && root.viewModel.tasks.length === 0
                text: qsTr("还没有匹配的任务")
                color: AppTheme.textSecondaryColor
            }

            Repeater {
                objectName: "taskListRepeater"
                model: root.viewModel ? root.viewModel.tasks : []
                delegate: ColumnLayout {
                    id: taskDelegate
                    required property var modelData
                    required property int index
                    readonly property bool startsGroup: taskDelegate.index === 0
                        || root.viewModel.tasks[taskDelegate.index - 1].groupLabel
                            !== taskDelegate.modelData.groupLabel
                    Layout.fillWidth: true
                    Layout.leftMargin: AppTheme.spacingLarge
                    Layout.rightMargin: AppTheme.spacingLarge
                    spacing: AppTheme.spacingSmall

                    Label {
                        visible: taskDelegate.startsGroup
                        text: taskDelegate.modelData.groupLabel
                        color: taskDelegate.modelData.overdue
                            ? AppTheme.errorColor : AppTheme.textSecondaryColor
                        font.bold: true
                        font.pixelSize: AppTheme.fontSizeSubtitle
                    }

                    Rectangle {
                        id: taskCard
                        property var task: taskDelegate.modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: cardContent.implicitHeight
                            + AppTheme.spacingMedium * 2
                        radius: AppTheme.radiusMedium
                        color: AppTheme.surfaceColor
                        border.color: taskCard.task.overdue
                            ? AppTheme.errorColor : AppTheme.borderColor

                        ColumnLayout {
                            id: cardContent
                            anchors.fill: parent
                            anchors.margins: AppTheme.spacingMedium
                            RowLayout {
                                Layout.fillWidth: true
                                Label { Layout.fillWidth: true; text: taskCard.task.title; color: AppTheme.textPrimaryColor; font.bold: true; font.pixelSize: AppTheme.fontSizeSubtitle; wrapMode: Text.WordWrap }
                                Label { text: taskCard.task.statusText; color: taskCard.task.overdue ? AppTheme.errorColor : AppTheme.primaryColor }
                            }
                            Label { Layout.fillWidth: true; visible: text.length > 0; text: taskCard.task.description; color: AppTheme.textSecondaryColor; wrapMode: Text.WordWrap }
                            Label {
                                Layout.fillWidth: true
                                text: [taskCard.task.categoryText, taskCard.task.priorityText + qsTr("优先级"), taskCard.task.courseName, taskCard.task.dueText.length ? qsTr("截止 %1").arg(taskCard.task.dueText) : ""].filter(value => value.length > 0).join(" · ")
                                color: AppTheme.textSecondaryColor
                                wrapMode: Text.WordWrap
                            }
                            Flow {
                                Layout.fillWidth: true
                                spacing: AppTheme.spacingSmall
                                AppButton { visible: taskCard.task.status === 0; text: qsTr("完成"); onClicked: root.viewModel.complete(taskCard.task.id) }
                                AppButton { visible: taskCard.task.status === 1; text: qsTr("重新打开"); onClicked: root.viewModel.reopen(taskCard.task.id) }
                                AppButton { visible: taskCard.task.status === 1; text: qsTr("归档"); onClicked: root.viewModel.archive(taskCard.task.id) }
                                AppButton { visible: taskCard.task.status === 2; text: qsTr("恢复归档"); onClicked: root.viewModel.restore(taskCard.task.id) }
                                AppButton { text: qsTr("编辑"); onClicked: root.openEditor(taskCard.task) }
                                AppButton { text: qsTr("删除"); onClicked: { root.deletingId = taskCard.task.id; deleteDialog.open() } }
                            }
                        }
                    }
                }
            }
            Item { Layout.preferredHeight: AppTheme.spacingLarge }
        }
    }

    Dialog {
        id: editorDialog
        anchors.centerIn: parent
        width: Math.min(root.width - AppTheme.spacingLarge * 2, 540)
        modal: true
        title: root.editingTask ? qsTr("编辑任务") : qsTr("新建任务")
        closePolicy: Popup.NoAutoClose
        ColumnLayout {
            width: parent.width
            TextField { id: titleField; Layout.fillWidth: true; placeholderText: qsTr("任务标题（必填）") }
            TextArea { id: descriptionField; Layout.fillWidth: true; placeholderText: qsTr("任务说明"); wrapMode: TextArea.Wrap }
            ComboBox {
                id: categoryBox
                Layout.fillWidth: true
                model: root.viewModel ? root.viewModel.categoryOptions : []
            }
            ComboBox { id: priorityBox; Layout.fillWidth: true; model: [qsTr("低优先级"), qsTr("普通优先级"), qsTr("高优先级"), qsTr("紧急")] }
            ComboBox { id: courseBox; Layout.fillWidth: true; model: root.viewModel ? root.viewModel.courseOptions : []; textRole: "label" }
            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: root.dueValue.length > 0
                        ? qsTr("截止：%1").arg(root.dueValue)
                        : qsTr("未设置截止时间")
                    color: AppTheme.textSecondaryColor
                }
                AppButton {
                    text: qsTr("选择")
                    onClicked: duePicker.openFor(root.dueValue)
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: root.reminderValue.length > 0
                        ? qsTr("提醒：%1").arg(root.reminderValue)
                        : qsTr("未设置提醒时间")
                    color: AppTheme.textSecondaryColor
                }
                AppButton {
                    text: qsTr("选择")
                    enabled: root.dueValue.length > 0
                    onClicked: reminderPicker.openFor(root.reminderValue)
                }
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                AppButton { text: qsTr("取消"); onClicked: editorDialog.close() }
                AppButton {
                    text: qsTr("保存")
                    highlighted: true
                    enabled: titleField.text.trim().length > 0
                    onClicked: {
                        const option = root.viewModel.courseOptions[courseBox.currentIndex]
                        const categoryName = categoryBox.currentText
                        if (root.viewModel.save(root.editingTask ? root.editingTask.id : "", titleField.text, descriptionField.text, root.dueValue, root.reminderValue, priorityBox.currentIndex, categoryName, option ? option.id : ""))
                            editorDialog.close()
                    }
                }
            }
        }
    }

    DateTimePickerDialog {
        id: duePicker
        objectName: "taskDuePicker"
        anchors.centerIn: parent
        width: Math.min(root.width - AppTheme.spacingLarge * 2, 620)
        onDateTimeSelected: value => {
            root.dueValue = value
            if (value.length === 0)
                root.reminderValue = ""
        }
    }

    DateTimePickerDialog {
        id: reminderPicker
        objectName: "taskReminderPicker"
        anchors.centerIn: parent
        width: Math.min(root.width - AppTheme.spacingLarge * 2, 620)
        onDateTimeSelected: value => root.reminderValue = value
    }

    Dialog {
        id: categoryDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("新增待办分类")
        standardButtons: Dialog.Save | Dialog.Cancel
        onAccepted: {
            if (!root.viewModel.addCategory(categoryNameField.text))
                Qt.callLater(function() { categoryDialog.open() })
        }

        ColumnLayout {
            width: Math.min(root.width - AppTheme.spacingLarge * 4, 360)
            Label {
                Layout.fillWidth: true
                text: qsTr("分类会保存在本机，之后新建待办时可以直接选择。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }
            TextField {
                id: categoryNameField
                Layout.fillWidth: true
                placeholderText: qsTr("分类名称，例如：社团、实习")
                maximumLength: 40
            }
        }
    }

    Dialog {
        id: deleteDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("删除任务")
        standardButtons: Dialog.Yes | Dialog.No
        Label { text: qsTr("此操作会永久删除任务，是否继续？"); wrapMode: Text.WordWrap }
        onAccepted: root.viewModel.remove(root.deletingId)
    }
}
