#pragma once

#include "MonthlyBudget.h"

#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::accounting {

enum class RemoveMonthlyBudgetResult
{
    Removed,
    NotFound,
    StorageFailure
};

class IMonthlyBudgetRepository
{
public:
    virtual ~IMonthlyBudgetRepository() = default;

    [[nodiscard]] virtual bool save(const MonthlyBudget &budget) = 0;
    [[nodiscard]] virtual RemoveMonthlyBudgetResult remove(
        const QUuid &categoryId, int year, int month) = 0;
    [[nodiscard]] virtual std::optional<std::vector<MonthlyBudget>> findForMonth(
        int year, int month) const = 0;
};

} // namespace stukit::accounting
