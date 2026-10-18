#pragma once

#include "IAccountRepository.h"

#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

namespace stukit::accounting {

class SqliteAccountRepository final : public IAccountRepository
{
public:
    explicit SqliteAccountRepository(QSqlDatabase database);

    [[nodiscard]] bool save(const Account &account) override;
    [[nodiscard]] bool remove(const QUuid &id) override;
    [[nodiscard]] std::optional<Account> findById(const QUuid &id) const override;
    [[nodiscard]] std::optional<std::vector<Account>> findAll() const override;

    [[nodiscard]] const QString &lastError() const noexcept;

private:
    [[nodiscard]] std::optional<Account>
    accountFromCurrentRow(const QSqlQuery &query) const;

    QSqlDatabase m_database;
    mutable QString m_lastError;
};

} // namespace stukit::accounting
