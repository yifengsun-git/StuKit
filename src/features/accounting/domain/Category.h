#pragma once

#include "TransactionType.h"

#include <QString>
#include <QUuid>

#include <optional>

namespace stukit::accounting {

struct CategoryData
{
    QUuid id;
    QString name;
    TransactionType type{TransactionType::Expense};
    bool archived{false};
};

class Category final
{
public:
    static constexpr qsizetype MaximumNameLength = 40;

    static std::optional<Category> create(CategoryData data);

    [[nodiscard]] const QUuid &id() const noexcept;
    [[nodiscard]] const QString &name() const noexcept;
    [[nodiscard]] TransactionType type() const noexcept;
    [[nodiscard]] bool isArchived() const noexcept;

private:
    explicit Category(CategoryData data);

    CategoryData m_data;
};

} // namespace stukit::accounting
