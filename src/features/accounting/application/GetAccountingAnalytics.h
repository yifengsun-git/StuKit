#pragma once

#include "IAccountRepository.h"
#include "ICategoryRepository.h"
#include "ITransactionRepository.h"

#include <QDate>
#include <QTimeZone>

#include <variant>
#include <vector>

namespace stukit::accounting {

struct GetAccountingAnalyticsRequest
{
    QDate selectedMonth;
    int monthCount{12};
    QTimeZone timeZone;
    std::vector<QUuid> accountIds;
    std::vector<QUuid> categoryIds;
};

struct MonthlyAnalyticsPoint
{
    QDate month;
    Money income{Money::fromCents(0)};
    Money expense{Money::fromCents(0)};
    Money balance{Money::fromCents(0)};
};

struct CategoryAnalyticsItem
{
    QUuid categoryId;
    QString categoryName;
    TransactionType type{TransactionType::Expense};
    Money amount{Money::fromCents(0)};
    int transactionCount{0};
    bool archived{false};
};

struct AccountAnalyticsItem
{
    QUuid accountId;
    QString accountName;
    Money income{Money::fromCents(0)};
    Money expense{Money::fromCents(0)};
    Money netFlow{Money::fromCents(0)};
    int transactionCount{0};
    bool archived{false};
};

struct AccountingAnalytics
{
    std::vector<MonthlyAnalyticsPoint> months;
    std::vector<CategoryAnalyticsItem> incomeCategories;
    std::vector<CategoryAnalyticsItem> expenseCategories;
    std::vector<AccountAnalyticsItem> accountFlows;
};

enum class GetAccountingAnalyticsError
{
    InvalidInput,
    StorageFailure,
    ArithmeticOverflow
};

using GetAccountingAnalyticsResult = std::variant<
    AccountingAnalytics,
    GetAccountingAnalyticsError>;

class GetAccountingAnalytics final
{
public:
    GetAccountingAnalytics(
        ITransactionRepository &transactionRepository,
        IAccountRepository &accountRepository,
        ICategoryRepository &categoryRepository);

    [[nodiscard]] GetAccountingAnalyticsResult execute(
        const GetAccountingAnalyticsRequest &request) const;

private:
    ITransactionRepository &m_transactionRepository;
    IAccountRepository &m_accountRepository;
    ICategoryRepository &m_categoryRepository;
};

} // namespace stukit::accounting
