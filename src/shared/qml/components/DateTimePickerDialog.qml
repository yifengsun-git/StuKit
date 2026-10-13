pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    property bool allowEmpty: true
    signal dateTimeSelected(string value)

    modal: true
    title: qsTr("选择日期和时间")
    closePolicy: Popup.NoAutoClose
    standardButtons: Dialog.NoButton

    function twoDigits(value) {
        return String(value).padStart(2, "0")
    }

    function daysInMonth(year, month) {
        return new Date(year, month, 0).getDate()
    }

    function openFor(value) {
        const now = new Date()
        const match = /^(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2})$/.exec(value)
        const year = match ? Number(match[1]) : now.getFullYear()
        const month = match ? Number(match[2]) : now.getMonth() + 1
        const day = match ? Number(match[3]) : now.getDate()
        const hour = match ? Number(match[4]) : now.getHours()
        const minute = match ? Number(match[5]) : now.getMinutes()
        yearTumbler.currentIndex = Math.max(0, Math.min(100, year - 2000))
        monthTumbler.currentIndex = month - 1
        dayTumbler.currentIndex = Math.max(0, Math.min(
            root.daysInMonth(year, month) - 1, day - 1))
        hourTumbler.currentIndex = hour
        minuteTumbler.currentIndex = minute
        open()
    }

    function selectedValue() {
        return String(2000 + yearTumbler.currentIndex) + "-"
            + twoDigits(monthTumbler.currentIndex + 1) + "-"
            + twoDigits(dayTumbler.currentIndex + 1) + " "
            + twoDigits(hourTumbler.currentIndex) + ":"
            + twoDigits(minuteTumbler.currentIndex)
    }

    contentItem: ColumnLayout {
        spacing: AppTheme.spacingMedium

        Label {
            Layout.fillWidth: true
            text: qsTr("上下滚动各列选择时间")
            color: AppTheme.textSecondaryColor
            horizontalAlignment: Text.AlignHCenter
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 190
            spacing: AppTheme.spacingExtraSmall

            Tumbler {
                id: yearTumbler
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
            }
            Tumbler {
                id: monthTumbler
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
            }
            Tumbler {
                id: dayTumbler
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
            }
            Tumbler {
                id: hourTumbler
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
            }
            Tumbler {
                id: minuteTumbler
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
                onClicked: {
                    root.dateTimeSelected(root.selectedValue())
                    root.close()
                }
            }
        }
    }
}
