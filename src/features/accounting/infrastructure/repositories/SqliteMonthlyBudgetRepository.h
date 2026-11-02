#pragma once

#include "IMonthlyBudgetRepository.h"

#include <QSqlDatabase>
#include <QString>

namespace stukit::accounting {

class SqliteMonthlyBudgetRepository final : public IMonthlyBudgetRepository
{
public:
    explicit SqliteMonthlyBudgetRepository(QSqlDatabase database);

    [[nodiscard]] bool save(const MonthlyBudget &budget) override;
    [[nodiscard]] RemoveMonthlyBudgetResult remove(
        const QUuid &categoryId, int year, int month) override;
    [[nodiscard]] std::optional<std::vector<MonthlyBudget>> findForMonth(
        int year, int month) const override;

    [[nodiscard]] const QString &lastError() const noexcept;

private:
    QSqlDatabase m_database;
    mutable QString m_lastError;
};

} // namespace stukit::accounting
