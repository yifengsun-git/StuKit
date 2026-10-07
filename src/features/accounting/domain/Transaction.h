#pragma once

#include "Money.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

#include <optional>

namespace stukit::accounting {

enum class TransactionType
{
    Income,
    Expense
};

struct TransactionData
{
    QUuid id;
    TransactionType type{TransactionType::Expense};
    Money amount{Money::fromCents(0)};
    QUuid accountId;
    QUuid categoryId;
    QDateTime occurredAt;
    QString note;
};

class Transaction final
{
public:
    static std::optional<Transaction> create(TransactionData data);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] TransactionType type() const noexcept;
    [[nodiscard]] Money amount() const noexcept;
    [[nodiscard]] const QUuid &accountId() const noexcept;
    [[nodiscard]] const QUuid &categoryId() const noexcept;
    [[nodiscard]] const QDateTime &occurredAt() const noexcept;
    [[nodiscard]] const QString &note() const noexcept;

private:
    explicit Transaction(TransactionData data);

    TransactionData m_data;
};

} // namespace stukit::accounting
