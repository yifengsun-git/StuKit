#include "AccountAnalyticsListModel.h"

#include "MoneyFormatter.h"

#include <utility>

namespace stukit::accounting::presentation {

AccountAnalyticsListModel::AccountAnalyticsListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int AccountAnalyticsListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant AccountAnalyticsListModel::data(
    const QModelIndex &index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= static_cast<int>(m_items.size())) {
        return {};
    }

    const AccountAnalyticsItem &item = m_items.at(
        static_cast<std::size_t>(index.row()));
    switch (role) {
    case AccountIdRole:
        return item.accountId.toString(QUuid::WithoutBraces);
    case NameRole:
        return item.accountName;
    case IncomeCentsRole:
        return QVariant::fromValue(item.income.cents());
    case IncomeTextRole:
        return formatMoney(item.income);
    case ExpenseCentsRole:
        return QVariant::fromValue(item.expense.cents());
    case ExpenseTextRole:
        return formatMoney(item.expense);
    case NetFlowCentsRole:
        return QVariant::fromValue(item.netFlow.cents());
    case NetFlowTextRole:
        return formatMoney(item.netFlow);
    case TransactionCountRole:
        return item.transactionCount;
    case ArchivedRole:
        return item.archived;
    default:
        return {};
    }
}

QHash<int, QByteArray> AccountAnalyticsListModel::roleNames() const
{
    return {
        {AccountIdRole, QByteArrayLiteral("accountId")},
        {NameRole, QByteArrayLiteral("name")},
        {IncomeCentsRole, QByteArrayLiteral("incomeCents")},
        {IncomeTextRole, QByteArrayLiteral("incomeText")},
        {ExpenseCentsRole, QByteArrayLiteral("expenseCents")},
        {ExpenseTextRole, QByteArrayLiteral("expenseText")},
        {NetFlowCentsRole, QByteArrayLiteral("netFlowCents")},
        {NetFlowTextRole, QByteArrayLiteral("netFlowText")},
        {TransactionCountRole, QByteArrayLiteral("transactionCount")},
        {ArchivedRole, QByteArrayLiteral("archived")},
    };
}

void AccountAnalyticsListModel::replaceItems(
    std::vector<AccountAnalyticsItem> items)
{
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
}

void AccountAnalyticsListModel::clear()
{
    replaceItems({});
}

} // namespace stukit::accounting::presentation
