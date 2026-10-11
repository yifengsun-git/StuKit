#include "AccountingOverviewViewModel.h"

#include "MoneyFormatter.h"

#include <QByteArray>
#include <QSet>
#include <QVariantMap>

#include <algorithm>
#include <utility>

namespace stukit::accounting::presentation {

AccountingOverviewViewModel::AccountingOverviewViewModel(
    GetMonthlyOverview &getMonthlyOverview,
    QTimeZone timeZone,
    QDate selectedMonth,
    QObject *parent)
    : QObject(parent)
    , m_getMonthlyOverview(getMonthlyOverview)
    , m_timeZone(std::move(timeZone))
    , m_selectedMonth(selectedMonth.year(), selectedMonth.month(), 1)
    , m_transactions()
    , m_filteredTransactions()
{
    m_transactions.setTimeZone(m_timeZone);
    m_filteredTransactions.setSourceModel(&m_transactions);
}

int AccountingOverviewViewModel::selectedYear() const noexcept
{
    return m_selectedMonth.year();
}

int AccountingOverviewViewModel::selectedMonth() const noexcept
{
    return m_selectedMonth.month();
}

QString AccountingOverviewViewModel::monthLabel() const
{
    return QStringLiteral("%1-%2")
        .arg(selectedYear(), 4, 10, QLatin1Char('0'))
        .arg(selectedMonth(), 2, 10, QLatin1Char('0'));
}

QString AccountingOverviewViewModel::timeZoneId() const
{
    return QString::fromUtf8(m_timeZone.id());
}

QStringList AccountingOverviewViewModel::availableTimeZoneIds() const
{
    QStringList ids;
    const QList<QByteArray> availableIds = QTimeZone::availableTimeZoneIds();
    ids.reserve(availableIds.size());
    for (const QByteArray &id : availableIds) {
        ids.push_back(QString::fromUtf8(id));
    }
    return ids;
}

const QTimeZone &AccountingOverviewViewModel::timeZone() const noexcept
{
    return m_timeZone;
}

const QString &AccountingOverviewViewModel::totalIncomeText() const noexcept
{
    return m_totalIncomeText;
}

const QString &AccountingOverviewViewModel::totalExpenseText() const noexcept
{
    return m_totalExpenseText;
}

const QString &AccountingOverviewViewModel::balanceText() const noexcept
{
    return m_balanceText;
}

bool AccountingOverviewViewModel::isEmpty() const noexcept
{
    return m_transactions.rowCount() == 0;
}

bool AccountingOverviewViewModel::hasError() const noexcept
{
    return !m_errorMessage.isEmpty();
}

const QString &AccountingOverviewViewModel::errorMessage() const noexcept
{
    return m_errorMessage;
}

QAbstractItemModel *AccountingOverviewViewModel::transactions() noexcept
{
    return &m_filteredTransactions;
}

int AccountingOverviewViewModel::typeFilter() const noexcept
{
    return static_cast<int>(m_filteredTransactions.typeFilter());
}

const QString &AccountingOverviewViewModel::accountIdFilter() const noexcept
{
    return m_filteredTransactions.accountIdFilter();
}

const QString &AccountingOverviewViewModel::categoryIdFilter() const noexcept
{
    return m_filteredTransactions.categoryIdFilter();
}

const QString &AccountingOverviewViewModel::searchText() const noexcept
{
    return m_filteredTransactions.searchText();
}

QVariantList AccountingOverviewViewModel::accountFilterOptions() const
{
    QVariantList options;
    options.reserve(static_cast<qsizetype>(m_accountFilterOptions.size()) + 1);
    options.push_back(QVariantMap{
        {QStringLiteral("label"), QStringLiteral("全部账户")},
        {QStringLiteral("value"), QString{}},
    });
    for (const NamedFilterOption &option : m_accountFilterOptions) {
        options.push_back(QVariantMap{
            {QStringLiteral("label"), option.name},
            {QStringLiteral("value"), option.id},
        });
    }
    return options;
}

QVariantList AccountingOverviewViewModel::categoryFilterOptions() const
{
    QVariantList options;
    options.push_back(QVariantMap{
        {QStringLiteral("label"), QStringLiteral("全部分类")},
        {QStringLiteral("value"), QString{}},
    });

    const auto filter = m_filteredTransactions.typeFilter();
    for (const CategoryFilterOption &option : m_categoryFilterOptions) {
        if ((filter == TransactionFilterProxyModel::IncomeTransactions
             && !option.income)
            || (filter == TransactionFilterProxyModel::ExpenseTransactions
                && option.income)) {
            continue;
        }

        const QString label = filter == TransactionFilterProxyModel::AllTransactions
            ? QStringLiteral("%1 · %2")
                  .arg(option.income ? QStringLiteral("收入")
                                     : QStringLiteral("支出"),
                       option.name)
            : option.name;
        options.push_back(QVariantMap{
            {QStringLiteral("label"), label},
            {QStringLiteral("value"), option.id},
        });
    }
    return options;
}

int AccountingOverviewViewModel::filteredTransactionCount() const noexcept
{
    return m_filteredTransactions.rowCount();
}

bool AccountingOverviewViewModel::hasActiveTransactionFilters() const noexcept
{
    return m_filteredTransactions.hasActiveFilters();
}

bool AccountingOverviewViewModel::noMatchingTransactions() const noexcept
{
    return !isEmpty() && m_filteredTransactions.rowCount() == 0;
}

void AccountingOverviewViewModel::setTypeFilter(int filter)
{
    if (filter < TransactionFilterProxyModel::AllTransactions
        || filter > TransactionFilterProxyModel::ExpenseTransactions) {
        return;
    }

    const auto nextFilter = static_cast<
        TransactionFilterProxyModel::TypeFilter>(filter);
    if (m_filteredTransactions.typeFilter() == nextFilter) {
        return;
    }

    m_filteredTransactions.setTypeFilter(nextFilter);
    if (!categoryMatchesType(
            m_filteredTransactions.categoryIdFilter(), nextFilter)) {
        m_filteredTransactions.setCategoryIdFilter({});
    }
    emit transactionFiltersChanged();
}

void AccountingOverviewViewModel::setAccountIdFilter(const QString &accountId)
{
    const QString normalized = accountId.trimmed();
    if (m_filteredTransactions.accountIdFilter() == normalized) {
        return;
    }

    m_filteredTransactions.setAccountIdFilter(normalized);
    emit transactionFiltersChanged();
}

void AccountingOverviewViewModel::setCategoryIdFilter(const QString &categoryId)
{
    const QString normalized = categoryId.trimmed();
    if (m_filteredTransactions.categoryIdFilter() == normalized) {
        return;
    }

    m_filteredTransactions.setCategoryIdFilter(normalized);
    emit transactionFiltersChanged();
}

void AccountingOverviewViewModel::setSearchText(const QString &searchText)
{
    const QString normalized = searchText.trimmed();
    if (m_filteredTransactions.searchText() == normalized) {
        return;
    }

    m_filteredTransactions.setSearchText(normalized);
    emit transactionFiltersChanged();
}

void AccountingOverviewViewModel::load()
{
    auto result = m_getMonthlyOverview.execute(GetMonthlyOverviewRequest{
        .year = selectedYear(),
        .month = selectedMonth(),
        .timeZone = m_timeZone,
    });

    if (std::holds_alternative<GetMonthlyOverviewError>(result)) {
        m_transactions.clear();
        rebuildFilterOptions({});
        m_totalIncomeText = QStringLiteral("0.00");
        m_totalExpenseText = QStringLiteral("0.00");
        m_balanceText = QStringLiteral("0.00");

        const auto error = std::get<GetMonthlyOverviewError>(result);
        setError(error == GetMonthlyOverviewError::InvalidInput
                     ? QStringLiteral("月份或时区无效")
                     : QStringLiteral("无法读取账单数据"));
        emit overviewChanged();
        emit transactionFiltersChanged();
        return;
    }

    auto overview = std::get<MonthlyOverview>(std::move(result));
    m_totalIncomeText = formatMoney(overview.totalIncome);
    m_totalExpenseText = formatMoney(overview.totalExpense);
    m_balanceText = formatMoney(overview.balance);
    rebuildFilterOptions(overview.transactions);
    m_transactions.replaceTransactions(std::move(overview.transactions));
    clearError();
    emit overviewChanged();
    emit transactionFiltersChanged();
}

void AccountingOverviewViewModel::showPreviousMonth()
{
    changeMonth(m_selectedMonth.addMonths(-1));
}

void AccountingOverviewViewModel::showNextMonth()
{
    changeMonth(m_selectedMonth.addMonths(1));
}

bool AccountingOverviewViewModel::selectTimeZone(const QString &timeZoneId)
{
    return applyTimeZone(QTimeZone(timeZoneId.toUtf8()));
}

bool AccountingOverviewViewModel::selectFixedOffsetMinutes(int offsetMinutes)
{
    if (offsetMinutes < -14 * 60 || offsetMinutes > 14 * 60) {
        setError(QStringLiteral("时区无效"));
        return false;
    }

    return applyTimeZone(
        QTimeZone::fromSecondsAheadOfUtc(offsetMinutes * 60));
}

void AccountingOverviewViewModel::clearTransactionFilters()
{
    if (!m_filteredTransactions.hasActiveFilters()) {
        return;
    }

    m_filteredTransactions.clearFilters();
    emit transactionFiltersChanged();
}

void AccountingOverviewViewModel::changeMonth(const QDate &month)
{
    if (!month.isValid()) {
        setError(QStringLiteral("月份或时区无效"));
        return;
    }

    m_selectedMonth = QDate(month.year(), month.month(), 1);
    emit selectedMonthChanged();
    load();
}

bool AccountingOverviewViewModel::applyTimeZone(const QTimeZone &timeZone)
{
    if (!timeZone.isValid()) {
        setError(QStringLiteral("时区无效"));
        return false;
    }

    if (m_timeZone == timeZone) {
        clearError();
        return true;
    }

    m_timeZone = timeZone;
    m_transactions.setTimeZone(m_timeZone);
    emit timeZoneChanged();
    load();
    return true;
}

void AccountingOverviewViewModel::rebuildFilterOptions(
    const std::vector<MonthlyTransactionItem> &transactions)
{
    m_accountFilterOptions.clear();
    m_categoryFilterOptions.clear();

    QSet<QString> accountIds;
    QSet<QString> categoryIds;
    for (const MonthlyTransactionItem &item : transactions) {
        const QString accountId = item.transaction.accountId().toString(
            QUuid::WithoutBraces);
        if (!accountIds.contains(accountId)) {
            accountIds.insert(accountId);
            m_accountFilterOptions.push_back(NamedFilterOption{
                .id = accountId,
                .name = item.accountName,
            });
        }

        const QString categoryId = item.transaction.categoryId().toString(
            QUuid::WithoutBraces);
        if (!categoryIds.contains(categoryId)) {
            categoryIds.insert(categoryId);
            m_categoryFilterOptions.push_back(CategoryFilterOption{
                .id = categoryId,
                .name = item.categoryName,
                .income = item.transaction.type() == TransactionType::Income,
            });
        }
    }

    const auto byNameAndId = [](const auto &left, const auto &right) {
        const int nameOrder = QString::localeAwareCompare(left.name, right.name);
        return nameOrder == 0 ? left.id < right.id : nameOrder < 0;
    };
    std::sort(m_accountFilterOptions.begin(),
              m_accountFilterOptions.end(), byNameAndId);
    std::sort(m_categoryFilterOptions.begin(),
              m_categoryFilterOptions.end(), byNameAndId);

    const QString selectedAccount = m_filteredTransactions.accountIdFilter();
    const bool hasSelectedAccount = selectedAccount.isEmpty()
        || std::any_of(
            m_accountFilterOptions.cbegin(), m_accountFilterOptions.cend(),
            [&selectedAccount](const NamedFilterOption &option) {
                return option.id == selectedAccount;
            });
    if (!hasSelectedAccount) {
        m_filteredTransactions.setAccountIdFilter({});
    }

    if (!categoryMatchesType(
            m_filteredTransactions.categoryIdFilter(),
            m_filteredTransactions.typeFilter())) {
        m_filteredTransactions.setCategoryIdFilter({});
    }
}

bool AccountingOverviewViewModel::categoryMatchesType(
    const QString &categoryId,
    TransactionFilterProxyModel::TypeFilter filter) const
{
    if (categoryId.isEmpty()) {
        return true;
    }

    const auto found = std::find_if(
        m_categoryFilterOptions.cbegin(), m_categoryFilterOptions.cend(),
        [&categoryId](const CategoryFilterOption &option) {
            return option.id == categoryId;
        });
    if (found == m_categoryFilterOptions.cend()) {
        return false;
    }

    return filter == TransactionFilterProxyModel::AllTransactions
        || (filter == TransactionFilterProxyModel::IncomeTransactions
            && found->income)
        || (filter == TransactionFilterProxyModel::ExpenseTransactions
            && !found->income);
}

void AccountingOverviewViewModel::setError(QString message)
{
    if (m_errorMessage == message) {
        return;
    }

    m_errorMessage = std::move(message);
    emit errorChanged();
}

void AccountingOverviewViewModel::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }

    m_errorMessage.clear();
    emit errorChanged();
}

} // namespace stukit::accounting::presentation
