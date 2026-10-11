#include "GetTransactionExportData.h"

#include <QHash>

#include <algorithm>
#include <utility>

namespace stukit::accounting {

GetTransactionExportData::GetTransactionExportData(
    ITransactionRepository &transactionRepository,
    IAccountRepository &accountRepository,
    ICategoryRepository &categoryRepository)
    : m_transactionRepository(transactionRepository)
    , m_accountRepository(accountRepository)
    , m_categoryRepository(categoryRepository)
{
}

GetTransactionExportDataResult GetTransactionExportData::execute() const
{
    auto transactions = m_transactionRepository.findAll();
    const auto accounts = m_accountRepository.findAll();
    const auto categories = m_categoryRepository.findAll();
    if (!transactions.has_value()
        || !accounts.has_value()
        || !categories.has_value()) {
        return GetTransactionExportDataError::StorageFailure;
    }

    QHash<QUuid, QString> accountNames;
    accountNames.reserve(static_cast<qsizetype>(accounts->size()));
    for (const Account &account : *accounts) {
        accountNames.insert(account.id(), account.name());
    }

    QHash<QUuid, QString> categoryNames;
    categoryNames.reserve(static_cast<qsizetype>(categories->size()));
    for (const Category &category : *categories) {
        categoryNames.insert(category.id(), category.name());
    }

    std::sort(
        transactions->begin(), transactions->end(),
        [](const Transaction &left, const Transaction &right) {
            if (left.occurredAt() != right.occurredAt()) {
                return left.occurredAt() > right.occurredAt();
            }
            return left.id().toString(QUuid::WithoutBraces)
                < right.id().toString(QUuid::WithoutBraces);
        });

    TransactionExportData data;
    data.transactions.reserve(transactions->size());
    for (Transaction &transaction : *transactions) {
        const auto accountName = accountNames.constFind(transaction.accountId());
        const auto categoryName = categoryNames.constFind(transaction.categoryId());
        if (accountName == accountNames.cend()
            || categoryName == categoryNames.cend()) {
            return GetTransactionExportDataError::StorageFailure;
        }

        data.transactions.push_back(TransactionExportItem{
            .transaction = std::move(transaction),
            .accountName = *accountName,
            .categoryName = *categoryName,
        });
    }

    return data;
}

} // namespace stukit::accounting
