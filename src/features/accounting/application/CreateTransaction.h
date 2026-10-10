#pragma once

#include "IAccountRepository.h"
#include "ICategoryRepository.h"
#include "ITransactionRepository.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

#include <variant>

namespace stukit::accounting {

struct CreateTransactionRequest
{
    TransactionType type{TransactionType::Expense};
    Money amount{Money::fromCents(0)};
    QUuid accountId;
    QUuid categoryId;
    QDateTime occurredAt;
    QString note;
};

enum class CreateTransactionError
{
    InvalidInput,
    AccountNotFound,
    AccountArchived,
    CategoryNotFound,
    CategoryArchived,
    CategoryTypeMismatch,
    StorageFailure
};

using CreateTransactionResult = std::variant<Transaction, CreateTransactionError>;

class CreateTransaction final
{
public:
    CreateTransaction(
        ITransactionRepository &transactionRepository,
        IAccountRepository &accountRepository,
        ICategoryRepository &categoryRepository);

    [[nodiscard]] CreateTransactionResult execute(
        const CreateTransactionRequest &request);

private:
    ITransactionRepository &m_transactionRepository;
    IAccountRepository &m_accountRepository;
    ICategoryRepository &m_categoryRepository;
};

} // namespace stukit::accounting
