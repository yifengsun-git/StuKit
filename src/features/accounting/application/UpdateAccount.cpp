#include "UpdateAccount.h"

namespace stukit::accounting {

UpdateAccount::UpdateAccount(IAccountRepository &repository)
    : m_repository(repository)
{
}

UpdateAccountResult UpdateAccount::execute(const UpdateAccountRequest &request)
{
    if (request.id.isNull()
        || (!request.name.has_value() && !request.archived.has_value())) {
        return UpdateAccountError::InvalidInput;
    }

    const auto existing = m_repository.findById(request.id);
    if (!existing.has_value()) {
        return UpdateAccountError::NotFound;
    }

    const auto updated = Account::create(AccountData{
        .id = existing->id(),
        .name = request.name.value_or(existing->name()),
        .openingBalance = existing->openingBalance(),
        .archived = request.archived.value_or(existing->isArchived()),
    });
    if (!updated.has_value()) {
        return UpdateAccountError::InvalidInput;
    }

    if (!m_repository.save(*updated)) {
        return UpdateAccountError::StorageFailure;
    }

    return *updated;
}

} // namespace stukit::accounting
