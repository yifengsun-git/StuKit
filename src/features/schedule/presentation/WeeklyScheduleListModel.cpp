#include "WeeklyScheduleListModel.h"

#include <utility>

namespace stukit::schedule::presentation {

WeeklyScheduleListModel::WeeklyScheduleListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int WeeklyScheduleListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant WeeklyScheduleListModel::data(
    const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
        || index.row() >= rowCount()) {
        return {};
    }

    const WeeklyScheduleItem &item = m_items.at(
        static_cast<std::size_t>(index.row()));
    switch (role) {
    case Qt::DisplayRole:
    case CourseNameRole:
        return item.courseName;
    case CourseIdRole:
        return item.courseId.toString(QUuid::WithoutBraces);
    case MeetingIdRole:
        return item.meetingId.toString(QUuid::WithoutBraces);
    case TeacherRole:
        return item.teacher;
    case LocationRole:
        return item.location;
    case ColorRole:
        return item.color;
    case WeekdayRole:
        return static_cast<int>(item.weekday);
    case WeekdayTextRole:
        return weekdayText(item.weekday);
    case StartPeriodRole:
        return item.startPeriod;
    case EndPeriodRole:
        return item.endPeriod;
    case PeriodTextRole:
        return periodText(item.startPeriod, item.endPeriod);
    default:
        return {};
    }
}

QHash<int, QByteArray> WeeklyScheduleListModel::roleNames() const
{
    return {
        {CourseIdRole, QByteArrayLiteral("courseId")},
        {MeetingIdRole, QByteArrayLiteral("meetingId")},
        {CourseNameRole, QByteArrayLiteral("courseName")},
        {TeacherRole, QByteArrayLiteral("teacher")},
        {LocationRole, QByteArrayLiteral("location")},
        {ColorRole, QByteArrayLiteral("courseColor")},
        {WeekdayRole, QByteArrayLiteral("weekday")},
        {WeekdayTextRole, QByteArrayLiteral("weekdayText")},
        {StartPeriodRole, QByteArrayLiteral("startPeriod")},
        {EndPeriodRole, QByteArrayLiteral("endPeriod")},
        {PeriodTextRole, QByteArrayLiteral("periodText")},
    };
}

void WeeklyScheduleListModel::replaceItems(
    std::vector<WeeklyScheduleItem> items)
{
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
}

void WeeklyScheduleListModel::clear()
{
    if (m_items.empty()) {
        return;
    }

    beginResetModel();
    m_items.clear();
    endResetModel();
}

QString WeeklyScheduleListModel::weekdayText(Weekday weekday)
{
    switch (weekday) {
    case Weekday::Monday:
        return QStringLiteral("周一");
    case Weekday::Tuesday:
        return QStringLiteral("周二");
    case Weekday::Wednesday:
        return QStringLiteral("周三");
    case Weekday::Thursday:
        return QStringLiteral("周四");
    case Weekday::Friday:
        return QStringLiteral("周五");
    case Weekday::Saturday:
        return QStringLiteral("周六");
    case Weekday::Sunday:
        return QStringLiteral("周日");
    }
    return {};
}

QString WeeklyScheduleListModel::periodText(
    int startPeriod, int endPeriod)
{
    if (startPeriod == endPeriod) {
        return QStringLiteral("第 %1 节").arg(startPeriod);
    }
    return QStringLiteral("第 %1～%2 节")
        .arg(startPeriod)
        .arg(endPeriod);
}

} // namespace stukit::schedule::presentation
