#pragma once

#include "PlanningWorkspace.h"
#include "ILanguageModelService.h"
#include "ISecretStore.h"
#include "IPlanFileWriter.h"
#include "PlanningAnalysis.h"

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantList>

class QSettings;

namespace stukit::planning::presentation {

class PlanningViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList projects READ projects NOTIFY workspaceChanged)
    Q_PROPERTY(QVariantList sources READ sources NOTIFY workspaceChanged)
    Q_PROPERTY(QString selectedProjectId READ selectedProjectId
        NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedProjectTitle READ selectedProjectTitle
        NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedProjectObjective READ selectedProjectObjective
        NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedProjectConstraints READ selectedProjectConstraints
        NOTIFY selectionChanged)
    Q_PROPERTY(bool hasProjects READ hasProjects NOTIFY workspaceChanged)
    Q_PROPERTY(bool hasSources READ hasSources NOTIFY workspaceChanged)
    Q_PROPERTY(QStringList sourceKindOptions READ sourceKindOptions CONSTANT)
    Q_PROPERTY(QStringList sourceFormatOptions READ sourceFormatOptions CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY messageChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY messageChanged)
    Q_PROPERTY(bool persistentStorage READ persistentStorage CONSTANT)
    Q_PROPERTY(bool hasSelectedSource READ hasSelectedSource
        NOTIFY sourceSelectionChanged)
    Q_PROPERTY(QString selectedSourceId READ selectedSourceId
        NOTIFY sourceSelectionChanged)
    Q_PROPERTY(QString selectedSourceTitle READ selectedSourceTitle
        NOTIFY sourceSelectionChanged)
    Q_PROPERTY(QString selectedSourceContent READ selectedSourceContent
        NOTIFY sourceSelectionChanged)
    Q_PROPERTY(QString selectedSourceFileName READ selectedSourceFileName
        NOTIFY sourceSelectionChanged)
    Q_PROPERTY(int selectedSourceKindIndex READ selectedSourceKindIndex
        NOTIFY sourceSelectionChanged)
    Q_PROPERTY(int selectedSourceFormatIndex READ selectedSourceFormatIndex
        NOTIFY sourceSelectionChanged)
    Q_PROPERTY(bool hasPlan READ hasPlan NOTIFY planChanged)
    Q_PROPERTY(QString planMarkdown READ planMarkdown NOTIFY planChanged)
    Q_PROPERTY(int planRevisionNumber READ planRevisionNumber NOTIFY planChanged)
    Q_PROPERTY(QVariantList planRevisions READ planRevisions NOTIFY planChanged)
    Q_PROPERTY(QString providerBaseUrl READ providerBaseUrl
        NOTIFY aiConfigurationChanged)
    Q_PROPERTY(QString providerModelId READ providerModelId
        NOTIFY aiConfigurationChanged)
    Q_PROPERTY(QStringList providerProtocolOptions READ providerProtocolOptions
        CONSTANT)
    Q_PROPERTY(int providerProtocolIndex READ providerProtocolIndex
        NOTIFY aiConfigurationChanged)
    Q_PROPERTY(QString providerWorkspaceId READ providerWorkspaceId
        NOTIFY aiConfigurationChanged)
    Q_PROPERTY(bool apiKeyConfigured READ apiKeyConfigured
        NOTIFY aiConfigurationChanged)
    Q_PROPERTY(bool aiAvailable READ aiAvailable
        NOTIFY aiConfigurationChanged)
    Q_PROPERTY(bool aiBusy READ aiBusy NOTIFY aiStateChanged)
    Q_PROPERTY(QVariantList extractedFacts READ extractedFacts
        NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList validationIssues READ validationIssues
        NOTIFY analysisChanged)
    Q_PROPERTY(int extractedFactCount READ extractedFactCount
        NOTIFY analysisChanged)
    Q_PROPERTY(int validationIssueCount READ validationIssueCount
        NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList opportunityAssessments READ opportunityAssessments
        NOTIFY analysisChanged)
    Q_PROPERTY(int opportunityCount READ opportunityCount
        NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList actionDrafts READ actionDrafts
        NOTIFY actionDraftsChanged)
    Q_PROPERTY(int actionDraftCount READ actionDraftCount
        NOTIFY actionDraftsChanged)

