#pragma once

#include "ITransactionRepository.h"

#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

namespace stukit::accounting {

class SqliteTransactionRepository final : public ITransactionRepository
{
public:
    explicit SqliteTransactionRepository(QSqlDatabase database);

    [[nodiscard]] bool save(const Transaction &transaction) override;
    [[nodiscard]] std::optional<Transaction> findById(
        const QUuid &id) const override;
    [[nodiscard]] RemoveTransactionResult removeById(
        const QUuid &id) override;
    [[nodiscard]] std::vector<Transaction> findAll() const override;
    [[nodiscard]] std::optional<std::vector<Transaction>> findOccurredBetween(
        const QDateTime &startInclusive,
        const QDateTime &endExclusive) const override;

    [[nodiscard]] const QString &lastError() const noexcept;

private:
    [[nodiscard]] std::optional<Transaction> transactionFromCurrentRow(
        const QSqlQuery &query) const;

    QSqlDatabase m_database;
    mutable QString m_lastError;
};

} // namespace stukit::accounting
