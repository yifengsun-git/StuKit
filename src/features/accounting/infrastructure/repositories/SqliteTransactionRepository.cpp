#include "repositories/SqliteTransactionRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <utility>

namespace stukit::accounting {
namespace {

constexpr int IncomeTypeCode = 0;
constexpr int ExpenseTypeCode = 1;

int transactionTypeToDatabase(TransactionType type)
{
    switch (type) {
    case TransactionType::Income:
        return IncomeTypeCode;
    case TransactionType::Expense:
        return ExpenseTypeCode;
    }

    Q_UNREACHABLE_RETURN(ExpenseTypeCode);
}

std::optional<TransactionType> transactionTypeFromDatabase(int value)
{
    switch (value) {
    case IncomeTypeCode:
        return TransactionType::Income;
    case ExpenseTypeCode:
        return TransactionType::Expense;
    default:
        return std::nullopt;
    }
}

} // namespace

SqliteTransactionRepository::SqliteTransactionRepository(QSqlDatabase database)
    : m_database(std::move(database))
{
}

bool SqliteTransactionRepository::save(const Transaction &transaction)
{
    m_lastError.clear();

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO transactions("
        "id, type, amount_cents, account_id, category_id, occurred_at, note) "
        "VALUES(:id, :type, :amountCents, :accountId, :categoryId, "
        ":occurredAt, :note) "
        "ON CONFLICT(id) DO UPDATE SET "
        "type = excluded.type, "
        "amount_cents = excluded.amount_cents, "
        "account_id = excluded.account_id, "
        "category_id = excluded.category_id, "
        "occurred_at = excluded.occurred_at, "
        "note = excluded.note"));
    query.bindValue(QStringLiteral(":id"),
                    transaction.id().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":type"),
                    transactionTypeToDatabase(transaction.type()));
    query.bindValue(QStringLiteral(":amountCents"), transaction.amount().cents());
    query.bindValue(QStringLiteral(":accountId"),
                    transaction.accountId().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":categoryId"),
                    transaction.categoryId().toString(QUuid::WithoutBraces));
    query.bindValue(
        QStringLiteral(":occurredAt"),
        transaction.occurredAt().toUTC().toString(Qt::ISODateWithMs));
    query.bindValue(QStringLiteral(":note"),
                    transaction.note().isNull() ? QStringLiteral("")
                                                : transaction.note());

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

std::optional<Transaction> SqliteTransactionRepository::findById(
    const QUuid &id) const
{
    m_lastError.clear();

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, type, amount_cents, account_id, category_id, "
        "occurred_at, note FROM transactions WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return transactionFromCurrentRow(query);
}

std::vector<Transaction> SqliteTransactionRepository::findAll() const
{
    m_lastError.clear();
    std::vector<Transaction> transactions;

    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return transactions;
    }

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT id, type, amount_cents, account_id, category_id, "
            "occurred_at, note FROM transactions "
            "ORDER BY occurred_at DESC, id"))) {
        m_lastError = query.lastError().text();
        return transactions;
    }

    while (query.next()) {
        auto transaction = transactionFromCurrentRow(query);
        if (!transaction.has_value()) {
            return {};
        }

        transactions.push_back(std::move(*transaction));
    }

    return transactions;
}

const QString &SqliteTransactionRepository::lastError() const noexcept
{
    return m_lastError;
}

std::optional<Transaction>
SqliteTransactionRepository::transactionFromCurrentRow(
    const QSqlQuery &query) const
{
    const auto type = transactionTypeFromDatabase(
        query.value(QStringLiteral("type")).toInt());
    if (!type.has_value()) {
        m_lastError = QStringLiteral("Database contains an invalid transaction type");
        return std::nullopt;
    }

    const auto occurredAt = QDateTime::fromString(
        query.value(QStringLiteral("occurred_at")).toString(),
        Qt::ISODateWithMs);
    if (!occurredAt.isValid()) {
        m_lastError = QStringLiteral("Database contains an invalid transaction time");
        return std::nullopt;
    }

    const auto transaction = Transaction::create(TransactionData{
        .id = QUuid(query.value(QStringLiteral("id")).toString()),
        .type = *type,
        .amount = Money::fromCents(
            query.value(QStringLiteral("amount_cents")).toLongLong()),
        .accountId = QUuid(query.value(QStringLiteral("account_id")).toString()),
        .categoryId = QUuid(
            query.value(QStringLiteral("category_id")).toString()),
        .occurredAt = occurredAt,
        .note = query.value(QStringLiteral("note")).toString(),
    });

    if (!transaction.has_value()) {
        m_lastError = QStringLiteral("Database contains an invalid transaction");
    }

    return transaction;
}

} // namespace stukit::accounting
