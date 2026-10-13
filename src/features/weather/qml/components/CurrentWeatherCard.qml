import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var viewModel
    property bool compactLayout: false

    objectName: "currentWeatherCard"
    implicitHeight: content.implicitHeight + AppTheme.spacingLarge * 2
    radius: AppTheme.radiusLarge
    color: AppTheme.surfaceColor
    border.color: AppTheme.borderColor

    ColumnLayout {
        id: content

        anchors.fill: parent
        anchors.margins: AppTheme.spacingLarge
        spacing: AppTheme.spacingMedium

        RowLayout {
            Layout.fillWidth: true
            spacing: AppTheme.spacingMedium

            ColumnLayout {
                Layout.fillWidth: true
                spacing: AppTheme.spacingExtraSmall

                Label {
                    Layout.fillWidth: true
                    text: root.viewModel ? root.viewModel.locationName : ""
                    color: AppTheme.textPrimaryColor
                    font.pixelSize: AppTheme.fontSizeHeading
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    visible: text.length > 0
                    text: root.viewModel
                        ? root.viewModel.locationSubtitle : ""
                    color: AppTheme.textSecondaryColor
                    wrapMode: Text.WordWrap
                }
            }

            Label {
                text: root.viewModel
                    ? root.viewModel.temperatureText : ""
                color: AppTheme.primaryColor
                font.pixelSize: root.compactLayout ? 32 : 42
                font.bold: true
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: root.viewModel ? root.viewModel.conditionText : ""
                color: AppTheme.textPrimaryColor
                font.pixelSize: AppTheme.fontSizeSubtitle
                font.bold: true
            }

            Label {
                text: root.viewModel
                    ? root.viewModel.apparentTemperatureText : ""
                color: AppTheme.textSecondaryColor
            }
        }

        GridLayout {
            id: weatherMetricGrid

            objectName: "weatherMetricGrid"
            Layout.fillWidth: true
            columns: root.compactLayout ? 2 : 3
            rowSpacing: AppTheme.spacingSmall
            columnSpacing: AppTheme.spacingSmall

            Repeater {
                model: [
                    {
                        "label": qsTr("湿度"),
                        "value": root.viewModel
                            ? root.viewModel.humidityText : ""
                    },
                    {
                        "label": qsTr("降水"),
                        "value": root.viewModel
                            ? root.viewModel.precipitationText : ""
                    },
                    {
                        "label": qsTr("风况"),
                        "value": root.viewModel
                            ? root.viewModel.windText : ""
                    }
                ]

                delegate: Rectangle {
                    id: metricCard

                    required property var modelData

                    Layout.fillWidth: true
                    Layout.preferredHeight: 68
                    radius: AppTheme.radiusSmall
                    color: AppTheme.surfaceVariantColor
                    border.color: AppTheme.borderColor

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: AppTheme.spacingSmall
                        spacing: AppTheme.spacingExtraSmall

                        Label {
                            Layout.fillWidth: true
                            text: metricCard.modelData.label
                            color: AppTheme.textSecondaryColor
                            font.pixelSize: 12
                        }

                        Label {
                            Layout.fillWidth: true
                            text: metricCard.modelData.value
                            color: AppTheme.textPrimaryColor
                            font.bold: true
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.viewModel ? root.viewModel.updatedText : ""
            color: AppTheme.textSecondaryColor
            font.pixelSize: 12
            horizontalAlignment: Text.AlignRight
        }
    }
}
