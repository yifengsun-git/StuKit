pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root
    required property var viewModel
    readonly property bool compact: width < AppTheme.breakpointCompact
    readonly property int pagePadding: AppTheme.pagePadding(width)
    property var editingTask: null
    property string deletingId: ""
    property string dueValue: ""
    property string reminderValue: ""
    property bool filtersExpanded: false
    readonly property var timeViewOptions: [
        qsTr("全部"),
        qsTr("今天"),
        qsTr("未来 7 天"),
        qsTr("已逾期"),
        qsTr("无截止时间")
    ]
    readonly property string emptyStateText: {
        if (!root.viewModel)
            return qsTr("还没有任务")
        switch (root.viewModel.timeView) {
        case 1:
            return qsTr("今天没有截止任务\n可以提前处理之后的安排。")
        case 2:
            return qsTr("未来七天没有截止任务\n这一周的安排比较轻松。")
        case 3:
            return qsTr("没有逾期任务\n保持得很好。")
        case 4:
            return qsTr("没有无截止时间的任务")
        default:
            return qsTr("还没有匹配的任务\n新建一项任务，开始安排今天。")
        }
    }

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

    function openEditorForCourse(courseId) {
        openEditor(null)
        if (!root.viewModel || !root.viewModel.courseOptions)
            return
        for (let index = 0; index < root.viewModel.courseOptions.length;
                ++index) {
            if (root.viewModel.courseOptions[index].id === courseId) {
                courseBox.currentIndex = index
                break
            }
        }
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
                objectName: "taskPrimaryCard"
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                Layout.topMargin: root.pagePadding
                accentColor: AppTheme.primaryColor

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.compact ? 1 : 2
                    columnSpacing: AppTheme.spacingMedium
                    rowSpacing: AppTheme.spacingMedium

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: AppTheme.spacingExtraSmall

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("待办")
                            color: AppTheme.textPrimaryColor
                            font.pixelSize: root.compact
                                ? AppTheme.fontSizeHeading
                                : AppTheme.fontSizeHeading
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("管理作业、考试与竞赛任务")
                            color: AppTheme.textSecondaryColor
                            wrapMode: Text.WordWrap
                        }
                    }

                    AppButton {
                        Layout.fillWidth: root.compact
                        Layout.alignment: root.compact
                            ? Qt.AlignLeft : Qt.AlignRight
                        text: qsTr("新建任务")
                        highlighted: true
                        onClicked: root.openEditor(null)
                    }
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
                    Layout.leftMargin: root.pagePadding
                    Layout.rightMargin: root.pagePadding
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
                Layout.fillWidth: true
                Layout.leftMargin: root.pagePadding
                Layout.rightMargin: root.pagePadding
                columns: root.compact ? 1 : 2
                columnSpacing: AppTheme.spacingMedium
                rowSpacing: AppTheme.spacingMedium

                ColumnLayout {
                    objectName: "taskSidebar"
                    Layout.fillWidth: root.compact
                    Layout.preferredWidth: root.compact ? 1 : 296
                    Layout.minimumWidth: root.compact ? 0 : 296
                    Layout.maximumWidth: root.compact ? 10000 : 296
                    Layout.alignment: Qt.AlignTop
                    spacing: AppTheme.spacingMedium

                    GridLayout {
                        objectName: "taskStatsGrid"
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        columns: 2
                        columnSpacing: AppTheme.spacingSmall
                        rowSpacing: AppTheme.spacingSmall

                        Repeater {
                            model: [
                                { label: qsTr("待完成"), value: root.viewModel ? root.viewModel.pendingCount : 0, accent: AppTheme.successColor },
                                { label: qsTr("已完成"), value: root.viewModel ? root.viewModel.completedCount : 0, accent: AppTheme.primaryColor },
                                { label: qsTr("已逾期"), value: root.viewModel ? root.viewModel.overdueCount : 0, accent: AppTheme.errorColor },
                                { label: qsTr("完成率"), value: root.viewModel ? root.viewModel.completionRateText : "0%", accent: AppTheme.warningColor }
                            ]
                            delegate: Rectangle {
                                id: statCard
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: 96
                                radius: AppTheme.radiusMedium
                                color: AppTheme.surfaceColor
                                border.color: AppTheme.borderColor
                                Column {
                                    anchors.fill: parent
                                    anchors.margins: AppTheme.spacingMedium
                                    spacing: AppTheme.spacingExtraSmall
                                    Rectangle { width: 28; height: 4; radius: 2; color: statCard.modelData.accent }
                                    Label { text: statCard.modelData.value; color: AppTheme.textPrimaryColor; font.pixelSize: 22; font.bold: true }
                                    Label { text: statCard.modelData.label; color: AppTheme.textSecondaryColor }
                                }
                            }
                        }
                    }

                    AppSectionCard {
                        objectName: "taskFilterCard"
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        contentPadding: AppTheme.spacingMedium
                        title: qsTr("筛选与分类")
                        description: root.compact
                            ? qsTr("搜索、分类筛选和归档显示集中在这里。")
                            : qsTr("快速缩小右侧任务范围。")

                        AppButton {
                            visible: root.compact
                            flat: true
                            text: root.filtersExpanded
                                ? qsTr("收起筛选") : qsTr("展开筛选")
                            onClicked: root.filtersExpanded = !root.filtersExpanded
                        }

                        GridLayout {
                            objectName: "taskFilterContent"
                            Layout.fillWidth: true
                            columns: 1
                            columnSpacing: AppTheme.spacingSmall
                            rowSpacing: AppTheme.spacingSmall
                            visible: !root.compact || root.filtersExpanded

                            AppTextField {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                placeholderText: qsTr("搜索任务")
                                onAccepted: root.viewModel.setKeyword(text)
                            }

                            AppComboBox {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                model: [qsTr("全部分类")].concat(
                                    root.viewModel ? root.viewModel.categoryOptions : [])
                                onActivated: root.viewModel.setCategoryFilter(
                                    currentIndex - 1)
                            }

                            CheckBox {
                                Layout.fillWidth: true
                                text: qsTr("显示已归档")
                                checked: root.viewModel
                                    ? root.viewModel.includeArchived : false
                                onToggled: root.viewModel.setIncludeArchived(checked)
                            }

                            AppButton {
                                Layout.fillWidth: true
                                text: qsTr("新增分类")
                                onClicked: {
                                    categoryNameField.text = ""
                                    categoryDialog.open()
                                    categoryNameField.forceActiveFocus()
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: AppTheme.spacingSmall

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("任务清单")
                            color: AppTheme.textPrimaryColor
                            font.pixelSize: AppTheme.fontSizeTitle
                            font.bold: true
                        }
                        Label {
                            text: root.viewModel
                                ? qsTr("%1 项").arg(root.viewModel.tasks.length)
                                : ""
                            color: AppTheme.textSecondaryColor
                        }
                    }

                    GridLayout {
                        objectName: "taskTimeViewBar"
                        Layout.fillWidth: true
                        columns: root.compact ? 2
                            : root.width >= 1100 ? 5 : 3
                        columnSpacing: AppTheme.spacingSmall
                        rowSpacing: AppTheme.spacingSmall

                        Repeater {
                            objectName: "taskTimeViewRepeater"
                            model: root.timeViewOptions

                            delegate: AppButton {
                                id: timeViewButton
                                required property int index
                                required property string modelData

                                objectName: "taskTimeViewButton" + index
                                Layout.fillWidth: true
                                text: modelData
                                flat: true
                                highlighted: root.viewModel
                                    && root.viewModel.timeView === index
                                onClicked: root.viewModel.setTimeView(index)
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

                    Label {
                        objectName: "taskEmptyState"
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: AppTheme.spacingExtraLarge
                        Layout.bottomMargin: AppTheme.spacingExtraLarge
                        visible: root.viewModel && root.viewModel.tasks.length === 0
                        text: root.emptyStateText
                        horizontalAlignment: Text.AlignHCenter
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
                }
            }
            Item { Layout.preferredHeight: AppTheme.spacingLarge }
        }
    }

    AppDialog {
        id: editorDialog
        objectName: "taskEditorDialog"

        anchors.centerIn: parent
        width: Math.min(root.width - AppTheme.spacingMedium * 2, 540)
        modal: true
        title: root.editingTask ? qsTr("编辑任务") : qsTr("新建任务")
        closePolicy: Popup.NoAutoClose

        footer: Item {
            implicitHeight: editorFooter.implicitHeight
                + AppTheme.spacingMedium * 2

            RowLayout {
                id: editorFooter

                anchors.right: parent.right
                anchors.rightMargin: AppTheme.spacingLarge
                anchors.verticalCenter: parent.verticalCenter
                spacing: AppTheme.spacingSmall

                AppButton {
                    text: qsTr("取消")
                    onClicked: editorDialog.close()
                }

                AppButton {
                    text: qsTr("保存")
                    highlighted: true
                    enabled: titleField.text.trim().length > 0
                    onClicked: {
                        const option = root.viewModel.courseOptions[
                            courseBox.currentIndex]
                        const categoryName = categoryBox.currentText
                        if (root.viewModel.save(
                                root.editingTask ? root.editingTask.id : "",
                                titleField.text,
                                descriptionField.text,
                                root.dueValue,
                                root.reminderValue,
                                priorityBox.currentIndex,
                                categoryName,
                                option ? option.id : "")) {
                            editorDialog.close()
                        }
                    }
                }
            }
        }

        ColumnLayout {
            width: parent.width
            spacing: AppTheme.spacingMedium

            AppTextField {
                id: titleField
                Layout.fillWidth: true
                placeholderText: qsTr("任务标题（必填）")
            }

            AppTextArea {
                id: descriptionField
                Layout.fillWidth: true
                Layout.preferredHeight: 96
                placeholderText: qsTr("任务说明")
                wrapMode: TextArea.Wrap
            }

            AppComboBox {
                id: categoryBox
                Layout.fillWidth: true
                model: root.viewModel ? root.viewModel.categoryOptions : []
            }

            AppComboBox {
                id: priorityBox
                Layout.fillWidth: true
                model: [qsTr("低优先级"), qsTr("普通优先级"),
                        qsTr("高优先级"), qsTr("紧急")]
            }

            AppComboBox {
                id: courseBox
                Layout.fillWidth: true
                model: root.viewModel ? root.viewModel.courseOptions : []
                textRole: "label"
            }

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

    AppDialog {
        id: categoryDialog
        anchors.centerIn: parent
        width: Math.min(root.width - AppTheme.spacingMedium * 2, 400)
        modal: true
        title: qsTr("新增待办分类")
        standardButtons: Dialog.Save | Dialog.Cancel
        onAccepted: {
            if (!root.viewModel.addCategory(categoryNameField.text))
                Qt.callLater(function() { categoryDialog.open() })
        }

        ColumnLayout {
            width: parent.width
            Label {
                Layout.fillWidth: true
                text: qsTr("分类会保存在本机，之后新建待办时可以直接选择。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }
            AppTextField {
                id: categoryNameField
                Layout.fillWidth: true
                placeholderText: qsTr("分类名称，例如：社团、实习")
                maximumLength: 40
            }
        }
    }

    AppDialog {
        id: deleteDialog
        anchors.centerIn: parent
        width: Math.min(root.width - AppTheme.spacingMedium * 2, 420)
        modal: true
        title: qsTr("删除任务")
        standardButtons: Dialog.Yes | Dialog.No
        ColumnLayout {
            width: parent.width

            Label {
                Layout.fillWidth: true
                text: qsTr("此操作会永久删除任务，是否继续？")
                color: AppTheme.textPrimaryColor
                wrapMode: Text.WordWrap
            }
        }
        onAccepted: root.viewModel.remove(root.deletingId)
    }
}
