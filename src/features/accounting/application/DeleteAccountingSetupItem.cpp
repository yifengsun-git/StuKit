#include "DeleteAccountingSetupItem.h"

#include <algorithm>

namespace stukit::accounting {

DeleteAccountingSetupItem::DeleteAccountingSetupItem(
    IAccountRepository &accountRepository,
    ICategoryRepository &categoryRepository,
    ITransactionRepository &transactionRepository)
    : m_accountRepository(accountRepository)
    , m_categoryRepository(categoryRepository)
    , m_transactionRepository(transactionRepository)
{
}

DeleteAccountingSetupItemError DeleteAccountingSetupItem::deleteAccount(
    const QUuid &id) const
{
    if (id.isNull()) {
        return DeleteAccountingSetupItemError::InvalidInput;
    }
    const auto account = m_accountRepository.findById(id);
    if (!account.has_value()) {
        return DeleteAccountingSetupItemError::ItemNotFound;
    }
    if (!account->isArchived()) {
        return DeleteAccountingSetupItemError::ItemNotArchived;
    }
    const auto transactions = m_transactionRepository.findAll();
    if (!transactions.has_value()) {
        return DeleteAccountingSetupItemError::StorageFailure;
    }
    if (std::ranges::any_of(
            *transactions,
            [&id](const Transaction &transaction) {
                return transaction.accountId() == id;
            })) {
        return DeleteAccountingSetupItemError::ItemInUse;
    }
    return m_accountRepository.remove(id)
        ? Success : DeleteAccountingSetupItemError::StorageFailure;
}

DeleteAccountingSetupItemError DeleteAccountingSetupItem::deleteCategory(
    const QUuid &id) const
{
    if (id.isNull()) {
        return DeleteAccountingSetupItemError::InvalidInput;
    }
    const auto category = m_categoryRepository.findById(id);
    if (!category.has_value()) {
        return DeleteAccountingSetupItemError::ItemNotFound;
    }
    if (!category->isArchived()) {
        return DeleteAccountingSetupItemError::ItemNotArchived;
    }
    const auto transactions = m_transactionRepository.findAll();
    if (!transactions.has_value()) {
        return DeleteAccountingSetupItemError::StorageFailure;
    }
    if (std::ranges::any_of(
            *transactions,
            [&id](const Transaction &transaction) {
                return transaction.categoryId() == id;
            })) {
        return DeleteAccountingSetupItemError::ItemInUse;
    }
    return m_categoryRepository.remove(id)
        ? Success : DeleteAccountingSetupItemError::StorageFailure;
}

} // namespace stukit::accounting
