#include "GetAccountBalances.h"

#include <QHash>

#include <limits>
#include <optional>
#include <utility>

namespace stukit::accounting {
namespace {

std::optional<qint64> checkedAdd(qint64 left, qint64 right)
{
    if (right > 0
        && left > std::numeric_limits<qint64>::max() - right) {
        return std::nullopt;
    }
    if (right < 0
        && left < std::numeric_limits<qint64>::min() - right) {
        return std::nullopt;
    }
    return left + right;
}

} // namespace

GetAccountBalances::GetAccountBalances(
    IAccountRepository &accountRepository,
    ITransactionRepository &transactionRepository)
    : m_accountRepository(accountRepository)
    , m_transactionRepository(transactionRepository)
{
}

GetAccountBalancesResult GetAccountBalances::execute() const
{
    auto accounts = m_accountRepository.findAll();
    const auto transactions = m_transactionRepository.findAll();
    if (!accounts.has_value() || !transactions.has_value()) {
        return GetAccountBalancesError::StorageFailure;
    }

    QHash<QUuid, qint64> balances;
    balances.reserve(static_cast<qsizetype>(accounts->size()));
    for (const Account &account : *accounts) {
        balances.insert(account.id(), account.openingBalance().cents());
    }

    for (const Transaction &transaction : *transactions) {
        auto balance = balances.find(transaction.accountId());
        if (balance == balances.end()) {
            return GetAccountBalancesError::StorageFailure;
        }

        const qint64 change = transaction.type() == TransactionType::Income
            ? transaction.amount().cents()
            : -transaction.amount().cents();
        const auto updated = checkedAdd(*balance, change);
        if (!updated.has_value()) {
            return GetAccountBalancesError::ArithmeticOverflow;
        }
        *balance = *updated;
    }

    AccountBalances result;
    result.accounts.reserve(accounts->size());
    qint64 activeTotalCents = 0;
    for (Account &account : *accounts) {
        const qint64 currentBalanceCents = balances.value(account.id());
        if (!account.isArchived()) {
            const auto updatedTotal = checkedAdd(
                activeTotalCents, currentBalanceCents);
            if (!updatedTotal.has_value()) {
                return GetAccountBalancesError::ArithmeticOverflow;
            }
            activeTotalCents = *updatedTotal;
        }

        result.accounts.push_back(AccountBalanceItem{
            .account = std::move(account),
            .currentBalance = Money::fromCents(currentBalanceCents),
        });
    }
    result.activeTotal = Money::fromCents(activeTotalCents);
    return result;
}

} // namespace stukit::accounting
