import QtQuick
import QtQuick.Controls
// qmllint disable import
import QtCharts

// qmllint enable import

Item {
    id: root

    property var seriesData: []
    property string firstSeriesId: "income"
    property string secondSeriesId: "expense"
    property string emptyText: qsTr("当前范围没有可绘制的数据")

    function seriesById(seriesId) {
        for (let index = 0; index < seriesData.length; ++index) {
            if (seriesData[index].seriesId === seriesId)
                return seriesData[index];
        }
        return null;
    }

    function pointValues(series) {
        const values = [];
        if (!series)
            return values;
        for (let index = 0; index < series.points.length; ++index)
            values.push(series.points[index].value);
        return values;
    }

    function pointLabels(series) {
        const labels = [];
        if (!series)
            return labels;
        for (let index = 0; index < series.points.length; ++index)
            labels.push(series.points[index].label.substring(2));
        return labels;
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

    function anyValues(first, second) {
        const groups = [first, second];
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

    readonly property var firstSeries: seriesById(firstSeriesId)
    readonly property var secondSeries: seriesById(secondSeriesId)
    readonly property bool hasValues: anyValues(firstSeries, secondSeries)

    implicitHeight: 320

    ChartView {
        anchors.fill: parent
        visible: root.hasValues
        antialiasing: true
        animationOptions: ChartView.SeriesAnimations
        backgroundColor: "transparent"
        plotAreaColor: "transparent"
        legend.alignment: Qt.AlignBottom
        legend.labelColor: AppTheme.textSecondaryColor

        BarCategoryAxis {
            id: categoryAxis

            categories: root.pointLabels(root.firstSeries)
            labelsColor: AppTheme.textSecondaryColor
            labelsAngle: categories.length > 12 ? -60 : -35
            gridVisible: false
        }

        ValueAxis {
            id: amountAxis

            min: 0
            max: root.maximumValue(root.firstSeries, root.secondSeries)
            labelFormat: "%.0f"
            labelsColor: AppTheme.textSecondaryColor
            gridLineColor: AppTheme.borderColor
        }

        BarSeries {
            axisX: categoryAxis
            axisY: amountAxis

            BarSet {
                label: root.firstSeries ? root.firstSeries.title : ""
                color: root.firstSeries ? root.firstSeries.color : "#2E7D32"
                values: root.pointValues(root.firstSeries)
            }

            BarSet {
                label: root.secondSeries ? root.secondSeries.title : ""
                color: root.secondSeries ? root.secondSeries.color : "#C62828"
                values: root.pointValues(root.secondSeries)
            }
        }
    }

    Label {
        objectName: "barChartEmptyState"
        anchors.centerIn: parent
        visible: !root.hasValues
        text: root.emptyText
        color: AppTheme.textSecondaryColor
    }
}
