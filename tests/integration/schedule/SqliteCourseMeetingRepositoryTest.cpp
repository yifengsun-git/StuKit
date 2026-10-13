#include "ScheduleSqliteTestSupport.h"
#include "database/SqliteDatabase.h"
#include "repositories/SqliteAcademicTermRepository.h"
#include "repositories/SqliteCourseMeetingRepository.h"
#include "repositories/SqliteCourseRepository.h"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace stukit::schedule;
using namespace stukit::schedule::testing;

class SqliteCourseMeetingRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void savesAndFindsAllFields();
    void returnsNulloptAndEmptyListForMissingData();
    void updatesMeetingWithSameId();
    void persistsCustomWeeksAndRemovesMeeting();
    void filtersByCourseAndSortsByTime();
    void rejectsUnknownCourse();
    void persistsAcrossRepositoryInstances();
    void rejectsInvalidStoredEnum();

private:
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<stukit::accounting::SqliteDatabase> m_database;
    std::unique_ptr<SqliteAcademicTermRepository> m_terms;
    std::unique_ptr<SqliteCourseRepository> m_courses;
    std::unique_ptr<SqliteCourseMeetingRepository> m_meetings;
    QUuid m_courseId;
};

void SqliteCourseMeetingRepositoryTest::init()
{
    m_directory = std::make_unique<QTemporaryDir>(QDir::current().filePath(
        QStringLiteral("stukit-meeting-repository-test-XXXXXX")));
    QVERIFY(m_directory->isValid());
    m_database = std::make_unique<stukit::accounting::SqliteDatabase>(
        uniqueScheduleConnectionName(QStringLiteral("meeting")));
    QVERIFY2(
        m_database->open(m_directory->filePath(QStringLiteral("stukit.sqlite"))),
        qPrintable(m_database->lastError()));
    QVERIFY2(m_database->migrate(), qPrintable(m_database->lastError()));
    m_terms = std::make_unique<SqliteAcademicTermRepository>(
        m_database->connection());
    m_courses = std::make_unique<SqliteCourseRepository>(
        m_database->connection());
    m_meetings = std::make_unique<SqliteCourseMeetingRepository>(
        m_database->connection());

    const QUuid termId = QUuid::createUuid();
    m_courseId = QUuid::createUuid();
    QVERIFY(m_terms->save(makeTerm(termId)));
    QVERIFY(m_courses->save(makeCourse(m_courseId, termId)));
}

void SqliteCourseMeetingRepositoryTest::cleanup()
{
    m_meetings.reset();
    m_courses.reset();
    m_terms.reset();
    m_database.reset();
    m_directory.reset();
}

void SqliteCourseMeetingRepositoryTest::savesAndFindsAllFields()
{
    const QUuid id = QUuid::createUuid();
    QVERIFY(m_meetings->save(makeMeeting(
        id,
        m_courseId,
        Weekday::Wednesday,
        3,
        4,
        2,
        16,
        WeekPattern::EvenWeeks)));

    const auto stored = m_meetings->findById(id);

    QVERIFY2(stored.has_value(), qPrintable(m_meetings->lastError()));
    QCOMPARE(stored->id(), id);
    QCOMPARE(stored->courseId(), m_courseId);
    QCOMPARE(stored->weekday(), Weekday::Wednesday);
    QCOMPARE(stored->startPeriod(), 3);
    QCOMPARE(stored->endPeriod(), 4);
    QCOMPARE(stored->startWeek(), 2);
    QCOMPARE(stored->endWeek(), 16);
    QCOMPARE(stored->weekPattern(), WeekPattern::EvenWeeks);
}

void SqliteCourseMeetingRepositoryTest::returnsNulloptAndEmptyListForMissingData()
{
    QVERIFY(!m_meetings->findById(QUuid::createUuid()).has_value());
    QVERIFY(m_meetings->lastError().isEmpty());
    const auto empty = m_meetings->findByCourseId(QUuid::createUuid());
    QVERIFY(empty.has_value());
    QVERIFY(empty->empty());
}

void SqliteCourseMeetingRepositoryTest::updatesMeetingWithSameId()
{
    const QUuid id = QUuid::createUuid();
    QVERIFY(m_meetings->save(makeMeeting(id, m_courseId)));
    QVERIFY(m_meetings->save(makeMeeting(
        id,
        m_courseId,
        Weekday::Friday,
        5,
        6,
        1,
        17,
        WeekPattern::OddWeeks)));

    const auto meetings = m_meetings->findAll();

    QVERIFY(meetings.has_value());
    QCOMPARE(meetings->size(), std::size_t{1});
    QCOMPARE(meetings->front().weekday(), Weekday::Friday);
    QCOMPARE(meetings->front().startPeriod(), 5);
    QCOMPARE(meetings->front().weekPattern(), WeekPattern::OddWeeks);
}

