pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Item {
    id: root

    required property var model
    required property var weekDays

    property bool editable: false
    property var periodDefinitions: []
    property bool showDates: true
    property bool showWeekRanges: false
    property string emptyText: qsTr("这一周还没有课程")

    signal courseActivated(var details)
    signal emptyCellActivated(int weekday, int startPeriod)

    property int periodCount: 12
    readonly property real periodLabelWidth: 78
    readonly property real headerHeight: 64
    readonly property real periodHeight: 48
    readonly property real dayWidth: (width - periodLabelWidth) / 7
    readonly property int meetingCount: {
        if (!model || !model.weekdayCounts)
            return 0
        return model.weekdayCounts.reduce(
            (total, count) => total + Number(count), 0)
    }
    readonly property int renderedMeetingCount: courseRepeater.count
    readonly property real tallestCourseHeight: {
        const count = courseRepeater.count
        let tallest = 0
        for (let index = 0; index < count; ++index) {
            const card = courseRepeater.itemAt(index)
            if (card)
                tallest = Math.max(tallest, card.height)
        }
        return tallest
    }

    function periodLabel(periodNumber) {
        for (let index = 0; index < periodDefinitions.length; ++index) {
            const definition = periodDefinitions[index]
            if (Number(definition.periodNumber) === periodNumber)
                return definition.displayText
        }
        return qsTr("第 %1 节").arg(periodNumber)
    }

    objectName: "weeklyScheduleGrid"
    implicitWidth: 900
    implicitHeight: headerHeight + periodCount * periodHeight

    Rectangle {
        anchors.fill: parent
        radius: AppTheme.radiusLarge
        color: AppTheme.surfaceColor
        border.color: AppTheme.borderColor
    }

    Rectangle {
        x: root.periodLabelWidth
        y: 0
        width: root.width - root.periodLabelWidth
        height: root.headerHeight
        color: AppTheme.surfaceVariantColor
        opacity: 0.55
    }

    Repeater {
        model: root.weekDays

        Rectangle {
            id: dayHeader
            required property int index
            required property var modelData

            x: root.periodLabelWidth + index * root.dayWidth
            y: 0
            width: root.dayWidth
            height: root.headerHeight
            color: modelData.isToday
                ? AppTheme.surfaceVariantColor
                : "transparent"

            Column {
                anchors.centerIn: parent
                spacing: 2

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: dayHeader.modelData.weekdayText
                    color: dayHeader.modelData.isToday
                        ? AppTheme.primaryColor
                        : AppTheme.textPrimaryColor
                    font.bold: dayHeader.modelData.isToday
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: root.showDates
                    text: dayHeader.modelData.dateText
                    color: dayHeader.modelData.isToday
                        ? AppTheme.primaryColor
                        : AppTheme.textSecondaryColor
                    font.pixelSize: AppTheme.fontSizeBody - 1
                }
            }
        }
    }

    Repeater {
        model: root.periodCount

        Label {
            required property int index
            x: 0
            y: root.headerHeight + index * root.periodHeight
            width: root.periodLabelWidth
            height: root.periodHeight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: root.periodLabel(index + 1)
            color: AppTheme.textSecondaryColor
            font.pixelSize: AppTheme.fontSizeBody - 1
            wrapMode: Text.WordWrap
        }
    }

    Repeater {
        model: root.periodCount + 1

        Rectangle {
            required property int index
            x: root.periodLabelWidth
            y: root.headerHeight + index * root.periodHeight
            width: root.width - root.periodLabelWidth
            height: 1
            color: AppTheme.borderColor
            opacity: 0.65
        }
    }

    Repeater {
        model: 8

        Rectangle {
            required property int index
            x: root.periodLabelWidth + index * root.dayWidth
            y: 0
            width: 1
            height: root.height
            color: AppTheme.borderColor
            opacity: 0.65
        }
    }

    Repeater {
        model: root.editable ? root.periodCount * 7 : 0

        Item {
            id: editCell

            required property int index

            readonly property int cellWeekday: index % 7 + 1
            readonly property int cellPeriod: Math.floor(index / 7) + 1

            x: root.periodLabelWidth
                + (cellWeekday - 1) * root.dayWidth
            y: root.headerHeight
                + (cellPeriod - 1) * root.periodHeight
            width: root.dayWidth
            height: root.periodHeight
            z: 1

            Rectangle {
                anchors.fill: parent
                anchors.margins: 3
                anchors.topMargin: 5
                radius: AppTheme.radiusSmall
                color: AppTheme.textPrimaryColor
                opacity: cellHover.hovered ? 0.16 : 0

                Behavior on opacity {
                    NumberAnimation { duration: 100 }
                }
            }

            Rectangle {
                anchors.fill: parent
                anchors.margins: 3
                anchors.bottomMargin: 5
                radius: AppTheme.radiusSmall
                color: AppTheme.surfaceVariantColor
                border.width: 1
                border.color: AppTheme.primaryColor
                opacity: cellHover.hovered ? 0.88 : 0

                Behavior on opacity {
                    NumberAnimation { duration: 100 }
                }
            }

            HoverHandler {
                id: cellHover
            }

            TapHandler {
                onTapped: root.emptyCellActivated(
                    editCell.cellWeekday, editCell.cellPeriod)
            }
        }
    }

    Repeater {
        id: courseRepeater
        model: root.model

        CourseMeetingCard {
            x: root.periodLabelWidth
                + Math.max(0, Math.min(6, weekday - 1)) * root.dayWidth + 3
            y: root.headerHeight
                + Math.max(0, startPeriod - 1) * root.periodHeight + 3
            width: Math.max(44, root.dayWidth - 6)
            height: Math.max(
                AppTheme.controlHeight,
                (Math.max(startPeriod, endPeriod) - startPeriod + 1)
                    * root.periodHeight - 6)
            z: 2
            condensed: false
            showWeekRange: root.showWeekRanges
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

    Label {
        anchors.centerIn: parent
        visible: root.meetingCount === 0
        text: root.emptyText
        color: AppTheme.textSecondaryColor
        font.pixelSize: AppTheme.fontSizeSubtitle
    }
}
