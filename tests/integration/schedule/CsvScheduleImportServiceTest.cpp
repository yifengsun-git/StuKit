#include "ScheduleSqliteTestSupport.h"
#include "SaveCourse.h"
#include "SaveCourseMeeting.h"
#include "database/SqliteDatabase.h"
#include "import/CsvScheduleImportService.h"
#include "repositories/SqliteAcademicTermRepository.h"
#include "repositories/SqliteCourseMeetingRepository.h"
#include "repositories/SqliteCourseRepository.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace stukit::schedule;
using namespace stukit::schedule::testing;

class CsvScheduleImportServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void importsCoursesAndCustomWeeks();
    void rollsBackAllRowsWhenConflictOccurs();
    void rejectsUnknownHeader();

private:
    QString writeCsv(const QByteArray &contents);

    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<stukit::accounting::SqliteDatabase> m_database;
    std::unique_ptr<SqliteAcademicTermRepository> m_terms;
    std::unique_ptr<SqliteCourseRepository> m_courses;
    std::unique_ptr<SqliteCourseMeetingRepository> m_meetings;
    std::unique_ptr<SaveCourse> m_saveCourse;
    std::unique_ptr<SaveCourseMeeting> m_saveMeeting;
    std::unique_ptr<CsvScheduleImportService> m_importer;
    QUuid m_termId;
    int m_fileIndex{0};
};

void CsvScheduleImportServiceTest::init()
{
    m_directory = std::make_unique<QTemporaryDir>(QDir::current().filePath(
        QStringLiteral("stukit-schedule-import-test-XXXXXX")));
    QVERIFY(m_directory->isValid());
    m_database = std::make_unique<stukit::accounting::SqliteDatabase>(
        uniqueScheduleConnectionName(QStringLiteral("csv-import")));
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
    m_saveCourse = std::make_unique<SaveCourse>(*m_terms, *m_courses);
    m_saveMeeting = std::make_unique<SaveCourseMeeting>(
        *m_terms, *m_courses, *m_meetings);
    m_importer = std::make_unique<CsvScheduleImportService>(
        *m_courses,
        *m_meetings,
        *m_saveCourse,
        *m_saveMeeting);
    m_termId = QUuid::createUuid();
    QVERIFY(m_terms->save(makeTerm(m_termId)));
}

void CsvScheduleImportServiceTest::cleanup()
{
    m_importer.reset();
    m_saveMeeting.reset();
    m_saveCourse.reset();
    m_meetings.reset();
    m_courses.reset();
    m_terms.reset();
    m_database.reset();
    m_directory.reset();
}

QString CsvScheduleImportServiceTest::writeCsv(const QByteArray &contents)
{
    const QString path = m_directory->filePath(
        QStringLiteral("schedule-%1.csv").arg(++m_fileIndex));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size()) {
        return {};
    }
    file.close();
    return path;
}

void CsvScheduleImportServiceTest::importsCoursesAndCustomWeeks()
{
    const QString path = writeCsv(QByteArray::fromStdString(
        "\xEF\xBB\xBF课程名称,教师,教室,颜色,星期,开始节次,结束节次,教学周,考核标准,教学安排\r\n"
        "高等数学,王老师,A101,#3B82F6,周一,1,2,\"1;3;5\",平时30%,第一章\r\n"
        "高等数学,王老师,A101,#3B82F6,3,3,4,1-18,平时30%,第一章\r\n"));
    QVERIFY(!path.isEmpty());

    const ScheduleImportResult result = m_importer->importCsv(path, m_termId);

    QVERIFY(result.succeeded());
    QCOMPARE(result.importedCourses, 1);
    QCOMPARE(result.importedMeetings, 2);
    QCOMPARE(m_courses->findAll()->size(), std::size_t{1});
    const auto meetings = m_meetings->findAll();
    QCOMPARE(meetings->size(), std::size_t{2});
    QCOMPARE(meetings->front().activeWeeks(), std::vector<int>({1, 3, 5}));
}

void CsvScheduleImportServiceTest::rollsBackAllRowsWhenConflictOccurs()
{
    const QString path = writeCsv(QByteArray::fromStdString(
        "course_name,teacher,location,color,weekday,start_period,end_period,weeks\n"
        "高等数学,王老师,A101,#3B82F6,1,1,2,1-18\n"
        "大学英语,李老师,B201,#EF4444,周一,2,3,1-18\n"));
    QVERIFY(!path.isEmpty());

    const ScheduleImportResult result = m_importer->importCsv(path, m_termId);

    QCOMPARE(result.error, ScheduleImportError::Conflict);
    QCOMPARE(result.rowNumber, 3);
    QVERIFY(m_courses->findAll()->empty());
    QVERIFY(m_meetings->findAll()->empty());
}

void CsvScheduleImportServiceTest::rejectsUnknownHeader()
{
    const QString path = writeCsv("name,day\nmath,monday\n");
    QVERIFY(!path.isEmpty());

    const ScheduleImportResult result = m_importer->importCsv(path, m_termId);

    QCOMPARE(result.error, ScheduleImportError::InvalidHeader);
    QVERIFY(m_courses->findAll()->empty());
}

QTEST_GUILESS_MAIN(CsvScheduleImportServiceTest)

#include "CsvScheduleImportServiceTest.moc"
