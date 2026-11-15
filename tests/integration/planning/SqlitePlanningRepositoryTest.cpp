#include "PlanRevision.h"
#include "PlanningProject.h"
#include "SourceDocument.h"
#include "database/SqliteDatabase.h"
#include "SqlitePlanRevisionRepository.h"
#include "SqlitePlanningProjectRepository.h"
#include "SqliteSourceDocumentRepository.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>

using namespace stukit::planning;

namespace {

QDateTime utc(int minute)
{
    return QDateTime(QDate(2026, 11, 3), QTime(10, minute),
                     QTimeZone::UTC);
}

} // namespace

class SqlitePlanningRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void persistsAndRestoresWorkspaceRecords();
};

void SqlitePlanningRepositoryTest::persistsAndRestoresWorkspaceRecords()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("planning-repository-test-XXXXXX")));
    QVERIFY(directory.isValid());
    stukit::accounting::SqliteDatabase database(
        QStringLiteral("planning-repository-test"));
    QVERIFY2(database.open(directory.filePath(QStringLiteral("test.sqlite"))),
             qPrintable(database.lastError()));
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));

    SqlitePlanningProjectRepository projects(database.connection());
    SqliteSourceDocumentRepository sources(database.connection());
    SqlitePlanRevisionRepository revisions(database.connection());
    QString error;

    const QUuid projectId = QUuid::createUuid();
    const auto project = PlanningProject::create(PlanningProjectData{
        .id = projectId,
        .title = QStringLiteral("本科成长规划"),
        .objective = QStringLiteral("完成培养要求并准备实习"),
        .constraints = QStringLiteral("每周投入 10 小时"),
        .status = PlanningProjectStatus::Active,
        .createdAtUtc = utc(0),
        .updatedAtUtc = utc(0),
    });
    QVERIFY(project.has_value());
    QVERIFY2(projects.save(*project, error), qPrintable(error));

    const QUuid sourceId = QUuid::createUuid();
    const auto source = SourceDocument::create(SourceDocumentData{
        .id = sourceId,
        .projectId = projectId,
        .title = QStringLiteral("培养方案"),
        .kind = SourceDocumentKind::Curriculum,
        .format = SourceDocumentFormat::Markdown,
        .originalFileName = QStringLiteral("curriculum.md"),
        .content = QStringLiteral("# 毕业要求\n实践学分不少于 8"),
        .contentSha256 = {},
        .importedAtUtc = utc(1),
    });
    QVERIFY(source.has_value());
    QVERIFY2(sources.save(*source, error), qPrintable(error));

    const auto revision = PlanRevision::create(PlanRevisionData{
        .id = QUuid::createUuid(),
        .projectId = projectId,
        .revisionNumber = 1,
        .markdownContent = QStringLiteral("# 路线图\n\n- [ ] 核对实践学分 [S1]"),
        .providerId = QStringLiteral("openai-compatible"),
        .modelId = QStringLiteral("deepseek-flash"),
        .sourceIds = {sourceId},
        .createdAtUtc = utc(2),
    });
    QVERIFY(revision.has_value());
    QVERIFY2(revisions.save(*revision, error), qPrintable(error));

    const auto restoredProject = projects.findById(projectId, error);
    QVERIFY2(restoredProject.has_value(), qPrintable(error));
    QCOMPARE(restoredProject->constraints(), QStringLiteral("每周投入 10 小时"));
    const auto restoredSources = sources.forProject(projectId, error);
    QCOMPARE(restoredSources.size(), 1U);
    QCOMPARE(restoredSources.front().content(), source->content());
    const auto restoredRevision = revisions.latest(projectId, error);
    QVERIFY2(restoredRevision.has_value(), qPrintable(error));
    QCOMPARE(restoredRevision->sourceIds(), std::vector<QUuid>{sourceId});
    QCOMPARE(restoredRevision->modelId(), QStringLiteral("deepseek-flash"));

    QVERIFY2(sources.remove(sourceId, error), qPrintable(error));
    QVERIFY(!sources.findById(sourceId, error).has_value());
    QVERIFY(error.isEmpty());
}

QTEST_GUILESS_MAIN(SqlitePlanningRepositoryTest)

#include "SqlitePlanningRepositoryTest.moc"
