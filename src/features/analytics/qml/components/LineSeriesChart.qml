pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// qmllint disable import
import QtCharts

// qmllint enable import

Item {
    id: root

    property var seriesData: []
    property string seriesId: "balance"
    property string secondSeriesId: ""
    property string emptyText: qsTr("当前范围没有可绘制的数据")

    function seriesById(identifier) {
        for (let index = 0; index < seriesData.length; ++index) {
            if (seriesData[index].seriesId === identifier)
                return seriesData[index];
        }
        return null;
    }

    function minimumValue(first, second) {
        let minimum = 0;
        const groups = [first, second];
        for (let groupIndex = 0; groupIndex < groups.length; ++groupIndex) {
            const series = groups[groupIndex];
            if (!series)
                continue;
            for (let index = 0; index < series.points.length; ++index)
                minimum = Math.min(minimum, series.points[index].value);
        }
        return minimum < 0 ? minimum * 1.15 : -1;
    }

    function maximumValue(first, second) {
        let maximum = 0;
        const groups = [first, second];
        for (let groupIndex = 0; groupIndex < groups.length; ++groupIndex) {
            const series = groups[groupIndex];
            if (!series)
                continue;
            for (let index = 0; index < series.points.length; ++index)
                maximum = Math.max(maximum, series.points[index].value);
        }
        return maximum > 0 ? maximum * 1.15 : 1;
    }

    function rebuildSeries() {
        balanceLine.clear();
        comparisonLine.clear();
        const first = seriesById(seriesId);
        const second = seriesById(secondSeriesId);
        if (first) {
            for (let index = 0; index < first.points.length; ++index)
                balanceLine.append(index, first.points[index].value);
        }
        if (second) {
            for (let index = 0; index < second.points.length; ++index)
                comparisonLine.append(index, second.points[index].value);
        }
    }

    readonly property var selectedSeries: seriesById(seriesId)
    readonly property var selectedSecondSeries: seriesById(secondSeriesId)
    readonly property bool hasValues: {
        const groups = [selectedSeries, selectedSecondSeries];
        for (let groupIndex = 0; groupIndex < groups.length; ++groupIndex) {
            const series = groups[groupIndex];
            if (!series)
                continue;
            for (let index = 0; index < series.points.length; ++index) {
                if (series.points[index].value !== 0)
                    return true;
            }
        }
        return false;
    }

    onSeriesDataChanged: Qt.callLater(rebuildSeries)
    onSeriesIdChanged: Qt.callLater(rebuildSeries)
    onSecondSeriesIdChanged: Qt.callLater(rebuildSeries)
    Component.onCompleted: rebuildSeries()

    implicitHeight: 320

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ChartView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.hasValues
            antialiasing: true
            animationOptions: ChartView.SeriesAnimations
            backgroundColor: "transparent"
            plotAreaColor: "transparent"
            legend.visible: root.selectedSecondSeries !== null
            legend.alignment: Qt.AlignBottom
            legend.labelColor: AppTheme.textSecondaryColor

            ValueAxis {
                id: monthAxis

                min: 0
                max: root.selectedSeries ? Math.max(1, root.selectedSeries.points.length - 1) : 1
                labelsVisible: false
                gridVisible: false
            }

            ValueAxis {
                id: balanceAxis

                min: root.minimumValue(root.selectedSeries, root.selectedSecondSeries)
                max: root.maximumValue(root.selectedSeries, root.selectedSecondSeries)
                labelFormat: "%.0f"
                labelsColor: AppTheme.textSecondaryColor
                gridLineColor: AppTheme.borderColor
            }

            LineSeries {
                id: balanceLine

                name: root.selectedSeries ? root.selectedSeries.title : ""
                color: root.selectedSeries ? root.selectedSeries.color : AppTheme.primaryColor
                width: 3
                pointsVisible: true
                axisX: monthAxis
                axisY: balanceAxis
            }

            LineSeries {
                id: comparisonLine

                name: root.selectedSecondSeries ? root.selectedSecondSeries.title : ""
                color: root.selectedSecondSeries ? root.selectedSecondSeries.color : "transparent"
                width: 3
                pointsVisible: true
                visible: root.selectedSecondSeries !== null
                axisX: monthAxis
                axisY: balanceAxis
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 64
            Layout.rightMargin: 20
            visible: root.hasValues && root.selectedSeries
            spacing: 0

            Repeater {
                model: root.selectedSeries ? root.selectedSeries.points : []

                delegate: Item {
                    id: monthLabelItem

                    required property int index
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.preferredHeight: 30

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: monthLabelItem.index === 0 || monthLabelItem.index === root.selectedSeries.points.length - 1 || monthLabelItem.index % Math.max(1, Math.ceil(root.selectedSeries.points.length / 6)) === 0
                        text: monthLabelItem.modelData.label.substring(2)
                        color: AppTheme.textSecondaryColor
                        font.pixelSize: 11
                    }
                }
            }
        }
    }

    Label {
        objectName: "lineChartEmptyState"
        anchors.centerIn: parent
        visible: !root.hasValues
        text: root.emptyText
        color: AppTheme.textSecondaryColor
    }
}
