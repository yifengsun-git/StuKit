#include "UpdateCategory.h"

namespace stukit::accounting {

UpdateCategory::UpdateCategory(ICategoryRepository &repository)
    : m_repository(repository)
{
}

UpdateCategoryResult UpdateCategory::execute(
    const UpdateCategoryRequest &request)
{
    if (request.id.isNull()
        || (!request.name.has_value() && !request.archived.has_value())) {
        return UpdateCategoryError::InvalidInput;
    }

    const auto existing = m_repository.findById(request.id);
    if (!existing.has_value()) {
        return UpdateCategoryError::NotFound;
    }

    const auto updated = Category::create(CategoryData{
        .id = existing->id(),
        .name = request.name.value_or(existing->name()),
        .type = existing->type(),
        .archived = request.archived.value_or(existing->isArchived()),
    });
    if (!updated.has_value()) {
        return UpdateCategoryError::InvalidInput;
    }

    if (!m_repository.save(*updated)) {
        return UpdateCategoryError::StorageFailure;
    }

    return *updated;
}

} // namespace stukit::accounting
