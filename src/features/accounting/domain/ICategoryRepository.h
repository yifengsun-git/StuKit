#pragma once

#include "Category.h"

#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::accounting {

class ICategoryRepository
{
public:
    virtual ~ICategoryRepository() = default;

    [[nodiscard]] virtual bool save(const Category &category) = 0;
    [[nodiscard]] virtual std::optional<Category> findById(
        const QUuid &id) const = 0;
    [[nodiscard]] virtual std::optional<std::vector<Category>> findAll() const = 0;
};

} // namespace stukit::accounting
