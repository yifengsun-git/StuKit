#include "InMemoryPlanningProjectRepository.h"
#include "InMemoryPlanRevisionRepository.h"
#include "InMemorySourceDocumentRepository.h"
#include "PlanningViewModel.h"
#include "PlanningWorkspace.h"
#include "PlanningAnalysis.h"
#include "TextSourceFileReader.h"
#include "MarkdownPlanFileWriter.h"
#include "ILanguageModelService.h"
#include "ISecretStore.h"

#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QSettings>
#include <QTest>
#include <QTimeZone>

using namespace stukit::planning;

namespace {

QDateTime nowUtc()
{
    return QDateTime(
        QDate(2026, 11, 3), QTime(9, 30), QTimeZone::UTC);
}

class WorkspaceFixture final
{
public:
    InMemoryPlanningProjectRepository projects;
    InMemorySourceDocumentRepository sources;
    InMemoryPlanRevisionRepository revisions;
    TextSourceFileReader reader;
    PlanningWorkspace workspace{projects, sources, revisions, reader};
};

class FakeSecretStore final : public ISecretStore
{
public:
    bool writeSecret(const QString &, const QString &value,
                     QString &error) override
    {
        error.clear();
        secret = value;
        return true;
    }
    bool readSecret(const QString &, QString &value,
                    QString &error) const override
    {
        error.clear();
        value = secret;
        return true;
    }
    bool removeSecret(const QString &, QString &error) override
    {
        error.clear();
        secret.clear();
        return true;
    }
    QString secret;
};

class FakeLanguageModelService final : public ILanguageModelService
{
public:
    void generate(LanguageModelRequest request,
                  Completion completion) override
    {
        lastRequest = std::move(request);
        completion(LanguageModelResult{
            .succeeded = true,
            .markdownContent = QStringLiteral(
                "# AI 规划\n\n## 目标摘要\n完成要求 [S1:L2]"
                "\n\n## 未来四周行动\n- [ ] 阅读培养方案 [S1:L2]"
                "\n\n## 风险与假设\n需核对版本 [S1]"
                "\n\n## 资料引用\n- [S1] 培养方案"),
            .promptTokens = 120,
            .completionTokens = 40,
        });
    }
    void cancel() override { cancelled = true; }
    LanguageModelRequest lastRequest;
    bool cancelled{false};
};

class FakePlanningActionPublisher final : public IPlanningActionPublisher
{
public:
    PlanningActionPublishResult publish(
        const std::vector<PlanningActionDraft> &drafts) override
    {
        published = drafts;
        return PlanningActionPublishResult{
            .publishedCount = static_cast<int>(drafts.size()),
        };
    }

    std::vector<PlanningActionDraft> published;
};

} // namespace

class PlanningWorkspaceTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsProjectAndAddsPastedSource();
    void rejectsDuplicateSourceContent();
    void importsSupportedUtf8File();
    void rejectsUnsupportedFile();
    void viewModelPublishesWorkspaceState();
    void savesIncrementingPlanRevisions();
    void aiGenerationCreatesAttributedRevision();
    void exportsUtf8MarkdownAtomically();
    void extractsFactsAndReportsDeterministicIssues();
    void validatesGeneratedMarkdownStructureAndCitations();
    void scoresOpportunitiesWithVisibleComponents();
    void editsProjectAndRefreshesOpportunityScoring();
    void publishesConfirmedMarkdownActionsToTasksPort();
    void sendsClaudeThroughAnthropicProtocol();
};

void PlanningWorkspaceTest::createsProjectAndAddsPastedSource()
{
    WorkspaceFixture fixture;
    QString error;
    const auto projectId = fixture.workspace.createProject(
        QStringLiteral("成长规划"),
        QStringLiteral("完成培养要求"),
        QStringLiteral("每周 10 小时"),
        nowUtc(),
        error);

    QVERIFY2(projectId.has_value(), qPrintable(error));
    QCOMPARE(fixture.workspace.projects(error).size(), 1U);

    const auto sourceId = fixture.workspace.addPastedSource(
        *projectId,
        QStringLiteral("培养方案"),
        SourceDocumentKind::Curriculum,
        SourceDocumentFormat::Markdown,
        QStringLiteral("# 毕业要求\n总学分 160"),
        nowUtc(),
        error);

    QVERIFY2(sourceId.has_value(), qPrintable(error));
    const auto sources = fixture.workspace.sources(*projectId, error);
    QCOMPARE(sources.size(), 1U);
    QCOMPARE(sources.front().kind(), SourceDocumentKind::Curriculum);
}

