#pragma once

#include "GetScheduleSetup.h"

#include <QAbstractListModel>

#include <vector>

namespace stukit::schedule::presentation {

class AcademicTermListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        TermIdRole = Qt::UserRole + 1,
        NameRole,
        StartDateRole,
        StartDateTextRole,
        TotalWeeksRole,
        ArchivedRole
    };
    Q_ENUM(Role)

    explicit AcademicTermListModel(QObject *parent = nullptr);
    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    void replaceItems(std::vector<AcademicTerm> items);
    void clear();

private:
    std::vector<AcademicTerm> m_items;
};

class CourseListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        CourseIdRole = Qt::UserRole + 1,
        TermIdRole,
        NameRole,
        TeacherRole,
        LocationRole,
        ColorRole
    };
    Q_ENUM(Role)

    explicit CourseListModel(QObject *parent = nullptr);
    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    void replaceItems(std::vector<Course> items);
    void clear();

private:
    std::vector<Course> m_items;
};

class CourseMeetingListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        MeetingIdRole = Qt::UserRole + 1,
        CourseIdRole,
        WeekdayRole,
        WeekdayTextRole,
        StartPeriodRole,
        EndPeriodRole,
        StartWeekRole,
        EndWeekRole,
        WeekPatternRole,
        WeekPatternTextRole,
        SummaryRole
    };
    Q_ENUM(Role)

    explicit CourseMeetingListModel(QObject *parent = nullptr);
    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    void replaceItems(std::vector<CourseMeeting> items);
    void clear();

private:
    [[nodiscard]] static QString weekdayText(Weekday weekday);
    [[nodiscard]] static QString weekPatternText(WeekPattern pattern);
    std::vector<CourseMeeting> m_items;
};

} // namespace stukit::schedule::presentation
