#pragma once

#include "GetMonthlyOverview.h"

#include <QAbstractListModel>
#include <QTimeZone>

#include <vector>

namespace stukit::accounting::presentation {

class TransactionListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        IdRole = Qt::UserRole + 1,
        IsIncomeRole,
        AmountTextRole,
        AccountIdRole,
        CategoryIdRole,
        OccurredAtRole,
        OccurredAtTextRole,
        NoteRole,
        AccountNameRole,
        CategoryNameRole,
        TypeTextRole,
        FullOccurredAtTextRole
    };
    Q_ENUM(Role)

    explicit TransactionListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void replaceTransactions(std::vector<MonthlyTransactionItem> transactions);
    void clear();
    void setTimeZone(QTimeZone timeZone);

private:
    std::vector<MonthlyTransactionItem> m_transactions;
    QTimeZone m_timeZone{QTimeZone::UTC};
};

} // namespace stukit::accounting::presentation
