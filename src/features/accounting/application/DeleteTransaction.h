#pragma once

#include "ITransactionRepository.h"

#include <QUuid>

#include <optional>

namespace stukit::accounting {

enum class DeleteTransactionError
{
    InvalidInput,
    TransactionNotFound,
    StorageFailure
};

class DeleteTransaction final
{
public:
    explicit DeleteTransaction(ITransactionRepository &transactionRepository);

    [[nodiscard]] std::optional<DeleteTransactionError> execute(
        const QUuid &transactionId);

private:
    ITransactionRepository &m_transactionRepository;
};

} // namespace stukit::accounting
