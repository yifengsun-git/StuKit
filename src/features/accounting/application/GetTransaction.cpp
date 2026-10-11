#include "GetTransaction.h"

namespace stukit::accounting {

GetTransaction::GetTransaction(ITransactionRepository &transactionRepository)
    : m_transactionRepository(transactionRepository)
{
}

GetTransactionResult GetTransaction::execute(const QUuid &transactionId) const
{
    if (transactionId.isNull()) {
        return GetTransactionError::InvalidInput;
    }

    const auto transaction = m_transactionRepository.findById(transactionId);
    if (!transaction.has_value()) {
        return GetTransactionError::NotFound;
    }
    return *transaction;
}

} // namespace stukit::accounting
