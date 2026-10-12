#pragma once

#include "GetWeeklySchedule.h"

#include <QAbstractListModel>

#include <vector>

namespace stukit::schedule::presentation {

class WeeklyScheduleListModel final : public QAbstractListModel
{
    Q_OBJECT

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
        PeriodTextRole
    };
    Q_ENUM(Role)

    explicit WeeklyScheduleListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void replaceItems(std::vector<WeeklyScheduleItem> items);
    void clear();

private:
    [[nodiscard]] static QString weekdayText(Weekday weekday);
    [[nodiscard]] static QString periodText(int startPeriod, int endPeriod);

    std::vector<WeeklyScheduleItem> m_items;
};

} // namespace stukit::schedule::presentation
