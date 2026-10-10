#pragma once

#include "IAccountRepository.h"
#include "Money.h"

#include <QString>

#include <variant>

namespace stukit::accounting {

struct CreateAccountRequest
{
    QString name;
    Money openingBalance{Money::fromCents(0)};
};

enum class CreateAccountError
{
    InvalidInput,
    StorageFailure
};

using CreateAccountResult = std::variant<Account, CreateAccountError>;

class CreateAccount final
{
public:
    explicit CreateAccount(IAccountRepository &repository);

    [[nodiscard]] CreateAccountResult execute(const CreateAccountRequest &request);

private:
    IAccountRepository &m_repository;
};

} // namespace stukit::accounting
