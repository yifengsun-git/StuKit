#pragma once

#include "IAccountRepository.h"
#include "ICategoryRepository.h"
#include "ITransactionRepository.h"

#include <QUuid>

namespace stukit::accounting {

enum class DeleteAccountingSetupItemError
{
    None,
    InvalidInput,
    ItemNotFound,
    ItemNotArchived,
    ItemInUse,
    StorageFailure,
};

class DeleteAccountingSetupItem final
{
public:
    DeleteAccountingSetupItem(
        IAccountRepository &accountRepository,
        ICategoryRepository &categoryRepository,
        ITransactionRepository &transactionRepository);

    [[nodiscard]] DeleteAccountingSetupItemError deleteAccount(
        const QUuid &id) const;
    [[nodiscard]] DeleteAccountingSetupItemError deleteCategory(
        const QUuid &id) const;

    static constexpr DeleteAccountingSetupItemError Success =
        DeleteAccountingSetupItemError::None;

private:
    IAccountRepository &m_accountRepository;
    ICategoryRepository &m_categoryRepository;
    ITransactionRepository &m_transactionRepository;
};

} // namespace stukit::accounting
