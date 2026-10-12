pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel
    required property var managementViewModel
    required property string startupError

    readonly property string activeError: startupError.length > 0
        ? startupError
        : viewModel && viewModel.hasError
            ? viewModel.errorMessage
            : ""

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: root.width
            spacing: AppTheme.spacingMedium

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge
                Layout.topMargin: AppTheme.spacingLarge
                spacing: AppTheme.spacingSmall

                ComboBox {
                    id: termSelector

                    Layout.fillWidth: true
                    Layout.maximumWidth: 360
                    model: root.viewModel ? root.viewModel.termOptions : []
                    textRole: "label"
                    valueRole: "id"
                    enabled: root.viewModel && root.viewModel.hasTerms
                    onActivated: root.viewModel.selectTerm(currentValue)

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

                Button {
                    text: qsTr("管理学期与课程")
                    enabled: root.managementViewModel !== null
                    onClicked: managementDialog.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AppTheme.spacingLarge
                Layout.rightMargin: AppTheme.spacingLarge

                Button {
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

                Button {
                    text: qsTr("下一周")
                    enabled: root.viewModel && root.viewModel.canShowNextWeek
                    onClicked: root.viewModel.showNextWeek()
                }
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("返回当前周")
                visible: root.viewModel && root.viewModel.currentWeekAvailable
                enabled: root.viewModel && !root.viewModel.showingCurrentWeek
                onClicked: root.viewModel.showCurrentWeek()
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
                Layout.bottomMargin: AppTheme.spacingLarge
                Layout.preferredHeight: Math.max(220, meetingList.contentHeight
                    + AppTheme.spacingMedium * 2)
                radius: AppTheme.radiusLarge
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor

                ListView {
                    id: meetingList
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    spacing: AppTheme.spacingSmall
                    interactive: false
                    model: root.viewModel ? root.viewModel.meetings : null

                    delegate: Rectangle {
                        id: meetingDelegate

                        required property string courseName
                        required property string teacher
                        required property string location
                        required property string courseColor
                        required property string weekdayText
                        required property string periodText

                        width: ListView.view.width
                        height: meetingContent.implicitHeight
                            + AppTheme.spacingMedium * 2
                        radius: AppTheme.radiusMedium
                        color: AppTheme.surfaceVariantColor

                        RowLayout {
                            id: meetingContent
                            anchors.fill: parent
                            anchors.margins: AppTheme.spacingMedium
                            spacing: AppTheme.spacingMedium

                            Rectangle {
                                Layout.preferredWidth: 8
                                Layout.fillHeight: true
                                radius: 4
                                color: meetingDelegate.courseColor
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: AppTheme.spacingExtraSmall

                                Label {
                                    Layout.fillWidth: true
                                    text: meetingDelegate.courseName
                                    color: AppTheme.textPrimaryColor
                                    font.bold: true
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("%1 · %2")
                                        .arg(meetingDelegate.weekdayText)
                                        .arg(meetingDelegate.periodText)
                                    color: AppTheme.textSecondaryColor
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: [meetingDelegate.teacher,
                                           meetingDelegate.location]
                                        .filter(value => value.length > 0)
                                        .join(" · ")
                                    visible: text.length > 0
                                    color: AppTheme.textSecondaryColor
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: AppTheme.spacingSmall
                    visible: root.viewModel && root.viewModel.empty

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: root.viewModel.hasTerms
                            ? qsTr("这一周还没有课程")
                            : qsTr("还没有学期")
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
        }
    }

    Dialog {
        id: managementDialog

        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        width: Math.min(root.width - AppTheme.spacingLarge * 2, 840)
        height: Math.min(root.height - AppTheme.spacingLarge * 2, 600)
        title: qsTr("学期与课程管理")
        standardButtons: Dialog.Close

        ScheduleManagementPanel {
            anchors.fill: parent
            viewModel: root.managementViewModel
        }
    }
}
