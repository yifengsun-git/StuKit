#include "WeeklyScheduleListModel.h"

#include <QStringList>

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
    case StartWeekRole:
        return item.startWeek;
    case EndWeekRole:
        return item.endWeek;
    case WeekPatternRole:
        return static_cast<int>(item.weekPattern);
    case ActiveWeeksRole: {
        QVariantList weeks;
        for (const int week : item.activeWeeks) {
            weeks.push_back(week);
        }
        return weeks;
    }
    case WeekRangeTextRole:
        return weekRangeText(
            item.startWeek,
            item.endWeek,
            item.weekPattern,
            item.activeWeeks);
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
        {StartWeekRole, QByteArrayLiteral("startWeek")},
        {EndWeekRole, QByteArrayLiteral("endWeek")},
        {WeekPatternRole, QByteArrayLiteral("weekPattern")},
        {ActiveWeeksRole, QByteArrayLiteral("activeWeeks")},
        {WeekRangeTextRole, QByteArrayLiteral("weekRangeText")},
    };
}

QVariantList WeeklyScheduleListModel::weekdayCounts() const
{
    QVariantList counts{0, 0, 0, 0, 0, 0, 0};
    for (const WeeklyScheduleItem &item : m_items) {
        const int index = static_cast<int>(item.weekday) - 1;
        if (index >= 0 && index < counts.size()) {
            counts[index] = counts.at(index).toInt() + 1;
        }
    }
    return counts;
}

void WeeklyScheduleListModel::replaceItems(
    std::vector<WeeklyScheduleItem> items)
{
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
    emit weekdayCountsChanged();
}

void WeeklyScheduleListModel::clear()
{
    if (m_items.empty()) {
        return;
    }

    beginResetModel();
    m_items.clear();
    endResetModel();
    emit weekdayCountsChanged();
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

QString WeeklyScheduleListModel::weekRangeText(
    int startWeek,
    int endWeek,
    WeekPattern pattern,
    const std::vector<int> &activeWeeks)
{
    std::vector<int> ruleWeeks;
    for (int week = startWeek; week <= endWeek; ++week) {
        if (matchesWeek(pattern, week)) {
            ruleWeeks.push_back(week);
        }
    }
    if (!activeWeeks.empty() && activeWeeks != ruleWeeks) {
        QStringList weekTexts;
        for (const int week : activeWeeks) {
            weekTexts.push_back(QString::number(week));
        }
        return QStringLiteral("第 %1 周")
            .arg(weekTexts.join(QStringLiteral("、")));
    }

    QString patternText;
    switch (pattern) {
    case WeekPattern::EveryWeek:
        patternText = QStringLiteral("每周");
        break;
    case WeekPattern::OddWeeks:
        patternText = QStringLiteral("单周");
        break;
    case WeekPattern::EvenWeeks:
        patternText = QStringLiteral("双周");
        break;
    }
    return QStringLiteral("第 %1～%2 周 · %3")
        .arg(startWeek)
        .arg(endWeek)
        .arg(patternText);
}

} // namespace stukit::schedule::presentation
