#include "SaveAcademicTerm.h"
#include "support/InMemoryAcademicTermRepository.h"
#include "support/InMemoryCourseMeetingRepository.h"
#include "support/InMemoryCourseRepository.h"
#include "support/ScheduleApplicationTestData.h"

#include <QTest>

#include <variant>

using namespace stukit::schedule;
using namespace stukit::schedule::testing;

class SaveAcademicTermTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsAndStoresTerm();
    void updatesAndArchivesExistingTerm();
    void rejectsShrinkingPastExistingMeeting();
    void allowsSafeWeekReduction();
    void reportsInvalidInputAndMissingTerm();
    void reportsRepositoryFailures();
};

void SaveAcademicTermTest::createsAndStoresTerm()
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    InMemoryCourseMeetingRepository meetings;
    SaveAcademicTerm useCase(terms, courses, meetings);

    const auto result = useCase.execute(SaveAcademicTermRequest{
        .name = QStringLiteral("  2026 秋季学期  "),
        .startDate = QDate(2026, 9, 7),
        .totalWeeks = 18,
    });

    QVERIFY(std::holds_alternative<AcademicTerm>(result));
    const auto &term = std::get<AcademicTerm>(result);
    QVERIFY(!term.id().isNull());
    QCOMPARE(term.name(), QStringLiteral("2026 秋季学期"));
    QVERIFY(terms.findById(term.id()).has_value());
}

void SaveAcademicTermTest::updatesAndArchivesExistingTerm()
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    InMemoryCourseMeetingRepository meetings;
    const QUuid id = QUuid::createUuid();
    QVERIFY(terms.save(applicationTestTerm(id)));
    SaveAcademicTerm useCase(terms, courses, meetings);

    const auto result = useCase.execute(SaveAcademicTermRequest{
        .id = id,
        .name = QStringLiteral("更新后的学期"),
        .startDate = QDate(2026, 9, 14),
        .totalWeeks = 20,
        .archived = true,
    });

    QVERIFY(std::holds_alternative<AcademicTerm>(result));
    const auto &term = std::get<AcademicTerm>(result);
    QCOMPARE(term.id(), id);
    QCOMPARE(term.totalWeeks(), 20);
    QVERIFY(term.isArchived());
    QCOMPARE(terms.findAll()->size(), std::size_t{1});
}

void SaveAcademicTermTest::rejectsShrinkingPastExistingMeeting()
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    InMemoryCourseMeetingRepository meetings;
    const QUuid termId = QUuid::createUuid();
    const QUuid courseId = QUuid::createUuid();
    QVERIFY(terms.save(applicationTestTerm(termId, 18)));
    QVERIFY(courses.save(applicationTestCourse(courseId, termId)));
    QVERIFY(meetings.save(applicationTestMeeting(
        QUuid::createUuid(), courseId,
        Weekday::Monday, 1, 2, 1, 18)));
    SaveAcademicTerm useCase(terms, courses, meetings);

    const auto result = useCase.execute(SaveAcademicTermRequest{
        .id = termId,
        .name = QStringLiteral("测试学期"),
        .startDate = QDate(2026, 9, 7),
        .totalWeeks = 16,
    });

    QCOMPARE(
        std::get<SaveAcademicTermError>(result),
        SaveAcademicTermError::HasOutOfRangeMeetings);
    QCOMPARE(terms.findById(termId)->totalWeeks(), 18);
}

void SaveAcademicTermTest::allowsSafeWeekReduction()
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    InMemoryCourseMeetingRepository meetings;
    const QUuid termId = QUuid::createUuid();
    const QUuid courseId = QUuid::createUuid();
    QVERIFY(terms.save(applicationTestTerm(termId, 18)));
    QVERIFY(courses.save(applicationTestCourse(courseId, termId)));
    QVERIFY(meetings.save(applicationTestMeeting(
        QUuid::createUuid(), courseId,
        Weekday::Monday, 1, 2, 1, 16)));
    SaveAcademicTerm useCase(terms, courses, meetings);

    const auto result = useCase.execute(SaveAcademicTermRequest{
        .id = termId,
        .name = QStringLiteral("测试学期"),
        .startDate = QDate(2026, 9, 7),
        .totalWeeks = 16,
    });

    QVERIFY(std::holds_alternative<AcademicTerm>(result));
    QCOMPARE(std::get<AcademicTerm>(result).totalWeeks(), 16);
}

void SaveAcademicTermTest::reportsInvalidInputAndMissingTerm()
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    InMemoryCourseMeetingRepository meetings;
    SaveAcademicTerm useCase(terms, courses, meetings);

    const auto invalid = useCase.execute(SaveAcademicTermRequest{
        .name = QStringLiteral("   "),
        .startDate = QDate(2026, 9, 7),
        .totalWeeks = 18,
    });
    QCOMPARE(
        std::get<SaveAcademicTermError>(invalid),
        SaveAcademicTermError::InvalidInput);

    const auto missing = useCase.execute(SaveAcademicTermRequest{
        .id = QUuid::createUuid(),
        .name = QStringLiteral("不存在"),
        .startDate = QDate(2026, 9, 7),
        .totalWeeks = 18,
    });
    QCOMPARE(
        std::get<SaveAcademicTermError>(missing),
        SaveAcademicTermError::NotFound);
}

void SaveAcademicTermTest::reportsRepositoryFailures()
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    InMemoryCourseMeetingRepository meetings;
    terms.setSaveShouldSucceed(false);
    SaveAcademicTerm useCase(terms, courses, meetings);

    const auto saveFailure = useCase.execute(SaveAcademicTermRequest{
        .name = QStringLiteral("测试学期"),
        .startDate = QDate(2026, 9, 7),
        .totalWeeks = 18,
    });
    QCOMPARE(
        std::get<SaveAcademicTermError>(saveFailure),
        SaveAcademicTermError::StorageFailure);

    terms.setSaveShouldSucceed(true);
    const QUuid id = QUuid::createUuid();
    QVERIFY(terms.save(applicationTestTerm(id)));
    terms.setQueryShouldSucceed(false);
    const auto queryFailure = useCase.execute(SaveAcademicTermRequest{
        .id = id,
        .name = QStringLiteral("测试学期"),
        .startDate = QDate(2026, 9, 7),
        .totalWeeks = 18,
    });
    QCOMPARE(
        std::get<SaveAcademicTermError>(queryFailure),
        SaveAcademicTermError::StorageFailure);
}

QTEST_APPLESS_MAIN(SaveAcademicTermTest)

#include "SaveAcademicTermTest.moc"
