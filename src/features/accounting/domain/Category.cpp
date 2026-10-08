#include "Category.h"

#include <utility>

namespace stukit::accounting {

std::optional<Category> Category::create(CategoryData data)
{
    data.name = data.name.trimmed();

    if (data.id.isNull()
        || data.name.isEmpty()
        || data.name.size() > MaximumNameLength) {
        return std::nullopt;
    }

    return Category(std::move(data));
}

Category::Category(CategoryData data)
    : m_data(std::move(data))
{
}

const QUuid &Category::id() const noexcept
{
    return m_data.id;
}

const QString &Category::name() const noexcept
{
    return m_data.name;
}

TransactionType Category::type() const noexcept
{
    return m_data.type;
}

bool Category::isArchived() const noexcept
{
    return m_data.archived;
}

} // namespace stukit::accounting
