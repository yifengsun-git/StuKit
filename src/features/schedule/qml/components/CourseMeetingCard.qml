pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AbstractButton {
    id: root

    required property string courseId
    required property string meetingId
    required property string courseName
    required property string teacher
    required property string location
    required property string courseColor
    required property int weekday
    required property string weekdayText
    required property int startPeriod
    required property int endPeriod
    required property string periodText
    required property int startWeek
    required property int endWeek
    required property int weekPattern
    required property var activeWeeks
    required property string weekRangeText

    property bool condensed: false
    property bool showWeekRange: false

    objectName: "courseMeetingCard"
    implicitHeight: condensed ? 72 : 64
    padding: AppTheme.spacingSmall
    hoverEnabled: true
    activeFocusOnTab: true
    Accessible.name: qsTr("%1，%2，%3")
        .arg(courseName)
        .arg(weekdayText)
        .arg(periodText)

    background: Item {
        Rectangle {
            x: 2
            y: 3
            width: parent.width - 4
            height: parent.height - 3
            radius: AppTheme.radiusSmall
            color: AppTheme.textPrimaryColor
            opacity: root.hovered ? 0.18 : 0

            Behavior on opacity {
                NumberAnimation { duration: 100 }
            }
        }

        Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: root.hovered ? 3 : 0
            radius: AppTheme.radiusSmall
            color: root.down
                ? AppTheme.surfaceVariantColor
                : root.hovered
                    ? AppTheme.surfaceVariantColor
                : AppTheme.surfaceColor
            border.width: root.activeFocus ? 2 : 1
            border.color: root.activeFocus
                ? AppTheme.primaryColor
                : root.courseColor

            Behavior on anchors.bottomMargin {
                NumberAnimation { duration: 100 }
            }
        }
    }

    contentItem: RowLayout {
        spacing: AppTheme.spacingSmall

        Rectangle {
            Layout.preferredWidth: 6
            Layout.fillHeight: true
            radius: 3
            color: root.courseColor
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: root.courseName
                color: AppTheme.textPrimaryColor
                font.bold: true
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                visible: root.condensed || root.height >= 58
                text: root.showWeekRange
                    ? qsTr("%1 · %2")
                        .arg(root.periodText)
                        .arg(root.weekRangeText)
                    : root.periodText
                color: AppTheme.textSecondaryColor
                font.pixelSize: AppTheme.fontSizeBody - 1
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                visible: root.height >= 82
                    && (root.location.length > 0 || root.teacher.length > 0)
                text: [root.location, root.teacher]
                    .filter(value => value.length > 0)
                    .join(" · ")
                color: AppTheme.textSecondaryColor
                font.pixelSize: AppTheme.fontSizeBody - 1
                elide: Text.ElideRight
            }
        }
    }
}
