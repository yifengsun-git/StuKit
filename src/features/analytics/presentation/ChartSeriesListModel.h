#pragma once

#include <QAbstractListModel>
#include <QString>

#include <vector>

namespace stukit::analytics::presentation {

struct ChartPoint
{
    QString label;
    double value{0.0};
    QString displayValue;
};

struct ChartSeries
{
    QString seriesId;
    QString title;
    QString color;
    std::vector<ChartPoint> points;
};

class ChartSeriesListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        SeriesIdRole = Qt::UserRole + 1,
        TitleRole,
        ColorRole,
        PointsRole
    };
    Q_ENUM(Role)

    explicit ChartSeriesListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void replaceSeries(std::vector<ChartSeries> series);
    void clear();

private:
    std::vector<ChartSeries> m_series;
};

} // namespace stukit::analytics::presentation
