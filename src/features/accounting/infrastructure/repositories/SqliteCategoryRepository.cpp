#include "repositories/SqliteCategoryRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <utility>

namespace stukit::accounting {
namespace {

constexpr int IncomeTypeCode = 0;
constexpr int ExpenseTypeCode = 1;

int transactionTypeToDatabase(TransactionType type)
{
    switch (type) {
    case TransactionType::Income:
        return IncomeTypeCode;
    case TransactionType::Expense:
        return ExpenseTypeCode;
    }

    Q_UNREACHABLE_RETURN(ExpenseTypeCode);
}

std::optional<TransactionType> transactionTypeFromDatabase(int value)
{
    switch (value) {
    case IncomeTypeCode:
        return TransactionType::Income;
    case ExpenseTypeCode:
        return TransactionType::Expense;
    default:
        return std::nullopt;
    }
}

} // namespace

SqliteCategoryRepository::SqliteCategoryRepository(QSqlDatabase database)
    : m_database(std::move(database))
{
}

bool SqliteCategoryRepository::save(const Category &category)
{
    m_lastError.clear();

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("INSERT INTO categories(id, name, type, archived) "
                                 "VALUES(:id, :name, :type, :archived) "
                                 "ON CONFLICT(id) DO UPDATE SET "
                                 "name = excluded.name, "
                                 "type = excluded.type, "
                                 "archived = excluded.archived"));
    query.bindValue(QStringLiteral(":id"),
                    category.id().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":name"), category.name());
    query.bindValue(QStringLiteral(":type"),
                    transactionTypeToDatabase(category.type()));
    query.bindValue(QStringLiteral(":archived"), category.isArchived() ? 1 : 0);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

std::optional<Category> SqliteCategoryRepository::findById(const QUuid &id) const
{
    m_lastError.clear();

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT id, name, type, archived "
                                 "FROM categories WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return categoryFromCurrentRow(query);
}

std::optional<std::vector<Category>> SqliteCategoryRepository::findAll() const
{
    m_lastError.clear();
    std::vector<Category> categories;

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("SELECT id, name, type, archived "
                                   "FROM categories "
                                   "ORDER BY type, name COLLATE NOCASE, id"))) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    while (query.next()) {
        auto category = categoryFromCurrentRow(query);
        if (!category.has_value()) {
            return std::nullopt;
        }

        categories.push_back(std::move(*category));
    }

    return categories;
}

const QString &SqliteCategoryRepository::lastError() const noexcept
{
    return m_lastError;
}

std::optional<Category> SqliteCategoryRepository::categoryFromCurrentRow(
    const QSqlQuery &query) const
{
    const auto type = transactionTypeFromDatabase(
        query.value(QStringLiteral("type")).toInt());
    if (!type.has_value()) {
        m_lastError = QStringLiteral("Database contains an invalid category type");
        return std::nullopt;
    }

    const auto category = Category::create(CategoryData{
        .id = QUuid(query.value(QStringLiteral("id")).toString()),
        .name = query.value(QStringLiteral("name")).toString(),
        .type = *type,
        .archived = query.value(QStringLiteral("archived")).toInt() != 0,
    });

    if (!category.has_value()) {
        m_lastError = QStringLiteral("Database contains an invalid category");
    }

    return category;
}

} // namespace stukit::accounting