void PlanningWorkspaceTest::rejectsDuplicateSourceContent()
{
    WorkspaceFixture fixture;
    QString error;
    const auto projectId = fixture.workspace.createProject(
        QStringLiteral("成长规划"),
        QStringLiteral("形成作品集"),
        {},
        nowUtc(),
        error);
    QVERIFY(projectId.has_value());

    QVERIFY(fixture.workspace.addPastedSource(
        *projectId,
        QStringLiteral("资料一"),
        SourceDocumentKind::Other,
        SourceDocumentFormat::PlainText,
        QStringLiteral("相同的正文"),
        nowUtc(),
        error).has_value());
    QVERIFY(!fixture.workspace.addPastedSource(
        *projectId,
        QStringLiteral("资料二"),
        SourceDocumentKind::Other,
        SourceDocumentFormat::PlainText,
        QStringLiteral("相同的正文"),
        nowUtc(),
        error).has_value());
    QVERIFY(error.contains(QStringLiteral("内容相同")));
}

void PlanningWorkspaceTest::importsSupportedUtf8File()
{
    WorkspaceFixture fixture;
    QString error;
    const auto projectId = fixture.workspace.createProject(
        QStringLiteral("成长规划"),
        QStringLiteral("筛选竞赛"),
        {},
        nowUtc(),
        error);
    QVERIFY(projectId.has_value());

    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("planning-source-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("contest.md"));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("# 竞赛通知\n报名截止：2026-11-20") > 0);
    file.close();

    const auto sourceId = fixture.workspace.importSource(
        *projectId,
        {},
        SourceDocumentKind::Opportunity,
        filePath,
        nowUtc(),
        error);

    QVERIFY2(sourceId.has_value(), qPrintable(error));
    const auto sources = fixture.workspace.sources(*projectId, error);
    QCOMPARE(sources.size(), 1U);
    QCOMPARE(sources.front().title(), QStringLiteral("contest.md"));
    QCOMPARE(sources.front().format(), SourceDocumentFormat::Markdown);
}

void PlanningWorkspaceTest::rejectsUnsupportedFile()
{
    TextSourceFileReader reader;
    QString error;

    QVERIFY(!reader.read(QStringLiteral("curriculum.pdf"), error).has_value());
    QVERIFY(error.contains(QStringLiteral("仅支持")));
}

void PlanningWorkspaceTest::viewModelPublishesWorkspaceState()
{
    WorkspaceFixture fixture;
    presentation::PlanningViewModel viewModel(fixture.workspace, false);
    viewModel.load();
    QVERIFY(!viewModel.hasProjects());

    QVERIFY(viewModel.createProject(
        QStringLiteral("本科规划"),
        QStringLiteral("完成毕业要求"),
        QStringLiteral("不与课程冲突")));
    QVERIFY(viewModel.hasProjects());
    QCOMPARE(viewModel.projects().size(), 1);
    QVERIFY(!viewModel.selectedProjectId().isEmpty());

    QVERIFY(viewModel.addPastedSource(
        QStringLiteral("培养方案"), 0, 1,
        QStringLiteral("# 培养要求\n需要完成 8 学分实践课程")));
    QVERIFY(viewModel.addPastedSource(
        QStringLiteral("软件开发训练营"), 1, 0,
        QStringLiteral("软件开发实习训练营，报名费 100 元，每周 6 小时。")));
    QVERIFY(viewModel.hasSources());
    QCOMPARE(viewModel.sources().size(), 2);
    QVERIFY(viewModel.extractedFactCount() >= 2);
    QCOMPARE(viewModel.opportunityCount(), 1);
}

void PlanningWorkspaceTest::savesIncrementingPlanRevisions()
{
    WorkspaceFixture fixture;
    QString error;
    const auto projectId = fixture.workspace.createProject(
        QStringLiteral("学习规划"), QStringLiteral("完成培养要求"), {},
        nowUtc(), error);
    QVERIFY(projectId.has_value());
    const auto first = fixture.workspace.savePlan(
        *projectId, QStringLiteral("# 版本一"), {}, {}, {}, nowUtc(), error);
    const auto second = fixture.workspace.savePlan(
        *projectId, QStringLiteral("# 版本二"), {}, {}, {},
        nowUtc().addSecs(60), error);

    QVERIFY(first.has_value());
    QVERIFY(second.has_value());
    QCOMPARE(first->revisionNumber(), 1);
    QCOMPARE(second->revisionNumber(), 2);
    QCOMPARE(fixture.workspace.revisions(*projectId, error).size(), 2U);
}

void PlanningWorkspaceTest::aiGenerationCreatesAttributedRevision()
{
    WorkspaceFixture fixture;
    presentation::PlanningViewModel viewModel(fixture.workspace, false);
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("planning-ai-settings-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    FakeSecretStore secrets;
    FakeLanguageModelService languageModel;
    viewModel.configureAi(&languageModel, &secrets, &settings);
    viewModel.load();

    QVERIFY(viewModel.createProject(
        QStringLiteral("本科规划"), QStringLiteral("完成培养要求"), {}));
    QVERIFY(viewModel.addPastedSource(
        QStringLiteral("培养方案"), 0, 1,
        QStringLiteral("# 培养要求\n需要完成 8 学分实践课程")));
    QVERIFY(viewModel.addPastedSource(
        QStringLiteral("软件开发训练营"), 1, 0,
        QStringLiteral("软件开发实习训练营，报名费 100 元，每周 6 小时。")));
    QVERIFY(viewModel.saveProviderSettings(
        QStringLiteral("https://api.example.com/v1"),
        QStringLiteral("test-model"), QStringLiteral("secret-key")));
    QVERIFY(viewModel.generatePlan(QStringLiteral("优先准备实习")));

    QVERIFY(!viewModel.aiBusy());
    QVERIFY(viewModel.hasPlan());
    QCOMPARE(viewModel.planRevisionNumber(), 1);
    QVERIFY(viewModel.planMarkdown().contains(QStringLiteral("AI 规划")));
    QCOMPARE(languageModel.lastRequest.modelId,
             QStringLiteral("test-model"));
    QVERIFY(languageModel.lastRequest.userPrompt.contains(
        QStringLiteral("优先准备实习")));
    QVERIFY(languageModel.lastRequest.userPrompt.contains(
        QStringLiteral("<deterministic_facts>")));
    QVERIFY(languageModel.lastRequest.userPrompt.contains(
        QStringLiteral("[S1:L2]")));
    QVERIFY(languageModel.lastRequest.userPrompt.contains(
        QStringLiteral("<opportunity_scores>")));
    QCOMPARE(viewModel.opportunityCount(), 1);
    const QVariantMap revision = viewModel.planRevisions().front().toMap();
    QCOMPARE(revision.value(QStringLiteral("origin")).toString(),
             QStringLiteral("openai-compatible · test-model"));
}

void PlanningWorkspaceTest::exportsUtf8MarkdownAtomically()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("planning-export-test-XXXXXX")));
    QVERIFY(directory.isValid());
    MarkdownPlanFileWriter writer;
    QString error;
    const QString path = directory.filePath(QStringLiteral("plan"));
    const QString markdown = QStringLiteral(
        "# 学期规划\n\n```mermaid\ngantt\n  title 路线图\n```");

    QVERIFY2(writer.writeMarkdown(path, markdown, error), qPrintable(error));
    QFile file(path + QStringLiteral(".md"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(file.readAll()), markdown);
}

