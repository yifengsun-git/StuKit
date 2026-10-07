#include "Transaction.h"

#include <utility>

namespace stukit::accounting {

std::optional<Transaction> Transaction::create(TransactionData data)
{
    if (data.id.isNull()
        || data.accountId.isNull()
        || data.categoryId.isNull()
        || !data.amount.isPositive()
        || !data.occurredAt.isValid()) {
        return std::nullopt;
    }

    data.note = data.note.trimmed();

    return Transaction(std::move(data));
}

const QUuid &Transaction::id() const noexcept
{
    return m_data.id;
}

TransactionType Transaction::type() const noexcept
{
    return m_data.type;
}

Money Transaction::amount() const noexcept
{
    return m_data.amount;
}

const QUuid &Transaction::accountId() const noexcept
{
    return m_data.accountId;
}

const QUuid &Transaction::categoryId() const noexcept
{
    return m_data.categoryId;
}

const QDateTime &Transaction::occurredAt() const noexcept
{
    return m_data.occurredAt;
}

const QString &Transaction::note() const noexcept
{
    return m_data.note;
}

Transaction::Transaction(TransactionData data)
    : m_data(std::move(data))
{
}

} // namespace stukit::accounting
