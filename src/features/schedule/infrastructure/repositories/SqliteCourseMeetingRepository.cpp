#include "repositories/SqliteCourseMeetingRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <utility>

namespace stukit::schedule {
namespace {

constexpr int EveryWeekCode = 0;
constexpr int OddWeeksCode = 1;
constexpr int EvenWeeksCode = 2;

int weekdayToDatabase(Weekday weekday)
{
    switch (weekday) {
    case Weekday::Monday:
        return 1;
    case Weekday::Tuesday:
        return 2;
    case Weekday::Wednesday:
        return 3;
    case Weekday::Thursday:
        return 4;
    case Weekday::Friday:
        return 5;
    case Weekday::Saturday:
        return 6;
    case Weekday::Sunday:
        return 7;
    }
    Q_UNREACHABLE_RETURN(1);
}

std::optional<Weekday> weekdayFromDatabase(int value)
{
    switch (value) {
    case 1:
        return Weekday::Monday;
    case 2:
        return Weekday::Tuesday;
    case 3:
        return Weekday::Wednesday;
    case 4:
        return Weekday::Thursday;
    case 5:
        return Weekday::Friday;
    case 6:
        return Weekday::Saturday;
    case 7:
        return Weekday::Sunday;
    default:
        return std::nullopt;
    }
}

int weekPatternToDatabase(WeekPattern pattern)
{
    switch (pattern) {
    case WeekPattern::EveryWeek:
        return EveryWeekCode;
    case WeekPattern::OddWeeks:
        return OddWeeksCode;
    case WeekPattern::EvenWeeks:
        return EvenWeeksCode;
    }
    Q_UNREACHABLE_RETURN(EveryWeekCode);
}

std::optional<WeekPattern> weekPatternFromDatabase(int value)
{
    switch (value) {
    case EveryWeekCode:
        return WeekPattern::EveryWeek;
    case OddWeeksCode:
        return WeekPattern::OddWeeks;
    case EvenWeeksCode:
        return WeekPattern::EvenWeeks;
    default:
        return std::nullopt;
    }
}

} // namespace

SqliteCourseMeetingRepository::SqliteCourseMeetingRepository(
    QSqlDatabase database)
    : m_database(std::move(database))
{
}

bool SqliteCourseMeetingRepository::save(const CourseMeeting &meeting)
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }

    if (!m_database.transaction()) {
        m_lastError = m_database.lastError().text();
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO course_meetings("
        "id, course_id, weekday, start_period, end_period, "
        "start_week, end_week, week_pattern) "
        "VALUES(:id, :courseId, :weekday, :startPeriod, :endPeriod, "
        ":startWeek, :endWeek, :weekPattern) "
        "ON CONFLICT(id) DO UPDATE SET "
        "course_id = excluded.course_id, "
        "weekday = excluded.weekday, "
        "start_period = excluded.start_period, "
        "end_period = excluded.end_period, "
        "start_week = excluded.start_week, "
        "end_week = excluded.end_week, "
        "week_pattern = excluded.week_pattern"));
    query.bindValue(
        QStringLiteral(":id"), meeting.id().toString(QUuid::WithoutBraces));
    query.bindValue(
        QStringLiteral(":courseId"),
        meeting.courseId().toString(QUuid::WithoutBraces));
    query.bindValue(
        QStringLiteral(":weekday"), weekdayToDatabase(meeting.weekday()));
    query.bindValue(QStringLiteral(":startPeriod"), meeting.startPeriod());
    query.bindValue(QStringLiteral(":endPeriod"), meeting.endPeriod());
    query.bindValue(QStringLiteral(":startWeek"), meeting.startWeek());
    query.bindValue(QStringLiteral(":endWeek"), meeting.endWeek());
    query.bindValue(
        QStringLiteral(":weekPattern"),
        weekPatternToDatabase(meeting.weekPattern()));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        m_database.rollback();
        return false;
    }

    const QString meetingId = meeting.id().toString(QUuid::WithoutBraces);
    QSqlQuery deleteWeeks(m_database);
    deleteWeeks.prepare(QStringLiteral(
        "DELETE FROM course_meeting_weeks WHERE meeting_id = :meetingId"));
    deleteWeeks.bindValue(QStringLiteral(":meetingId"), meetingId);
    if (!deleteWeeks.exec()) {
        m_lastError = deleteWeeks.lastError().text();
        m_database.rollback();
        return false;
    }

    QSqlQuery insertWeek(m_database);
    insertWeek.prepare(QStringLiteral(
        "INSERT INTO course_meeting_weeks(meeting_id, week_number) "
        "VALUES(:meetingId, :weekNumber)"));
    for (const int week : meeting.activeWeeks()) {
        insertWeek.bindValue(QStringLiteral(":meetingId"), meetingId);
        insertWeek.bindValue(QStringLiteral(":weekNumber"), week);
        if (!insertWeek.exec()) {
            m_lastError = insertWeek.lastError().text();
            m_database.rollback();
            return false;
        }
    }
    if (!m_database.commit()) {
        m_lastError = m_database.lastError().text();
        m_database.rollback();
        return false;
    }
    return true;
}

