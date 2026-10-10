#pragma once

#include "Transaction.h"

#include <QAbstractListModel>

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
        NoteRole
    };
    Q_ENUM(Role)

    explicit TransactionListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void replaceTransactions(std::vector<Transaction> transactions);
    void clear();

private:
    std::vector<Transaction> m_transactions;
};

} // namespace stukit::accounting::presentation
