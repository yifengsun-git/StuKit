#pragma once

#include "GetAccountBalances.h"

#include <QAbstractListModel>

#include <vector>

namespace stukit::accounting::presentation {

class AccountBalanceListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        AccountIdRole = Qt::UserRole + 1,
        NameRole,
        OpeningBalanceTextRole,
        CurrentBalanceTextRole,
        ArchivedRole
    };
    Q_ENUM(Role)

    explicit AccountBalanceListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void replaceBalances(std::vector<AccountBalanceItem> balances);
    void clear();

private:
    std::vector<AccountBalanceItem> m_balances;
};

} // namespace stukit::accounting::presentation
