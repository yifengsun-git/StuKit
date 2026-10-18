#pragma once

#include "ICategoryRepository.h"

#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

namespace stukit::accounting {

class SqliteCategoryRepository final : public ICategoryRepository
{
public:
    explicit SqliteCategoryRepository(QSqlDatabase database);

    [[nodiscard]] bool save(const Category &category) override;
    [[nodiscard]] bool remove(const QUuid &id) override;
    [[nodiscard]] std::optional<Category> findById(const QUuid &id) const override;
    [[nodiscard]] std::optional<std::vector<Category>> findAll() const override;

    [[nodiscard]] const QString &lastError() const noexcept;

private:
    [[nodiscard]] std::optional<Category>
    categoryFromCurrentRow(const QSqlQuery &query) const;

    QSqlDatabase m_database;
    mutable QString m_lastError;
};

} // namespace stukit::accounting
