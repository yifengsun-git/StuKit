#include "CreateAccount.h"

#include <QUuid>

namespace stukit::accounting {

CreateAccount::CreateAccount(IAccountRepository &repository)
    : m_repository(repository)
{
}

CreateAccountResult CreateAccount::execute(const CreateAccountRequest &request)
{
    const auto account = Account::create(AccountData{
        .id = QUuid::createUuid(),
        .name = request.name,
        .openingBalance = request.openingBalance,
        .archived = false,
    });

    if (!account.has_value()) {
        return CreateAccountError::InvalidInput;
    }

    if (!m_repository.save(*account)) {
        return CreateAccountError::StorageFailure;
    }

    return *account;
}

} // namespace stukit::accounting
