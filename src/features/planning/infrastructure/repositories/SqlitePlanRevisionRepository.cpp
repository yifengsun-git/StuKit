#include "SqlitePlanRevisionRepository.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>

#include <utility>

namespace stukit::planning {
namespace {

QString sourceIdsJson(const std::vector<QUuid> &sourceIds)
{
    QJsonArray values;
    for (const QUuid &id : sourceIds) {
        values.append(id.toString(QUuid::WithoutBraces));
    }
    return QString::fromUtf8(
        QJsonDocument(values).toJson(QJsonDocument::Compact));
}

std::optional<std::vector<QUuid>> sourceIdsFromJson(const QString &text)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(
        text.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        return std::nullopt;
    }
    std::vector<QUuid> result;
    for (const QJsonValue &value : document.array()) {
        const QUuid id(value.toString());
        if (id.isNull()) {
            return std::nullopt;
        }
        result.push_back(id);
    }
    return result;
}

} // namespace

SqlitePlanRevisionRepository::SqlitePlanRevisionRepository(
    QSqlDatabase database)
    : m_database(std::move(database))
{
}

bool SqlitePlanRevisionRepository::save(
    const PlanRevision &revision, QString &error)
{
    error.clear();
    if (!m_database.isOpen()) {
        error = QStringLiteral("规划数据库没有打开");
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO planning_plan_revisions("
        "id, project_id, revision_number, markdown_content, provider_id, "
        "model_id, source_ids_json, created_at) VALUES("
        ":id, :projectId, :number, :markdown, :provider, :model, :sourceIds, "
        ":createdAt)"));
    query.bindValue(QStringLiteral(":id"),
        revision.id().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":projectId"),
        revision.projectId().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":number"), revision.revisionNumber());
    query.bindValue(QStringLiteral(":markdown"), revision.markdownContent());
    query.bindValue(QStringLiteral(":provider"), revision.providerId());
    query.bindValue(QStringLiteral(":model"), revision.modelId());
    query.bindValue(QStringLiteral(":sourceIds"),
        sourceIdsJson(revision.sourceIds()));
    query.bindValue(QStringLiteral(":createdAt"),
        revision.createdAtUtc().toString(Qt::ISODateWithMs));
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    return true;
}

std::vector<PlanRevision> SqlitePlanRevisionRepository::forProject(
    const QUuid &projectId, QString &error) const
{
    error.clear();
    std::vector<PlanRevision> result;
    if (!m_database.isOpen() || projectId.isNull()) {
        error = QStringLiteral("规划数据库没有打开或项目标识无效");
        return result;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, project_id, revision_number, markdown_content, provider_id, "
        "model_id, source_ids_json, created_at "
        "FROM planning_plan_revisions WHERE project_id = :projectId "
        "ORDER BY revision_number DESC"));
    query.bindValue(QStringLiteral(":projectId"),
        projectId.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        error = query.lastError().text();
        return result;
    }
    while (query.next()) {
        auto revision = revisionFromCurrentRow(query, error);
        if (!revision) {
            result.clear();
            return result;
        }
        result.push_back(std::move(*revision));
    }
    return result;
}

std::optional<PlanRevision> SqlitePlanRevisionRepository::latest(
    const QUuid &projectId, QString &error) const
{
    error.clear();
    if (!m_database.isOpen() || projectId.isNull()) {
        error = QStringLiteral("规划数据库没有打开或项目标识无效");
        return std::nullopt;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, project_id, revision_number, markdown_content, provider_id, "
        "model_id, source_ids_json, created_at "
        "FROM planning_plan_revisions WHERE project_id = :projectId "
        "ORDER BY revision_number DESC LIMIT 1"));
    query.bindValue(QStringLiteral(":projectId"),
        projectId.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        error = query.lastError().text();
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }
    return revisionFromCurrentRow(query, error);
}

std::optional<PlanRevision>
SqlitePlanRevisionRepository::revisionFromCurrentRow(
    const QSqlQuery &query, QString &error) const
{
    const auto sourceIds = sourceIdsFromJson(
        query.value(QStringLiteral("source_ids_json")).toString());
    if (!sourceIds) {
        error = QStringLiteral("数据库中的规划来源列表无效");
        return std::nullopt;
    }
    auto revision = PlanRevision::create(PlanRevisionData{
        .id = QUuid(query.value(QStringLiteral("id")).toString()),
        .projectId = QUuid(
            query.value(QStringLiteral("project_id")).toString()),
        .revisionNumber = query.value(
            QStringLiteral("revision_number")).toInt(),
        .markdownContent = query.value(
            QStringLiteral("markdown_content")).toString(),
        .providerId = query.value(QStringLiteral("provider_id")).toString(),
        .modelId = query.value(QStringLiteral("model_id")).toString(),
        .sourceIds = *sourceIds,
        .createdAtUtc = QDateTime::fromString(
            query.value(QStringLiteral("created_at")).toString(),
            Qt::ISODateWithMs),
    });
    if (!revision) {
        error = QStringLiteral("数据库中存在无效的规划版本");
    }
    return revision;
}

} // namespace stukit::planning
