#include "SaveCourseMeeting.h"
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
    QUuid courseId{QUuid::createUuid()};
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    InMemoryCourseMeetingRepository meetings;

    explicit Scenario(int totalWeeks = 18)
    {
        terms.save(applicationTestTerm(termId, totalWeeks));
        courses.save(applicationTestCourse(courseId, termId));
    }

    [[nodiscard]] SaveCourseMeetingRequest request() const
    {
        return SaveCourseMeetingRequest{
            .id = {},
            .courseId = courseId,
            .weekday = Weekday::Monday,
            .startPeriod = 1,
            .endPeriod = 2,
            .startWeek = 1,
            .endWeek = 18,
            .weekPattern = WeekPattern::EveryWeek,
        };
    }
};

} // namespace

class SaveCourseMeetingTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsAndStoresMeeting();
    void returnsConflictWithoutSaving();
    void allowsOddAndEvenMeetingsAtTheSameTime();
    void updatesMeetingAndExcludesItselfFromConflicts();
    void reportsMissingMeeting();
    void reportsValidationAndRelationErrors();
    void reportsStorageFailure();
};

void SaveCourseMeetingTest::createsAndStoresMeeting()
{
    Scenario scenario;
    SaveCourseMeeting useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.request());

    QVERIFY(std::holds_alternative<CourseMeeting>(result));
    const auto &meeting = std::get<CourseMeeting>(result);
    QVERIFY(!meeting.id().isNull());
    QCOMPARE(meeting.courseId(), scenario.courseId);
    const auto stored = scenario.meetings.findById(meeting.id());
    QVERIFY(stored.has_value());
    QCOMPARE(stored->startPeriod(), 1);
}

void SaveCourseMeetingTest::returnsConflictWithoutSaving()
{
    Scenario scenario;
    const QUuid otherCourseId = QUuid::createUuid();
    scenario.courses.save(applicationTestCourse(
        otherCourseId, scenario.termId, QStringLiteral("大学英语")));
    scenario.meetings.save(applicationTestMeeting(
        QUuid::createUuid(),
        otherCourseId,
        Weekday::Monday,
        2,
        3,
        3,
        15,
        WeekPattern::OddWeeks));
    SaveCourseMeeting useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(scenario.request());

    QVERIFY(std::holds_alternative<CourseMeetingConflicts>(result));
    const auto &conflicts = std::get<CourseMeetingConflicts>(result).items;
    QCOMPARE(conflicts.size(), std::size_t{1});
    QCOMPARE(conflicts.front().course.name(), QStringLiteral("大学英语"));
    QCOMPARE(conflicts.front().firstConflictingWeek, 3);
    QCOMPARE(scenario.meetings.findAll()->size(), std::size_t{1});
}

void SaveCourseMeetingTest::allowsOddAndEvenMeetingsAtTheSameTime()
{
    Scenario scenario;
    scenario.meetings.save(applicationTestMeeting(
        QUuid::createUuid(),
        scenario.courseId,
        Weekday::Monday,
        1,
        2,
        1,
        18,
        WeekPattern::OddWeeks));
    auto request = scenario.request();
    request.weekPattern = WeekPattern::EvenWeeks;
    SaveCourseMeeting useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(request);

    QVERIFY(std::holds_alternative<CourseMeeting>(result));
    QCOMPARE(scenario.meetings.findAll()->size(), std::size_t{2});
}

void SaveCourseMeetingTest::updatesMeetingAndExcludesItselfFromConflicts()
{
    Scenario scenario;
    const QUuid meetingId = QUuid::createUuid();
    scenario.meetings.save(applicationTestMeeting(
        meetingId, scenario.courseId));
    auto request = scenario.request();
    request.id = meetingId;
    request.weekday = Weekday::Tuesday;
    request.startPeriod = 3;
    request.endPeriod = 4;
    SaveCourseMeeting useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(request);

    QVERIFY(std::holds_alternative<CourseMeeting>(result));
    QCOMPARE(std::get<CourseMeeting>(result).id(), meetingId);
    QCOMPARE(scenario.meetings.findAll()->size(), std::size_t{1});
    QCOMPARE(
        scenario.meetings.findById(meetingId)->weekday(),
        Weekday::Tuesday);
}

void SaveCourseMeetingTest::reportsMissingMeeting()
{
    Scenario scenario;
    auto request = scenario.request();
    request.id = QUuid::createUuid();
    SaveCourseMeeting useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    const auto result = useCase.execute(request);

    QCOMPARE(
        std::get<SaveCourseMeetingError>(result),
        SaveCourseMeetingError::MeetingNotFound);
}

void SaveCourseMeetingTest::reportsValidationAndRelationErrors()
{
    Scenario scenario(12);
    SaveCourseMeeting useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    auto request = scenario.request();
    request.startPeriod = 0;
    QCOMPARE(
        std::get<SaveCourseMeetingError>(useCase.execute(request)),
        SaveCourseMeetingError::InvalidInput);

    request = scenario.request();
    request.courseId = QUuid::createUuid();
    QCOMPARE(
        std::get<SaveCourseMeetingError>(useCase.execute(request)),
        SaveCourseMeetingError::CourseNotFound);

    const QUuid orphanCourseId = QUuid::createUuid();
    scenario.courses.save(applicationTestCourse(
        orphanCourseId, QUuid::createUuid()));
    request = scenario.request();
    request.courseId = orphanCourseId;
    QCOMPARE(
        std::get<SaveCourseMeetingError>(useCase.execute(request)),
        SaveCourseMeetingError::TermNotFound);

    request = scenario.request();
    request.endWeek = 13;
    QCOMPARE(
        std::get<SaveCourseMeetingError>(useCase.execute(request)),
        SaveCourseMeetingError::MeetingExceedsTerm);
}

void SaveCourseMeetingTest::reportsStorageFailure()
{
    Scenario scenario;
    SaveCourseMeeting useCase(
        scenario.terms, scenario.courses, scenario.meetings);

    scenario.courses.setQueryShouldSucceed(false);
    QCOMPARE(
        std::get<SaveCourseMeetingError>(useCase.execute(scenario.request())),
        SaveCourseMeetingError::StorageFailure);

    scenario.courses.setQueryShouldSucceed(true);
    scenario.meetings.setSaveShouldSucceed(false);
    QCOMPARE(
        std::get<SaveCourseMeetingError>(useCase.execute(scenario.request())),
        SaveCourseMeetingError::StorageFailure);
}

QTEST_APPLESS_MAIN(SaveCourseMeetingTest)

#include "SaveCourseMeetingTest.moc"
