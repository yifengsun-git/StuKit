#pragma once

#include "ICategoryRepository.h"
#include "IMonthlyBudgetRepository.h"
#include "ITransactionRepository.h"

#include <QTimeZone>

#include <optional>
#include <variant>
#include <vector>

namespace stukit::accounting {

struct CategoryBudgetProgress
{
    QUuid categoryId;
    QString categoryName;
    std::optional<Money> limit;
    Money spent{Money::fromCents(0)};
};

struct MonthlyBudgetOverview
{
    std::vector<CategoryBudgetProgress> categories;
};

enum class ManageMonthlyBudgetError
{
    InvalidInput,
    CategoryNotFound,
    CategoryUnavailable,
    CategoryTypeMismatch,
    BudgetNotFound,
    StorageFailure
};

using MonthlyBudgetOverviewResult = std::variant<
    MonthlyBudgetOverview,
    ManageMonthlyBudgetError>;

class ManageMonthlyBudgets final
{
public:
    ManageMonthlyBudgets(
        IMonthlyBudgetRepository &budgetRepository,
        ICategoryRepository &categoryRepository,
        ITransactionRepository &transactionRepository);

    [[nodiscard]] MonthlyBudgetOverviewResult overview(
        int year, int month, const QTimeZone &timeZone) const;
    [[nodiscard]] std::optional<ManageMonthlyBudgetError> save(
        const QUuid &categoryId, int year, int month, Money limit);
    [[nodiscard]] std::optional<ManageMonthlyBudgetError> remove(
        const QUuid &categoryId, int year, int month);

private:
    IMonthlyBudgetRepository &m_budgetRepository;
    ICategoryRepository &m_categoryRepository;
    ITransactionRepository &m_transactionRepository;
};

} // namespace stukit::accounting