void PlanningWorkspaceTest::extractsFactsAndReportsDeterministicIssues()
{
    const auto source = SourceDocument::create(SourceDocumentData{
        .id = QUuid::createUuid(),
        .projectId = QUuid::createUuid(),
        .title = QStringLiteral("培养与竞赛说明"),
        .kind = SourceDocumentKind::Curriculum,
        .format = SourceDocumentFormat::PlainText,
        .originalFileName = {},
        .content = QStringLiteral(
            "毕业总学分不少于 160 学分。\n"
            "另一版本要求毕业总学分 165 学分。\n"
            "报名截止：2026年10月20日，报名费 50 元。\n"
            "复赛截止：12月5日。"),
        .contentSha256 = {},
        .importedAtUtc = nowUtc(),
    });
    QVERIFY(source.has_value());

    const PlanningFactExtractionResult extraction =
        PlanningFactExtractor::extract(
            {*source}, QDate(2026, 11, 4));

    QVERIFY(extraction.facts.size() >= 7U);
    QVERIFY(std::any_of(extraction.facts.cbegin(), extraction.facts.cend(),
        [](const ExtractedPlanningFact &fact) {
            return fact.kind == PlanningFactKind::Credit
                && fact.normalizedValue == QStringLiteral("160 学分")
                && fact.evidence.citation() == QStringLiteral("S1:L1");
        }));
    QVERIFY(std::any_of(extraction.facts.cbegin(), extraction.facts.cend(),
        [](const ExtractedPlanningFact &fact) {
            return fact.kind == PlanningFactKind::Money
                && fact.normalizedValue == QStringLiteral("50.00 元");
        }));
    QVERIFY(std::any_of(extraction.issues.cbegin(), extraction.issues.cend(),
        [](const PlanningValidationIssue &issue) {
            return issue.message.contains(QStringLiteral("已早于今天"));
        }));
    QVERIFY(std::any_of(extraction.issues.cbegin(), extraction.issues.cend(),
        [](const PlanningValidationIssue &issue) {
            return issue.message.contains(QStringLiteral("缺少年份"));
        }));
    QVERIFY(std::any_of(extraction.issues.cbegin(), extraction.issues.cend(),
        [](const PlanningValidationIssue &issue) {
            return issue.message.contains(QStringLiteral("多个毕业总学分值"));
        }));
}

