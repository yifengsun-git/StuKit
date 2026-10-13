#include "ScheduleSqliteTestSupport.h"
#include "database/SqliteDatabase.h"
#include "repositories/SqliteAcademicTermRepository.h"
#include "repositories/SqliteCourseRepository.h"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace stukit::schedule;
using namespace stukit::schedule::testing;

class SqliteCourseRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void savesAndFindsAllFields();
    void returnsNulloptForMissingCourse();
    void updatesCourseWithSameId();
    void removesCourseAndItsMeetings();
    void filtersByTermAndSortsByName();
    void rejectsUnknownTerm();
    void persistsAcrossRepositoryInstances();
    void rejectsInvalidStoredRow();

private:
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<stukit::accounting::SqliteDatabase> m_database;
    std::unique_ptr<SqliteAcademicTermRepository> m_terms;
    std::unique_ptr<SqliteCourseRepository> m_courses;
    QUuid m_termId;
};

void SqliteCourseRepositoryTest::init()
{
    m_directory = std::make_unique<QTemporaryDir>(QDir::current().filePath(
        QStringLiteral("stukit-course-repository-test-XXXXXX")));
    QVERIFY(m_directory->isValid());
    m_database = std::make_unique<stukit::accounting::SqliteDatabase>(
        uniqueScheduleConnectionName(QStringLiteral("course")));
    QVERIFY2(
        m_database->open(m_directory->filePath(QStringLiteral("stukit.sqlite"))),
        qPrintable(m_database->lastError()));
    QVERIFY2(m_database->migrate(), qPrintable(m_database->lastError()));
    m_terms = std::make_unique<SqliteAcademicTermRepository>(
        m_database->connection());
    m_courses = std::make_unique<SqliteCourseRepository>(
        m_database->connection());
    m_termId = QUuid::createUuid();
    QVERIFY(m_terms->save(makeTerm(m_termId)));
}

void SqliteCourseRepositoryTest::cleanup()
{
    m_courses.reset();
    m_terms.reset();
    m_database.reset();
    m_directory.reset();
}

void SqliteCourseRepositoryTest::savesAndFindsAllFields()
{
    const QUuid id = QUuid::createUuid();
    QVERIFY(m_courses->save(makeCourse(
        id,
        m_termId,
        QStringLiteral("高等数学"),
        QStringLiteral("王老师"),
        QStringLiteral("A101"),
        QStringLiteral("#3B82F6"),
        QStringLiteral("平时 30%，期末 70%"),
        QStringLiteral("第 1～4 周：基础章节"))));

    const auto stored = m_courses->findById(id);

    QVERIFY2(stored.has_value(), qPrintable(m_courses->lastError()));
    QCOMPARE(stored->id(), id);
    QCOMPARE(stored->termId(), m_termId);
    QCOMPARE(stored->name(), QStringLiteral("高等数学"));
    QCOMPARE(stored->teacher(), QStringLiteral("王老师"));
    QCOMPARE(stored->location(), QStringLiteral("A101"));
    QCOMPARE(
        stored->assessmentStandard(), QStringLiteral("平时 30%，期末 70%"));
    QCOMPARE(
        stored->teachingPlan(), QStringLiteral("第 1～4 周：基础章节"));
    QCOMPARE(stored->color(), QStringLiteral("#3B82F6"));
}

void SqliteCourseRepositoryTest::returnsNulloptForMissingCourse()
{
    QVERIFY(!m_courses->findById(QUuid::createUuid()).has_value());
    QVERIFY(m_courses->lastError().isEmpty());
    const auto empty = m_courses->findByTermId(QUuid::createUuid());
    QVERIFY(empty.has_value());
    QVERIFY(empty->empty());
}

void SqliteCourseRepositoryTest::updatesCourseWithSameId()
{
    const QUuid id = QUuid::createUuid();
    QVERIFY(m_courses->save(makeCourse(id, m_termId)));
    QVERIFY(m_courses->save(makeCourse(
        id,
        m_termId,
        QStringLiteral("线性代数"),
        QString{},
        QStringLiteral("B202"),
        QStringLiteral("#EF4444"))));

    const auto courses = m_courses->findAll();

    QVERIFY(courses.has_value());
    QCOMPARE(courses->size(), std::size_t{1});
    QCOMPARE(courses->front().name(), QStringLiteral("线性代数"));
    QVERIFY(courses->front().teacher().isEmpty());
    QCOMPARE(courses->front().location(), QStringLiteral("B202"));
    QCOMPARE(courses->front().color(), QStringLiteral("#EF4444"));
}

