#include "DetectScheduleConflicts.h"
#include "support/InMemoryAcademicTermRepository.h"
#include "support/InMemoryCourseMeetingRepository.h"
#include "support/InMemoryCourseRepository.h"

#include <QTest>

#include <variant>

using namespace stukit::schedule;
using namespace stukit::schedule::testing;

namespace {

AcademicTerm makeTerm(const QUuid &id, int totalWeeks = 18)
{
    return AcademicTerm::create(AcademicTermData{
        .id = id,
        .name = QStringLiteral("测试学期"),
        .startDate = QDate(2026, 9, 7),
        .totalWeeks = totalWeeks,
        .archived = false,
    }).value();
}

Course makeCourse(
    const QUuid &id,
    const QUuid &termId,
    const QString &name = QStringLiteral("候选课程"))
{
    return Course::create(CourseData{
        .id = id,
        .termId = termId,
        .name = name,
        .teacher = {},
        .location = {},
        .color = QStringLiteral("#3B82F6"),
    }).value();
}

CourseMeeting makeMeeting(
    const QUuid &id,
    const QUuid &courseId,
    Weekday weekday = Weekday::Monday,
    int startPeriod = 1,
    int endPeriod = 2,
    int startWeek = 1,
    int endWeek = 16,
    WeekPattern pattern = WeekPattern::EveryWeek)
{
    return CourseMeeting::create(CourseMeetingData{
        .id = id,
        .courseId = courseId,
        .weekday = weekday,
        .startPeriod = startPeriod,
        .endPeriod = endPeriod,
        .startWeek = startWeek,
        .endWeek = endWeek,
        .weekPattern = pattern,
    }).value();
}

struct Scenario
{
    QUuid termId{QUuid::createUuid()};
    QUuid courseId{QUuid::createUuid()};
    QUuid candidateMeetingId{QUuid::createUuid()};
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    InMemoryCourseMeetingRepository meetings;

    explicit Scenario(int totalWeeks = 18)
    {
        terms.save(makeTerm(termId, totalWeeks));
        courses.save(makeCourse(courseId, termId));
    }

    [[nodiscard]] DetectScheduleConflictsRequest request() const
    {
        return DetectScheduleConflictsRequest{
            .meetingId = candidateMeetingId,
            .courseId = courseId,
            .weekday = Weekday::Monday,
            .startPeriod = 1,
            .endPeriod = 2,
            .startWeek = 1,
            .endWeek = 16,
            .weekPattern = WeekPattern::EveryWeek,
        };
    }
};

const std::vector<ScheduleConflict> &conflictsOf(
    const DetectScheduleConflictsResult &result)
{
    return std::get<std::vector<ScheduleConflict>>(result);
}

} // namespace

class DetectScheduleConflictsTest final : public QObject
{
    Q_OBJECT

private slots:
    void returnsEmptyResultWhenScheduleIsFree();
    void detectsOverlapAndReportsFirstWeek();
    void ignoresNonConflictingMeeting_data();
    void ignoresNonConflictingMeeting();
    void excludesCandidateItselfWhenEditing();
    void ignoresCoursesFromAnotherTerm();
    void sortsConflictsDeterministically();
    void rejectsInvalidInput();
    void reportsMissingCourse();
    void reportsMissingTerm();
    void rejectsMeetingBeyondTerm();
    void reportsRepositoryFailure_data();
    void reportsRepositoryFailure();
    void reportsOrphanStoredMeeting();
    void queriesEachRepositoryOnce();
};

void DetectScheduleConflictsTest::returnsEmptyResultWhenScheduleIsFree()
{
    Scenario scenario;
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.request());

    QVERIFY(std::holds_alternative<std::vector<ScheduleConflict>>(result));
    QVERIFY(conflictsOf(result).empty());
}