void PlanningWorkspaceTest::validatesGeneratedMarkdownStructureAndCitations()
{
    const QString valid = QStringLiteral(
        "# 规划\n## 目标摘要\n说明 [S1:L3]\n"
        "## 未来四周行动\n- [ ] 行动 [S1]\n"
        "## 风险与假设\n假设 [S1]\n"
        "## 资料引用\n- [S1] 培养方案");
    const PlanningMarkdownValidationResult accepted =
        PlanningMarkdownValidator::validate(valid, 1);
    QVERIFY2(accepted.valid,
             qPrintable(accepted.errors.join(QStringLiteral("；"))));

    const PlanningMarkdownValidationResult rejected =
        PlanningMarkdownValidator::validate(
            QStringLiteral("# 规划\n结论 [S2]"), 1);
    QVERIFY(!rejected.valid);
    QVERIFY(rejected.errors.join(QStringLiteral("；"))
        .contains(QStringLiteral("没有对应的资料")));
    QVERIFY(rejected.errors.join(QStringLiteral("；"))
        .contains(QStringLiteral("缺少“目标摘要”部分")));
}

void PlanningWorkspaceTest::scoresOpportunitiesWithVisibleComponents()
{
    const QUuid projectId = QUuid::createUuid();
    const auto recommended = SourceDocument::create(SourceDocumentData{
        .id = QUuid::createUuid(),
        .projectId = projectId,
        .title = QStringLiteral("软件开发训练营"),
        .kind = SourceDocumentKind::Opportunity,
        .format = SourceDocumentFormat::PlainText,
        .originalFileName = {},
        .content = QStringLiteral(
            "软件开发实习项目，需要完成作品集。\n"
            "报名截止：2026年11月20日。\n"
            "报名费 800 元，每周投入 8 小时。"),
        .contentSha256 = {},
        .importedAtUtc = nowUtc(),
    });
    const auto expensive = SourceDocument::create(SourceDocumentData{
        .id = QUuid::createUuid(),
        .projectId = projectId,
        .title = QStringLiteral("高成本活动"),
        .kind = SourceDocumentKind::Opportunity,
        .format = SourceDocumentFormat::PlainText,
        .originalFileName = {},
        .content = QStringLiteral(
            "普通训练活动。\n报名截止：2026年11月21日。\n"
            "报名费 3000 元，每周投入 20 小时。"),
        .contentSha256 = {},
        .importedAtUtc = nowUtc(),
    });
    QVERIFY(recommended.has_value());
    QVERIFY(expensive.has_value());
    const std::vector<SourceDocument> sources{*recommended, *expensive};
    const PlanningFactExtractionResult extraction =
        PlanningFactExtractor::extract(sources, QDate(2026, 11, 4));

    const auto assessments = PlanningOpportunityScorer::score(
        sources, extraction,
        QStringLiteral("准备软件开发实习并形成作品集"),
        QStringLiteral("每周投入不超过 10 小时，预算不超过 1000 元"),
        QDate(2026, 11, 4));

    QCOMPARE(assessments.size(), 2U);
    QCOMPARE(assessments.front().title, QStringLiteral("软件开发训练营"));
    QVERIFY(assessments.front().totalScore >= 75);
    QCOMPARE(assessments.front().feasibilityScore, 25);
    QCOMPARE(assessments.back().feasibilityScore, 0);
    QVERIFY(assessments.back().warnings.join(QStringLiteral(" "))
        .contains(QStringLiteral("超过预算")));
    QVERIFY(assessments.back().warnings.join(QStringLiteral(" "))
        .contains(QStringLiteral("超过限制")));
}

