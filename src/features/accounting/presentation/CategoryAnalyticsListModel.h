#pragma once

#include "GetAccountingAnalytics.h"

#include <QAbstractListModel>

#include <vector>

namespace stukit::accounting::presentation {

class CategoryAnalyticsListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        CategoryIdRole = Qt::UserRole + 1,
        NameRole,
        IsIncomeRole,
        AmountCentsRole,
        AmountTextRole,
        PercentageRole,
        PercentageTextRole,
        TransactionCountRole,
        ArchivedRole
    };
    Q_ENUM(Role)

    explicit CategoryAnalyticsListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void replaceItems(std::vector<CategoryAnalyticsItem> items, Money total);
    void clear();

private:
    struct Row
    {
        CategoryAnalyticsItem item;
        double percentage{0.0};
    };

    std::vector<Row> m_rows;
};

} // namespace stukit::accounting::presentation
