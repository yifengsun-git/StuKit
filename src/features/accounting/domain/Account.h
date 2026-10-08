#pragma once

#include "Money.h"

#include <QString>
#include <QUuid>

#include <optional>

namespace stukit::accounting {

struct AccountData
{
    QUuid id;
    QString name;
    Money openingBalance{Money::fromCents(0)};
    bool archived{false};
};

class Account final
{
public:
    static constexpr qsizetype MaximumNameLength = 40;

    static std::optional<Account> create(AccountData data);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] const QString &name() const noexcept;
    [[nodiscard]] Money openingBalance() const noexcept;
    [[nodiscard]] bool isArchived() const noexcept;

private:
    explicit Account(AccountData data);

    AccountData m_data;
};

} // namespace stukit::accounting
