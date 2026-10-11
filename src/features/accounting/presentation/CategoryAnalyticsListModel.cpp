#include "CategoryAnalyticsListModel.h"

#include "MoneyFormatter.h"

#include <utility>

namespace stukit::accounting::presentation {

CategoryAnalyticsListModel::CategoryAnalyticsListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CategoryAnalyticsListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant CategoryAnalyticsListModel::data(
    const QModelIndex &index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= static_cast<int>(m_rows.size())) {
        return {};
    }

    const Row &row = m_rows.at(static_cast<std::size_t>(index.row()));
    switch (role) {
    case CategoryIdRole:
        return row.item.categoryId.toString(QUuid::WithoutBraces);
    case NameRole:
        return row.item.categoryName;
    case IsIncomeRole:
        return row.item.type == TransactionType::Income;
    case AmountCentsRole:
        return QVariant::fromValue(row.item.amount.cents());
    case AmountTextRole:
        return formatMoney(row.item.amount);
    case PercentageRole:
        return row.percentage;
    case PercentageTextRole:
        return QStringLiteral("%1%").arg(
            QString::number(row.percentage, 'f', 1));
    case TransactionCountRole:
        return row.item.transactionCount;
    case ArchivedRole:
        return row.item.archived;
    default:
        return {};
    }
}

QHash<int, QByteArray> CategoryAnalyticsListModel::roleNames() const
{
    return {
        {CategoryIdRole, QByteArrayLiteral("categoryId")},
        {NameRole, QByteArrayLiteral("name")},
        {IsIncomeRole, QByteArrayLiteral("isIncome")},
        {AmountCentsRole, QByteArrayLiteral("amountCents")},
        {AmountTextRole, QByteArrayLiteral("amountText")},
        {PercentageRole, QByteArrayLiteral("percentage")},
        {PercentageTextRole, QByteArrayLiteral("percentageText")},
        {TransactionCountRole, QByteArrayLiteral("transactionCount")},
        {ArchivedRole, QByteArrayLiteral("archived")},
    };
}

void CategoryAnalyticsListModel::replaceItems(
    std::vector<CategoryAnalyticsItem> items, Money total)
{
    std::vector<Row> rows;
    rows.reserve(items.size());
    for (CategoryAnalyticsItem &item : items) {
        const double percentage = total.cents() > 0
            ? static_cast<double>(item.amount.cents())
                / static_cast<double>(total.cents()) * 100.0
            : 0.0;
        rows.push_back({
            .item = std::move(item),
            .percentage = percentage,
        });
    }

    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
}

void CategoryAnalyticsListModel::clear()
{
    beginResetModel();
    m_rows.clear();
    endResetModel();
}

} // namespace stukit::accounting::presentation