void SqliteCourseRepositoryTest::removesCourseAndItsMeetings()
{
    const QUuid courseId = QUuid::createUuid();
    const QString courseText = courseId.toString(QUuid::WithoutBraces);
    QVERIFY(m_courses->save(makeCourse(courseId, m_termId)));

    QSqlQuery query(m_database->connection());
    query.prepare(QStringLiteral(
        "INSERT INTO course_meetings(id, course_id, weekday, start_period, "
        "end_period, start_week, end_week, week_pattern) "
        "VALUES('meeting', :courseId, 1, 1, 2, 1, 18, 0)"));
    query.bindValue(QStringLiteral(":courseId"), courseText);
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO course_meeting_weeks(meeting_id, week_number) "
        "VALUES('meeting', 1)")));

    QVERIFY(m_courses->remove(courseId));
    QVERIFY(query.exec(QStringLiteral(
        "SELECT (SELECT COUNT(*) FROM course_meetings), "
        "(SELECT COUNT(*) FROM course_meeting_weeks)")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);
    QCOMPARE(query.value(1).toInt(), 0);
}

void SqliteCourseRepositoryTest::filtersByTermAndSortsByName()
{
    const QUuid otherTermId = QUuid::createUuid();
    QVERIFY(m_terms->save(makeTerm(
        otherTermId, QStringLiteral("另一学期"), QDate(2027, 2, 22))));
    QVERIFY(m_courses->save(makeCourse(
        QUuid::createUuid(), m_termId, QStringLiteral("线性代数"))));
    QVERIFY(m_courses->save(makeCourse(
        QUuid::createUuid(), m_termId, QStringLiteral("大学英语"))));
    QVERIFY(m_courses->save(makeCourse(
        QUuid::createUuid(), otherTermId, QStringLiteral("其他课程"))));

    const auto courses = m_courses->findByTermId(m_termId);

    QVERIFY(courses.has_value());
    QCOMPARE(courses->size(), std::size_t{2});
    QCOMPARE(courses->at(0).name(), QStringLiteral("大学英语"));
    QCOMPARE(courses->at(1).name(), QStringLiteral("线性代数"));
}

void SqliteCourseRepositoryTest::rejectsUnknownTerm()
{
    QVERIFY(!m_courses->save(makeCourse(
        QUuid::createUuid(), QUuid::createUuid())));
    QVERIFY(!m_courses->lastError().isEmpty());
}

void SqliteCourseRepositoryTest::persistsAcrossRepositoryInstances()
{
    const QUuid id = QUuid::createUuid();
    QVERIFY(m_courses->save(makeCourse(id, m_termId)));
    m_courses.reset();
    m_courses = std::make_unique<SqliteCourseRepository>(
        m_database->connection());

    QVERIFY(m_courses->findById(id).has_value());
}

void SqliteCourseRepositoryTest::rejectsInvalidStoredRow()
{
    QSqlQuery query(m_database->connection());
    QVERIFY(query.exec(QStringLiteral("PRAGMA ignore_check_constraints = ON")));
    query.prepare(QStringLiteral(
        "INSERT INTO courses(id, term_id, name, teacher, location, color) "
        "VALUES('broken', :termId, '   ', '', '', '#3B82F6')"));
    query.bindValue(
        QStringLiteral(":termId"), m_termId.toString(QUuid::WithoutBraces));
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
    QVERIFY(query.exec(QStringLiteral("PRAGMA ignore_check_constraints = OFF")));

    QVERIFY(!m_courses->findAll().has_value());
    QVERIFY(m_courses->lastError().contains(QStringLiteral("invalid")));
}

QTEST_GUILESS_MAIN(SqliteCourseRepositoryTest)

#include "SqliteCourseRepositoryTest.moc"
