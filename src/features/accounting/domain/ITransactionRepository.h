#pragma once

#include "Transaction.h"

#include <QDateTime>
#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::accounting {

enum class RemoveTransactionResult
{
    Removed,
    NotFound,
    StorageFailure
};

class ITransactionRepository
{
public:
    virtual ~ITransactionRepository() = default;

    [[nodiscard]] virtual bool save(const Transaction &transaction) = 0;
    [[nodiscard]] virtual std::optional<Transaction> findById(
        const QUuid &id) const = 0;
    [[nodiscard]] virtual RemoveTransactionResult removeById(
        const QUuid &id) = 0;
    [[nodiscard]] virtual std::vector<Transaction> findAll() const = 0;
    [[nodiscard]] virtual std::optional<std::vector<Transaction>>
    findOccurredBetween(const QDateTime &startInclusive,
                        const QDateTime &endExclusive) const = 0;
};

} // namespace stukit::accounting
