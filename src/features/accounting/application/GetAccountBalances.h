#pragma once

#include "IAccountRepository.h"
#include "ITransactionRepository.h"

#include <variant>
#include <vector>

namespace stukit::accounting {

struct AccountBalanceItem
{
    Account account;
    Money currentBalance{Money::fromCents(0)};
};

struct AccountBalances
{
    std::vector<AccountBalanceItem> accounts;
    Money activeTotal{Money::fromCents(0)};
};

enum class GetAccountBalancesError
{
    StorageFailure,
    ArithmeticOverflow
};

using GetAccountBalancesResult = std::variant<
    AccountBalances,
    GetAccountBalancesError>;

class GetAccountBalances final
{
public:
    GetAccountBalances(
        IAccountRepository &accountRepository,
        ITransactionRepository &transactionRepository);

    [[nodiscard]] GetAccountBalancesResult execute() const;

private:
    IAccountRepository &m_accountRepository;
    ITransactionRepository &m_transactionRepository;
};

} // namespace stukit::accounting
