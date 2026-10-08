#include "Account.h"

#include <utility>

namespace stukit::accounting {

std::optional<Account> Account::create(AccountData data)
{
    data.name = data.name.trimmed();

    if (data.id.isNull()
        || data.name.isEmpty()
        || data.name.size() > MaximumNameLength) {
        return std::nullopt;
    }

    return Account(std::move(data));
}

Account::Account(AccountData data)
    : m_data(std::move(data))
{
}

const QUuid &Account::id() const noexcept
{
    return m_data.id;
}

const QString &Account::name() const noexcept
{
    return m_data.name;
}

Money Account::openingBalance() const noexcept
{
    return m_data.openingBalance;
}

bool Account::isArchived() const noexcept
{
    return m_data.archived;
}

} // namespace stukit::accounting
