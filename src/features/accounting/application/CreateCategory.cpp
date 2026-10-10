#include "CreateCategory.h"

#include <QUuid>

namespace stukit::accounting {

CreateCategory::CreateCategory(ICategoryRepository &repository)
    : m_repository(repository)
{
}

CreateCategoryResult CreateCategory::execute(
    const CreateCategoryRequest &request)
{
    const auto category = Category::create(CategoryData{
        .id = QUuid::createUuid(),
        .name = request.name,
        .type = request.type,
        .archived = false,
    });

    if (!category.has_value()) {
        return CreateCategoryError::InvalidInput;
    }

    if (!m_repository.save(*category)) {
        return CreateCategoryError::StorageFailure;
    }

    return *category;
}

} // namespace stukit::accounting
