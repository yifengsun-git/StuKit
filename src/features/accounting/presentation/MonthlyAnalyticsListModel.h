#pragma once

#include "GetAccountingAnalytics.h"

#include <QAbstractListModel>

#include <vector>

namespace stukit::accounting::presentation {

class MonthlyAnalyticsListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        MonthRole = Qt::UserRole + 1,
        MonthLabelRole,
        IncomeCentsRole,
        IncomeTextRole,
        ExpenseCentsRole,
        ExpenseTextRole,
        BalanceCentsRole,
        BalanceTextRole
    };
    Q_ENUM(Role)

    explicit MonthlyAnalyticsListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void replacePoints(std::vector<MonthlyAnalyticsPoint> points);
    void clear();

private:
    std::vector<MonthlyAnalyticsPoint> m_points;
};

} // namespace stukit::accounting::presentation
