#include "PlanningWorkspace.h"

#include <utility>

namespace stukit::planning {

PlanningWorkspace::PlanningWorkspace(
    IPlanningProjectRepository &projects,
    ISourceDocumentRepository &sources,
    IPlanRevisionRepository &revisions,
    ISourceFileReader &fileReader)
    : m_projects(projects)
    , m_sources(sources)
    , m_revisions(revisions)
    , m_fileReader(fileReader)
{
}

std::optional<QUuid> PlanningWorkspace::createProject(
    QString title,
    QString objective,
    QString constraints,
    const QDateTime &nowUtc,
    QString &error)
{
    error.clear();
    const QUuid id = QUuid::createUuid();
    const auto project = PlanningProject::create(PlanningProjectData{
        .id = id,
        .title = std::move(title),
        .objective = std::move(objective),
        .constraints = std::move(constraints),
        .status = PlanningProjectStatus::Active,
        .createdAtUtc = nowUtc,
        .updatedAtUtc = nowUtc,
    });
    if (!project) {
        error = QStringLiteral("规划名称和目标不能为空，且内容不能超过长度限制");
        return std::nullopt;
    }
    if (!m_projects.save(*project, error)) {
        return std::nullopt;
    }
    return id;
}

bool PlanningWorkspace::updateProject(
    const QUuid &projectId,
    QString title,
    QString objective,
    QString constraints,
    const QDateTime &nowUtc,
    QString &error)
{
    error.clear();
    const auto existing = m_projects.findById(projectId, error);
    if (!existing) {
        if (error.isEmpty()) {
            error = QStringLiteral("要修改的规划项目不存在");
        }
        return false;
    }

    const auto updated = PlanningProject::create(PlanningProjectData{
        .id = existing->id(),
        .title = std::move(title),
        .objective = std::move(objective),
        .constraints = std::move(constraints),
        .status = existing->status(),
        .createdAtUtc = existing->createdAtUtc(),
        .updatedAtUtc = nowUtc,
    });
    if (!updated) {
        error = QStringLiteral("规划名称和目标不能为空，且内容不能超过长度限制");
        return false;
    }
    return m_projects.save(*updated, error);
}

std::optional<QUuid> PlanningWorkspace::addPastedSource(
    const QUuid &projectId,
    QString title,
    SourceDocumentKind kind,
    SourceDocumentFormat format,
    QString content,
    const QDateTime &nowUtc,
    QString &error)
{
    error.clear();
    if (!projectExists(projectId, error)) {
        return std::nullopt;
    }

    const QUuid id = QUuid::createUuid();
    const auto document = SourceDocument::create(SourceDocumentData{
        .id = id,
        .projectId = projectId,
        .title = std::move(title),
        .kind = kind,
        .format = format,
        .originalFileName = {},
        .content = std::move(content),
        .contentSha256 = {},
        .importedAtUtc = nowUtc,
    });
    if (!document) {
        error = QStringLiteral("资料标题和内容不能为空，或资料内容过长");
        return std::nullopt;
    }
    if (!m_sources.save(*document, error)) {
        return std::nullopt;
    }
    return id;
}

std::optional<QUuid> PlanningWorkspace::importSource(
    const QUuid &projectId,
    QString title,
    SourceDocumentKind kind,
    const QString &filePath,
    const QDateTime &nowUtc,
    QString &error)
{
    error.clear();
    if (!projectExists(projectId, error)) {
        return std::nullopt;
    }
    const auto file = m_fileReader.read(filePath, error);
    if (!file) {
        return std::nullopt;
    }
    if (title.trimmed().isEmpty()) {
        title = file->fileName;
    }

    const QUuid id = QUuid::createUuid();
    const auto document = SourceDocument::create(SourceDocumentData{
        .id = id,
        .projectId = projectId,
        .title = std::move(title),
        .kind = kind,
        .format = file->format,
        .originalFileName = file->fileName,
        .content = file->content,
        .contentSha256 = {},
        .importedAtUtc = nowUtc,
    });
    if (!document) {
        error = QStringLiteral("导入文件为空或超过资料大小限制");
        return std::nullopt;
    }
    if (!m_sources.save(*document, error)) {
        return std::nullopt;
    }
    return id;
}

bool PlanningWorkspace::updateSource(
    const QUuid &sourceId,
    QString title,
    SourceDocumentKind kind,
    SourceDocumentFormat format,
    QString content,
    QString &error)
{
    error.clear();
    const auto existing = m_sources.findById(sourceId, error);
    if (!existing) {
        if (error.isEmpty()) {
            error = QStringLiteral("要修改的资料不存在");
        }
        return false;
    }
    const auto updated = SourceDocument::create(SourceDocumentData{
        .id = existing->id(),
        .projectId = existing->projectId(),
        .title = std::move(title),
        .kind = kind,
        .format = format,
        .originalFileName = existing->originalFileName(),
        .content = std::move(content),
        .contentSha256 = {},
        .importedAtUtc = existing->importedAtUtc(),
    });
    if (!updated) {
        error = QStringLiteral("资料标题和内容不能为空，或资料内容过长");
        return false;
    }
    return m_sources.save(*updated, error);
}

bool PlanningWorkspace::deleteSource(
    const QUuid &sourceId, QString &error)
{
    error.clear();
    return m_sources.remove(sourceId, error);
}

std::optional<SourceDocument> PlanningWorkspace::source(
    const QUuid &sourceId, QString &error) const
{
    error.clear();
    return m_sources.findById(sourceId, error);
}

std::optional<PlanRevision> PlanningWorkspace::savePlan(
    const QUuid &projectId,
    QString markdownContent,
    QString providerId,
    QString modelId,
    std::vector<QUuid> sourceIds,
    const QDateTime &nowUtc,
    QString &error)
{
    error.clear();
    if (!projectExists(projectId, error)) {
        return std::nullopt;
    }
    for (const QUuid &sourceId : sourceIds) {
        const auto sourceDocument = m_sources.findById(sourceId, error);
        if (!sourceDocument || sourceDocument->projectId() != projectId) {
            if (error.isEmpty()) {
                error = QStringLiteral("规划引用了不存在或属于其他项目的资料");
            }
            return std::nullopt;
        }
    }
    const auto latestRevision = m_revisions.latest(projectId, error);
    if (!error.isEmpty()) {
        return std::nullopt;
    }
    const int nextNumber = latestRevision
        ? latestRevision->revisionNumber() + 1 : 1;
    auto revision = PlanRevision::create(PlanRevisionData{
        .id = QUuid::createUuid(),
        .projectId = projectId,
        .revisionNumber = nextNumber,
        .markdownContent = std::move(markdownContent),
        .providerId = std::move(providerId),
        .modelId = std::move(modelId),
        .sourceIds = std::move(sourceIds),
        .createdAtUtc = nowUtc,
    });
    if (!revision) {
        error = QStringLiteral("规划正文不能为空或超过长度限制");
        return std::nullopt;
    }
    if (!m_revisions.save(*revision, error)) {
        return std::nullopt;
    }
    return revision;
}

std::vector<PlanRevision> PlanningWorkspace::revisions(
    const QUuid &projectId, QString &error) const
{
    error.clear();
    if (projectId.isNull()) {
        return {};
    }
    return m_revisions.forProject(projectId, error);
}

std::vector<PlanningProject> PlanningWorkspace::projects(QString &error) const
{
    return m_projects.all(error);
}

std::vector<SourceDocument> PlanningWorkspace::sources(
    const QUuid &projectId, QString &error) const
{
    if (projectId.isNull()) {
        error.clear();
        return {};
    }
    return m_sources.forProject(projectId, error);
}

bool PlanningWorkspace::projectExists(
    const QUuid &projectId, QString &error) const
{
    if (projectId.isNull()) {
        error = QStringLiteral("请先选择一个规划项目");
        return false;
    }
    const auto project = m_projects.findById(projectId, error);
    if (!project && error.isEmpty()) {
        error = QStringLiteral("选择的规划项目不存在");
    }
    return project.has_value();
}

} // namespace stukit::planning
