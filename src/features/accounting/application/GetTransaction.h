#pragma once

#include "ITransactionRepository.h"

#include <QUuid>

#include <variant>

namespace stukit::accounting {

enum class GetTransactionError
{
    InvalidInput,
    NotFound
};

using GetTransactionResult = std::variant<Transaction, GetTransactionError>;

class GetTransaction final
{
public:
    explicit GetTransaction(ITransactionRepository &transactionRepository);

    [[nodiscard]] GetTransactionResult execute(
        const QUuid &transactionId) const;

private:
    ITransactionRepository &m_transactionRepository;
};

} // namespace stukit::accounting
