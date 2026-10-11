#include "GetAccountingAnalytics.h"

#include <QHash>
#include <QSet>

#include <algorithm>
#include <limits>
#include <optional>
#include <utility>

namespace stukit::accounting {
namespace {

struct CategoryInfo
{
    QString name;
    TransactionType type{TransactionType::Expense};
    bool archived{false};
};

struct AccountInfo
{
    QString name;
    bool archived{false};
};

struct AmountAccumulator
{
    qint64 income{0};
    qint64 expense{0};
    int transactionCount{0};
};

std::optional<qint64> checkedAdd(qint64 left, qint64 right)
{
    if (right > 0
        && left > std::numeric_limits<qint64>::max() - right) {
        return std::nullopt;
    }
    if (right < 0
        && left < std::numeric_limits<qint64>::min() - right) {
        return std::nullopt;
    }
    return left + right;
}

bool containsNullId(const std::vector<QUuid> &ids)
{
    return std::any_of(ids.cbegin(), ids.cend(), [](const QUuid &id) {
        return id.isNull();
    });
}

QSet<QUuid> toSet(const std::vector<QUuid> &ids)
{
    QSet<QUuid> result;
    result.reserve(static_cast<qsizetype>(ids.size()));
    for (const QUuid &id : ids) {
        result.insert(id);
    }
    return result;
}

bool isInFilter(const QSet<QUuid> &filter, const QUuid &id)
{
    return filter.isEmpty() || filter.contains(id);
}

int monthIndex(const QDate &firstMonth, const QDate &date)
{
    return (date.year() - firstMonth.year()) * 12
        + date.month() - firstMonth.month();
}

} // namespace

GetAccountingAnalytics::GetAccountingAnalytics(
    ITransactionRepository &transactionRepository,
    IAccountRepository &accountRepository,
    ICategoryRepository &categoryRepository)
    : m_transactionRepository(transactionRepository)
    , m_accountRepository(accountRepository)
    , m_categoryRepository(categoryRepository)
{
}

GetAccountingAnalyticsResult GetAccountingAnalytics::execute(
    const GetAccountingAnalyticsRequest &request) const
{
    if (!request.selectedMonth.isValid()
        || request.monthCount < 2
        || request.monthCount > 24
        || !request.timeZone.isValid()
        || containsNullId(request.accountIds)
        || containsNullId(request.categoryIds)) {
        return GetAccountingAnalyticsError::InvalidInput;
    }

    const QDate selectedMonth(
        request.selectedMonth.year(), request.selectedMonth.month(), 1);
    const QDate firstMonth = selectedMonth.addMonths(1 - request.monthCount);
    const QDate monthAfterSelection = selectedMonth.addMonths(1);
    const QDateTime startInclusive = firstMonth.startOfDay(request.timeZone);
    const QDateTime endExclusive = monthAfterSelection.startOfDay(
        request.timeZone);
    if (!startInclusive.isValid()
        || !endExclusive.isValid()
        || startInclusive >= endExclusive) {
        return GetAccountingAnalyticsError::InvalidInput;
    }

    const auto transactions = m_transactionRepository.findOccurredBetween(
        startInclusive.toUTC(), endExclusive.toUTC());
    const auto accounts = m_accountRepository.findAll();
    const auto categories = m_categoryRepository.findAll();
    if (!transactions.has_value()
        || !accounts.has_value()
        || !categories.has_value()) {
        return GetAccountingAnalyticsError::StorageFailure;
    }

    QHash<QUuid, AccountInfo> accountInfo;
    accountInfo.reserve(static_cast<qsizetype>(accounts->size()));
    for (const Account &account : *accounts) {
        accountInfo.insert(account.id(), {
            .name = account.name(),
            .archived = account.isArchived(),
        });
    }

    QHash<QUuid, CategoryInfo> categoryInfo;
    categoryInfo.reserve(static_cast<qsizetype>(categories->size()));
    for (const Category &category : *categories) {
        categoryInfo.insert(category.id(), {
            .name = category.name(),
            .type = category.type(),
            .archived = category.isArchived(),
        });
    }

    const QSet<QUuid> accountFilter = toSet(request.accountIds);
    const QSet<QUuid> categoryFilter = toSet(request.categoryIds);
    for (const QUuid &id : accountFilter) {
        if (!accountInfo.contains(id)) {
            return GetAccountingAnalyticsError::InvalidInput;
        }
    }
    for (const QUuid &id : categoryFilter) {
        if (!categoryInfo.contains(id)) {
            return GetAccountingAnalyticsError::InvalidInput;
        }
    }

    AccountingAnalytics analytics;
    analytics.months.reserve(static_cast<std::size_t>(request.monthCount));
    for (int index = 0; index < request.monthCount; ++index) {
        analytics.months.push_back({
            .month = firstMonth.addMonths(index),
        });
    }

    QHash<QUuid, AmountAccumulator> categoryAmounts;
    QHash<QUuid, AmountAccumulator> accountAmounts;
    for (const Transaction &transaction : *transactions) {
        const auto account = accountInfo.constFind(transaction.accountId());
        const auto category = categoryInfo.constFind(transaction.categoryId());
        if (account == accountInfo.cend()
            || category == categoryInfo.cend()
            || category->type != transaction.type()) {
            return GetAccountingAnalyticsError::StorageFailure;
        }
        if (!isInFilter(accountFilter, transaction.accountId())
            || !isInFilter(categoryFilter, transaction.categoryId())) {
            continue;
        }

        const QDate localDate = transaction.occurredAt()
                                    .toTimeZone(request.timeZone)
                                    .date();
        const int index = monthIndex(firstMonth, localDate);
        if (index < 0 || index >= request.monthCount) {
            return GetAccountingAnalyticsError::StorageFailure;
        }

        MonthlyAnalyticsPoint &point = analytics.months.at(
            static_cast<std::size_t>(index));
        const qint64 cents = transaction.amount().cents();
        const bool income = transaction.type() == TransactionType::Income;
        const qint64 current = income
            ? point.income.cents()
            : point.expense.cents();
        const auto updated = checkedAdd(current, cents);
        if (!updated.has_value()) {
            return GetAccountingAnalyticsError::ArithmeticOverflow;
        }
        if (income) {
            point.income = Money::fromCents(*updated);
        } else {
            point.expense = Money::fromCents(*updated);
        }

        if (localDate.year() != selectedMonth.year()
            || localDate.month() != selectedMonth.month()) {
            continue;
        }

        AmountAccumulator &categoryAmount = categoryAmounts[
            transaction.categoryId()];
        qint64 &categoryCents = income
            ? categoryAmount.income
            : categoryAmount.expense;
        const auto updatedCategory = checkedAdd(categoryCents, cents);
        if (!updatedCategory.has_value()) {
            return GetAccountingAnalyticsError::ArithmeticOverflow;
        }
        categoryCents = *updatedCategory;
        ++categoryAmount.transactionCount;

        AmountAccumulator &accountAmount = accountAmounts[
            transaction.accountId()];
        qint64 &accountCents = income
            ? accountAmount.income
            : accountAmount.expense;
        const auto updatedAccount = checkedAdd(accountCents, cents);
        if (!updatedAccount.has_value()) {
            return GetAccountingAnalyticsError::ArithmeticOverflow;
        }
        accountCents = *updatedAccount;
        ++accountAmount.transactionCount;
    }

    for (MonthlyAnalyticsPoint &point : analytics.months) {
        point.balance = Money::fromCents(
            point.income.cents() - point.expense.cents());
    }

    for (auto iterator = categoryAmounts.cbegin();
         iterator != categoryAmounts.cend(); ++iterator) {
        const CategoryInfo &info = categoryInfo.value(iterator.key());
        const bool income = info.type == TransactionType::Income;
        CategoryAnalyticsItem item{
            .categoryId = iterator.key(),
            .categoryName = info.name,
            .type = info.type,
            .amount = Money::fromCents(
                income ? iterator->income : iterator->expense),
            .transactionCount = iterator->transactionCount,
            .archived = info.archived,
        };
        (income ? analytics.incomeCategories : analytics.expenseCategories)
            .push_back(std::move(item));
    }

    const auto categoryOrder = [](
        const CategoryAnalyticsItem &left,
        const CategoryAnalyticsItem &right) {
        if (left.amount != right.amount) {
            return left.amount.cents() > right.amount.cents();
        }
        if (left.categoryName != right.categoryName) {
            return left.categoryName < right.categoryName;
        }
        return left.categoryId.toString(QUuid::WithoutBraces)
            < right.categoryId.toString(QUuid::WithoutBraces);
    };
    std::sort(analytics.incomeCategories.begin(),
              analytics.incomeCategories.end(), categoryOrder);
    std::sort(analytics.expenseCategories.begin(),
              analytics.expenseCategories.end(), categoryOrder);

    analytics.accountFlows.reserve(accountAmounts.size());
    for (auto iterator = accountAmounts.cbegin();
         iterator != accountAmounts.cend(); ++iterator) {
        const AccountInfo &info = accountInfo.value(iterator.key());
        analytics.accountFlows.push_back({
            .accountId = iterator.key(),
            .accountName = info.name,
            .income = Money::fromCents(iterator->income),
            .expense = Money::fromCents(iterator->expense),
            .netFlow = Money::fromCents(
                iterator->income - iterator->expense),
            .transactionCount = iterator->transactionCount,
            .archived = info.archived,
        });
    }
    std::sort(
        analytics.accountFlows.begin(), analytics.accountFlows.end(),
        [](const AccountAnalyticsItem &left, const AccountAnalyticsItem &right) {
            if (left.expense != right.expense) {
                return left.expense.cents() > right.expense.cents();
            }
            if (left.income != right.income) {
                return left.income.cents() > right.income.cents();
            }
            if (left.accountName != right.accountName) {
                return left.accountName < right.accountName;
            }
            return left.accountId.toString(QUuid::WithoutBraces)
                < right.accountId.toString(QUuid::WithoutBraces);
        });

    return analytics;
}

} // namespace stukit::accounting
