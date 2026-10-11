#include "TransactionFilterProxyModel.h"

#include "TransactionListModel.h"

#include <QAbstractItemModel>

#include <utility>

namespace stukit::accounting::presentation {

TransactionFilterProxyModel::TransactionFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

TransactionFilterProxyModel::TypeFilter
TransactionFilterProxyModel::typeFilter() const noexcept
{
    return m_typeFilter;
}

const QString &TransactionFilterProxyModel::accountIdFilter() const noexcept
{
    return m_accountIdFilter;
}

const QString &TransactionFilterProxyModel::categoryIdFilter() const noexcept
{
    return m_categoryIdFilter;
}

const QString &TransactionFilterProxyModel::searchText() const noexcept
{
    return m_searchText;
}

bool TransactionFilterProxyModel::hasActiveFilters() const noexcept
{
    return m_typeFilter != AllTransactions
        || !m_accountIdFilter.isEmpty()
        || !m_categoryIdFilter.isEmpty()
        || !m_searchText.isEmpty();
}

void TransactionFilterProxyModel::setTypeFilter(TypeFilter filter)
{
    if (m_typeFilter == filter) {
        return;
    }

    m_typeFilter = filter;
    invalidateRowsFilter();
}

void TransactionFilterProxyModel::setAccountIdFilter(QString accountId)
{
    accountId = accountId.trimmed();
    if (m_accountIdFilter == accountId) {
        return;
    }

    m_accountIdFilter = std::move(accountId);
    invalidateRowsFilter();
}

void TransactionFilterProxyModel::setCategoryIdFilter(QString categoryId)
{
    categoryId = categoryId.trimmed();
    if (m_categoryIdFilter == categoryId) {
        return;
    }

    m_categoryIdFilter = std::move(categoryId);
    invalidateRowsFilter();
}

void TransactionFilterProxyModel::setSearchText(QString searchText)
{
    searchText = searchText.trimmed();
    if (m_searchText == searchText) {
        return;
    }

    m_searchText = std::move(searchText);
    invalidateRowsFilter();
}

void TransactionFilterProxyModel::clearFilters()
{
    if (!hasActiveFilters()) {
        return;
    }

    m_typeFilter = AllTransactions;
    m_accountIdFilter.clear();
    m_categoryIdFilter.clear();
    m_searchText.clear();
    invalidateRowsFilter();
}

bool TransactionFilterProxyModel::filterAcceptsRow(
    int sourceRow,
    const QModelIndex &sourceParent) const
{
    const QAbstractItemModel *model = sourceModel();
    if (model == nullptr) {
        return false;
    }

    const QModelIndex sourceIndex = model->index(sourceRow, 0, sourceParent);
    const bool income = model->data(
        sourceIndex, TransactionListModel::IsIncomeRole).toBool();
    if ((m_typeFilter == IncomeTransactions && !income)
        || (m_typeFilter == ExpenseTransactions && income)) {
        return false;
    }

    if (!m_accountIdFilter.isEmpty()
        && model->data(sourceIndex, TransactionListModel::AccountIdRole)
               .toString() != m_accountIdFilter) {
        return false;
    }

    if (!m_categoryIdFilter.isEmpty()
        && model->data(sourceIndex, TransactionListModel::CategoryIdRole)
               .toString() != m_categoryIdFilter) {
        return false;
    }

    if (m_searchText.isEmpty()) {
        return true;
    }

    return model->data(sourceIndex, TransactionListModel::NoteRole)
               .toString().contains(m_searchText, Qt::CaseInsensitive)
        || model->data(sourceIndex, TransactionListModel::AccountNameRole)
               .toString().contains(m_searchText, Qt::CaseInsensitive)
        || model->data(sourceIndex, TransactionListModel::CategoryNameRole)
               .toString().contains(m_searchText, Qt::CaseInsensitive);
}

} // namespace stukit::accounting::presentation
