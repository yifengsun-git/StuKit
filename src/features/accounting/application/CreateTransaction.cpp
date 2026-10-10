#include "CreateTransaction.h"

#include <QUuid>

namespace stukit::accounting {

CreateTransaction::CreateTransaction(
    ITransactionRepository &transactionRepository,
    IAccountRepository &accountRepository,
    ICategoryRepository &categoryRepository)
    : m_transactionRepository(transactionRepository)
    , m_accountRepository(accountRepository)
    , m_categoryRepository(categoryRepository)
{
}

CreateTransactionResult CreateTransaction::execute(
    const CreateTransactionRequest &request)
{
    const auto transaction = Transaction::create(TransactionData{
        .id = QUuid::createUuid(),
        .type = request.type,
        .amount = request.amount,
        .accountId = request.accountId,
        .categoryId = request.categoryId,
        .occurredAt = request.occurredAt,
        .note = request.note,
    });

    if (!transaction.has_value()) {
        return CreateTransactionError::InvalidInput;
    }

    const auto account = m_accountRepository.findById(request.accountId);
    if (!account.has_value()) {
        return CreateTransactionError::AccountNotFound;
    }
    if (account->isArchived()) {
        return CreateTransactionError::AccountArchived;
    }

    const auto category = m_categoryRepository.findById(request.categoryId);
    if (!category.has_value()) {
        return CreateTransactionError::CategoryNotFound;
    }
    if (category->isArchived()) {
        return CreateTransactionError::CategoryArchived;
    }
    if (category->type() != request.type) {
        return CreateTransactionError::CategoryTypeMismatch;
    }

    if (!m_transactionRepository.save(*transaction)) {
        return CreateTransactionError::StorageFailure;
    }

    return *transaction;
}

} // namespace stukit::accounting
