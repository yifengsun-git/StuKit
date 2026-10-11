#pragma once

#include "ICategoryRepository.h"

#include <QString>
#include <QUuid>

#include <optional>
#include <variant>

namespace stukit::accounting {

struct UpdateCategoryRequest
{
    QUuid id;
    std::optional<QString> name;
    std::optional<bool> archived;
};

enum class UpdateCategoryError
{
    InvalidInput,
    NotFound,
    StorageFailure
};

using UpdateCategoryResult = std::variant<Category, UpdateCategoryError>;

class UpdateCategory final
{
public:
    explicit UpdateCategory(ICategoryRepository &repository);

    [[nodiscard]] UpdateCategoryResult execute(
        const UpdateCategoryRequest &request);

private:
    ICategoryRepository &m_repository;
};

} // namespace stukit::accounting
