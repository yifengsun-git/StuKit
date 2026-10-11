#include "DeleteTransaction.h"

namespace stukit::accounting {

DeleteTransaction::DeleteTransaction(
    ITransactionRepository &transactionRepository)
    : m_transactionRepository(transactionRepository)
{
}

std::optional<DeleteTransactionError> DeleteTransaction::execute(
    const QUuid &transactionId)
{
    if (transactionId.isNull()) {
        return DeleteTransactionError::InvalidInput;
    }

    switch (m_transactionRepository.removeById(transactionId)) {
    case RemoveTransactionResult::Removed:
        return std::nullopt;
    case RemoveTransactionResult::NotFound:
        return DeleteTransactionError::TransactionNotFound;
    case RemoveTransactionResult::StorageFailure:
        return DeleteTransactionError::StorageFailure;
    }

    Q_UNREACHABLE_RETURN(DeleteTransactionError::StorageFailure);
}

} // namespace stukit::accounting
