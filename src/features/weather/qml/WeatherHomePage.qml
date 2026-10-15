pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel
    readonly property bool compactLayout:
        width < AppTheme.breakpointCompact
    readonly property int pagePadding: AppTheme.pagePadding(width)
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
        id: pageScrollView

        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: Math.min(
                pageScrollView.availableWidth - root.pagePadding * 2,
                AppTheme.contentMaxWidth)
            x: Math.max(root.pagePadding,
                        (pageScrollView.availableWidth - width) / 2)
            spacing: AppTheme.spacingMedium

            Item {
                Layout.preferredHeight: root.pagePadding
            }

            AppSectionCard {
                objectName: "weatherSearchCard"
                Layout.fillWidth: true
                title: qsTr("天气")
                description: qsTr("搜索并保存城市，查看当前天气与未来 7 天预报")
                accentColor: AppTheme.primaryColor

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.compactLayout ? 1 : 2
                    rowSpacing: AppTheme.spacingSmall
                    columnSpacing: AppTheme.spacingSmall

                    AppTextField {
                        id: citySearchField

                        objectName: "weatherCitySearchField"
                        Layout.fillWidth: true
                        placeholderText: qsTr("输入城市名，例如：上海或 Shanghai")
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
            }

            AppSectionCard {
                objectName: "weatherSearchResultsCard"
                Layout.fillWidth: true
                visible: root.searchResultCount > 0
                title: qsTr("选择城市")
                description: qsTr("请选择最符合的地点，选择后会立即加载天气。")

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: AppTheme.spacingExtraSmall

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

            AppSectionCard {
                objectName: "weatherErrorCard"
                Layout.fillWidth: true
                visible: root.errorShown
                title: qsTr("天气更新失败")
                accentColor: AppTheme.errorColor

                Label {
                    id: weatherErrorLabel

                    objectName: "weatherErrorLabel"
                    Layout.fillWidth: true
                    text: root.viewModel
                        ? root.viewModel.errorMessage : ""
                    color: AppTheme.errorColor
                    wrapMode: Text.WordWrap
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: root.hasLocation

                Label {
                    Layout.fillWidth: true
                    text: root.viewModel && root.viewModel.loading
                        ? qsTr("正在刷新天气…")
                        : qsTr("已保存：%1").arg(
                            root.viewModel ? root.viewModel.locationName : "")
                    color: AppTheme.textSecondaryColor
                    elide: Text.ElideRight
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

            AppSectionCard {
                objectName: "weatherCacheBanner"
                Layout.fillWidth: true
                visible: root.viewModel
                    && root.viewModel.usingCachedForecast
                title: qsTr("正在使用离线天气")
                description: qsTr("显示上次缓存的数据 · %1")
                    .arg(root.viewModel
                        ? root.viewModel.updatedText : "")
                accentColor: AppTheme.primaryColor
                contentPadding: AppTheme.spacingMedium
            }

            AppSectionCard {
                objectName: "weatherEmptyState"
                Layout.fillWidth: true
                visible: root.emptyStateShown
                title: qsTr("还没有选择城市")
                description: qsTr("在上方输入至少两个字符，然后从结果中选择地点。城市会保存在本机，下次启动可以直接刷新。")
            }

            CurrentWeatherCard {
                Layout.fillWidth: true
                visible: root.forecastShown
                viewModel: root.viewModel
                compactLayout: root.compactLayout
            }

            Label {
                Layout.fillWidth: true
                visible: root.forecastShown
                text: qsTr("未来 7 天")
                color: AppTheme.textPrimaryColor
                font.pixelSize: AppTheme.fontSizeHeading
                font.bold: true
            }

            DailyForecastGrid {
                Layout.fillWidth: true
                visible: root.forecastShown
                forecastModel: root.viewModel
                    ? root.viewModel.dailyForecasts : []
                compactLayout: root.compactLayout
            }

            Label {
                objectName: "weatherAttributionLabel"
                Layout.fillWidth: true
                Layout.bottomMargin: root.pagePadding
                visible: root.forecastShown
                text: qsTr("天气服务由 <a href=\"https://www.qweather.com/\">QWeather 和风天气</a> 提供 · <a href=\"https://developer.qweather.com/attribution.html\">数据来源</a>")
                textFormat: Text.RichText
                linkColor: AppTheme.primaryColor
                color: AppTheme.textSecondaryColor
                font.pixelSize: AppTheme.fontSizeCaption
                horizontalAlignment: root.compactLayout
                    ? Text.AlignLeft : Text.AlignRight
                wrapMode: Text.WordWrap
                onLinkActivated: link => Qt.openUrlExternally(link)
            }

            Item {
                Layout.preferredHeight: root.forecastShown
                    ? 0 : root.pagePadding
            }
        }
    }
}
