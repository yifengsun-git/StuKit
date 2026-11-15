#include "SqlitePlanningProjectRepository.h"

#include <QSqlError>
#include <QSqlQuery>

#include <utility>

namespace stukit::planning {

SqlitePlanningProjectRepository::SqlitePlanningProjectRepository(
    QSqlDatabase database)
    : m_database(std::move(database))
{
}

bool SqlitePlanningProjectRepository::save(
    const PlanningProject &project, QString &error)
{
    error.clear();
    if (!m_database.isOpen()) {
        error = QStringLiteral("规划数据库没有打开");
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO planning_projects("
        "id, title, objective, constraints_text, status, created_at, updated_at) "
        "VALUES(:id, :title, :objective, :constraints, :status, :createdAt, "
        ":updatedAt) ON CONFLICT(id) DO UPDATE SET "
        "title = excluded.title, objective = excluded.objective, "
        "constraints_text = excluded.constraints_text, status = excluded.status, "
        "updated_at = excluded.updated_at"));
    query.bindValue(QStringLiteral(":id"),
        project.id().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":title"), project.title());
    query.bindValue(QStringLiteral(":objective"), project.objective());
    query.bindValue(QStringLiteral(":constraints"), project.constraints());
    query.bindValue(QStringLiteral(":status"),
        static_cast<int>(project.status()));
    query.bindValue(QStringLiteral(":createdAt"),
        project.createdAtUtc().toString(Qt::ISODateWithMs));
    query.bindValue(QStringLiteral(":updatedAt"),
        project.updatedAtUtc().toString(Qt::ISODateWithMs));
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    return true;
}

std::vector<PlanningProject> SqlitePlanningProjectRepository::all(
    QString &error) const
{
    error.clear();
    std::vector<PlanningProject> result;
    if (!m_database.isOpen()) {
        error = QStringLiteral("规划数据库没有打开");
        return result;
    }
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT id, title, objective, constraints_text, status, created_at, "
            "updated_at FROM planning_projects "
            "ORDER BY status, updated_at DESC, title COLLATE NOCASE"))) {
        error = query.lastError().text();
        return result;
    }
    while (query.next()) {
        auto project = projectFromCurrentRow(query, error);
        if (!project) {
            result.clear();
            return result;
        }
        result.push_back(std::move(*project));
    }
    return result;
}

std::optional<PlanningProject>
SqlitePlanningProjectRepository::findById(
    const QUuid &id, QString &error) const
{
    error.clear();
    if (!m_database.isOpen() || id.isNull()) {
        error = QStringLiteral("规划数据库没有打开或项目标识无效");
        return std::nullopt;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, title, objective, constraints_text, status, created_at, "
        "updated_at FROM planning_projects WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        error = query.lastError().text();
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }
    return projectFromCurrentRow(query, error);
}

std::optional<PlanningProject>
SqlitePlanningProjectRepository::projectFromCurrentRow(
    const QSqlQuery &query, QString &error) const
{
    const auto project = PlanningProject::create(PlanningProjectData{
        .id = QUuid(query.value(QStringLiteral("id")).toString()),
        .title = query.value(QStringLiteral("title")).toString(),
        .objective = query.value(QStringLiteral("objective")).toString(),
        .constraints = query.value(
            QStringLiteral("constraints_text")).toString(),
        .status = static_cast<PlanningProjectStatus>(
            query.value(QStringLiteral("status")).toInt()),
        .createdAtUtc = QDateTime::fromString(
            query.value(QStringLiteral("created_at")).toString(),
            Qt::ISODateWithMs),
        .updatedAtUtc = QDateTime::fromString(
            query.value(QStringLiteral("updated_at")).toString(),
            Qt::ISODateWithMs),
    });
    if (!project) {
        error = QStringLiteral("数据库中存在无效的规划项目");
    }
    return project;
}

} // namespace stukit::planning
