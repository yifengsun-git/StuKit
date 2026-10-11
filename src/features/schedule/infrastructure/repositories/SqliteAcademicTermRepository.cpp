#include "repositories/SqliteAcademicTermRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <utility>

namespace stukit::schedule {

SqliteAcademicTermRepository::SqliteAcademicTermRepository(
    QSqlDatabase database)
    : m_database(std::move(database))
{
}

bool SqliteAcademicTermRepository::save(const AcademicTerm &term)
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO academic_terms("
        "id, name, start_date, total_weeks, archived) "
        "VALUES(:id, :name, :startDate, :totalWeeks, :archived) "
        "ON CONFLICT(id) DO UPDATE SET "
        "name = excluded.name, "
        "start_date = excluded.start_date, "
        "total_weeks = excluded.total_weeks, "
        "archived = excluded.archived"));
    query.bindValue(
        QStringLiteral(":id"), term.id().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":name"), term.name());
    query.bindValue(
        QStringLiteral(":startDate"), term.startDate().toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":totalWeeks"), term.totalWeeks());
    query.bindValue(QStringLiteral(":archived"), term.isArchived() ? 1 : 0);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

std::optional<AcademicTerm> SqliteAcademicTermRepository::findById(
    const QUuid &id) const
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, name, start_date, total_weeks, archived "
        "FROM academic_terms WHERE id = :id"));
    query.bindValue(
        QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }
    return termFromCurrentRow(query);
}

std::optional<std::vector<AcademicTerm>>
SqliteAcademicTermRepository::findAll() const
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT id, name, start_date, total_weeks, archived "
            "FROM academic_terms "
            "ORDER BY archived, start_date DESC, name COLLATE NOCASE, id"))) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    std::vector<AcademicTerm> terms;
    while (query.next()) {
        auto term = termFromCurrentRow(query);
        if (!term.has_value()) {
            return std::nullopt;
        }
        terms.push_back(std::move(*term));
    }
    return terms;
}

const QString &SqliteAcademicTermRepository::lastError() const noexcept
{
    return m_lastError;
}

std::optional<AcademicTerm>
SqliteAcademicTermRepository::termFromCurrentRow(
    const QSqlQuery &query) const
{
    const auto term = AcademicTerm::create(AcademicTermData{
        .id = QUuid(query.value(QStringLiteral("id")).toString()),
        .name = query.value(QStringLiteral("name")).toString(),
        .startDate = QDate::fromString(
            query.value(QStringLiteral("start_date")).toString(), Qt::ISODate),
        .totalWeeks = query.value(QStringLiteral("total_weeks")).toInt(),
        .archived = query.value(QStringLiteral("archived")).toInt() != 0,
    });
    if (!term.has_value()) {
        m_lastError = QStringLiteral("Database contains an invalid academic term");
    }
    return term;
}

} // namespace stukit::schedule