void DetectScheduleConflictsTest::detectsOverlapAndReportsFirstWeek()
{
    Scenario scenario;
    const QUuid existingCourseId = QUuid::createUuid();
    const auto existingCourse = makeCourse(
        existingCourseId, scenario.termId, QStringLiteral("大学英语"));
    scenario.courses.save(existingCourse);
    scenario.meetings.save(makeMeeting(
        QUuid::createUuid(),
        existingCourseId,
        Weekday::Monday,
        2,
        3,
        3,
        15,
        WeekPattern::OddWeeks));
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.request());

    QVERIFY(std::holds_alternative<std::vector<ScheduleConflict>>(result));
    QCOMPARE(conflictsOf(result).size(), std::size_t{1});
    QCOMPARE(conflictsOf(result).front().course.name(), QStringLiteral("大学英语"));
    QCOMPARE(conflictsOf(result).front().firstConflictingWeek, 3);
}

void DetectScheduleConflictsTest::ignoresNonConflictingMeeting_data()
{
    QTest::addColumn<Weekday>("weekday");
    QTest::addColumn<int>("startPeriod");
    QTest::addColumn<int>("endPeriod");
    QTest::addColumn<int>("startWeek");
    QTest::addColumn<int>("endWeek");
    QTest::addColumn<WeekPattern>("candidatePattern");
    QTest::addColumn<WeekPattern>("existingPattern");

    QTest::newRow("different weekday")
        << Weekday::Tuesday << 1 << 2 << 1 << 16
        << WeekPattern::EveryWeek << WeekPattern::EveryWeek;
    QTest::newRow("adjacent periods")
        << Weekday::Monday << 3 << 4 << 1 << 16
        << WeekPattern::EveryWeek << WeekPattern::EveryWeek;
    QTest::newRow("separate week ranges")
        << Weekday::Monday << 1 << 2 << 17 << 18
        << WeekPattern::EveryWeek << WeekPattern::EveryWeek;
    QTest::newRow("odd versus even")
        << Weekday::Monday << 1 << 2 << 1 << 16
        << WeekPattern::OddWeeks << WeekPattern::EvenWeeks;
}

void DetectScheduleConflictsTest::ignoresNonConflictingMeeting()
{
    QFETCH(Weekday, weekday);
    QFETCH(int, startPeriod);
    QFETCH(int, endPeriod);
    QFETCH(int, startWeek);
    QFETCH(int, endWeek);
    QFETCH(WeekPattern, candidatePattern);
    QFETCH(WeekPattern, existingPattern);

    Scenario scenario;
    scenario.meetings.save(makeMeeting(
        QUuid::createUuid(),
        scenario.courseId,
        weekday,
        startPeriod,
        endPeriod,
        startWeek,
        endWeek,
        existingPattern));
    auto request = scenario.request();
    request.weekPattern = candidatePattern;
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(request);

    QVERIFY(std::holds_alternative<std::vector<ScheduleConflict>>(result));
    QVERIFY(conflictsOf(result).empty());
}

void DetectScheduleConflictsTest::excludesCandidateItselfWhenEditing()
{
    Scenario scenario;
    scenario.meetings.save(makeMeeting(
        scenario.candidateMeetingId, scenario.courseId));
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.request());

    QVERIFY(std::holds_alternative<std::vector<ScheduleConflict>>(result));
    QVERIFY(conflictsOf(result).empty());
}

void DetectScheduleConflictsTest::ignoresCoursesFromAnotherTerm()
{
    Scenario scenario;
    const QUuid otherTermId = QUuid::createUuid();
    const QUuid otherCourseId = QUuid::createUuid();
    scenario.terms.save(makeTerm(otherTermId));
    scenario.courses.save(makeCourse(otherCourseId, otherTermId));
    scenario.meetings.save(makeMeeting(QUuid::createUuid(), otherCourseId));
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.request());

    QVERIFY(std::holds_alternative<std::vector<ScheduleConflict>>(result));
    QVERIFY(conflictsOf(result).empty());
}

