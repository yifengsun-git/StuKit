#include "ChartSeriesListModel.h"

#include <QVariantList>
#include <QVariantMap>

#include <utility>

namespace stukit::analytics::presentation {

ChartSeriesListModel::ChartSeriesListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ChartSeriesListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_series.size());
}

QVariant ChartSeriesListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= static_cast<int>(m_series.size())) {
        return {};
    }

    const ChartSeries &series = m_series.at(
        static_cast<std::size_t>(index.row()));
    switch (role) {
    case SeriesIdRole:
        return series.seriesId;
    case TitleRole:
        return series.title;
    case ColorRole:
        return series.color;
    case PointsRole: {
        QVariantList points;
        points.reserve(static_cast<qsizetype>(series.points.size()));
        for (const ChartPoint &point : series.points) {
            points.push_back(QVariantMap{
                {QStringLiteral("label"), point.label},
                {QStringLiteral("value"), point.value},
                {QStringLiteral("displayValue"), point.displayValue},
            });
        }
        return points;
    }
    default:
        return {};
    }
}

QHash<int, QByteArray> ChartSeriesListModel::roleNames() const
{
    return {
        {SeriesIdRole, QByteArrayLiteral("seriesId")},
        {TitleRole, QByteArrayLiteral("title")},
        {ColorRole, QByteArrayLiteral("color")},
        {PointsRole, QByteArrayLiteral("points")},
    };
}

void ChartSeriesListModel::replaceSeries(std::vector<ChartSeries> series)
{
    beginResetModel();
    m_series = std::move(series);
    endResetModel();
}

void ChartSeriesListModel::clear()
{
    replaceSeries({});
}

} // namespace stukit::analytics::presentation
