#pragma once

#include "Category.h"

#include <QAbstractListModel>

#include <vector>

namespace stukit::accounting::presentation {

class CategoryListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        CategoryIdRole = Qt::UserRole + 1,
        NameRole,
        IsIncomeRole,
        ArchivedRole
    };
    Q_ENUM(Role)

    explicit CategoryListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void replaceCategories(std::vector<Category> categories);
    void clear();

private:
    std::vector<Category> m_categories;
};

} // namespace stukit::accounting::presentation
