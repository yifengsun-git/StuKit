#pragma once

#include <memory>

#include <QSqlDatabase>
#include <QString>

class QNetworkAccessManager;
class QSettings;

namespace stukit::planning {
class InMemoryPlanningProjectRepository;
class InMemorySourceDocumentRepository;
class IPlanningProjectRepository;
class IPlanRevisionRepository;
class ISourceDocumentRepository;
class ILanguageModelService;
class AnthropicLanguageModelService;
class OpenAiCompatibleLanguageModelService;
class ISecretStore;
class IPlanFileWriter;
class IPlanningActionPublisher;
class PlanningWorkspace;
class TextSourceFileReader;
namespace presentation { class PlanningViewModel; }
}

namespace stukit::app {

class PlanningModule final
{
public:
    PlanningModule();
    ~PlanningModule();

    PlanningModule(const PlanningModule &) = delete;
    PlanningModule &operator=(const PlanningModule &) = delete;

    [[nodiscard]] bool initialize(
        QSqlDatabase database,
        QSettings &settings,
        planning::IPlanningActionPublisher *actionPublisher = nullptr);
    [[nodiscard]] planning::presentation::PlanningViewModel *viewModel()
        const noexcept;
    [[nodiscard]] const QString &lastError() const noexcept;

private:
    std::unique_ptr<planning::IPlanningProjectRepository> m_projectRepository;
    std::unique_ptr<planning::ISourceDocumentRepository> m_sourceRepository;
    std::unique_ptr<planning::IPlanRevisionRepository> m_revisionRepository;
    std::unique_ptr<planning::TextSourceFileReader> m_fileReader;
    std::unique_ptr<QNetworkAccessManager> m_network;
    std::unique_ptr<planning::OpenAiCompatibleLanguageModelService>
        m_openAiLanguageModel;
    std::unique_ptr<planning::AnthropicLanguageModelService>
        m_anthropicLanguageModel;
    std::unique_ptr<planning::ILanguageModelService> m_languageModel;
    std::unique_ptr<planning::ISecretStore> m_secretStore;
    std::unique_ptr<planning::IPlanFileWriter> m_planFileWriter;
    std::unique_ptr<planning::PlanningWorkspace> m_workspace;
    std::unique_ptr<planning::presentation::PlanningViewModel> m_viewModel;
    QString m_lastError;
};

} // namespace stukit::app
