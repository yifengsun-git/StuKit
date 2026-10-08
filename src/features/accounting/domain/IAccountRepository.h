#pragma once

#include "Account.h"

#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::accounting {

class IAccountRepository
{
public:
    virtual ~IAccountRepository() = default;

    [[nodiscard]] virtual bool save(const Account &account) = 0;
    [[nodiscard]] virtual std::optional<Account> findById(const QUuid &id) const = 0;
    [[nodiscard]] virtual std::vector<Account> findAll() const = 0;
};

} // namespace stukit::accounting
