#include "ScheduleManagementModels.h"

#include <utility>

namespace stukit::schedule::presentation {

namespace {

template<typename T>
const T *itemAt(
    const std::vector<T> &items, const QModelIndex &index)
{
    if (!index.isValid() || index.row() < 0
        || index.row() >= static_cast<int>(items.size())) {
        return nullptr;
    }
    return &items.at(static_cast<std::size_t>(index.row()));
}

} // namespace

AcademicTermListModel::AcademicTermListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int AcademicTermListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant AcademicTermListModel::data(
    const QModelIndex &index, int role) const
{
    const AcademicTerm *term = itemAt(m_items, index);
    if (term == nullptr) {
        return {};
    }
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return term->name();
    case TermIdRole:
        return term->id().toString(QUuid::WithoutBraces);
    case StartDateRole:
        return term->startDate();
    case StartDateTextRole:
        return term->startDate().toString(Qt::ISODate);
    case TotalWeeksRole:
        return term->totalWeeks();
    case ArchivedRole:
        return term->isArchived();
    default:
        return {};
    }
}

QHash<int, QByteArray> AcademicTermListModel::roleNames() const
{
    return {
        {TermIdRole, QByteArrayLiteral("termId")},
        {NameRole, QByteArrayLiteral("name")},
        {StartDateRole, QByteArrayLiteral("startDate")},
        {StartDateTextRole, QByteArrayLiteral("startDateText")},
        {TotalWeeksRole, QByteArrayLiteral("totalWeeks")},
        {ArchivedRole, QByteArrayLiteral("archived")},
    };
}

void AcademicTermListModel::replaceItems(std::vector<AcademicTerm> items)
{
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
}

void AcademicTermListModel::clear()
{
    replaceItems({});
}

CourseListModel::CourseListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CourseListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant CourseListModel::data(const QModelIndex &index, int role) const
{
    const Course *course = itemAt(m_items, index);
    if (course == nullptr) {
        return {};
    }
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return course->name();
    case CourseIdRole:
        return course->id().toString(QUuid::WithoutBraces);
    case TermIdRole:
        return course->termId().toString(QUuid::WithoutBraces);
    case TeacherRole:
        return course->teacher();
    case LocationRole:
        return course->location();
    case AssessmentStandardRole:
        return course->assessmentStandard();
    case TeachingPlanRole:
        return course->teachingPlan();
    case ColorRole:
        return course->color();
    default:
        return {};
    }
}

QHash<int, QByteArray> CourseListModel::roleNames() const
{
    return {
        {CourseIdRole, QByteArrayLiteral("courseId")},
        {TermIdRole, QByteArrayLiteral("termId")},
        {NameRole, QByteArrayLiteral("name")},
        {TeacherRole, QByteArrayLiteral("teacher")},
        {LocationRole, QByteArrayLiteral("location")},
        {AssessmentStandardRole, QByteArrayLiteral("assessmentStandard")},
        {TeachingPlanRole, QByteArrayLiteral("teachingPlan")},
        {ColorRole, QByteArrayLiteral("courseColor")},
    };
}

void CourseListModel::replaceItems(std::vector<Course> items)
{
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
}

void CourseListModel::clear()
{
    replaceItems({});
}

CourseMeetingListModel::CourseMeetingListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CourseMeetingListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant CourseMeetingListModel::data(
    const QModelIndex &index, int role) const
{
    const CourseMeeting *meeting = itemAt(m_items, index);
    if (meeting == nullptr) {
        return {};
    }
    switch (role) {
    case MeetingIdRole:
        return meeting->id().toString(QUuid::WithoutBraces);
    case CourseIdRole:
        return meeting->courseId().toString(QUuid::WithoutBraces);
    case WeekdayRole:
        return static_cast<int>(meeting->weekday());
    case WeekdayTextRole:
        return weekdayText(meeting->weekday());
    case StartPeriodRole:
        return meeting->startPeriod();
    case EndPeriodRole:
        return meeting->endPeriod();
    case StartWeekRole:
        return meeting->startWeek();
    case EndWeekRole:
        return meeting->endWeek();
    case WeekPatternRole:
        return static_cast<int>(meeting->weekPattern());
    case WeekPatternTextRole:
        return weekPatternText(meeting->weekPattern());
    case Qt::DisplayRole:
    case SummaryRole:
        return QStringLiteral("%1 · 第 %2～%3 节 · %4～%5 周 · %6")
            .arg(weekdayText(meeting->weekday()))
            .arg(meeting->startPeriod())
            .arg(meeting->endPeriod())
            .arg(meeting->startWeek())
            .arg(meeting->endWeek())
            .arg(weekPatternText(meeting->weekPattern()));
    default:
        return {};
    }
}

QHash<int, QByteArray> CourseMeetingListModel::roleNames() const
{
    return {
        {MeetingIdRole, QByteArrayLiteral("meetingId")},
        {CourseIdRole, QByteArrayLiteral("courseId")},
        {WeekdayRole, QByteArrayLiteral("weekday")},
        {WeekdayTextRole, QByteArrayLiteral("weekdayText")},
        {StartPeriodRole, QByteArrayLiteral("startPeriod")},
        {EndPeriodRole, QByteArrayLiteral("endPeriod")},
        {StartWeekRole, QByteArrayLiteral("startWeek")},
        {EndWeekRole, QByteArrayLiteral("endWeek")},
        {WeekPatternRole, QByteArrayLiteral("weekPattern")},
        {WeekPatternTextRole, QByteArrayLiteral("weekPatternText")},
        {SummaryRole, QByteArrayLiteral("summary")},
    };
}

void CourseMeetingListModel::replaceItems(
    std::vector<CourseMeeting> items)
{
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
}

void CourseMeetingListModel::clear()
{
    replaceItems({});
}

QString CourseMeetingListModel::weekdayText(Weekday weekday)
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

QString CourseMeetingListModel::weekPatternText(WeekPattern pattern)
{
    switch (pattern) {
    case WeekPattern::EveryWeek:
        return QStringLiteral("每周");
    case WeekPattern::OddWeeks:
        return QStringLiteral("单周");
    case WeekPattern::EvenWeeks:
        return QStringLiteral("双周");
    }
    return {};
}

} // namespace stukit::schedule::presentation
