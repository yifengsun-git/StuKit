#pragma once

#include "IAccountRepository.h"
#include "ICategoryRepository.h"

#include <variant>
#include <vector>

namespace stukit::accounting {

struct AccountingSetup
{
    std::vector<Account> accounts;
    std::vector<Category> categories;
};

enum class GetAccountingSetupError
{
    StorageFailure
};

using GetAccountingSetupResult =
    std::variant<AccountingSetup, GetAccountingSetupError>;

class GetAccountingSetup final
{
public:
    GetAccountingSetup(
        IAccountRepository &accountRepository,
        ICategoryRepository &categoryRepository);

    [[nodiscard]] GetAccountingSetupResult execute() const;

private:
    IAccountRepository &m_accountRepository;
    ICategoryRepository &m_categoryRepository;
};

} // namespace stukit::accounting
