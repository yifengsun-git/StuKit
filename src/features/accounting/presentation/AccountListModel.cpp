#include "AccountListModel.h"

#include "MoneyFormatter.h"

#include <utility>

namespace stukit::accounting::presentation {

AccountListModel::AccountListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int AccountListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_accounts.size());
}

QVariant AccountListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= static_cast<int>(m_accounts.size())) {
        return {};
    }

    const Account &account = m_accounts.at(static_cast<std::size_t>(index.row()));
    switch (role) {
    case AccountIdRole:
        return account.id().toString(QUuid::WithoutBraces);
    case NameRole:
        return account.name();
    case OpeningBalanceTextRole:
        return formatMoney(account.openingBalance());
    case ArchivedRole:
        return account.isArchived();
    default:
        return {};
    }
}

QHash<int, QByteArray> AccountListModel::roleNames() const
{
    return {
        {AccountIdRole, QByteArrayLiteral("accountId")},
        {NameRole, QByteArrayLiteral("name")},
        {OpeningBalanceTextRole, QByteArrayLiteral("openingBalanceText")},
        {ArchivedRole, QByteArrayLiteral("archived")},
    };
}

void AccountListModel::replaceAccounts(std::vector<Account> accounts)
{
    beginResetModel();
    m_accounts = std::move(accounts);
    endResetModel();
}

void AccountListModel::clear()
{
    replaceAccounts({});
}

} // namespace stukit::accounting::presentation
