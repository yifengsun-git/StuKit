#include "GetMonthlyOverview.h"

#include <QDate>

#include <utility>

namespace stukit::accounting {

GetMonthlyOverview::GetMonthlyOverview(
    ITransactionRepository &transactionRepository)
    : m_transactionRepository(transactionRepository)
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

    MonthlyOverview overview;
    overview.transactions = std::move(*transactions);

    for (const Transaction &transaction : overview.transactions) {
        if (transaction.type() == TransactionType::Income) {
            overview.totalIncome = overview.totalIncome + transaction.amount();
        } else {
            overview.totalExpense = overview.totalExpense + transaction.amount();
        }
    }

    overview.balance = overview.totalIncome - overview.totalExpense;
    return overview;
}

} // namespace stukit::accounting
