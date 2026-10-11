#pragma once

#include "IAccountRepository.h"
#include "ICategoryRepository.h"
#include "ITransactionRepository.h"

#include <variant>
#include <vector>

namespace stukit::accounting {

struct TransactionExportItem
{
    Transaction transaction;
    QString accountName;
    QString categoryName;
};

struct TransactionExportData
{
    std::vector<TransactionExportItem> transactions;
};

enum class GetTransactionExportDataError
{
    StorageFailure
};

using GetTransactionExportDataResult = std::variant<
    TransactionExportData,
    GetTransactionExportDataError>;

class GetTransactionExportData final
{
public:
    GetTransactionExportData(
        ITransactionRepository &transactionRepository,
        IAccountRepository &accountRepository,
        ICategoryRepository &categoryRepository);

    [[nodiscard]] GetTransactionExportDataResult execute() const;

private:
    ITransactionRepository &m_transactionRepository;
    IAccountRepository &m_accountRepository;
    ICategoryRepository &m_categoryRepository;
};

} // namespace stukit::accounting
