#include "PlanningModule.h"

#include "IPlanningProjectRepository.h"
#include "IPlanRevisionRepository.h"
#include "ISourceDocumentRepository.h"
#include "InMemoryPlanRevisionRepository.h"
#include "InMemoryPlanningProjectRepository.h"
#include "InMemorySourceDocumentRepository.h"
#include "ILanguageModelService.h"
#include "ISecretStore.h"
#include "IPlanFileWriter.h"
#include "IPlanningActionPublisher.h"
#include "AnthropicLanguageModelService.h"
#include "MarkdownPlanFileWriter.h"
#include "OpenAiCompatibleLanguageModelService.h"
#include "RoutingLanguageModelService.h"
#include "PlanningViewModel.h"
#include "PlanningWorkspace.h"
#include "SqlitePlanningProjectRepository.h"
#include "SqlitePlanRevisionRepository.h"
#include "SqliteSourceDocumentRepository.h"
#include "TextSourceFileReader.h"
#include "WindowsCredentialSecretStore.h"

#include <QNetworkAccessManager>
#include <QSettings>

namespace stukit::app {

PlanningModule::PlanningModule() = default;
PlanningModule::~PlanningModule() = default;

bool PlanningModule::initialize(
    QSqlDatabase database,
    QSettings &settings,
    planning::IPlanningActionPublisher *actionPublisher)
{
    m_lastError.clear();
    m_viewModel.reset();
    m_workspace.reset();
    m_secretStore.reset();
    m_planFileWriter.reset();
    m_languageModel.reset();
    m_anthropicLanguageModel.reset();
    m_openAiLanguageModel.reset();
    m_network.reset();
    m_fileReader.reset();
    m_revisionRepository.reset();
    m_sourceRepository.reset();
    m_projectRepository.reset();

    const bool persistentStorage = database.isOpen();
    if (persistentStorage) {
        m_projectRepository =
            std::make_unique<planning::SqlitePlanningProjectRepository>(
                database);
        m_sourceRepository =
            std::make_unique<planning::SqliteSourceDocumentRepository>(
                database);
        m_revisionRepository =
            std::make_unique<planning::SqlitePlanRevisionRepository>(database);
    } else {
        m_lastError = QStringLiteral(
            "规划数据库不可用，本次运行将使用临时存储");
        m_projectRepository =
            std::make_unique<planning::InMemoryPlanningProjectRepository>();
        m_sourceRepository =
            std::make_unique<planning::InMemorySourceDocumentRepository>();
        m_revisionRepository =
            std::make_unique<planning::InMemoryPlanRevisionRepository>();
    }
    m_fileReader = std::make_unique<planning::TextSourceFileReader>();
    m_network = std::make_unique<QNetworkAccessManager>();
    m_openAiLanguageModel =
        std::make_unique<planning::OpenAiCompatibleLanguageModelService>(
            *m_network);
    m_anthropicLanguageModel =
        std::make_unique<planning::AnthropicLanguageModelService>(*m_network);
    m_languageModel =
        std::make_unique<planning::RoutingLanguageModelService>(
            *m_openAiLanguageModel, *m_anthropicLanguageModel);
    m_secretStore =
        std::make_unique<planning::WindowsCredentialSecretStore>();
    m_planFileWriter =
        std::make_unique<planning::MarkdownPlanFileWriter>();
    m_workspace = std::make_unique<planning::PlanningWorkspace>(
        *m_projectRepository,
        *m_sourceRepository,
        *m_revisionRepository,
        *m_fileReader);
    m_viewModel =
        std::make_unique<planning::presentation::PlanningViewModel>(
            *m_workspace, persistentStorage);
    m_viewModel->configureAi(
        m_languageModel.get(), m_secretStore.get(), &settings);
    m_viewModel->configurePlanFileWriter(m_planFileWriter.get());
    m_viewModel->configureActionPublisher(actionPublisher);
    m_viewModel->load();
    return persistentStorage;
}

const QString &PlanningModule::lastError() const noexcept
{
    return m_lastError;
}

planning::presentation::PlanningViewModel *PlanningModule::viewModel()
    const noexcept
{
    return m_viewModel.get();
}

} // namespace stukit::app
