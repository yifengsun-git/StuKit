pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// qmllint disable import
import QtCharts

// qmllint enable import

Item {
    id: root

    property var model: null
    property string emptyText: qsTr("本月暂无分类数据")
    property bool compactLayout: false
    readonly property bool hasData: sliceItems.count > 0
    readonly property var palette: ["#2563EB", "#7C3AED", "#DB2777", "#EA580C", "#CA8A04", "#16A34A", "#0891B2", "#4F46E5"]

    function clearSlices() {
        pieSeries.clear();
    }

    implicitHeight: compactLayout
        ? 210 + Math.max(1, sliceItems.count) * 30
        : 330

    Connections {
        target: root.model

        function onModelAboutToBeReset() {
            root.clearSlices();
        }
    }

    GridLayout {
        anchors.fill: parent
        columns: root.compactLayout ? 1 : 2
        rowSpacing: AppTheme.spacingSmall
        columnSpacing: AppTheme.spacingMedium

        ChartView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: root.compactLayout ? 200 : -1
            visible: sliceItems.count > 0
            antialiasing: true
            animationOptions: ChartView.SeriesAnimations
            backgroundColor: "transparent"
            plotAreaColor: "transparent"
            legend.visible: false

            PieSeries {
                id: pieSeries

                size: 0.9
                holeSize: 0.58
            }
        }

        ColumnLayout {
            Layout.fillWidth: root.compactLayout
            Layout.preferredWidth: root.compactLayout ? -1 : 150
            Layout.alignment: Qt.AlignVCenter
            visible: sliceItems.count > 0
            spacing: AppTheme.spacingSmall

            Repeater {
                model: root.model

                delegate: RowLayout {
                    id: legendRow

                    required property int index
                    required property string name
                    required property string percentageText
                    required property bool archived

                    Layout.fillWidth: true

                    Rectangle {
                        Layout.preferredWidth: 10
                        Layout.preferredHeight: 10
                        radius: 2
                        color: root.palette[
                            Math.max(0, legendRow.index) % root.palette.length]
                    }

                    Label {
                        Layout.fillWidth: true
                        text: legendRow.archived ? qsTr("%1（归档）").arg(legendRow.name) : legendRow.name
                        color: AppTheme.textPrimaryColor
                        elide: Text.ElideRight
                    }

                    Label {
                        text: legendRow.percentageText
                        color: AppTheme.textSecondaryColor
                    }
                }
            }
        }
    }

    Repeater {
        id: sliceItems

        model: root.model

        delegate: Item {
            id: sliceItem

            required property int index
            required property string name
            required property double amountCents

            Component.onCompleted: {
                const slice = pieSeries.append(sliceItem.name, sliceItem.amountCents);
                slice.color = root.palette[sliceItem.index % root.palette.length];
                slice.borderColor = AppTheme.surfaceColor;
                slice.borderWidth = 2;
            }
        }
    }

    Label {
        objectName: "donutEmptyState"
        anchors.centerIn: parent
        visible: sliceItems.count === 0
        text: root.emptyText
        color: AppTheme.textSecondaryColor
    }
}