void SqliteCourseMeetingRepositoryTest::persistsCustomWeeksAndRemovesMeeting()
{
    const QUuid id = QUuid::createUuid();
    const auto meeting = CourseMeeting::create(CourseMeetingData{
        .id = id,
        .courseId = m_courseId,
        .weekday = Weekday::Thursday,
        .startPeriod = 7,
        .endPeriod = 8,
        .startWeek = 1,
        .endWeek = 14,
        .weekPattern = WeekPattern::EveryWeek,
        .activeWeeks = {1, 2, 5, 9, 14},
    });
    QVERIFY(meeting.has_value());
    QVERIFY(m_meetings->save(*meeting));

    const auto stored = m_meetings->findById(id);
    QVERIFY2(stored.has_value(), qPrintable(m_meetings->lastError()));
    QCOMPARE(stored->activeWeeks(), std::vector<int>({1, 2, 5, 9, 14}));
    QVERIFY(stored->occursInWeek(5));
    QVERIFY(!stored->occursInWeek(6));

    QVERIFY(m_meetings->remove(id));
    QVERIFY(!m_meetings->findById(id).has_value());
}

void SqliteCourseMeetingRepositoryTest::filtersByCourseAndSortsByTime()
{
    const QUuid otherCourseId = QUuid::createUuid();
    const auto term = m_terms->findAll();
    QVERIFY(term.has_value());
    QVERIFY(m_courses->save(makeCourse(otherCourseId, term->front().id())));

    QVERIFY(m_meetings->save(makeMeeting(
        QUuid::createUuid(), m_courseId, Weekday::Tuesday, 3, 4)));
    QVERIFY(m_meetings->save(makeMeeting(
        QUuid::createUuid(), m_courseId, Weekday::Monday, 5, 6)));
    QVERIFY(m_meetings->save(makeMeeting(
        QUuid::createUuid(), m_courseId, Weekday::Monday, 1, 2)));
    QVERIFY(m_meetings->save(makeMeeting(
        QUuid::createUuid(), otherCourseId, Weekday::Monday, 1, 2)));

    const auto meetings = m_meetings->findByCourseId(m_courseId);

    QVERIFY(meetings.has_value());
    QCOMPARE(meetings->size(), std::size_t{3});
    QCOMPARE(meetings->at(0).weekday(), Weekday::Monday);
    QCOMPARE(meetings->at(0).startPeriod(), 1);
    QCOMPARE(meetings->at(1).weekday(), Weekday::Monday);
    QCOMPARE(meetings->at(1).startPeriod(), 5);
    QCOMPARE(meetings->at(2).weekday(), Weekday::Tuesday);
}

void SqliteCourseMeetingRepositoryTest::rejectsUnknownCourse()
{
    QVERIFY(!m_meetings->save(makeMeeting(
        QUuid::createUuid(), QUuid::createUuid())));
    QVERIFY(!m_meetings->lastError().isEmpty());
}

void SqliteCourseMeetingRepositoryTest::persistsAcrossRepositoryInstances()
{
    const QUuid id = QUuid::createUuid();
    QVERIFY(m_meetings->save(makeMeeting(id, m_courseId)));
    m_meetings.reset();
    m_meetings = std::make_unique<SqliteCourseMeetingRepository>(
        m_database->connection());

    QVERIFY(m_meetings->findById(id).has_value());
}

void SqliteCourseMeetingRepositoryTest::rejectsInvalidStoredEnum()
{
    QSqlQuery query(m_database->connection());
    QVERIFY(query.exec(QStringLiteral("PRAGMA ignore_check_constraints = ON")));
    query.prepare(QStringLiteral(
        "INSERT INTO course_meetings("
        "id, course_id, weekday, start_period, end_period, "
        "start_week, end_week, week_pattern) "
        "VALUES('broken', :courseId, 9, 1, 2, 1, 18, 7)"));
    query.bindValue(
        QStringLiteral(":courseId"),
        m_courseId.toString(QUuid::WithoutBraces));
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
    QVERIFY(query.exec(QStringLiteral("PRAGMA ignore_check_constraints = OFF")));

    QVERIFY(!m_meetings->findAll().has_value());
    QVERIFY(m_meetings->lastError().contains(QStringLiteral("invalid")));
}

QTEST_GUILESS_MAIN(SqliteCourseMeetingRepositoryTest)

#include "SqliteCourseMeetingRepositoryTest.moc"
