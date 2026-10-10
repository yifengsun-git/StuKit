#pragma once

#include "ICategoryRepository.h"
#include "TransactionType.h"

#include <QString>

#include <variant>

namespace stukit::accounting {

struct CreateCategoryRequest
{
    QString name;
    TransactionType type{TransactionType::Expense};
};

enum class CreateCategoryError
{
    InvalidInput,
    StorageFailure
};

using CreateCategoryResult = std::variant<Category, CreateCategoryError>;

class CreateCategory final
{
public:
    explicit CreateCategory(ICategoryRepository &repository);

    [[nodiscard]] CreateCategoryResult execute(
        const CreateCategoryRequest &request);

private:
    ICategoryRepository &m_repository;
};

} // namespace stukit::accounting
