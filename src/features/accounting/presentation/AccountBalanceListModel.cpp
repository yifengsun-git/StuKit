#include "AccountBalanceListModel.h"

#include "MoneyFormatter.h"

#include <utility>

namespace stukit::accounting::presentation {

AccountBalanceListModel::AccountBalanceListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int AccountBalanceListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_balances.size());
}

QVariant AccountBalanceListModel::data(
    const QModelIndex &index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= static_cast<int>(m_balances.size())) {
        return {};
    }

    const AccountBalanceItem &item = m_balances.at(
        static_cast<std::size_t>(index.row()));
    switch (role) {
    case AccountIdRole:
        return item.account.id().toString(QUuid::WithoutBraces);
    case NameRole:
        return item.account.name();
    case OpeningBalanceTextRole:
        return formatMoney(item.account.openingBalance());
    case CurrentBalanceTextRole:
        return formatMoney(item.currentBalance);
    case ArchivedRole:
        return item.account.isArchived();
    default:
        return {};
    }
}

QHash<int, QByteArray> AccountBalanceListModel::roleNames() const
{
    return {
        {AccountIdRole, QByteArrayLiteral("accountId")},
        {NameRole, QByteArrayLiteral("name")},
        {OpeningBalanceTextRole, QByteArrayLiteral("openingBalanceText")},
        {CurrentBalanceTextRole, QByteArrayLiteral("currentBalanceText")},
        {ArchivedRole, QByteArrayLiteral("archived")},
    };
}

void AccountBalanceListModel::replaceBalances(
    std::vector<AccountBalanceItem> balances)
{
    beginResetModel();
    m_balances = std::move(balances);
    endResetModel();
}

void AccountBalanceListModel::clear()
{
    replaceBalances({});
}

} // namespace stukit::accounting::presentation
