#pragma once

#include "ITransactionRepository.h"

#include <algorithm>
#include <iterator>

namespace stukit::accounting::testing {

class InMemoryTransactionRepository final : public ITransactionRepository
{
public:
    [[nodiscard]] bool save(const Transaction &transaction) override
    {
        if (!m_saveShouldSucceed) {
            return false;
        }

        const auto existing = std::find_if(
            m_transactions.begin(),
            m_transactions.end(),
            [&transaction](const Transaction &stored) {
                return stored.id() == transaction.id();
            });

        if (existing == m_transactions.end()) {
            m_transactions.push_back(transaction);
        } else {
            *existing = transaction;
        }

        return true;
    }

    [[nodiscard]] std::optional<Transaction> findById(
        const QUuid &id) const override
    {
        const auto transaction = std::find_if(
            m_transactions.cbegin(),
            m_transactions.cend(),
            [&id](const Transaction &stored) {
                return stored.id() == id;
            });

        if (transaction == m_transactions.cend()) {
            return std::nullopt;
        }

        return *transaction;
    }

    [[nodiscard]] std::vector<Transaction> findAll() const override
    {
        return m_transactions;
    }

    [[nodiscard]] std::optional<std::vector<Transaction>> findOccurredBetween(
        const QDateTime &startInclusive,
        const QDateTime &endExclusive) const override
    {
        if (!m_findOccurredBetweenShouldSucceed
            || !startInclusive.isValid()
            || !endExclusive.isValid()
            || startInclusive >= endExclusive) {
            return std::nullopt;
        }

        std::vector<Transaction> transactions;
        std::copy_if(
            m_transactions.cbegin(), m_transactions.cend(),
            std::back_inserter(transactions),
            [&startInclusive, &endExclusive](const Transaction &transaction) {
                return transaction.occurredAt() >= startInclusive
                    && transaction.occurredAt() < endExclusive;
            });

        std::sort(
            transactions.begin(), transactions.end(),
            [](const Transaction &left, const Transaction &right) {
                if (left.occurredAt() != right.occurredAt()) {
                    return left.occurredAt() > right.occurredAt();
                }
                return left.id().toString(QUuid::WithoutBraces)
                    < right.id().toString(QUuid::WithoutBraces);
            });

        return transactions;
    }

    void setSaveShouldSucceed(bool shouldSucceed) noexcept
    {
        m_saveShouldSucceed = shouldSucceed;
    }

    void setFindOccurredBetweenShouldSucceed(bool shouldSucceed) noexcept
    {
        m_findOccurredBetweenShouldSucceed = shouldSucceed;
    }

private:
    std::vector<Transaction> m_transactions;
    bool m_saveShouldSucceed{true};
    bool m_findOccurredBetweenShouldSucceed{true};
};

} // namespace stukit::accounting::testing
