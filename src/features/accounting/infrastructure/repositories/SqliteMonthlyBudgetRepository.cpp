#include "repositories/SqliteMonthlyBudgetRepository.h"

#include <QDate>
#include <QSqlError>
#include <QSqlQuery>

#include <utility>

namespace stukit::accounting {

SqliteMonthlyBudgetRepository::SqliteMonthlyBudgetRepository(
    QSqlDatabase database)
    : m_database(std::move(database))
{
}

bool SqliteMonthlyBudgetRepository::save(const MonthlyBudget &budget)
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO monthly_category_budgets("
        "category_id, budget_year, budget_month, limit_cents) "
        "VALUES(:categoryId, :year, :month, :limit) "
        "ON CONFLICT(category_id, budget_year, budget_month) DO UPDATE SET "
        "limit_cents = excluded.limit_cents"));
    query.bindValue(QStringLiteral(":categoryId"),
                    budget.categoryId().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":year"), budget.year());
    query.bindValue(QStringLiteral(":month"), budget.month());
    query.bindValue(QStringLiteral(":limit"), budget.limit().cents());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

RemoveMonthlyBudgetResult SqliteMonthlyBudgetRepository::remove(
    const QUuid &categoryId, int year, int month)
{
    m_lastError.clear();
    if (!m_database.isOpen() || categoryId.isNull()
        || !QDate(year, month, 1).isValid()) {
        m_lastError = QStringLiteral("Database is not open or input is invalid");
        return RemoveMonthlyBudgetResult::StorageFailure;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "DELETE FROM monthly_category_budgets "
        "WHERE category_id = :categoryId "
        "AND budget_year = :year AND budget_month = :month"));
    query.bindValue(QStringLiteral(":categoryId"),
                    categoryId.toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":year"), year);
    query.bindValue(QStringLiteral(":month"), month);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return RemoveMonthlyBudgetResult::StorageFailure;
    }
    return query.numRowsAffected() == 1
        ? RemoveMonthlyBudgetResult::Removed
        : RemoveMonthlyBudgetResult::NotFound;
}

std::optional<std::vector<MonthlyBudget>>
SqliteMonthlyBudgetRepository::findForMonth(int year, int month) const
{
    m_lastError.clear();
    if (!m_database.isOpen() || !QDate(year, month, 1).isValid()) {
        m_lastError = QStringLiteral("Database is not open or month is invalid");
        return std::nullopt;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT category_id, budget_year, budget_month, limit_cents "
        "FROM monthly_category_budgets "
        "WHERE budget_year = :year AND budget_month = :month "
        "ORDER BY category_id"));
    query.bindValue(QStringLiteral(":year"), year);
    query.bindValue(QStringLiteral(":month"), month);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    std::vector<MonthlyBudget> budgets;
    while (query.next()) {
        const auto budget = MonthlyBudget::create(MonthlyBudgetData{
            .categoryId = QUuid(query.value(0).toString()),
            .year = query.value(1).toInt(),
            .month = query.value(2).toInt(),
            .limit = Money::fromCents(query.value(3).toLongLong()),
        });
        if (!budget.has_value()) {
            m_lastError = QStringLiteral("Database contains an invalid budget");
            return std::nullopt;
        }
        budgets.push_back(*budget);
    }
    return budgets;
}

const QString &SqliteMonthlyBudgetRepository::lastError() const noexcept
{
    return m_lastError;
}

} // namespace stukit::accounting