void DetectScheduleConflictsTest::sortsConflictsDeterministically()
{
    Scenario scenario;
    const QUuid laterCourseId = QUuid::createUuid();
    const QUuid earlierCourseId = QUuid::createUuid();
    scenario.courses.save(makeCourse(
        laterCourseId, scenario.termId, QStringLiteral("后发生")));
    scenario.courses.save(makeCourse(
        earlierCourseId, scenario.termId, QStringLiteral("先发生")));
    scenario.meetings.save(makeMeeting(
        QUuid::createUuid(), laterCourseId,
        Weekday::Monday, 1, 2, 5, 9));
    scenario.meetings.save(makeMeeting(
        QUuid::createUuid(), earlierCourseId,
        Weekday::Monday, 1, 2, 3, 9));
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.request());

    QVERIFY(std::holds_alternative<std::vector<ScheduleConflict>>(result));
    QCOMPARE(conflictsOf(result).size(), std::size_t{2});
    QCOMPARE(conflictsOf(result).at(0).firstConflictingWeek, 3);
    QCOMPARE(conflictsOf(result).at(1).firstConflictingWeek, 5);
}

void DetectScheduleConflictsTest::rejectsInvalidInput()
{
    Scenario scenario;
    auto request = scenario.request();
    request.startPeriod = 0;
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(request);

    QVERIFY(std::holds_alternative<DetectScheduleConflictsError>(result));
    QCOMPARE(
        std::get<DetectScheduleConflictsError>(result),
        DetectScheduleConflictsError::InvalidInput);
}

void DetectScheduleConflictsTest::reportsMissingCourse()
{
    Scenario scenario;
    auto request = scenario.request();
    request.courseId = QUuid::createUuid();
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(request);

    QCOMPARE(
        std::get<DetectScheduleConflictsError>(result),
        DetectScheduleConflictsError::CourseNotFound);
}

void DetectScheduleConflictsTest::reportsMissingTerm()
{
    Scenario scenario;
    const QUuid orphanCourseId = QUuid::createUuid();
    scenario.courses.save(makeCourse(orphanCourseId, QUuid::createUuid()));
    auto request = scenario.request();
    request.courseId = orphanCourseId;
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(request);

    QCOMPARE(
        std::get<DetectScheduleConflictsError>(result),
        DetectScheduleConflictsError::TermNotFound);
}

void DetectScheduleConflictsTest::rejectsMeetingBeyondTerm()
{
    Scenario scenario(12);
    auto request = scenario.request();
    request.endWeek = 13;
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(request);

    QCOMPARE(
        std::get<DetectScheduleConflictsError>(result),
        DetectScheduleConflictsError::MeetingExceedsTerm);
}

void DetectScheduleConflictsTest::reportsRepositoryFailure_data()
{
    QTest::addColumn<int>("repositoryIndex");

    QTest::newRow("term repository") << 0;
    QTest::newRow("course repository") << 1;
    QTest::newRow("meeting repository") << 2;
}

void DetectScheduleConflictsTest::reportsRepositoryFailure()
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
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.request());

    QCOMPARE(
        std::get<DetectScheduleConflictsError>(result),
        DetectScheduleConflictsError::StorageFailure);
}

void DetectScheduleConflictsTest::reportsOrphanStoredMeeting()
{
    Scenario scenario;
    scenario.meetings.save(makeMeeting(
        QUuid::createUuid(), QUuid::createUuid()));
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.request());

    QCOMPARE(
        std::get<DetectScheduleConflictsError>(result),
        DetectScheduleConflictsError::StorageFailure);
}

void DetectScheduleConflictsTest::queriesEachRepositoryOnce()
{
    Scenario scenario;
    DetectScheduleConflicts useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.request());

    QVERIFY(std::holds_alternative<std::vector<ScheduleConflict>>(result));
    QCOMPARE(scenario.terms.findAllCallCount(), 1);
    QCOMPARE(scenario.courses.findAllCallCount(), 1);
    QCOMPARE(scenario.meetings.findAllCallCount(), 1);
}

QTEST_APPLESS_MAIN(DetectScheduleConflictsTest)

#include "DetectScheduleConflictsTest.moc"
