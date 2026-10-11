#include "TransactionListModel.h"

#include "MoneyFormatter.h"

#include <utility>

namespace stukit::accounting::presentation {

TransactionListModel::TransactionListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TransactionListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_transactions.size());
}

QVariant TransactionListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= static_cast<int>(m_transactions.size())) {
        return {};
    }

    const MonthlyTransactionItem &item = m_transactions.at(
        static_cast<std::size_t>(index.row()));
    const Transaction &transaction = item.transaction;
    switch (role) {
    case IdRole:
        return transaction.id().toString(QUuid::WithoutBraces);
    case IsIncomeRole:
        return transaction.type() == TransactionType::Income;
    case AmountTextRole:
        return formatMoney(transaction.amount());
    case AccountIdRole:
        return transaction.accountId().toString(QUuid::WithoutBraces);
    case CategoryIdRole:
        return transaction.categoryId().toString(QUuid::WithoutBraces);
    case OccurredAtRole:
        return transaction.occurredAt();
    case OccurredAtTextRole:
        return transaction.occurredAt()
            .toTimeZone(m_timeZone)
            .toString(QStringLiteral("MM-dd HH:mm"));
    case NoteRole:
        return transaction.note();
    case AccountNameRole:
        return item.accountName;
    case CategoryNameRole:
        return item.categoryName;
    case TypeTextRole:
        return transaction.type() == TransactionType::Income
            ? QStringLiteral("收入")
            : QStringLiteral("支出");
    case FullOccurredAtTextRole:
        return transaction.occurredAt()
            .toTimeZone(m_timeZone)
            .toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    default:
        return {};
    }
}

QHash<int, QByteArray> TransactionListModel::roleNames() const
{
    return {
        {IdRole, QByteArrayLiteral("transactionId")},
        {IsIncomeRole, QByteArrayLiteral("isIncome")},
        {AmountTextRole, QByteArrayLiteral("amountText")},
        {AccountIdRole, QByteArrayLiteral("accountId")},
        {CategoryIdRole, QByteArrayLiteral("categoryId")},
        {OccurredAtRole, QByteArrayLiteral("occurredAt")},
        {OccurredAtTextRole, QByteArrayLiteral("occurredAtText")},
        {NoteRole, QByteArrayLiteral("note")},
        {AccountNameRole, QByteArrayLiteral("accountName")},
        {CategoryNameRole, QByteArrayLiteral("categoryName")},
        {TypeTextRole, QByteArrayLiteral("typeText")},
        {FullOccurredAtTextRole, QByteArrayLiteral("fullOccurredAtText")},
    };
}

void TransactionListModel::replaceTransactions(
    std::vector<MonthlyTransactionItem> transactions)
{
    beginResetModel();
    m_transactions = std::move(transactions);
    endResetModel();
}

void TransactionListModel::clear()
{
    replaceTransactions({});
}

void TransactionListModel::setTimeZone(QTimeZone timeZone)
{
    if (!timeZone.isValid() || m_timeZone == timeZone) {
        return;
    }

    m_timeZone = std::move(timeZone);
    if (m_transactions.empty()) {
        return;
    }

    emit dataChanged(
        index(0), index(rowCount() - 1),
        {OccurredAtTextRole, FullOccurredAtTextRole});
}

} // namespace stukit::accounting::presentation