void PlanningWorkspaceTest::editsProjectAndRefreshesOpportunityScoring()
{
    WorkspaceFixture fixture;
    presentation::PlanningViewModel viewModel(fixture.workspace, false);
    viewModel.load();

    QVERIFY(viewModel.createProject(
        QStringLiteral("实习准备"),
        QStringLiteral("准备软件开发实习"),
        QStringLiteral("每周投入不超过 10 小时，预算不超过 1000 元")));
    QVERIFY(viewModel.addPastedSource(
        QStringLiteral("软件开发训练营"), 1, 0,
        QStringLiteral("软件开发实习训练，报名费 800 元，每周 8 小时。")));

    QCOMPARE(viewModel.opportunityCount(), 1);
    QCOMPARE(viewModel.opportunityAssessments().front().toMap()
                 .value(QStringLiteral("feasibilityScore")).toInt(), 25);

    QVERIFY(viewModel.updateSelectedProject(
        QStringLiteral("作品集准备"),
        QStringLiteral("完成软件开发作品集"),
        QStringLiteral("每周投入不超过 5 小时，预算不超过 500 元")));

    QCOMPARE(viewModel.selectedProjectTitle(), QStringLiteral("作品集准备"));
    QCOMPARE(viewModel.selectedProjectObjective(),
             QStringLiteral("完成软件开发作品集"));
    QCOMPARE(viewModel.opportunityAssessments().front().toMap()
                 .value(QStringLiteral("feasibilityScore")).toInt(), 0);
}

void PlanningWorkspaceTest::publishesConfirmedMarkdownActionsToTasksPort()
{
    WorkspaceFixture fixture;
    FakePlanningActionPublisher publisher;
    presentation::PlanningViewModel viewModel(fixture.workspace, false);
    viewModel.configureActionPublisher(&publisher);
    viewModel.load();
    QVERIFY(viewModel.createProject(
        QStringLiteral("学期规划"),
        QStringLiteral("完成培养要求"), {}));

    const QString markdown = QStringLiteral(
        "# 规划\n"
        "- [ ] 整理必修课学分 [S1:L2]\n"
        "- [x] 已完成的事\n"
        "- [ ] 报名软件开发训练营\n"
        "- [ ] 整理必修课学分 [S1:L2]");
    QVERIFY(viewModel.preparePlanActions(markdown));
    QCOMPARE(viewModel.actionDraftCount(), 2);
    QCOMPARE(viewModel.actionDrafts().front().toMap()
                 .value(QStringLiteral("title")).toString(),
             QStringLiteral("整理必修课学分"));

    QVERIFY(viewModel.publishPreparedActions());
    QCOMPARE(publisher.published.size(), 2U);
    QCOMPARE(publisher.published.front().title,
             QStringLiteral("整理必修课学分"));
    QVERIFY(publisher.published.front().description
        .contains(QStringLiteral("学期规划")));
    QCOMPARE(viewModel.actionDraftCount(), 0);
}

void PlanningWorkspaceTest::sendsClaudeThroughAnthropicProtocol()
{
    WorkspaceFixture fixture;
    presentation::PlanningViewModel viewModel(fixture.workspace, false);
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("planning-claude-settings-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    FakeSecretStore secrets;
    FakeLanguageModelService languageModel;
    viewModel.configureAi(&languageModel, &secrets, &settings);
    viewModel.load();

    QVERIFY(viewModel.createProject(
        QStringLiteral("本科规划"), QStringLiteral("完成培养要求"), {}));
    QVERIFY(viewModel.addPastedSource(
        QStringLiteral("培养方案"), 0, 1,
        QStringLiteral("# 培养要求\n需要完成 8 学分实践课程")));
    QVERIFY(viewModel.saveProviderSettings(
        QStringLiteral("https://api.anthropic.com"),
        QStringLiteral("claude-sonnet-5-5"),
        QStringLiteral("claude-secret"), 1,
        QStringLiteral("wrkspc_test")));
    QCOMPARE(viewModel.providerProtocolIndex(), 1);

    QVERIFY(viewModel.generatePlan({}));
    QCOMPARE(languageModel.lastRequest.protocol,
             LanguageModelProtocol::AnthropicMessages);
    QCOMPARE(languageModel.lastRequest.baseUrl,
             QStringLiteral("https://api.anthropic.com"));
    QCOMPARE(languageModel.lastRequest.workspaceId,
             QStringLiteral("wrkspc_test"));
    const QVariantMap revision = viewModel.planRevisions().front().toMap();
    QCOMPARE(revision.value(QStringLiteral("origin")).toString(),
             QStringLiteral("anthropic-messages · claude-sonnet-5-5"));
}

QTEST_APPLESS_MAIN(PlanningWorkspaceTest)

#include "PlanningWorkspaceTest.moc"
