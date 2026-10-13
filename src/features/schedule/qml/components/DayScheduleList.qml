pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var model
    required property var weekDays

    signal courseActivated(var details)

    objectName: "dayScheduleList"
    implicitHeight: dayColumn.implicitHeight

    ColumnLayout {
        id: dayColumn
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: AppTheme.spacingSmall

        Repeater {
            model: root.weekDays

            Rectangle {
                id: daySection
                required property var modelData

                property var dayData: modelData
                readonly property int meetingCount: root.model
                    && root.model.weekdayCounts
                    ? Number(root.model.weekdayCounts[dayData.weekday - 1])
                    : 0

                Layout.fillWidth: true
                Layout.preferredHeight: sectionContent.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: dayData.isToday
                    ? AppTheme.surfaceVariantColor
                    : AppTheme.surfaceColor
                border.width: dayData.isToday ? 2 : 1
                border.color: dayData.isToday
                    ? AppTheme.primaryColor
                    : AppTheme.borderColor

                ColumnLayout {
                    id: sectionContent
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    spacing: AppTheme.spacingSmall

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("%1 · %2")
                                .arg(daySection.dayData.weekdayText)
                                .arg(daySection.dayData.fullDateText)
                            color: daySection.dayData.isToday
                                ? AppTheme.primaryColor
                                : AppTheme.textPrimaryColor
                            font.bold: true
                            font.pixelSize: AppTheme.fontSizeSubtitle
                        }

                        Label {
                            visible: daySection.dayData.isToday
                            text: qsTr("今天")
                            color: AppTheme.primaryColor
                            font.bold: true
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: daySection.meetingCount === 0
                        text: qsTr("没有课程")
                        color: AppTheme.textSecondaryColor
                    }

                    Repeater {
                        model: root.model

                        CourseMeetingCard {
                            Layout.fillWidth: true
                            Layout.preferredHeight: visible ? implicitHeight : 0
                            visible: weekday === daySection.dayData.weekday
                            enabled: visible
                            condensed: true
                            onClicked: root.courseActivated({
                                "courseId": courseId,
                                "meetingId": meetingId,
                                "courseName": courseName,
                                "teacher": teacher,
                                "location": location,
                                "courseColor": courseColor,
                                "weekday": weekday,
                                "weekdayText": weekdayText,
                                "startPeriod": startPeriod,
                                "endPeriod": endPeriod,
                                "periodText": periodText,
                                "startWeek": startWeek,
                                "endWeek": endWeek,
                                "weekPattern": weekPattern,
                                "activeWeeks": activeWeeks,
                                "weekRangeText": weekRangeText
                            })
                        }
                    }
                }
            }
        }
    }
}
