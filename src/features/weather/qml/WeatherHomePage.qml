pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel
    readonly property bool compactLayout: width <= 640
    readonly property bool hasLocation: viewModel
        && viewModel.locationName.length > 0
    readonly property int searchResultCount: viewModel
        ? viewModel.searchResults.length : 0
    readonly property bool emptyStateShown: !hasLocation
        && searchResultCount === 0
        && viewModel
        && !viewModel.searching
    readonly property bool errorShown: viewModel && viewModel.hasError
    readonly property bool forecastShown: viewModel && viewModel.hasForecast

    objectName: "weatherHomePage"

    function selectSearchResult(index) {
        if (root.viewModel)
            root.viewModel.selectLocation(index)
    }

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: root.width
            spacing: AppTheme.spacingMedium

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.rightMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.topMargin: AppTheme.spacingLarge
                spacing: AppTheme.spacingExtraSmall

                Label {
                    Layout.fillWidth: true
                    text: qsTr("天气")
                    color: AppTheme.textPrimaryColor
                    font.pixelSize: AppTheme.fontSizeDisplay
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("搜索城市，查看当前天气与未来 7 天预报")
                    color: AppTheme.textSecondaryColor
                    wrapMode: Text.WordWrap
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.rightMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                columns: root.compactLayout ? 1 : 2
                rowSpacing: AppTheme.spacingSmall
                columnSpacing: AppTheme.spacingSmall

                TextField {
                    id: citySearchField

                    objectName: "weatherCitySearchField"
                    Layout.fillWidth: true
                    placeholderText: qsTr("输入城市名，例如：上海或 Shanghai")
                    selectByMouse: true
                    enabled: root.viewModel && !root.viewModel.searching
                    onAccepted: {
                        if (searchButton.enabled)
                            root.viewModel.search(text)
                    }
                }

                AppButton {
                    id: searchButton

                    objectName: "weatherSearchButton"
                    Layout.fillWidth: root.compactLayout
                    Layout.preferredWidth: root.compactLayout ? -1 : 120
                    text: root.viewModel && root.viewModel.searching
                        ? qsTr("搜索中…") : qsTr("搜索")
                    highlighted: true
                    enabled: root.viewModel
                        && !root.viewModel.searching
                        && citySearchField.text.trim().length >= 2
                    onClicked: root.viewModel.search(citySearchField.text)
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.rightMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.preferredHeight: searchResultsColumn.implicitHeight
                    + AppTheme.spacingSmall * 2
                visible: root.searchResultCount > 0
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor

                ColumnLayout {
                    id: searchResultsColumn

                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingSmall
                    spacing: AppTheme.spacingExtraSmall

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("选择城市")
                        color: AppTheme.textSecondaryColor
                        font.pixelSize: 12
                    }

                    Repeater {
                        id: searchResultsRepeater

                        objectName: "weatherSearchResultsRepeater"

                        model: root.viewModel
                            ? root.viewModel.searchResults : []

                        delegate: AppButton {
                            required property var modelData

                            objectName: "weatherSearchResult-"
                                + modelData.index
                            Layout.fillWidth: true
                            flat: true
                            text: modelData.subtitle.length > 0
                                ? qsTr("%1 · %2")
                                    .arg(modelData.name)
                                    .arg(modelData.subtitle)
                                : modelData.name
                            onClicked: root.selectSearchResult(
                                modelData.index)
                        }
                    }
                }
            }

            Rectangle {
                objectName: "weatherEmptyState"
                Layout.fillWidth: true
                Layout.leftMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.rightMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.preferredHeight: weatherErrorLabel.implicitHeight
                    + AppTheme.spacingMedium * 2
                visible: root.errorShown
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.errorColor

                Label {
                    id: weatherErrorLabel

                    objectName: "weatherErrorLabel"
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    text: root.viewModel
                        ? root.viewModel.errorMessage : ""
                    color: AppTheme.errorColor
                    wrapMode: Text.WordWrap
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.rightMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                visible: root.hasLocation

                Label {
                    Layout.fillWidth: true
                    text: root.viewModel && root.viewModel.loading
                        ? qsTr("正在刷新天气…") : ""
                    color: AppTheme.textSecondaryColor
                }

                BusyIndicator {
                    objectName: "weatherBusyIndicator"
                    running: root.viewModel && root.viewModel.loading
                    visible: running
                    implicitWidth: 36
                    implicitHeight: 36
                }

                AppButton {
                    objectName: "weatherRefreshButton"
                    text: qsTr("刷新")
                    enabled: root.hasLocation
                        && root.viewModel
                        && !root.viewModel.loading
                    onClicked: root.viewModel.refresh()
                }
            }

            Rectangle {
                objectName: "weatherCacheBanner"
                Layout.fillWidth: true
                Layout.leftMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.rightMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.preferredHeight: cacheLabel.implicitHeight
                    + AppTheme.spacingSmall * 2
                visible: root.viewModel
                    && root.viewModel.usingCachedForecast
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.primaryColor

                Label {
                    id: cacheLabel

                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingSmall
                    text: qsTr("正在显示上次缓存的数据 · %1")
                        .arg(root.viewModel
                            ? root.viewModel.updatedText : "")
                    color: AppTheme.textSecondaryColor
                    wrapMode: Text.WordWrap
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.rightMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.preferredHeight: emptyStateContent.implicitHeight
                    + AppTheme.spacingLarge * 2
                visible: root.emptyStateShown
                radius: AppTheme.radiusLarge
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor

                ColumnLayout {
                    id: emptyStateContent

                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingLarge
                    spacing: AppTheme.spacingSmall

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("还没有选择城市")
                        color: AppTheme.textPrimaryColor
                        font.pixelSize: AppTheme.fontSizeSubtitle
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("在上方输入至少两个字符，然后从结果中选择地点")
                        color: AppTheme.textSecondaryColor
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }
            }

            CurrentWeatherCard {
                Layout.fillWidth: true
                Layout.leftMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.rightMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                visible: root.forecastShown
                viewModel: root.viewModel
                compactLayout: root.compactLayout
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.rightMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                visible: root.forecastShown
                text: qsTr("未来 7 天")
                color: AppTheme.textPrimaryColor
                font.pixelSize: AppTheme.fontSizeHeading
                font.bold: true
            }

            DailyForecastGrid {
                Layout.fillWidth: true
                Layout.leftMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.rightMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                visible: root.forecastShown
                forecastModel: root.viewModel
                    ? root.viewModel.dailyForecasts : []
                compactLayout: root.compactLayout
            }

            Label {
                objectName: "weatherAttributionLabel"
                Layout.fillWidth: true
                Layout.leftMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.rightMargin: root.compactLayout
                    ? AppTheme.spacingMedium : AppTheme.spacingLarge
                Layout.bottomMargin: AppTheme.spacingLarge
                visible: root.forecastShown
                text: qsTr("天气数据：<a href=\"https://open-meteo.com/\">Open-Meteo</a> · CC BY 4.0")
                textFormat: Text.RichText
                linkColor: AppTheme.primaryColor
                color: AppTheme.textSecondaryColor
                font.pixelSize: 12
                horizontalAlignment: Text.AlignRight
                wrapMode: Text.WordWrap
                onLinkActivated: link => Qt.openUrlExternally(link)
            }
        }
    }
}
