#pragma once

#include "GetAccountingAnalytics.h"

#include <QAbstractListModel>

#include <vector>

namespace stukit::accounting::presentation {

class AccountAnalyticsListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        AccountIdRole = Qt::UserRole + 1,
        NameRole,
        IncomeCentsRole,
        IncomeTextRole,
        ExpenseCentsRole,
        ExpenseTextRole,
        NetFlowCentsRole,
        NetFlowTextRole,
        TransactionCountRole,
        ArchivedRole
    };
    Q_ENUM(Role)

    explicit AccountAnalyticsListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void replaceItems(std::vector<AccountAnalyticsItem> items);
    void clear();

private:
    std::vector<AccountAnalyticsItem> m_items;
};

} // namespace stukit::accounting::presentation
