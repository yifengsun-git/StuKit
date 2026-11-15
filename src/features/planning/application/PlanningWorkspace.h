#pragma once

#include "IPlanningProjectRepository.h"
#include "IPlanRevisionRepository.h"
#include "ISourceDocumentRepository.h"
#include "ISourceFileReader.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::planning {

class PlanningWorkspace final
{
public:
    PlanningWorkspace(
        IPlanningProjectRepository &projects,
        ISourceDocumentRepository &sources,
        IPlanRevisionRepository &revisions,
        ISourceFileReader &fileReader);

    std::optional<QUuid> createProject(
        QString title,
        QString objective,
        QString constraints,
        const QDateTime &nowUtc,
        QString &error);
    bool updateProject(
        const QUuid &projectId,
        QString title,
        QString objective,
        QString constraints,
        const QDateTime &nowUtc,
        QString &error);
    std::optional<QUuid> addPastedSource(
        const QUuid &projectId,
        QString title,
        SourceDocumentKind kind,
        SourceDocumentFormat format,
        QString content,
        const QDateTime &nowUtc,
        QString &error);
    std::optional<QUuid> importSource(
        const QUuid &projectId,
        QString title,
        SourceDocumentKind kind,
        const QString &filePath,
        const QDateTime &nowUtc,
        QString &error);
    bool updateSource(
        const QUuid &sourceId,
        QString title,
        SourceDocumentKind kind,
        SourceDocumentFormat format,
        QString content,
        QString &error);
    bool deleteSource(const QUuid &sourceId, QString &error);
    std::optional<SourceDocument> source(
        const QUuid &sourceId, QString &error) const;
    std::optional<PlanRevision> savePlan(
        const QUuid &projectId,
        QString markdownContent,
        QString providerId,
        QString modelId,
        std::vector<QUuid> sourceIds,
        const QDateTime &nowUtc,
        QString &error);
    std::vector<PlanRevision> revisions(
        const QUuid &projectId, QString &error) const;

    std::vector<PlanningProject> projects(QString &error) const;
    std::vector<SourceDocument> sources(
        const QUuid &projectId, QString &error) const;

private:
    bool projectExists(const QUuid &projectId, QString &error) const;

    IPlanningProjectRepository &m_projects;
    ISourceDocumentRepository &m_sources;
    IPlanRevisionRepository &m_revisions;
    ISourceFileReader &m_fileReader;
};

} // namespace stukit::planning
