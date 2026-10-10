#include "GetAccountingSetup.h"

#include <utility>

namespace stukit::accounting {

GetAccountingSetup::GetAccountingSetup(
    IAccountRepository &accountRepository,
    ICategoryRepository &categoryRepository)
    : m_accountRepository(accountRepository)
    , m_categoryRepository(categoryRepository)
{
}

GetAccountingSetupResult GetAccountingSetup::execute() const
{
    auto accounts = m_accountRepository.findAll();
    if (!accounts.has_value()) {
        return GetAccountingSetupError::StorageFailure;
    }

    auto categories = m_categoryRepository.findAll();
    if (!categories.has_value()) {
        return GetAccountingSetupError::StorageFailure;
    }

    return AccountingSetup{
        .accounts = std::move(*accounts),
        .categories = std::move(*categories),
    };
}

} // namespace stukit::accounting
