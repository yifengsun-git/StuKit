#include "TransferFunds.h"

namespace stukit::accounting {
namespace {

QString transferNote(const QString &prefix, const QString &note)
{
    const QString normalized = note.trimmed();
    return normalized.isEmpty()
        ? prefix
        : QStringLiteral("%1 · %2").arg(prefix, normalized);
}

} // namespace

TransferFunds::TransferFunds(
    ITransactionRepository &transactionRepository,
    IAccountRepository &accountRepository,
    ICategoryRepository &categoryRepository)
    : m_transactionRepository(transactionRepository)
    , m_accountRepository(accountRepository)
    , m_categoryRepository(categoryRepository)
{
}

TransferFundsResult TransferFunds::execute(
    const TransferFundsRequest &request)
{
    if (!request.amount.isPositive()
        || request.fee.isNegative()
        || !request.occurredAt.isValid()
        || request.sourceAccountId.isNull()
        || request.destinationAccountId.isNull()) {
        return TransferFundsError::InvalidInput;
    }
    if (request.sourceAccountId == request.destinationAccountId) {
        return TransferFundsError::SameAccount;
    }

    const auto source = m_accountRepository.findById(request.sourceAccountId);
    const auto destination = m_accountRepository.findById(
        request.destinationAccountId);
    if (!source.has_value() || !destination.has_value()) {
        return TransferFundsError::AccountNotFound;
    }
    if (source->isArchived() || destination->isArchived()) {
        return TransferFundsError::AccountArchived;
    }

    const auto outgoingCategory = m_categoryRepository.findById(
        request.outgoingCategoryId);
    const auto incomingCategory = m_categoryRepository.findById(
        request.incomingCategoryId);
    const auto feeCategory = request.fee.isZero()
        ? std::optional<Category>{}
        : m_categoryRepository.findById(request.feeCategoryId);
    if (!outgoingCategory.has_value()
        || !incomingCategory.has_value()
        || (!request.fee.isZero() && !feeCategory.has_value())) {
        return TransferFundsError::CategoryNotFound;
    }
    if (outgoingCategory->isArchived()
        || incomingCategory->isArchived()
        || (feeCategory.has_value() && feeCategory->isArchived())) {
        return TransferFundsError::CategoryArchived;
    }
    if (outgoingCategory->type() != TransactionType::Expense
        || incomingCategory->type() != TransactionType::Income
        || (feeCategory.has_value()
            && feeCategory->type() != TransactionType::Expense)) {
        return TransferFundsError::CategoryTypeMismatch;
    }

    const auto outgoing = Transaction::create(TransactionData{
        .id = QUuid::createUuid(),
        .type = TransactionType::Expense,
        .amount = request.amount,
        .accountId = source->id(),
        .categoryId = outgoingCategory->id(),
        .occurredAt = request.occurredAt,
        .note = transferNote(
            QStringLiteral("转账至 %1").arg(destination->name()),
            request.note),
    });
    const auto incoming = Transaction::create(TransactionData{
        .id = QUuid::createUuid(),
        .type = TransactionType::Income,
        .amount = request.amount,
        .accountId = destination->id(),
        .categoryId = incomingCategory->id(),
        .occurredAt = request.occurredAt,
        .note = transferNote(
            QStringLiteral("来自 %1 的转账").arg(source->name()),
            request.note),
    });
    if (!outgoing.has_value() || !incoming.has_value()) {
        return TransferFundsError::InvalidInput;
    }

    std::vector<Transaction> transactions{*outgoing, *incoming};
    if (!request.fee.isZero()) {
        const auto fee = Transaction::create(TransactionData{
            .id = QUuid::createUuid(),
            .type = TransactionType::Expense,
            .amount = request.fee,
            .accountId = source->id(),
            .categoryId = feeCategory->id(),
            .occurredAt = request.occurredAt,
            .note = transferNote(QStringLiteral("转账手续费"), request.note),
        });
        if (!fee.has_value()) {
            return TransferFundsError::InvalidInput;
        }
        transactions.push_back(*fee);
    }

    if (!m_transactionRepository.saveAll(transactions)) {
        return TransferFundsError::StorageFailure;
    }
    return transactions;
}

} // namespace stukit::accounting