public:
    explicit PlanningViewModel(
        PlanningWorkspace &workspace,
        bool persistentStorage,
        QObject *parent = nullptr);

    void load();
    void configureAi(ILanguageModelService *languageModel,
                     ISecretStore *secretStore,
                     QSettings *settings);
    void configurePlanFileWriter(IPlanFileWriter *fileWriter);
    void configureActionPublisher(IPlanningActionPublisher *publisher);

    [[nodiscard]] QVariantList projects() const;
    [[nodiscard]] QVariantList sources() const;
    [[nodiscard]] QString selectedProjectId() const;
    [[nodiscard]] QString selectedProjectTitle() const;
    [[nodiscard]] QString selectedProjectObjective() const;
    [[nodiscard]] QString selectedProjectConstraints() const;
    [[nodiscard]] bool hasProjects() const noexcept;
    [[nodiscard]] bool hasSources() const noexcept;
    [[nodiscard]] QStringList sourceKindOptions() const;
    [[nodiscard]] QStringList sourceFormatOptions() const;
    [[nodiscard]] const QString &errorMessage() const noexcept;
    [[nodiscard]] const QString &statusMessage() const noexcept;
    [[nodiscard]] bool persistentStorage() const noexcept;
    [[nodiscard]] bool hasSelectedSource() const noexcept;
    [[nodiscard]] QString selectedSourceId() const;
    [[nodiscard]] QString selectedSourceTitle() const;
    [[nodiscard]] QString selectedSourceContent() const;
    [[nodiscard]] QString selectedSourceFileName() const;
    [[nodiscard]] int selectedSourceKindIndex() const noexcept;
    [[nodiscard]] int selectedSourceFormatIndex() const noexcept;
    [[nodiscard]] bool hasPlan() const noexcept;
    [[nodiscard]] QString planMarkdown() const;
    [[nodiscard]] int planRevisionNumber() const noexcept;
    [[nodiscard]] QVariantList planRevisions() const;
    [[nodiscard]] const QString &providerBaseUrl() const noexcept;
    [[nodiscard]] const QString &providerModelId() const noexcept;
    [[nodiscard]] QStringList providerProtocolOptions() const;
    [[nodiscard]] int providerProtocolIndex() const noexcept;
    [[nodiscard]] const QString &providerWorkspaceId() const noexcept;
    [[nodiscard]] bool apiKeyConfigured() const noexcept;
    [[nodiscard]] bool aiAvailable() const noexcept;
    [[nodiscard]] bool aiBusy() const noexcept;
    [[nodiscard]] QVariantList extractedFacts() const;
    [[nodiscard]] QVariantList validationIssues() const;
    [[nodiscard]] int extractedFactCount() const noexcept;
    [[nodiscard]] int validationIssueCount() const noexcept;
    [[nodiscard]] QVariantList opportunityAssessments() const;
    [[nodiscard]] int opportunityCount() const noexcept;
    [[nodiscard]] QVariantList actionDrafts() const;
    [[nodiscard]] int actionDraftCount() const noexcept;

    Q_INVOKABLE bool createProject(
        const QString &title,
        const QString &objective,
        const QString &constraints);
    Q_INVOKABLE bool updateSelectedProject(
        const QString &title,
        const QString &objective,
        const QString &constraints);
    Q_INVOKABLE bool selectProject(const QString &projectId);
    Q_INVOKABLE bool addPastedSource(
        const QString &title,
        int kindIndex,
        int formatIndex,
        const QString &content);
    Q_INVOKABLE bool importSource(
        const QUrl &fileUrl,
        const QString &title,
        int kindIndex);
    Q_INVOKABLE bool selectSource(const QString &sourceId);
    Q_INVOKABLE bool updateSelectedSource(
        const QString &title,
        int kindIndex,
        int formatIndex,
        const QString &content);
    Q_INVOKABLE bool deleteSelectedSource();
    Q_INVOKABLE void clearSelectedSource();
    Q_INVOKABLE bool savePlan(const QString &markdownContent);
    Q_INVOKABLE bool selectPlanRevision(const QString &revisionId);
    Q_INVOKABLE bool exportPlan(const QUrl &fileUrl,
                                const QString &markdownContent);
    Q_INVOKABLE bool saveProviderSettings(const QString &baseUrl,
                                          const QString &modelId,
                                          const QString &apiKey,
                                          int protocolIndex = 0,
                                          const QString &workspaceId = {});
    Q_INVOKABLE bool removeApiKey();
    Q_INVOKABLE bool generatePlan(const QString &instructions);
    Q_INVOKABLE void cancelGeneration();
    Q_INVOKABLE bool preparePlanActions(const QString &markdownContent);
    Q_INVOKABLE bool publishPreparedActions();
    Q_INVOKABLE void clearPreparedActions();
    Q_INVOKABLE void clearMessages();

signals:
    void workspaceChanged();
    void selectionChanged();
    void messageChanged();
    void sourceSelectionChanged();
    void planChanged();
    void aiConfigurationChanged();
    void aiStateChanged();
    void generatedPlanReady(const QString &markdownContent);
    void analysisChanged();
    void actionDraftsChanged();

private:
    void reloadProjects();
    void reloadSources();
    void reloadRevisions();
    void reloadAnalysis();
    void setError(QString message);
    void setStatus(QString message);
    static std::optional<SourceDocumentKind> kindFromIndex(int index);
    static std::optional<SourceDocumentFormat> formatFromIndex(int index);

    PlanningWorkspace &m_workspace;
    std::vector<PlanningProject> m_projects;
    std::vector<SourceDocument> m_sources;
    QUuid m_selectedProjectId;
    QString m_errorMessage;
    QString m_statusMessage;
    bool m_persistentStorage{false};
    std::optional<SourceDocument> m_selectedSource;
    std::vector<PlanRevision> m_revisions;
    std::optional<PlanRevision> m_selectedRevision;
    ILanguageModelService *m_languageModel{nullptr};
    ISecretStore *m_secretStore{nullptr};
    IPlanFileWriter *m_planFileWriter{nullptr};
    IPlanningActionPublisher *m_actionPublisher{nullptr};
    QSettings *m_settings{nullptr};
    QString m_providerBaseUrl{QStringLiteral("https://api.deepseek.com")};
    QString m_providerModelId{QStringLiteral("deepseek-flash")};
    QString m_providerWorkspaceId;
    LanguageModelProtocol m_providerProtocol{
        LanguageModelProtocol::OpenAiCompatible};
    bool m_apiKeyConfigured{false};
    bool m_aiBusy{false};
    PlanningFactExtractionResult m_factExtraction;
    std::vector<PlanningOpportunityAssessment> m_opportunityAssessments;
    std::vector<PlanningActionDraft> m_actionDrafts;
};

} // namespace stukit::planning::presentation
