pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GridLayout {
    id: root

    property var forecastModel: []
    property bool compactLayout: false
    readonly property int forecastCount: forecastRepeater.count

    objectName: "dailyForecastGrid"
    columns: compactLayout ? 1 : Math.max(1, forecastRepeater.count)
    rowSpacing: AppTheme.spacingSmall
    columnSpacing: AppTheme.spacingSmall

    Repeater {
        id: forecastRepeater
        model: root.forecastModel

        delegate: Rectangle {
            id: card

            required property var modelData

            Layout.fillWidth: true
            Layout.preferredHeight: root.compactLayout ? 88 : 150
            radius: AppTheme.radiusMedium
            color: AppTheme.surfaceColor
            border.color: AppTheme.borderColor

            GridLayout {
                anchors.fill: parent
                anchors.margins: AppTheme.spacingSmall
                columns: root.compactLayout ? 4 : 1
                rowSpacing: AppTheme.spacingExtraSmall
                columnSpacing: AppTheme.spacingSmall

                Label {
                    Layout.fillWidth: true
                    text: card.modelData.weekday
                    color: AppTheme.textPrimaryColor
                    font.bold: true
                    horizontalAlignment: root.compactLayout
                        ? Text.AlignLeft : Text.AlignHCenter
                }

                Label {
                    Layout.fillWidth: true
                    text: card.modelData.date
                    color: AppTheme.textSecondaryColor
                    font.pixelSize: 12
                    horizontalAlignment: root.compactLayout
                        ? Text.AlignLeft : Text.AlignHCenter
                }

                Label {
                    Layout.fillWidth: true
                    text: card.modelData.condition
                    color: AppTheme.textPrimaryColor
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: AppTheme.spacingExtraSmall

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("%1 / %2")
                            .arg(card.modelData.minimum)
                            .arg(card.modelData.maximum)
                        color: AppTheme.textPrimaryColor
                        font.bold: true
                        horizontalAlignment: Text.AlignRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("降水 %1").arg(
                            card.modelData.precipitationProbability)
                        color: AppTheme.textSecondaryColor
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }
        }
    }
}
