#pragma once

#include "ITransactionRepository.h"

#include <QTimeZone>

#include <variant>
#include <vector>

namespace stukit::accounting {

struct GetMonthlyOverviewRequest
{
    int year{0};
    int month{0};
    QTimeZone timeZone;
};

struct MonthlyOverview
{
    std::vector<Transaction> transactions;
    Money totalIncome{Money::fromCents(0)};
    Money totalExpense{Money::fromCents(0)};
    Money balance{Money::fromCents(0)};
};

enum class GetMonthlyOverviewError
{
    InvalidInput,
    StorageFailure
};

using GetMonthlyOverviewResult = std::variant<
    MonthlyOverview,
    GetMonthlyOverviewError>;

class GetMonthlyOverview final
{
public:
    explicit GetMonthlyOverview(ITransactionRepository &transactionRepository);

    [[nodiscard]] GetMonthlyOverviewResult execute(
        const GetMonthlyOverviewRequest &request) const;

private:
    ITransactionRepository &m_transactionRepository;
};

} // namespace stukit::accounting
