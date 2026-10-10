#pragma once

#include "Account.h"

#include <QAbstractListModel>

#include <vector>

namespace stukit::accounting::presentation {

class AccountListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        AccountIdRole = Qt::UserRole + 1,
        NameRole,
        OpeningBalanceTextRole,
        ArchivedRole
    };
    Q_ENUM(Role)

    explicit AccountListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void replaceAccounts(std::vector<Account> accounts);
    void clear();

private:
    std::vector<Account> m_accounts;
};

} // namespace stukit::accounting::presentation
