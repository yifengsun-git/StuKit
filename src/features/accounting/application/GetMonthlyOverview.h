#pragma once

#include "IAccountRepository.h"
#include "ICategoryRepository.h"
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

struct MonthlyTransactionItem
{
    Transaction transaction;
    QString accountName;
    QString categoryName;
};

struct MonthlyOverview
{
    std::vector<MonthlyTransactionItem> transactions;
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
    GetMonthlyOverview(
        ITransactionRepository &transactionRepository,
        IAccountRepository &accountRepository,
        ICategoryRepository &categoryRepository);

    [[nodiscard]] GetMonthlyOverviewResult execute(
        const GetMonthlyOverviewRequest &request) const;

private:
    ITransactionRepository &m_transactionRepository;
    IAccountRepository &m_accountRepository;
    ICategoryRepository &m_categoryRepository;
};

} // namespace stukit::accounting
