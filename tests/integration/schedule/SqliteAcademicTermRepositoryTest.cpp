#include "ScheduleSqliteTestSupport.h"
#include "database/SqliteDatabase.h"
#include "repositories/SqliteAcademicTermRepository.h"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace stukit::schedule;
using namespace stukit::schedule::testing;

class SqliteAcademicTermRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void savesAndFindsAllFields();
    void returnsNulloptForMissingTerm();
    void updatesTermWithSameId();
    void returnsTermsInStableOrder();
    void persistsAcrossRepositoryInstances();
    void removesTermAndItsScheduleData();
    void rejectsInvalidStoredRow();
    void reportsClosedDatabase();

private:
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<stukit::accounting::SqliteDatabase> m_database;
    std::unique_ptr<SqliteAcademicTermRepository> m_repository;
};

void SqliteAcademicTermRepositoryTest::init()
{
    m_directory = std::make_unique<QTemporaryDir>(QDir::current().filePath(
        QStringLiteral("stukit-term-repository-test-XXXXXX")));
    QVERIFY(m_directory->isValid());
    m_database = std::make_unique<stukit::accounting::SqliteDatabase>(
        uniqueScheduleConnectionName(QStringLiteral("term")));
    QVERIFY2(
        m_database->open(m_directory->filePath(QStringLiteral("stukit.sqlite"))),
        qPrintable(m_database->lastError()));
    QVERIFY2(m_database->migrate(), qPrintable(m_database->lastError()));
    m_repository = std::make_unique<SqliteAcademicTermRepository>(
        m_database->connection());
}

void SqliteAcademicTermRepositoryTest::cleanup()
{
    m_repository.reset();
    m_database.reset();
    m_directory.reset();
}

void SqliteAcademicTermRepositoryTest::savesAndFindsAllFields()
{
    const QUuid id = QUuid::createUuid();
    QVERIFY(m_repository->save(makeTerm(
        id, QStringLiteral("秋季学期"), QDate(2026, 9, 7), 18, true)));

    const auto stored = m_repository->findById(id);

    QVERIFY2(stored.has_value(), qPrintable(m_repository->lastError()));
    QCOMPARE(stored->id(), id);
    QCOMPARE(stored->name(), QStringLiteral("秋季学期"));
    QCOMPARE(stored->startDate(), QDate(2026, 9, 7));
    QCOMPARE(stored->totalWeeks(), 18);
    QVERIFY(stored->isArchived());
}

void SqliteAcademicTermRepositoryTest::returnsNulloptForMissingTerm()
{
    QVERIFY(!m_repository->findById(QUuid::createUuid()).has_value());
    QVERIFY(m_repository->lastError().isEmpty());
}

void SqliteAcademicTermRepositoryTest::updatesTermWithSameId()
{
    const QUuid id = QUuid::createUuid();
    QVERIFY(m_repository->save(makeTerm(id)));
    QVERIFY(m_repository->save(makeTerm(
        id, QStringLiteral("更新学期"), QDate(2027, 2, 22), 20, true)));

    const auto terms = m_repository->findAll();

    QVERIFY(terms.has_value());
    QCOMPARE(terms->size(), std::size_t{1});
    QCOMPARE(terms->front().name(), QStringLiteral("更新学期"));
    QCOMPARE(terms->front().totalWeeks(), 20);
    QVERIFY(terms->front().isArchived());
}

void SqliteAcademicTermRepositoryTest::returnsTermsInStableOrder()
{
    QVERIFY(m_repository->save(makeTerm(
        QUuid::createUuid(), QStringLiteral("较早"), QDate(2025, 9, 1))));
    QVERIFY(m_repository->save(makeTerm(
        QUuid::createUuid(), QStringLiteral("较新"), QDate(2026, 9, 7))));
    QVERIFY(m_repository->save(makeTerm(
        QUuid::createUuid(), QStringLiteral("已归档"),
        QDate(2027, 9, 6), 18, true)));

    const auto terms = m_repository->findAll();

    QVERIFY(terms.has_value());
    QCOMPARE(terms->size(), std::size_t{3});
    QCOMPARE(terms->at(0).name(), QStringLiteral("较新"));
    QCOMPARE(terms->at(1).name(), QStringLiteral("较早"));
    QCOMPARE(terms->at(2).name(), QStringLiteral("已归档"));
}

void SqliteAcademicTermRepositoryTest::persistsAcrossRepositoryInstances()
{
    const QUuid id = QUuid::createUuid();
    QVERIFY(m_repository->save(makeTerm(id)));
    m_repository.reset();
    m_repository = std::make_unique<SqliteAcademicTermRepository>(
        m_database->connection());

    QVERIFY(m_repository->findById(id).has_value());
}

void SqliteAcademicTermRepositoryTest::removesTermAndItsScheduleData()
{
    const QUuid termId = QUuid::createUuid();
    const QUuid courseId = QUuid::createUuid();
    const QUuid meetingId = QUuid::createUuid();
    QVERIFY(m_repository->save(makeTerm(
        termId, QStringLiteral("待删除学期"), QDate(2026, 9, 7), 18, true)));

    QSqlQuery query(m_database->connection());
    query.prepare(QStringLiteral(
        "INSERT INTO courses(id, term_id, name, teacher, location, color) "
        "VALUES(:id, :termId, '测试课程', '', '', '#3F51B5')"));
    query.bindValue(QStringLiteral(":id"),
                    courseId.toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":termId"),
                    termId.toString(QUuid::WithoutBraces));
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));

    query.prepare(QStringLiteral(
        "INSERT INTO course_meetings("
        "id, course_id, weekday, start_period, end_period, "
        "start_week, end_week, week_pattern) "
        "VALUES(:id, :courseId, 1, 1, 2, 1, 18, 0)"));
    query.bindValue(QStringLiteral(":id"),
                    meetingId.toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":courseId"),
                    courseId.toString(QUuid::WithoutBraces));
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));

    QVERIFY2(m_repository->remove(termId), qPrintable(m_repository->lastError()));
    QVERIFY(!m_repository->findById(termId).has_value());

    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM courses")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);
    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM course_meetings")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);
}

void SqliteAcademicTermRepositoryTest::rejectsInvalidStoredRow()
{
    QSqlQuery query(m_database->connection());
    QVERIFY(query.exec(QStringLiteral("PRAGMA ignore_check_constraints = ON")));
    QVERIFY2(query.exec(QStringLiteral(
                 "INSERT INTO academic_terms("
                 "id, name, start_date, total_weeks, archived) "
                 "VALUES('broken', '损坏', 'not-a-date', 18, 0)")),
             qPrintable(query.lastError().text()));
    QVERIFY(query.exec(QStringLiteral("PRAGMA ignore_check_constraints = OFF")));

    QVERIFY(!m_repository->findAll().has_value());
    QVERIFY(m_repository->lastError().contains(QStringLiteral("invalid")));
}

void SqliteAcademicTermRepositoryTest::reportsClosedDatabase()
{
    m_database->connection().close();

    QVERIFY(!m_repository->findAll().has_value());
    QVERIFY(!m_repository->lastError().isEmpty());
}

QTEST_GUILESS_MAIN(SqliteAcademicTermRepositoryTest)

#include "SqliteAcademicTermRepositoryTest.moc"
