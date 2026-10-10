#pragma once

#include "Transaction.h"

#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::accounting {

class ITransactionRepository
{
public:
    virtual ~ITransactionRepository() = default;

    [[nodiscard]] virtual bool save(const Transaction &transaction) = 0;
    [[nodiscard]] virtual std::optional<Transaction> findById(
        const QUuid &id) const = 0;
    [[nodiscard]] virtual std::vector<Transaction> findAll() const = 0;
};

} // namespace stukit::accounting
