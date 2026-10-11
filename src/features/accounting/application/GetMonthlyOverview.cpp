#include "GetMonthlyOverview.h"

#include <QDate>
#include <QHash>

#include <utility>

namespace stukit::accounting {

GetMonthlyOverview::GetMonthlyOverview(
    ITransactionRepository &transactionRepository,
    IAccountRepository &accountRepository,
    ICategoryRepository &categoryRepository)
    : m_transactionRepository(transactionRepository)
    , m_accountRepository(accountRepository)
    , m_categoryRepository(categoryRepository)
{
}

GetMonthlyOverviewResult GetMonthlyOverview::execute(
    const GetMonthlyOverviewRequest &request) const
{
    const QDate firstDay(request.year, request.month, 1);
    if (!firstDay.isValid() || !request.timeZone.isValid()) {
        return GetMonthlyOverviewError::InvalidInput;
    }

    const QDateTime startInclusive = firstDay.startOfDay(request.timeZone);
    const QDateTime endExclusive = firstDay.addMonths(1).startOfDay(
        request.timeZone);
    if (!startInclusive.isValid()
        || !endExclusive.isValid()
        || startInclusive >= endExclusive) {
        return GetMonthlyOverviewError::InvalidInput;
    }

    auto transactions = m_transactionRepository.findOccurredBetween(
        startInclusive.toUTC(), endExclusive.toUTC());
    if (!transactions.has_value()) {
        return GetMonthlyOverviewError::StorageFailure;
    }

    const auto accounts = m_accountRepository.findAll();
    const auto categories = m_categoryRepository.findAll();
    if (!accounts.has_value() || !categories.has_value()) {
        return GetMonthlyOverviewError::StorageFailure;
    }

    QHash<QUuid, QString> accountNames;
    accountNames.reserve(static_cast<qsizetype>(accounts->size()));
    for (const Account &account : *accounts) {
        accountNames.insert(account.id(), account.name());
    }

    QHash<QUuid, QString> categoryNames;
    categoryNames.reserve(static_cast<qsizetype>(categories->size()));
    for (const Category &category : *categories) {
        categoryNames.insert(category.id(), category.name());
    }

    MonthlyOverview overview;
    overview.transactions.reserve(transactions->size());
    for (Transaction &transaction : *transactions) {
        const auto accountName = accountNames.constFind(transaction.accountId());
        const auto categoryName = categoryNames.constFind(transaction.categoryId());
        if (accountName == accountNames.cend()
            || categoryName == categoryNames.cend()) {
            return GetMonthlyOverviewError::StorageFailure;
        }

        if (transaction.type() == TransactionType::Income) {
            overview.totalIncome = overview.totalIncome + transaction.amount();
        } else {
            overview.totalExpense = overview.totalExpense + transaction.amount();
        }

        overview.transactions.push_back(MonthlyTransactionItem{
            .transaction = std::move(transaction),
            .accountName = *accountName,
            .categoryName = *categoryName,
        });
    }

    overview.balance = overview.totalIncome - overview.totalExpense;
    return overview;
}

} // namespace stukit::accounting
