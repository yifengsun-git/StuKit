#include "support/InMemoryAcademicTermRepository.h"
#include "support/InMemoryCourseMeetingRepository.h"
#include "support/InMemoryCourseRepository.h"

#include <QTest>

using namespace stukit::schedule;
using namespace stukit::schedule::testing;

namespace {

AcademicTerm makeTerm(
    const QUuid &id = QUuid::createUuid(),
    const QString &name = QStringLiteral("2026 秋季学期"))
{
    return AcademicTerm::create(AcademicTermData{
        .id = id,
        .name = name,
        .startDate = QDate(2026, 9, 7),
        .totalWeeks = 18,
        .archived = false,
    }).value();
}

Course makeCourse(
    const QUuid &termId,
    const QUuid &id = QUuid::createUuid(),
    const QString &name = QStringLiteral("高等数学"))
{
    return Course::create(CourseData{
        .id = id,
        .termId = termId,
        .name = name,
        .teacher = QStringLiteral("王老师"),
        .location = QStringLiteral("A101"),
        .color = QStringLiteral("#3B82F6"),
    }).value();
}

CourseMeeting makeMeeting(
    const QUuid &courseId,
    const QUuid &id = QUuid::createUuid(),
    int startPeriod = 1,
    int endPeriod = 2)
{
    return CourseMeeting::create(CourseMeetingData{
        .id = id,
        .courseId = courseId,
        .weekday = Weekday::Monday,
        .startPeriod = startPeriod,
        .endPeriod = endPeriod,
        .startWeek = 1,
        .endWeek = 16,
        .weekPattern = WeekPattern::EveryWeek,
    }).value();
}

} // namespace

class ScheduleRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void storesUpdatesAndFindsTerms();
    void filtersCoursesByTerm();
    void filtersMeetingsByCourse();
    void reportsSaveAndQueryFailures();
};

void ScheduleRepositoryTest::storesUpdatesAndFindsTerms()
{
    InMemoryAcademicTermRepository repository;
    const QUuid id = QUuid::createUuid();

    QVERIFY(repository.save(makeTerm(id)));
    QVERIFY(repository.save(makeTerm(id, QStringLiteral("更新后的学期"))));

    const auto stored = repository.findById(id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->name(), QStringLiteral("更新后的学期"));
    QVERIFY(!repository.findById(QUuid::createUuid()).has_value());
    QCOMPARE(repository.findAll()->size(), std::size_t{1});
}

void ScheduleRepositoryTest::filtersCoursesByTerm()
{
    InMemoryCourseRepository repository;
    const QUuid firstTermId = QUuid::createUuid();
    const QUuid secondTermId = QUuid::createUuid();
    const QUuid courseId = QUuid::createUuid();

    QVERIFY(repository.save(makeCourse(firstTermId, courseId)));
    QVERIFY(repository.save(makeCourse(
        firstTermId, courseId, QStringLiteral("更新后的课程"))));
    QVERIFY(repository.save(makeCourse(secondTermId)));

    const auto firstTermCourses = repository.findByTermId(firstTermId);
    QVERIFY(firstTermCourses.has_value());
    QCOMPARE(firstTermCourses->size(), std::size_t{1});
    QCOMPARE(firstTermCourses->front().name(), QStringLiteral("更新后的课程"));
    QCOMPARE(repository.findByTermId(QUuid::createUuid())->size(), std::size_t{0});
    QCOMPARE(repository.findAll()->size(), std::size_t{2});
}

void ScheduleRepositoryTest::filtersMeetingsByCourse()
{
    InMemoryCourseMeetingRepository repository;
    const QUuid firstCourseId = QUuid::createUuid();
    const QUuid secondCourseId = QUuid::createUuid();
    const QUuid meetingId = QUuid::createUuid();

    QVERIFY(repository.save(makeMeeting(firstCourseId, meetingId)));
    QVERIFY(repository.save(makeMeeting(firstCourseId, meetingId, 3, 4)));
    QVERIFY(repository.save(makeMeeting(secondCourseId)));

    const auto firstCourseMeetings = repository.findByCourseId(firstCourseId);
    QVERIFY(firstCourseMeetings.has_value());
    QCOMPARE(firstCourseMeetings->size(), std::size_t{1});
    QCOMPARE(firstCourseMeetings->front().startPeriod(), 3);
    QCOMPARE(repository.findByCourseId(QUuid::createUuid())->size(), std::size_t{0});
    QCOMPARE(repository.findAll()->size(), std::size_t{2});
}

void ScheduleRepositoryTest::reportsSaveAndQueryFailures()
{
    InMemoryAcademicTermRepository terms;
    terms.setSaveShouldSucceed(false);
    QVERIFY(!terms.save(makeTerm()));
    terms.setQueryShouldSucceed(false);
    QVERIFY(!terms.findAll().has_value());

    InMemoryCourseRepository courses;
    courses.setSaveShouldSucceed(false);
    QVERIFY(!courses.save(makeCourse(QUuid::createUuid())));
    courses.setQueryShouldSucceed(false);
    QVERIFY(!courses.findByTermId(QUuid::createUuid()).has_value());

    InMemoryCourseMeetingRepository meetings;
    meetings.setSaveShouldSucceed(false);
    QVERIFY(!meetings.save(makeMeeting(QUuid::createUuid())));
    meetings.setQueryShouldSucceed(false);
    QVERIFY(!meetings.findByCourseId(QUuid::createUuid()).has_value());
}

QTEST_APPLESS_MAIN(ScheduleRepositoryTest)

#include "ScheduleRepositoryTest.moc"
