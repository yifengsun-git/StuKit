#include "repositories/SqliteAccountRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <utility>

namespace stukit::accounting {

SqliteAccountRepository::SqliteAccountRepository(QSqlDatabase database)
    : m_database(std::move(database))
{
}

const QString &SqliteAccountRepository::lastError() const noexcept
{
    return m_lastError;
}

bool SqliteAccountRepository::save(const Account &account)
{
    m_lastError.clear();

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(
        QStringLiteral("INSERT INTO accounts("
                       "id, name, opening_balance_cents, archived) "
                       "VALUES(:id, :name, :openingBalanceCents, :archived) "
                       "ON CONFLICT(id) DO UPDATE SET "
                       "name = excluded.name, "
                       "opening_balance_cents = excluded.opening_balance_cents, "
                       "archived = excluded.archived"));

    query.bindValue(QStringLiteral(":id"), account.id().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":name"), account.name());
    query.bindValue(QStringLiteral(":openingBalanceCents"),
                    account.openingBalance().cents());
    query.bindValue(QStringLiteral(":archived"), account.isArchived() ? 1 : 0);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

std::optional<Account> SqliteAccountRepository::findById(const QUuid &id) const
{
    m_lastError.clear();

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT id, name, opening_balance_cents, archived "
                                 "FROM accounts WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return accountFromCurrentRow(query);
}

std::optional<std::vector<Account>> SqliteAccountRepository::findAll() const
{
    m_lastError.clear();
    std::vector<Account> accounts;

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("SELECT id, name, opening_balance_cents, archived "
                                   "FROM accounts ORDER BY name COLLATE NOCASE, id"))) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    while (query.next()) {
        auto account = accountFromCurrentRow(query);
        if (!account.has_value()) {
            return std::nullopt;
        }

        accounts.push_back(std::move(*account));
    }

    return accounts;
}

std::optional<Account>
SqliteAccountRepository::accountFromCurrentRow(const QSqlQuery &query) const
{
    const auto account = Account::create(AccountData{
        .id = QUuid(query.value(QStringLiteral("id")).toString()),
        .name = query.value(QStringLiteral("name")).toString(),
        .openingBalance = Money::fromCents(
            query.value(QStringLiteral("opening_balance_cents")).toLongLong()),
        .archived = query.value(QStringLiteral("archived")).toInt() != 0,
    });

    if (!account.has_value()) {
        m_lastError = QStringLiteral("Database contains an invalid account");
    }

    return account;
}

} // namespace stukit::accounting
