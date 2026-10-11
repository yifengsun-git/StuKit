#include "SaveCourse.h"
#include "support/InMemoryAcademicTermRepository.h"
#include "support/InMemoryCourseRepository.h"
#include "support/ScheduleApplicationTestData.h"

#include <QTest>

#include <variant>

using namespace stukit::schedule;
using namespace stukit::schedule::testing;

namespace {

SaveCourseRequest validRequest(const QUuid &termId)
{
    return SaveCourseRequest{
        .termId = termId,
        .name = QStringLiteral("  高等数学  "),
        .teacher = QStringLiteral("  王老师  "),
        .location = QStringLiteral("  A101  "),
        .color = QStringLiteral("#3b82f6"),
    };
}

} // namespace

class SaveCourseTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsAndStoresCourse();
    void updatesCourseWithoutChangingId();
    void rejectsMissingAndArchivedTerm();
    void rejectsChangingTerm();
    void reportsInvalidInputAndMissingCourse();
    void reportsRepositoryFailures();
};

void SaveCourseTest::createsAndStoresCourse()
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    const QUuid termId = QUuid::createUuid();
    QVERIFY(terms.save(applicationTestTerm(termId)));
    SaveCourse useCase(terms, courses);

    const auto result = useCase.execute(validRequest(termId));

    QVERIFY(std::holds_alternative<Course>(result));
    const auto &course = std::get<Course>(result);
    QVERIFY(!course.id().isNull());
    QCOMPARE(course.name(), QStringLiteral("高等数学"));
    QCOMPARE(course.teacher(), QStringLiteral("王老师"));
    QCOMPARE(course.location(), QStringLiteral("A101"));
    QCOMPARE(course.color(), QStringLiteral("#3B82F6"));
    QVERIFY(courses.findById(course.id()).has_value());
}

void SaveCourseTest::updatesCourseWithoutChangingId()
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    const QUuid termId = QUuid::createUuid();
    const QUuid courseId = QUuid::createUuid();
    QVERIFY(terms.save(applicationTestTerm(termId)));
    QVERIFY(courses.save(applicationTestCourse(courseId, termId)));
    SaveCourse useCase(terms, courses);
    auto request = validRequest(termId);
    request.id = courseId;
    request.name = QStringLiteral("线性代数");

    const auto result = useCase.execute(request);

    QVERIFY(std::holds_alternative<Course>(result));
    QCOMPARE(std::get<Course>(result).id(), courseId);
    QCOMPARE(courses.findAll()->size(), std::size_t{1});
    QCOMPARE(courses.findById(courseId)->name(), QStringLiteral("线性代数"));
}

void SaveCourseTest::rejectsMissingAndArchivedTerm()
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    SaveCourse useCase(terms, courses);

    const auto missing = useCase.execute(validRequest(QUuid::createUuid()));
    QCOMPARE(
        std::get<SaveCourseError>(missing), SaveCourseError::TermNotFound);

    const QUuid archivedId = QUuid::createUuid();
    QVERIFY(terms.save(applicationTestTerm(archivedId, 18, true)));
    const auto archived = useCase.execute(validRequest(archivedId));
    QCOMPARE(
        std::get<SaveCourseError>(archived), SaveCourseError::TermArchived);
    QVERIFY(courses.findAll()->empty());
}

void SaveCourseTest::rejectsChangingTerm()
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    const QUuid firstTermId = QUuid::createUuid();
    const QUuid secondTermId = QUuid::createUuid();
    const QUuid courseId = QUuid::createUuid();
    QVERIFY(terms.save(applicationTestTerm(firstTermId)));
    QVERIFY(terms.save(applicationTestTerm(secondTermId)));
    QVERIFY(courses.save(applicationTestCourse(courseId, firstTermId)));
    SaveCourse useCase(terms, courses);
    auto request = validRequest(secondTermId);
    request.id = courseId;

    const auto result = useCase.execute(request);

    QCOMPARE(
        std::get<SaveCourseError>(result),
        SaveCourseError::TermCannotChange);
    QCOMPARE(courses.findById(courseId)->termId(), firstTermId);
}

void SaveCourseTest::reportsInvalidInputAndMissingCourse()
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    const QUuid termId = QUuid::createUuid();
    QVERIFY(terms.save(applicationTestTerm(termId)));
    SaveCourse useCase(terms, courses);

    auto invalidRequest = validRequest(termId);
    invalidRequest.name = QStringLiteral("   ");
    const auto invalid = useCase.execute(invalidRequest);
    QCOMPARE(
        std::get<SaveCourseError>(invalid), SaveCourseError::InvalidInput);

    auto missingTermIdRequest = validRequest({});
    const auto missingTermId = useCase.execute(missingTermIdRequest);
    QCOMPARE(
        std::get<SaveCourseError>(missingTermId),
        SaveCourseError::InvalidInput);

    auto missingRequest = validRequest(termId);
    missingRequest.id = QUuid::createUuid();
    const auto missing = useCase.execute(missingRequest);
    QCOMPARE(
        std::get<SaveCourseError>(missing), SaveCourseError::CourseNotFound);
}

void SaveCourseTest::reportsRepositoryFailures()
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    const QUuid termId = QUuid::createUuid();
    QVERIFY(terms.save(applicationTestTerm(termId)));
    SaveCourse useCase(terms, courses);

    terms.setQueryShouldSucceed(false);
    const auto termFailure = useCase.execute(validRequest(termId));
    QCOMPARE(
        std::get<SaveCourseError>(termFailure),
        SaveCourseError::StorageFailure);

    terms.setQueryShouldSucceed(true);
    courses.setSaveShouldSucceed(false);
    const auto saveFailure = useCase.execute(validRequest(termId));
    QCOMPARE(
        std::get<SaveCourseError>(saveFailure),
        SaveCourseError::StorageFailure);
}

QTEST_APPLESS_MAIN(SaveCourseTest)

#include "SaveCourseTest.moc"
