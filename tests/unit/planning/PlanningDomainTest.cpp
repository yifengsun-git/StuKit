#include "PlanningProject.h"
#include "PlanRevision.h"
#include "SourceDocument.h"

#include <QTest>
#include <QTimeZone>

using namespace stukit::planning;

namespace {

QDateTime utc(int day, int hour = 8)
{
    return QDateTime(QDate(2026, 11, day), QTime(hour, 0), QTimeZone::UTC);
}

PlanningProjectData validProjectData()
{
    return PlanningProjectData{
        .id = QUuid::createUuid(),
        .title = QStringLiteral("本科阶段成长规划"),
        .objective = QStringLiteral("完成培养要求并形成项目作品集"),
        .constraints = QStringLiteral("每周课外投入不超过 12 小时"),
        .status = PlanningProjectStatus::Active,
        .createdAtUtc = utc(1),
        .updatedAtUtc = utc(2),
    };
}

SourceDocumentData validSourceData()
{
    return SourceDocumentData{
        .id = QUuid::createUuid(),
        .projectId = QUuid::createUuid(),
        .title = QStringLiteral("2026 版培养方案"),
        .kind = SourceDocumentKind::Curriculum,
        .format = SourceDocumentFormat::Markdown,
        .originalFileName = QStringLiteral("curriculum.md"),
        .content = QStringLiteral("# 培养方案\n\n毕业要求 160 学分。"),
        .contentSha256 = {},
        .importedAtUtc = utc(3),
    };
}

PlanRevisionData validRevisionData()
{
    return PlanRevisionData{
        .id = QUuid::createUuid(),
        .projectId = QUuid::createUuid(),
        .revisionNumber = 1,
        .markdownContent = QStringLiteral("# 学期规划\n\n- [ ] 完成课程项目"),
        .providerId = QStringLiteral("openai-compatible"),
        .modelId = QStringLiteral("deepseek-flash"),
        .sourceIds = {QUuid::createUuid(), QUuid::createUuid()},
        .createdAtUtc = utc(3),
    };
}

} // namespace

class PlanningDomainTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsAndNormalizesProject();
    void rejectsInvalidProject();
    void createsSourceAndCalculatesHash();
    void normalizesSourceLineEndings();
    void rejectsInvalidSourceAndHashMismatch();
    void createsPlanRevision();
    void rejectsInvalidPlanRevision();
};

void PlanningDomainTest::createsAndNormalizesProject()
{
    auto data = validProjectData();
    data.title = QStringLiteral("  本科阶段成长规划  ");
    data.objective = QStringLiteral("  完成培养要求  ");

    const auto project = PlanningProject::create(data);

    QVERIFY(project.has_value());
    QCOMPARE(project->title(), QStringLiteral("本科阶段成长规划"));
    QCOMPARE(project->objective(), QStringLiteral("完成培养要求"));
    QVERIFY(!project->isArchived());
}

void PlanningDomainTest::rejectsInvalidProject()
{
    auto nullId = validProjectData();
    nullId.id = {};
    QVERIFY(!PlanningProject::create(nullId).has_value());

    auto emptyObjective = validProjectData();
    emptyObjective.objective = QStringLiteral("   ");
    QVERIFY(!PlanningProject::create(emptyObjective).has_value());

    auto invalidTime = validProjectData();
    invalidTime.updatedAtUtc = utc(1, 7);
    QVERIFY(!PlanningProject::create(invalidTime).has_value());

    auto localTime = validProjectData();
    localTime.createdAtUtc = QDateTime(QDate(2026, 11, 1), QTime(8, 0));
    QVERIFY(!PlanningProject::create(localTime).has_value());
}

void PlanningDomainTest::createsSourceAndCalculatesHash()
{
    const auto source = SourceDocument::create(validSourceData());

    QVERIFY(source.has_value());
    QCOMPARE(source->title(), QStringLiteral("2026 版培养方案"));
    QCOMPARE(source->contentSha256().size(), 64);
    QCOMPARE(source->contentSha256(),
        SourceDocument::calculateContentHash(source->content()));
}

void PlanningDomainTest::normalizesSourceLineEndings()
{
    auto data = validSourceData();
    data.content = QStringLiteral("第一行\r\n第二行\r第三行");

    const auto source = SourceDocument::create(data);

    QVERIFY(source.has_value());
    QCOMPARE(source->content(), QStringLiteral("第一行\n第二行\n第三行"));
}

void PlanningDomainTest::rejectsInvalidSourceAndHashMismatch()
{
    auto nullProject = validSourceData();
    nullProject.projectId = {};
    QVERIFY(!SourceDocument::create(nullProject).has_value());

    auto blankContent = validSourceData();
    blankContent.content = QStringLiteral(" \n ");
    QVERIFY(!SourceDocument::create(blankContent).has_value());

    auto wrongHash = validSourceData();
    wrongHash.contentSha256 = QString(64, QLatin1Char('0'));
    QVERIFY(!SourceDocument::create(wrongHash).has_value());

    auto localTime = validSourceData();
    localTime.importedAtUtc = QDateTime(
        QDate(2026, 11, 3), QTime(8, 0));
    QVERIFY(!SourceDocument::create(localTime).has_value());
}

void PlanningDomainTest::createsPlanRevision()
{
    const auto revision = PlanRevision::create(validRevisionData());

    QVERIFY(revision.has_value());
    QCOMPARE(revision->revisionNumber(), 1);
    QCOMPARE(revision->sourceIds().size(), 2U);
    QCOMPARE(revision->modelId(), QStringLiteral("deepseek-flash"));
}

void PlanningDomainTest::rejectsInvalidPlanRevision()
{
    auto emptyContent = validRevisionData();
    emptyContent.markdownContent = QStringLiteral("  ");
    QVERIFY(!PlanRevision::create(emptyContent).has_value());

    auto duplicateSources = validRevisionData();
    duplicateSources.sourceIds.push_back(
        duplicateSources.sourceIds.front());
    QVERIFY(!PlanRevision::create(duplicateSources).has_value());

    auto invalidNumber = validRevisionData();
    invalidNumber.revisionNumber = 0;
    QVERIFY(!PlanRevision::create(invalidNumber).has_value());
}

QTEST_APPLESS_MAIN(PlanningDomainTest)

#include "PlanningDomainTest.moc"