bool SqliteCourseMeetingRepository::remove(const QUuid &id)
{
    m_lastError.clear();
    if (!m_database.isOpen() || id.isNull()) {
        m_lastError = QStringLiteral("Database is not open or id is invalid");
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM course_meetings WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));
    if (!query.exec() || query.numRowsAffected() != 1) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool SqliteCourseMeetingRepository::removeByCourseId(const QUuid &courseId)
{
    m_lastError.clear();
    if (!m_database.isOpen() || courseId.isNull()) {
        m_lastError = QStringLiteral("Database is not open or id is invalid");
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "DELETE FROM course_meetings WHERE course_id = :courseId"));
    query.bindValue(
        QStringLiteral(":courseId"),
        courseId.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

std::optional<CourseMeeting> SqliteCourseMeetingRepository::findById(
    const QUuid &id) const
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, course_id, weekday, start_period, end_period, "
        "start_week, end_week, week_pattern "
        "FROM course_meetings WHERE id = :id"));
    query.bindValue(
        QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }
    return meetingFromCurrentRow(query);
}

std::optional<std::vector<CourseMeeting>>
SqliteCourseMeetingRepository::findByCourseId(const QUuid &courseId) const
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, course_id, weekday, start_period, end_period, "
        "start_week, end_week, week_pattern "
        "FROM course_meetings WHERE course_id = :courseId "
        "ORDER BY weekday, start_period, start_week, end_period, end_week, id"));
    query.bindValue(
        QStringLiteral(":courseId"),
        courseId.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    return meetingsFromQuery(query);
}

std::optional<std::vector<CourseMeeting>>
SqliteCourseMeetingRepository::findAll() const
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT id, course_id, weekday, start_period, end_period, "
            "start_week, end_week, week_pattern FROM course_meetings "
            "ORDER BY course_id, weekday, start_period, start_week, "
            "end_period, end_week, id"))) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    return meetingsFromQuery(query);
}

const QString &SqliteCourseMeetingRepository::lastError() const noexcept
{
    return m_lastError;
}

std::optional<CourseMeeting>
SqliteCourseMeetingRepository::meetingFromCurrentRow(
    const QSqlQuery &query) const
{
    const auto weekday = weekdayFromDatabase(
        query.value(QStringLiteral("weekday")).toInt());
    const auto pattern = weekPatternFromDatabase(
        query.value(QStringLiteral("week_pattern")).toInt());
    if (!weekday.has_value() || !pattern.has_value()) {
        m_lastError = QStringLiteral(
            "Database contains an invalid course meeting enum");
        return std::nullopt;
    }

    const QUuid meetingId(query.value(QStringLiteral("id")).toString());
    const auto activeWeeks = activeWeeksFor(meetingId);
    if (!activeWeeks.has_value()) {
        return std::nullopt;
    }

    const auto meeting = CourseMeeting::create(CourseMeetingData{
        .id = meetingId,
        .courseId = QUuid(
            query.value(QStringLiteral("course_id")).toString()),
        .weekday = *weekday,
        .startPeriod = query.value(QStringLiteral("start_period")).toInt(),
        .endPeriod = query.value(QStringLiteral("end_period")).toInt(),
        .startWeek = query.value(QStringLiteral("start_week")).toInt(),
        .endWeek = query.value(QStringLiteral("end_week")).toInt(),
        .weekPattern = *pattern,
        .activeWeeks = *activeWeeks,
    });
    if (!meeting.has_value()) {
        m_lastError = QStringLiteral("Database contains an invalid course meeting");
    }
    return meeting;
}

std::optional<std::vector<int>>
SqliteCourseMeetingRepository::activeWeeksFor(const QUuid &meetingId) const
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT week_number FROM course_meeting_weeks "
        "WHERE meeting_id = :meetingId ORDER BY week_number"));
    query.bindValue(
        QStringLiteral(":meetingId"),
        meetingId.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    std::vector<int> weeks;
    while (query.next()) {
        weeks.push_back(query.value(0).toInt());
    }
    if (weeks.empty()) {
        m_lastError = QStringLiteral(
            "Database contains a course meeting without active weeks");
        return std::nullopt;
    }
    return weeks;
}

std::optional<std::vector<CourseMeeting>>
SqliteCourseMeetingRepository::meetingsFromQuery(QSqlQuery &query) const
{
    std::vector<CourseMeeting> meetings;
    while (query.next()) {
        auto meeting = meetingFromCurrentRow(query);
        if (!meeting.has_value()) {
            return std::nullopt;
        }
        meetings.push_back(std::move(*meeting));
    }
    return meetings;
}

} // namespace stukit::schedule
