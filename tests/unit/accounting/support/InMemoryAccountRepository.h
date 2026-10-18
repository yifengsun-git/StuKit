#pragma once

#include "IAccountRepository.h"

#include <algorithm>
#include <vector>

namespace stukit::accounting::testing {

class InMemoryAccountRepository final : public IAccountRepository
{
public:
    void setSaveShouldSucceed(bool shouldSucceed) noexcept
    {
        m_saveShouldSucceed = shouldSucceed;
    }

    void setFindAllShouldSucceed(bool shouldSucceed) noexcept
    {
        m_findAllShouldSucceed = shouldSucceed;
    }

    bool save(const Account &account) override
    {
        if (!m_saveShouldSucceed) {
            return false;
        }

        const auto existing = std::find_if(
            m_accounts.begin(),
            m_accounts.end(),
            [&account](const Account &candidate) {
                return candidate.id() == account.id();
            });

        if (existing == m_accounts.end()) {
            m_accounts.push_back(account);
        } else {
            *existing = account;
        }

        return true;
    }

    bool remove(const QUuid &id) override
    {
        const auto existing = std::find_if(
            m_accounts.begin(), m_accounts.end(),
            [&id](const Account &candidate) { return candidate.id() == id; });
        if (existing == m_accounts.end()) {
            return false;
        }
        m_accounts.erase(existing);
        return true;
    }

    std::optional<Account> findById(const QUuid &id) const override
    {
        const auto account = std::find_if(
            m_accounts.cbegin(),
            m_accounts.cend(),
            [&id](const Account &candidate) {
                return candidate.id() == id;
            });

        if (account == m_accounts.cend()) {
            return std::nullopt;
        }

        return *account;
    }

    std::optional<std::vector<Account>> findAll() const override
    {
        ++m_findAllCallCount;
        if (!m_findAllShouldSucceed) {
            return std::nullopt;
        }

        return m_accounts;
    }

    [[nodiscard]] int findAllCallCount() const noexcept
    {
        return m_findAllCallCount;
    }

private:
    bool m_saveShouldSucceed{true};
    bool m_findAllShouldSucceed{true};
    mutable int m_findAllCallCount{0};
    std::vector<Account> m_accounts;
};

} // namespace stukit::accounting::testing
