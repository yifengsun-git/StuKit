pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    property bool allowEmpty: true
    property bool includeTime: true
    signal dateTimeSelected(string value)

    modal: true
    title: includeTime ? qsTr("选择日期和时间") : qsTr("选择日期")
    closePolicy: Popup.NoAutoClose
    standardButtons: Dialog.NoButton

    function twoDigits(value) {
        return String(value).padStart(2, "0")
    }

    function daysInMonth(year, month) {
        return new Date(year, month, 0).getDate()
    }

    function moveTumbler(tumbler, delta) {
        if (!tumbler || tumbler.count <= 0 || delta === 0)
            return

        const direction = delta > 0 ? -1 : 1
        let nextIndex = tumbler.currentIndex + direction
        if (tumbler.wrap) {
            nextIndex = (nextIndex + tumbler.count) % tumbler.count
        } else {
            nextIndex = Math.max(0, Math.min(tumbler.count - 1, nextIndex))
        }
        tumbler.currentIndex = nextIndex
    }

    function acceptSelection() {
        root.dateTimeSelected(root.selectedValue())
        root.close()
    }

    function openFor(value) {
        const now = new Date()
        const match = /^(\d{4})-(\d{2})-(\d{2})(?:\s+(\d{2}):(\d{2}))?$/.exec(
            value.trim())
        const year = match ? Number(match[1]) : now.getFullYear()
        const month = match ? Number(match[2]) : now.getMonth() + 1
        const day = match ? Number(match[3]) : now.getDate()
        const hour = match && match[4] !== undefined
            ? Number(match[4]) : now.getHours()
        const minute = match && match[5] !== undefined
            ? Number(match[5]) : now.getMinutes()
        yearTumbler.currentIndex = Math.max(0, Math.min(100, year - 2000))
        monthTumbler.currentIndex = month - 1
        dayTumbler.currentIndex = Math.max(0, Math.min(
            root.daysInMonth(year, month) - 1, day - 1))
        hourTumbler.currentIndex = hour
        minuteTumbler.currentIndex = minute
        open()
    }

    function selectedValue() {
        const dateValue = String(2000 + yearTumbler.currentIndex) + "-"
            + twoDigits(monthTumbler.currentIndex + 1) + "-"
            + twoDigits(dayTumbler.currentIndex + 1)
        return includeTime
            ? dateValue + " " + twoDigits(hourTumbler.currentIndex) + ":"
                + twoDigits(minuteTumbler.currentIndex)
            : dateValue
    }

    contentItem: ColumnLayout {
        spacing: AppTheme.spacingMedium

        Label {
            Layout.fillWidth: true
            text: includeTime
                ? qsTr("上下滚动各列选择日期和时间")
                : qsTr("上下滚动各列选择日期，也可以在输入框中手动填写")
            color: AppTheme.textSecondaryColor
            horizontalAlignment: Text.AlignHCenter
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 190
            spacing: AppTheme.spacingExtraSmall

            Tumbler {
                id: yearTumbler
                objectName: "datePickerYearTumbler"
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: 101
                delegate: Label {
                    required property int modelData
                    required property int index
                    text: 2000 + modelData + qsTr("年")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    opacity: 1.0 - Math.abs(Tumbler.displacement) / 2.2
                }
                WheelHandler {
                    target: null
                    onWheel: event => {
                        root.moveTumbler(yearTumbler, event.angleDelta.y)
                        event.accepted = true
                    }
                }
            }
            Tumbler {
                id: monthTumbler
                objectName: "datePickerMonthTumbler"
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: 12
                delegate: Label {
                    required property int modelData
                    required property int index
                    text: modelData + 1 + qsTr("月")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    opacity: 1.0 - Math.abs(Tumbler.displacement) / 2.2
                }
                WheelHandler {
                    target: null
                    onWheel: event => {
                        root.moveTumbler(monthTumbler, event.angleDelta.y)
                        event.accepted = true
                    }
                }
            }
            Tumbler {
                id: dayTumbler
                objectName: "datePickerDayTumbler"
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: root.daysInMonth(
                    2000 + yearTumbler.currentIndex,
                    monthTumbler.currentIndex + 1)
                delegate: Label {
                    required property int modelData
                    required property int index
                    text: modelData + 1 + qsTr("日")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    opacity: 1.0 - Math.abs(Tumbler.displacement) / 2.2
                }
                WheelHandler {
                    target: null
                    onWheel: event => {
                        root.moveTumbler(dayTumbler, event.angleDelta.y)
                        event.accepted = true
                    }
                }
            }
            Tumbler {
                id: hourTumbler
                visible: root.includeTime
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: 24
                delegate: Label {
                    required property int modelData
                    required property int index
                    text: root.twoDigits(modelData) + qsTr("时")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    opacity: 1.0 - Math.abs(Tumbler.displacement) / 2.2
                }
                WheelHandler {
                    target: null
                    onWheel: event => {
                        root.moveTumbler(hourTumbler, event.angleDelta.y)
                        event.accepted = true
                    }
                }
            }
            Tumbler {
                id: minuteTumbler
                visible: root.includeTime
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: 60
                delegate: Label {
                    required property int modelData
                    required property int index
                    text: root.twoDigits(modelData) + qsTr("分")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    opacity: 1.0 - Math.abs(Tumbler.displacement) / 2.2
                }
                WheelHandler {
                    target: null
                    onWheel: event => {
                        root.moveTumbler(minuteTumbler, event.angleDelta.y)
                        event.accepted = true
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            AppButton {
                visible: root.allowEmpty
                text: qsTr("清除")
                onClicked: {
                    root.dateTimeSelected("")
                    root.close()
                }
            }
            AppButton {
                text: qsTr("取消")
                onClicked: root.close()
            }
            AppButton {
                text: qsTr("确定")
                highlighted: true
                onClicked: root.acceptSelection()
            }
        }
    }

    Shortcut {
        sequence: "Return"
        context: Qt.WindowShortcut
        enabled: Qt.platform.os !== "android" && root.opened
        onActivated: root.acceptSelection()
    }

    Shortcut {
        sequence: "Enter"
        context: Qt.WindowShortcut
        enabled: Qt.platform.os !== "android" && root.opened
        onActivated: root.acceptSelection()
    }
}
