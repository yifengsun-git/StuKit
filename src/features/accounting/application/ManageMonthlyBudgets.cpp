#include "ManageMonthlyBudgets.h"

#include <QDate>
#include <QHash>

#include <algorithm>

namespace stukit::accounting {

ManageMonthlyBudgets::ManageMonthlyBudgets(
    IMonthlyBudgetRepository &budgetRepository,
    ICategoryRepository &categoryRepository,
    ITransactionRepository &transactionRepository)
    : m_budgetRepository(budgetRepository)
    , m_categoryRepository(categoryRepository)
    , m_transactionRepository(transactionRepository)
{
}

MonthlyBudgetOverviewResult ManageMonthlyBudgets::overview(
    int year, int month, const QTimeZone &timeZone) const
{
    const QDate firstDay(year, month, 1);
    if (!firstDay.isValid() || !timeZone.isValid()) {
        return ManageMonthlyBudgetError::InvalidInput;
    }
    const QDateTime start = firstDay.startOfDay(timeZone);
    const QDateTime end = firstDay.addMonths(1).startOfDay(timeZone);
    if (!start.isValid() || !end.isValid()) {
        return ManageMonthlyBudgetError::InvalidInput;
    }

    const auto categories = m_categoryRepository.findAll();
    const auto budgets = m_budgetRepository.findForMonth(year, month);
    const auto transactions = m_transactionRepository.findOccurredBetween(
        start.toUTC(), end.toUTC());
    if (!categories.has_value() || !budgets.has_value()
        || !transactions.has_value()) {
        return ManageMonthlyBudgetError::StorageFailure;
    }

    QHash<QUuid, Money> limits;
    for (const MonthlyBudget &budget : *budgets) {
        limits.insert(budget.categoryId(), budget.limit());
    }
    QHash<QUuid, Money> spending;
    for (const Transaction &transaction : *transactions) {
        if (transaction.type() != TransactionType::Expense) {
            continue;
        }
        const Money previous = spending.value(
            transaction.categoryId(), Money::fromCents(0));
        spending.insert(transaction.categoryId(), previous + transaction.amount());
    }

    MonthlyBudgetOverview result;
    for (const Category &category : *categories) {
        if (category.isArchived()
            || category.type() != TransactionType::Expense) {
            continue;
        }
        const auto limit = limits.constFind(category.id());
        result.categories.push_back(CategoryBudgetProgress{
            .categoryId = category.id(),
            .categoryName = category.name(),
            .limit = limit == limits.cend()
                ? std::optional<Money>{} : std::optional<Money>{*limit},
            .spent = spending.value(category.id(), Money::fromCents(0)),
        });
    }
    std::sort(
        result.categories.begin(), result.categories.end(),
        [](const CategoryBudgetProgress &left,
           const CategoryBudgetProgress &right) {
            return left.categoryName.localeAwareCompare(right.categoryName) < 0;
        });
    return result;
}

std::optional<ManageMonthlyBudgetError> ManageMonthlyBudgets::save(
    const QUuid &categoryId, int year, int month, Money limit)
{
    const auto budget = MonthlyBudget::create(MonthlyBudgetData{
        .categoryId = categoryId,
        .year = year,
        .month = month,
        .limit = limit,
    });
    if (!budget.has_value()) {
        return ManageMonthlyBudgetError::InvalidInput;
    }
    const auto category = m_categoryRepository.findById(categoryId);
    if (!category.has_value()) {
        return ManageMonthlyBudgetError::CategoryNotFound;
    }
    if (category->isArchived()) {
        return ManageMonthlyBudgetError::CategoryUnavailable;
    }
    if (category->type() != TransactionType::Expense) {
        return ManageMonthlyBudgetError::CategoryTypeMismatch;
    }
    if (!m_budgetRepository.save(*budget)) {
        return ManageMonthlyBudgetError::StorageFailure;
    }
    return std::nullopt;
}

std::optional<ManageMonthlyBudgetError> ManageMonthlyBudgets::remove(
    const QUuid &categoryId, int year, int month)
{
    if (categoryId.isNull() || !QDate(year, month, 1).isValid()) {
        return ManageMonthlyBudgetError::InvalidInput;
    }
    switch (m_budgetRepository.remove(categoryId, year, month)) {
    case RemoveMonthlyBudgetResult::Removed:
        return std::nullopt;
    case RemoveMonthlyBudgetResult::NotFound:
        return ManageMonthlyBudgetError::BudgetNotFound;
    case RemoveMonthlyBudgetResult::StorageFailure:
        return ManageMonthlyBudgetError::StorageFailure;
    }
    return ManageMonthlyBudgetError::StorageFailure;
}

} // namespace stukit::accounting
