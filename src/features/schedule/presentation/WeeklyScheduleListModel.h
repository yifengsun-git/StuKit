#pragma once

#include "GetWeeklySchedule.h"

#include <QAbstractListModel>
#include <QVariantList>

#include <vector>

namespace stukit::schedule::presentation {

class WeeklyScheduleListModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QVariantList weekdayCounts READ weekdayCounts NOTIFY weekdayCountsChanged)

public:
    enum Role
    {
        CourseIdRole = Qt::UserRole + 1,
        MeetingIdRole,
        CourseNameRole,
        TeacherRole,
        LocationRole,
        ColorRole,
        WeekdayRole,
        WeekdayTextRole,
        StartPeriodRole,
        EndPeriodRole,
        PeriodTextRole,
        StartWeekRole,
        EndWeekRole,
        WeekPatternRole,
        ActiveWeeksRole,
        WeekRangeTextRole
    };
    Q_ENUM(Role)

    explicit WeeklyScheduleListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] QVariantList weekdayCounts() const;

    void replaceItems(std::vector<WeeklyScheduleItem> items);
    void clear();

signals:
    void weekdayCountsChanged();

private:
    [[nodiscard]] static QString weekdayText(Weekday weekday);
    [[nodiscard]] static QString periodText(int startPeriod, int endPeriod);
    [[nodiscard]] static QString weekRangeText(
        int startWeek,
        int endWeek,
        WeekPattern pattern,
        const std::vector<int> &activeWeeks);

    std::vector<WeeklyScheduleItem> m_items;
};

} // namespace stukit::schedule::presentation
