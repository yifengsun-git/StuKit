#pragma once

#include "IAccountRepository.h"
#include "ICategoryRepository.h"
#include "ITransactionRepository.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

#include <variant>
#include <vector>

namespace stukit::accounting {

struct TransferFundsRequest
{
    Money amount{Money::fromCents(0)};
    Money fee{Money::fromCents(0)};
    QUuid sourceAccountId;
    QUuid destinationAccountId;
    QUuid outgoingCategoryId;
    QUuid incomingCategoryId;
    QUuid feeCategoryId;
    QDateTime occurredAt;
    QString note;
};

enum class TransferFundsError
{
    InvalidInput,
    SameAccount,
    AccountNotFound,
    AccountArchived,
    CategoryNotFound,
    CategoryArchived,
    CategoryTypeMismatch,
    StorageFailure
};

using TransferFundsResult =
    std::variant<std::vector<Transaction>, TransferFundsError>;

class TransferFunds final
{
public:
    TransferFunds(
        ITransactionRepository &transactionRepository,
        IAccountRepository &accountRepository,
        ICategoryRepository &categoryRepository);

    [[nodiscard]] TransferFundsResult execute(
        const TransferFundsRequest &request);

private:
    ITransactionRepository &m_transactionRepository;
    IAccountRepository &m_accountRepository;
    ICategoryRepository &m_categoryRepository;
};

} // namespace stukit::accounting
