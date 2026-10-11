#pragma once

#include "IAccountRepository.h"

#include <QString>
#include <QUuid>

#include <optional>
#include <variant>

namespace stukit::accounting {

struct UpdateAccountRequest
{
    QUuid id;
    std::optional<QString> name;
    std::optional<bool> archived;
};

enum class UpdateAccountError
{
    InvalidInput,
    NotFound,
    StorageFailure
};

using UpdateAccountResult = std::variant<Account, UpdateAccountError>;

class UpdateAccount final
{
public:
    explicit UpdateAccount(IAccountRepository &repository);

    [[nodiscard]] UpdateAccountResult execute(
        const UpdateAccountRequest &request);

private:
    IAccountRepository &m_repository;
};

} // namespace stukit::accounting
