#pragma once

#include <QSortFilterProxyModel>
#include <QString>

namespace stukit::accounting::presentation {

class TransactionFilterProxyModel final : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum TypeFilter
    {
        AllTransactions = 0,
        IncomeTransactions,
        ExpenseTransactions
    };
    Q_ENUM(TypeFilter)

    explicit TransactionFilterProxyModel(QObject *parent = nullptr);

    [[nodiscard]] TypeFilter typeFilter() const noexcept;
    [[nodiscard]] const QString &accountIdFilter() const noexcept;
    [[nodiscard]] const QString &categoryIdFilter() const noexcept;
    [[nodiscard]] const QString &searchText() const noexcept;
    [[nodiscard]] bool hasActiveFilters() const noexcept;

    void setTypeFilter(TypeFilter filter);
    void setAccountIdFilter(QString accountId);
    void setCategoryIdFilter(QString categoryId);
    void setSearchText(QString searchText);
    void clearFilters();

protected:
    [[nodiscard]] bool filterAcceptsRow(
        int sourceRow,
        const QModelIndex &sourceParent) const override;

private:
    TypeFilter m_typeFilter{AllTransactions};
    QString m_accountIdFilter;
    QString m_categoryIdFilter;
    QString m_searchText;
};

} // namespace stukit::accounting::presentation
