#pragma once

#include "ITransactionRepository.h"

#include <algorithm>

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

    void setSaveShouldSucceed(bool shouldSucceed) noexcept
    {
        m_saveShouldSucceed = shouldSucceed;
    }

private:
    std::vector<Transaction> m_transactions;
    bool m_saveShouldSucceed{true};
};

} // namespace stukit::accounting::testing
