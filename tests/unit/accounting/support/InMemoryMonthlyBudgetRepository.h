#pragma once

#include "IMonthlyBudgetRepository.h"

#include <algorithm>

namespace stukit::accounting::testing {

class InMemoryMonthlyBudgetRepository final : public IMonthlyBudgetRepository
{
public:
    [[nodiscard]] bool save(const MonthlyBudget &budget) override
    {
        if (!m_succeeds) {
            return false;
        }
        const auto existing = std::find_if(
            m_budgets.begin(), m_budgets.end(),
            [&budget](const MonthlyBudget &stored) {
                return stored.categoryId() == budget.categoryId()
                    && stored.year() == budget.year()
                    && stored.month() == budget.month();
            });
        if (existing == m_budgets.end()) {
            m_budgets.push_back(budget);
        } else {
            *existing = budget;
        }
        return true;
    }

    [[nodiscard]] RemoveMonthlyBudgetResult remove(
        const QUuid &categoryId, int year, int month) override
    {
        if (!m_succeeds) {
            return RemoveMonthlyBudgetResult::StorageFailure;
        }
        const auto existing = std::find_if(
            m_budgets.begin(), m_budgets.end(),
            [&](const MonthlyBudget &stored) {
                return stored.categoryId() == categoryId
                    && stored.year() == year && stored.month() == month;
            });
        if (existing == m_budgets.end()) {
            return RemoveMonthlyBudgetResult::NotFound;
        }
        m_budgets.erase(existing);
        return RemoveMonthlyBudgetResult::Removed;
    }

    [[nodiscard]] std::optional<std::vector<MonthlyBudget>> findForMonth(
        int year, int month) const override
    {
        if (!m_succeeds) {
            return std::nullopt;
        }
        std::vector<MonthlyBudget> result;
        std::copy_if(
            m_budgets.cbegin(), m_budgets.cend(), std::back_inserter(result),
            [&](const MonthlyBudget &stored) {
                return stored.year() == year && stored.month() == month;
            });
        return result;
    }

    void setSucceeds(bool succeeds) noexcept
    {
        m_succeeds = succeeds;
    }

private:
    std::vector<MonthlyBudget> m_budgets;
    bool m_succeeds{true};
};

} // namespace stukit::accounting::testing
