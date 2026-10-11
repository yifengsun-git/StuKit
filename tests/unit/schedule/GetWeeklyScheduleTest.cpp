#include "GetWeeklySchedule.h"
#include "support/InMemoryAcademicTermRepository.h"
#include "support/InMemoryCourseMeetingRepository.h"
#include "support/InMemoryCourseRepository.h"
#include "support/ScheduleApplicationTestData.h"

#include <QTest>

#include <variant>

using namespace stukit::schedule;
using namespace stukit::schedule::testing;

namespace {

struct Scenario
{
    QUuid termId{QUuid::createUuid()};
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    InMemoryCourseMeetingRepository meetings;

    explicit Scenario(int totalWeeks = 18)
    {
        terms.save(applicationTestTerm(termId, totalWeeks));
    }
};

const WeeklySchedule &scheduleOf(const GetWeeklyScheduleResult &result)
{
    return std::get<WeeklySchedule>(result);
}

} // namespace

class GetWeeklyScheduleTest final : public QObject
{
    Q_OBJECT

private slots:
    void returnsEmptySchedule();
    void filtersWeekPatternsAndSortsReadableItems();
    void reportsInvalidMissingAndOutOfRangeRequests();
    void reportsRepositoryFailure_data();
    void reportsRepositoryFailure();
    void reportsBrokenStoredRelations();
    void queriesEachRepositoryOnce();
};

void GetWeeklyScheduleTest::returnsEmptySchedule()
{
    Scenario scenario;
    GetWeeklySchedule useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.termId, 1);

    QVERIFY(std::holds_alternative<WeeklySchedule>(result));
    QCOMPARE(scheduleOf(result).term.id(), scenario.termId);
    QCOMPARE(scheduleOf(result).week, 1);
    QVERIFY(scheduleOf(result).items.empty());
}

void GetWeeklyScheduleTest::filtersWeekPatternsAndSortsReadableItems()
{
    Scenario scenario;
    const QUuid mathId = QUuid::createUuid();
    const QUuid englishId = QUuid::createUuid();
    scenario.courses.save(applicationTestCourse(
        mathId, scenario.termId, QStringLiteral("高等数学")));
    scenario.courses.save(applicationTestCourse(
        englishId, scenario.termId, QStringLiteral("大学英语")));
    scenario.meetings.save(applicationTestMeeting(
        QUuid::createUuid(),
        mathId,
        Weekday::Tuesday,
        3,
        4));
    scenario.meetings.save(applicationTestMeeting(
        QUuid::createUuid(),
        englishId,
        Weekday::Monday,
        1,
        2,
        1,
        18,
        WeekPattern::EvenWeeks));
    scenario.meetings.save(applicationTestMeeting(
        QUuid::createUuid(),
        englishId,
        Weekday::Monday,
        5,
        6,
        1,
        18,
        WeekPattern::OddWeeks));
    GetWeeklySchedule useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.termId, 2);

    QVERIFY(std::holds_alternative<WeeklySchedule>(result));
    const auto &items = scheduleOf(result).items;
    QCOMPARE(items.size(), std::size_t{2});
    QCOMPARE(items.at(0).courseName, QStringLiteral("大学英语"));
    QCOMPARE(items.at(0).weekday, Weekday::Monday);
    QCOMPARE(items.at(0).teacher, QStringLiteral("王老师"));
    QCOMPARE(items.at(0).location, QStringLiteral("A101"));
    QCOMPARE(items.at(0).color, QStringLiteral("#3B82F6"));
    QCOMPARE(items.at(1).courseName, QStringLiteral("高等数学"));
    QCOMPARE(items.at(1).weekday, Weekday::Tuesday);
}

void GetWeeklyScheduleTest::reportsInvalidMissingAndOutOfRangeRequests()
{
    Scenario scenario(12);
    GetWeeklySchedule useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    QCOMPARE(
        std::get<GetWeeklyScheduleError>(useCase.execute({}, 1)),
        GetWeeklyScheduleError::InvalidInput);
    QCOMPARE(
        std::get<GetWeeklyScheduleError>(
            useCase.execute(scenario.termId, 0)),
        GetWeeklyScheduleError::InvalidInput);
    QCOMPARE(
        std::get<GetWeeklyScheduleError>(
            useCase.execute(QUuid::createUuid(), 1)),
        GetWeeklyScheduleError::TermNotFound);
    QCOMPARE(
        std::get<GetWeeklyScheduleError>(
            useCase.execute(scenario.termId, 13)),
        GetWeeklyScheduleError::WeekOutOfRange);
}

void GetWeeklyScheduleTest::reportsRepositoryFailure_data()
{
    QTest::addColumn<int>("repositoryIndex");

    QTest::newRow("term repository") << 0;
    QTest::newRow("course repository") << 1;
    QTest::newRow("meeting repository") << 2;
}

void GetWeeklyScheduleTest::reportsRepositoryFailure()
{
    QFETCH(int, repositoryIndex);

    Scenario scenario;
    if (repositoryIndex == 0) {
        scenario.terms.setQueryShouldSucceed(false);
    } else if (repositoryIndex == 1) {
        scenario.courses.setQueryShouldSucceed(false);
    } else {
        scenario.meetings.setQueryShouldSucceed(false);
    }
    GetWeeklySchedule useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.termId, 1);

    QCOMPARE(
        std::get<GetWeeklyScheduleError>(result),
        GetWeeklyScheduleError::StorageFailure);
}

void GetWeeklyScheduleTest::reportsBrokenStoredRelations()
{
    Scenario scenario(12);
    scenario.meetings.save(applicationTestMeeting(
        QUuid::createUuid(), QUuid::createUuid()));
    GetWeeklySchedule useCase(
        scenario.terms, scenario.courses, scenario.meetings);
    QCOMPARE(
        std::get<GetWeeklyScheduleError>(
            useCase.execute(scenario.termId, 1)),
        GetWeeklyScheduleError::StorageFailure);

    Scenario outOfRangeScenario(12);
    const QUuid courseId = QUuid::createUuid();
    outOfRangeScenario.courses.save(applicationTestCourse(
        courseId, outOfRangeScenario.termId));
    outOfRangeScenario.meetings.save(applicationTestMeeting(
        QUuid::createUuid(), courseId));
    GetWeeklySchedule outOfRangeUseCase(
        outOfRangeScenario.terms,
        outOfRangeScenario.courses,
        outOfRangeScenario.meetings);
    QCOMPARE(
        std::get<GetWeeklyScheduleError>(
            outOfRangeUseCase.execute(outOfRangeScenario.termId, 1)),
        GetWeeklyScheduleError::StorageFailure);
}

void GetWeeklyScheduleTest::queriesEachRepositoryOnce()
{
    Scenario scenario;
    GetWeeklySchedule useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.termId, 1);

    QVERIFY(std::holds_alternative<WeeklySchedule>(result));
    QCOMPARE(scenario.terms.findAllCallCount(), 1);
    QCOMPARE(scenario.courses.findAllCallCount(), 1);
    QCOMPARE(scenario.meetings.findAllCallCount(), 1);
}

QTEST_APPLESS_MAIN(GetWeeklyScheduleTest)

#include "GetWeeklyScheduleTest.moc"
