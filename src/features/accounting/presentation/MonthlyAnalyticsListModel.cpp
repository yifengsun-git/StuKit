#include "MonthlyAnalyticsListModel.h"

#include "MoneyFormatter.h"

#include <utility>

namespace stukit::accounting::presentation {

MonthlyAnalyticsListModel::MonthlyAnalyticsListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MonthlyAnalyticsListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_points.size());
}

QVariant MonthlyAnalyticsListModel::data(
    const QModelIndex &index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= static_cast<int>(m_points.size())) {
        return {};
    }

    const MonthlyAnalyticsPoint &point = m_points.at(
        static_cast<std::size_t>(index.row()));
    switch (role) {
    case MonthRole:
        return point.month;
    case MonthLabelRole:
        return point.month.toString(QStringLiteral("yyyy-MM"));
    case IncomeCentsRole:
        return QVariant::fromValue(point.income.cents());
    case IncomeTextRole:
        return formatMoney(point.income);
    case ExpenseCentsRole:
        return QVariant::fromValue(point.expense.cents());
    case ExpenseTextRole:
        return formatMoney(point.expense);
    case BalanceCentsRole:
        return QVariant::fromValue(point.balance.cents());
    case BalanceTextRole:
        return formatMoney(point.balance);
    default:
        return {};
    }
}

QHash<int, QByteArray> MonthlyAnalyticsListModel::roleNames() const
{
    return {
        {MonthRole, QByteArrayLiteral("month")},
        {MonthLabelRole, QByteArrayLiteral("monthLabel")},
        {IncomeCentsRole, QByteArrayLiteral("incomeCents")},
        {IncomeTextRole, QByteArrayLiteral("incomeText")},
        {ExpenseCentsRole, QByteArrayLiteral("expenseCents")},
        {ExpenseTextRole, QByteArrayLiteral("expenseText")},
        {BalanceCentsRole, QByteArrayLiteral("balanceCents")},
        {BalanceTextRole, QByteArrayLiteral("balanceText")},
    };
}

void MonthlyAnalyticsListModel::replacePoints(
    std::vector<MonthlyAnalyticsPoint> points)
{
    beginResetModel();
    m_points = std::move(points);
    endResetModel();
}

void MonthlyAnalyticsListModel::clear()
{
    replacePoints({});
}

} // namespace stukit::accounting::presentation
