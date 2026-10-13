#include "ScheduleSqliteTestSupport.h"
#include "database/SqliteDatabase.h"
#include "settings/SqlitePeriodDefinitionService.h"

#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace stukit::schedule;
using namespace stukit::schedule::testing;

class SqlitePeriodDefinitionServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void loadsDefaultsAndSavesCustomization();
    void rejectsInvalidTimes();

private:
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<stukit::accounting::SqliteDatabase> m_database;
    std::unique_ptr<SqlitePeriodDefinitionService> m_service;
};

void SqlitePeriodDefinitionServiceTest::init()
{
    m_directory = std::make_unique<QTemporaryDir>(QDir(
        QCoreApplication::applicationDirPath()).filePath(
            QStringLiteral("stukit-period-definition-test-XXXXXX")));
    QVERIFY(m_directory->isValid());
    m_database = std::make_unique<stukit::accounting::SqliteDatabase>(
        uniqueScheduleConnectionName(QStringLiteral("period-definition")));
    QVERIFY2(
        m_database->open(m_directory->filePath(QStringLiteral("stukit.sqlite"))),
        qPrintable(m_database->lastError()));
    QVERIFY2(m_database->migrate(), qPrintable(m_database->lastError()));
    m_service = std::make_unique<SqlitePeriodDefinitionService>(
        m_database->connection());
}

void SqlitePeriodDefinitionServiceTest::cleanup()
{
    m_service.reset();
    m_database.reset();
    m_directory.reset();
}

void SqlitePeriodDefinitionServiceTest::loadsDefaultsAndSavesCustomization()
{
    const LoadPeriodDefinitionsResult defaults = m_service->load();
    QVERIFY(std::holds_alternative<std::vector<PeriodDefinition>>(defaults));
    const auto &items = std::get<std::vector<PeriodDefinition>>(defaults);
    QCOMPARE(items.size(), std::size_t{12});
    QCOMPARE(items.front().name, QStringLiteral("第 1 节"));
    QCOMPARE(items.front().startTime, QStringLiteral("08:00"));

    QCOMPARE(
        m_service->save(PeriodDefinition{
            .periodNumber = 1,
            .name = QStringLiteral("早读"),
            .startTime = QStringLiteral("07:30"),
            .endTime = QStringLiteral("07:55"),
        }),
        PeriodDefinitionError::None);

    const auto updated = std::get<std::vector<PeriodDefinition>>(
        m_service->load());
    QCOMPARE(updated.front().name, QStringLiteral("早读"));
    QCOMPARE(updated.front().startTime, QStringLiteral("07:30"));
    QCOMPARE(updated.front().endTime, QStringLiteral("07:55"));
}

void SqlitePeriodDefinitionServiceTest::rejectsInvalidTimes()
{
    QCOMPARE(
        m_service->save(PeriodDefinition{
            .periodNumber = 2,
            .name = QStringLiteral("第二节"),
            .startTime = QStringLiteral("09:40"),
            .endTime = QStringLiteral("08:55"),
        }),
        PeriodDefinitionError::InvalidInput);
    QCOMPARE(
        std::get<std::vector<PeriodDefinition>>(m_service->load()).at(1).name,
        QStringLiteral("第 2 节"));
}

QTEST_GUILESS_MAIN(SqlitePeriodDefinitionServiceTest)

#include "SqlitePeriodDefinitionServiceTest.moc"
