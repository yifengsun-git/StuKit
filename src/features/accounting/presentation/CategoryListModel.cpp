#include "CategoryListModel.h"

#include <utility>

namespace stukit::accounting::presentation {

CategoryListModel::CategoryListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CategoryListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_categories.size());
}

QVariant CategoryListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= static_cast<int>(m_categories.size())) {
        return {};
    }

    const Category &category = m_categories.at(
        static_cast<std::size_t>(index.row()));
    switch (role) {
    case CategoryIdRole:
        return category.id().toString(QUuid::WithoutBraces);
    case NameRole:
        return category.name();
    case IsIncomeRole:
        return category.type() == TransactionType::Income;
    case ArchivedRole:
        return category.isArchived();
    default:
        return {};
    }
}

QHash<int, QByteArray> CategoryListModel::roleNames() const
{
    return {
        {CategoryIdRole, QByteArrayLiteral("categoryId")},
        {NameRole, QByteArrayLiteral("name")},
        {IsIncomeRole, QByteArrayLiteral("isIncome")},
        {ArchivedRole, QByteArrayLiteral("archived")},
    };
}

void CategoryListModel::replaceCategories(std::vector<Category> categories)
{
    beginResetModel();
    m_categories = std::move(categories);
    endResetModel();
}

void CategoryListModel::clear()
{
    replaceCategories({});
}

} // namespace stukit::accounting::presentation
