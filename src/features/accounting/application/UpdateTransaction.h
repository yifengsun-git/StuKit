#pragma once

#include "IAccountRepository.h"
#include "ICategoryRepository.h"
#include "ITransactionRepository.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

#include <variant>

namespace stukit::accounting {

struct UpdateTransactionRequest
{
    QUuid id;
    TransactionType type{TransactionType::Expense};
    Money amount{Money::fromCents(0)};
    QUuid accountId;
    QUuid categoryId;
    QDateTime occurredAt;
    QString note;
};

enum class UpdateTransactionError
{
    InvalidInput,
    TransactionNotFound,
    AccountNotFound,
    AccountArchived,
    CategoryNotFound,
    CategoryArchived,
    CategoryTypeMismatch,
    StorageFailure
};

using UpdateTransactionResult = std::variant<Transaction, UpdateTransactionError>;

class UpdateTransaction final
{
public:
    UpdateTransaction(
        ITransactionRepository &transactionRepository,
        IAccountRepository &accountRepository,
        ICategoryRepository &categoryRepository);

    [[nodiscard]] UpdateTransactionResult execute(
        const UpdateTransactionRequest &request);

private:
    ITransactionRepository &m_transactionRepository;
    IAccountRepository &m_accountRepository;
    ICategoryRepository &m_categoryRepository;
};

} // namespace stukit::accounting
