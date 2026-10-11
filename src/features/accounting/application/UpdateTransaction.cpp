#include "UpdateTransaction.h"

namespace stukit::accounting {

UpdateTransaction::UpdateTransaction(
    ITransactionRepository &transactionRepository,
    IAccountRepository &accountRepository,
    ICategoryRepository &categoryRepository)
    : m_transactionRepository(transactionRepository)
    , m_accountRepository(accountRepository)
    , m_categoryRepository(categoryRepository)
{
}

UpdateTransactionResult UpdateTransaction::execute(
    const UpdateTransactionRequest &request)
{
    if (request.id.isNull()) {
        return UpdateTransactionError::InvalidInput;
    }
    if (!m_transactionRepository.findById(request.id).has_value()) {
        return UpdateTransactionError::TransactionNotFound;
    }

    const auto transaction = Transaction::create(TransactionData{
        .id = request.id,
        .type = request.type,
        .amount = request.amount,
        .accountId = request.accountId,
        .categoryId = request.categoryId,
        .occurredAt = request.occurredAt,
        .note = request.note,
    });
    if (!transaction.has_value()) {
        return UpdateTransactionError::InvalidInput;
    }

    const auto account = m_accountRepository.findById(request.accountId);
    if (!account.has_value()) {
        return UpdateTransactionError::AccountNotFound;
    }
    if (account->isArchived()) {
        return UpdateTransactionError::AccountArchived;
    }

    const auto category = m_categoryRepository.findById(request.categoryId);
    if (!category.has_value()) {
        return UpdateTransactionError::CategoryNotFound;
    }
    if (category->isArchived()) {
        return UpdateTransactionError::CategoryArchived;
    }
    if (category->type() != request.type) {
        return UpdateTransactionError::CategoryTypeMismatch;
    }

    if (!m_transactionRepository.save(*transaction)) {
        return UpdateTransactionError::StorageFailure;
    }
    return *transaction;
}

} // namespace stukit::accounting
