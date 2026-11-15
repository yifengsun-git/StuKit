#include "SqliteSourceDocumentRepository.h"

#include <QSqlError>
#include <QSqlQuery>

#include <utility>

namespace stukit::planning {

SqliteSourceDocumentRepository::SqliteSourceDocumentRepository(
    QSqlDatabase database)
    : m_database(std::move(database))
{
}

bool SqliteSourceDocumentRepository::save(
    const SourceDocument &document, QString &error)
{
    error.clear();
    if (!m_database.isOpen()) {
        error = QStringLiteral("规划数据库没有打开");
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO planning_source_documents("
        "id, project_id, title, kind, format, original_file_name, content, "
        "content_sha256, imported_at) VALUES("
        ":id, :projectId, :title, :kind, :format, :fileName, :content, :hash, "
        ":importedAt) ON CONFLICT(id) DO UPDATE SET "
        "title = excluded.title, kind = excluded.kind, format = excluded.format, "
        "original_file_name = excluded.original_file_name, "
        "content = excluded.content, content_sha256 = excluded.content_sha256"));
    query.bindValue(QStringLiteral(":id"),
        document.id().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":projectId"),
        document.projectId().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":title"), document.title());
    query.bindValue(QStringLiteral(":kind"), static_cast<int>(document.kind()));
    query.bindValue(QStringLiteral(":format"),
        static_cast<int>(document.format()));
    query.bindValue(QStringLiteral(":fileName"), document.originalFileName());
    query.bindValue(QStringLiteral(":content"), document.content());
    query.bindValue(QStringLiteral(":hash"), document.contentSha256());
    query.bindValue(QStringLiteral(":importedAt"),
        document.importedAtUtc().toString(Qt::ISODateWithMs));
    if (!query.exec()) {
        error = query.lastError().nativeErrorCode() == QStringLiteral("2067")
            ? QStringLiteral("该规划中已经存在内容相同的资料")
            : query.lastError().text();
        return false;
    }
    return true;
}

std::vector<SourceDocument> SqliteSourceDocumentRepository::forProject(
    const QUuid &projectId, QString &error) const
{
    error.clear();
    std::vector<SourceDocument> result;
    if (!m_database.isOpen() || projectId.isNull()) {
        error = QStringLiteral("规划数据库没有打开或项目标识无效");
        return result;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, project_id, title, kind, format, original_file_name, "
        "content, content_sha256, imported_at "
        "FROM planning_source_documents WHERE project_id = :projectId "
        "ORDER BY imported_at DESC, title COLLATE NOCASE"));
    query.bindValue(QStringLiteral(":projectId"),
        projectId.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        error = query.lastError().text();
        return result;
    }
    while (query.next()) {
        auto document = documentFromCurrentRow(query, error);
        if (!document) {
            result.clear();
            return result;
        }
        result.push_back(std::move(*document));
    }
    return result;
}

std::optional<SourceDocument> SqliteSourceDocumentRepository::findById(
    const QUuid &id, QString &error) const
{
    error.clear();
    if (!m_database.isOpen() || id.isNull()) {
        error = QStringLiteral("规划数据库没有打开或资料标识无效");
        return std::nullopt;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, project_id, title, kind, format, original_file_name, "
        "content, content_sha256, imported_at "
        "FROM planning_source_documents WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        error = query.lastError().text();
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }
    return documentFromCurrentRow(query, error);
}

bool SqliteSourceDocumentRepository::remove(
    const QUuid &id, QString &error)
{
    error.clear();
    if (!m_database.isOpen() || id.isNull()) {
        error = QStringLiteral("规划数据库没有打开或资料标识无效");
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "DELETE FROM planning_source_documents WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() != 1) {
        error = QStringLiteral("要删除的资料不存在");
        return false;
    }
    return true;
}

std::optional<SourceDocument>
SqliteSourceDocumentRepository::documentFromCurrentRow(
    const QSqlQuery &query, QString &error) const
{
    const auto document = SourceDocument::create(SourceDocumentData{
        .id = QUuid(query.value(QStringLiteral("id")).toString()),
        .projectId = QUuid(query.value(QStringLiteral("project_id")).toString()),
        .title = query.value(QStringLiteral("title")).toString(),
        .kind = static_cast<SourceDocumentKind>(
            query.value(QStringLiteral("kind")).toInt()),
        .format = static_cast<SourceDocumentFormat>(
            query.value(QStringLiteral("format")).toInt()),
        .originalFileName = query.value(
            QStringLiteral("original_file_name")).toString(),
        .content = query.value(QStringLiteral("content")).toString(),
        .contentSha256 = query.value(
            QStringLiteral("content_sha256")).toString(),
        .importedAtUtc = QDateTime::fromString(
            query.value(QStringLiteral("imported_at")).toString(),
            Qt::ISODateWithMs),
    });
    if (!document) {
        error = QStringLiteral("数据库中存在无效的规划资料");
    }
    return document;
}

} // namespace stukit::planning
