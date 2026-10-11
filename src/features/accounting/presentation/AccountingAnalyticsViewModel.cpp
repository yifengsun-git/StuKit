#include "AccountingAnalyticsViewModel.h"

#include "MoneyFormatter.h"

#include <QVariantMap>

#include <algorithm>
#include <limits>
#include <optional>
#include <utility>
#include <variant>

namespace stukit::accounting::presentation {
namespace {

QString idText(const QUuid &id)
{
    return id.toString(QUuid::WithoutBraces);
}

QStringList sortedIdTexts(const QSet<QUuid> &ids)
{
    QStringList result;
    result.reserve(ids.size());
    for (const QUuid &id : ids) {
        result.push_back(idText(id));
    }
    result.sort();
    return result;
}

std::vector<QUuid> sortedIds(const QSet<QUuid> &ids)
{
    std::vector<QUuid> result(ids.cbegin(), ids.cend());
    std::sort(result.begin(), result.end(), [](const QUuid &left,
                                               const QUuid &right) {
        return idText(left) < idText(right);
    });
    return result;
}

std::optional<qint64> checkedDifference(qint64 current, qint64 previous)
{
    if (previous < 0
        && current > std::numeric_limits<qint64>::max() + previous) {
        return std::nullopt;
    }
    if (previous > 0
        && current < std::numeric_limits<qint64>::min() + previous) {
        return std::nullopt;
    }
    return current - previous;
}

QString formatChange(Money current, Money previous)
{
    const auto difference = checkedDifference(
        current.cents(), previous.cents());
    if (!difference.has_value()) {
        return QStringLiteral("超出范围");
    }

    const QString formatted = formatMoney(Money::fromCents(*difference));
    return *difference > 0 ? QStringLiteral("+%1").arg(formatted) : formatted;
}

analytics::presentation::ChartSeries makeSeries(
    QString id,
    QString title,
    QString color,
    const std::vector<MonthlyAnalyticsPoint> &months,
    Money MonthlyAnalyticsPoint::*member)
{
    analytics::presentation::ChartSeries series{
        .seriesId = std::move(id),
        .title = std::move(title),
        .color = std::move(color),
    };
    series.points.reserve(months.size());
    for (const MonthlyAnalyticsPoint &month : months) {
        const Money amount = month.*member;
        series.points.push_back({
            .label = month.month.toString(QStringLiteral("yyyy-MM")),
            .value = static_cast<double>(amount.cents()) / 100.0,
            .displayValue = formatMoney(amount),
        });
    }
    return series;
}

} // namespace

AccountingAnalyticsViewModel::AccountingAnalyticsViewModel(
    GetAccountingAnalytics &getAccountingAnalytics,
    GetAccountingSetup &getAccountingSetup,
    QTimeZone timeZone,
    QDate selectedMonth,
    QObject *parent)
    : QObject(parent)
    , m_getAccountingAnalytics(getAccountingAnalytics)
    , m_getAccountingSetup(getAccountingSetup)
    , m_timeZone(std::move(timeZone))
    , m_defaultSelectedMonth(
          selectedMonth.year(), selectedMonth.month(), 1)
    , m_selectedMonth(m_defaultSelectedMonth)
    , m_months(this)
    , m_incomeCategories(this)
    , m_expenseCategories(this)
    , m_accountFlows(this)
    , m_trendSeries(this)
{
}

int AccountingAnalyticsViewModel::selectedYear() const noexcept
{
    return m_selectedMonth.year();
}

int AccountingAnalyticsViewModel::selectedMonth() const noexcept
{
    return m_selectedMonth.month();
}

QString AccountingAnalyticsViewModel::monthLabel() const
{
    return m_selectedMonth.toString(QStringLiteral("yyyy-MM"));
}

int AccountingAnalyticsViewModel::monthCount() const noexcept
{
    return m_monthCount;
}

QString AccountingAnalyticsViewModel::timeZoneId() const
{
    return QString::fromUtf8(m_timeZone.id());
}

QStringList AccountingAnalyticsViewModel::selectedAccountIds() const
{
    return sortedIdTexts(m_selectedAccountIds);
}

QStringList AccountingAnalyticsViewModel::selectedCategoryIds() const
{
    return sortedIdTexts(m_selectedCategoryIds);
}

const QVariantList &AccountingAnalyticsViewModel::accountFilterOptions() const noexcept
{
    return m_accountFilterOptions;
}

const QVariantList &AccountingAnalyticsViewModel::categoryFilterOptions() const noexcept
{
    return m_categoryFilterOptions;
}

QAbstractItemModel *AccountingAnalyticsViewModel::months() noexcept
{
    return &m_months;
}

QAbstractItemModel *AccountingAnalyticsViewModel::incomeCategories() noexcept
{
    return &m_incomeCategories;
}

QAbstractItemModel *AccountingAnalyticsViewModel::expenseCategories() noexcept
{
    return &m_expenseCategories;
}

QAbstractItemModel *AccountingAnalyticsViewModel::accountFlows() noexcept
{
    return &m_accountFlows;
}

QAbstractItemModel *AccountingAnalyticsViewModel::trendSeries() noexcept
{
    return &m_trendSeries;
}

const QString &AccountingAnalyticsViewModel::selectedIncomeText() const noexcept
{
    return m_selectedIncomeText;
}

const QString &AccountingAnalyticsViewModel::selectedExpenseText() const noexcept
{
    return m_selectedExpenseText;
}

const QString &AccountingAnalyticsViewModel::selectedBalanceText() const noexcept
{
    return m_selectedBalanceText;
}

const QString &AccountingAnalyticsViewModel::incomeChangeText() const noexcept
{
    return m_incomeChangeText;
}

const QString &AccountingAnalyticsViewModel::expenseChangeText() const noexcept
{
    return m_expenseChangeText;
}

const QString &AccountingAnalyticsViewModel::balanceChangeText() const noexcept
{
    return m_balanceChangeText;
}

bool AccountingAnalyticsViewModel::hasData() const noexcept
{
    return m_hasData;
}

bool AccountingAnalyticsViewModel::isLoading() const noexcept
{
    return m_loading;
}

bool AccountingAnalyticsViewModel::hasError() const noexcept
{
    return !m_errorMessage.isEmpty();
}

const QString &AccountingAnalyticsViewModel::errorMessage() const noexcept
{
    return m_errorMessage;
}

const QTimeZone &AccountingAnalyticsViewModel::timeZone() const noexcept
{
    return m_timeZone;
}

void AccountingAnalyticsViewModel::setMonthCount(int count)
{
    if (count < 2 || count > 24) {
        setError(QStringLiteral("分析条件无效，请恢复默认条件"));
        return;
    }
    if (m_monthCount == count) {
        return;
    }

    m_monthCount = count;
    emit filtersChanged();
    load();
}

void AccountingAnalyticsViewModel::setTimeZone(const QTimeZone &timeZone)
{
    if (!timeZone.isValid()) {
        setError(QStringLiteral("分析条件无效，请恢复默认条件"));
        return;
    }
    if (m_timeZone == timeZone) {
        return;
    }

    m_timeZone = timeZone;
    emit timeZoneChanged();
    load();
}

void AccountingAnalyticsViewModel::load()
{
    setLoading(true);

    auto setupResult = m_getAccountingSetup.execute();
    if (!std::holds_alternative<AccountingSetup>(setupResult)) {
        m_availableAccountIds.clear();
        m_availableCategoryIds.clear();
        m_accountFilterOptions.clear();
        m_categoryFilterOptions.clear();
        clearAnalytics();
        setError(QStringLiteral("暂时无法读取记账数据"));
        emit filtersChanged();
        setLoading(false);
        return;
    }

    const AccountingSetup &setup = std::get<AccountingSetup>(setupResult);
    rebuildFilterOptions(setup);

    auto result = m_getAccountingAnalytics.execute({
        .selectedMonth = m_selectedMonth,
        .monthCount = m_monthCount,
        .timeZone = m_timeZone,
        .accountIds = sortedIds(m_selectedAccountIds),
        .categoryIds = sortedIds(m_selectedCategoryIds),
    });
    if (std::holds_alternative<GetAccountingAnalyticsError>(result)) {
        clearAnalytics();
        switch (std::get<GetAccountingAnalyticsError>(result)) {
        case GetAccountingAnalyticsError::InvalidInput:
            setError(QStringLiteral("分析条件无效，请恢复默认条件"));
            break;
        case GetAccountingAnalyticsError::StorageFailure:
            setError(QStringLiteral("暂时无法读取记账数据"));
            break;
        case GetAccountingAnalyticsError::ArithmeticOverflow:
            setError(QStringLiteral("金额过大，无法安全完成统计"));
            break;
        }
        setLoading(false);
        return;
    }

    applyAnalytics(std::get<AccountingAnalytics>(std::move(result)));
    clearError();
    setLoading(false);
}

void AccountingAnalyticsViewModel::showPreviousMonth()
{
    changeMonth(m_selectedMonth.addMonths(-1));
}

void AccountingAnalyticsViewModel::showNextMonth()
{
    changeMonth(m_selectedMonth.addMonths(1));
}

bool AccountingAnalyticsViewModel::setAccountSelected(
    const QString &accountId, bool selected)
{
    const QUuid id(accountId.trimmed());
    if (id.isNull() || !m_availableAccountIds.contains(id)) {
        setError(QStringLiteral("分析条件无效，请恢复默认条件"));
        return false;
    }

    bool changed = false;
    if (selected && !m_selectedAccountIds.contains(id)) {
        m_selectedAccountIds.insert(id);
        changed = true;
    } else if (!selected) {
        changed = m_selectedAccountIds.remove(id);
    }
    if (changed) {
        emit filtersChanged();
        load();
    }
    return true;
}

bool AccountingAnalyticsViewModel::setCategorySelected(
    const QString &categoryId, bool selected)
{
    const QUuid id(categoryId.trimmed());
    if (id.isNull() || !m_availableCategoryIds.contains(id)) {
        setError(QStringLiteral("分析条件无效，请恢复默认条件"));
        return false;
    }

    bool changed = false;
    if (selected && !m_selectedCategoryIds.contains(id)) {
        m_selectedCategoryIds.insert(id);
        changed = true;
    } else if (!selected) {
        changed = m_selectedCategoryIds.remove(id);
    }
    if (changed) {
        emit filtersChanged();
        load();
    }
    return true;
}

void AccountingAnalyticsViewModel::resetFilters()
{
    const bool monthChanged = m_selectedMonth != m_defaultSelectedMonth;
    const bool changed = monthChanged
        || m_monthCount != 12
        || !m_selectedAccountIds.isEmpty()
        || !m_selectedCategoryIds.isEmpty();
    if (!changed) {
        clearError();
        return;
    }

    m_selectedMonth = m_defaultSelectedMonth;
    m_monthCount = 12;
    m_selectedAccountIds.clear();
    m_selectedCategoryIds.clear();
    if (monthChanged) {
        emit selectedMonthChanged();
    }
    emit filtersChanged();
    load();
}

void AccountingAnalyticsViewModel::changeMonth(const QDate &month)
{
    if (!month.isValid()) {
        setError(QStringLiteral("分析条件无效，请恢复默认条件"));
        return;
    }

    m_selectedMonth = QDate(month.year(), month.month(), 1);
    emit selectedMonthChanged();
    load();
}

void AccountingAnalyticsViewModel::rebuildFilterOptions(
    const AccountingSetup &setup)
{
    m_availableAccountIds.clear();
    m_availableCategoryIds.clear();
    m_accountFilterOptions.clear();
    m_categoryFilterOptions.clear();

    for (const Account &account : setup.accounts) {
        m_availableAccountIds.insert(account.id());
        m_accountFilterOptions.push_back(QVariantMap{
            {QStringLiteral("label"), account.name()},
            {QStringLiteral("value"), idText(account.id())},
            {QStringLiteral("archived"), account.isArchived()},
            {QStringLiteral("selected"),
             m_selectedAccountIds.contains(account.id())},
        });
    }
    for (const Category &category : setup.categories) {
        m_availableCategoryIds.insert(category.id());
        m_categoryFilterOptions.push_back(QVariantMap{
            {QStringLiteral("label"), category.name()},
            {QStringLiteral("value"), idText(category.id())},
            {QStringLiteral("isIncome"),
             category.type() == TransactionType::Income},
            {QStringLiteral("archived"), category.isArchived()},
            {QStringLiteral("selected"),
             m_selectedCategoryIds.contains(category.id())},
        });
    }

    bool selectionChanged = false;
    for (auto iterator = m_selectedAccountIds.begin();
         iterator != m_selectedAccountIds.end();) {
        if (!m_availableAccountIds.contains(*iterator)) {
            iterator = m_selectedAccountIds.erase(iterator);
            selectionChanged = true;
        } else {
            ++iterator;
        }
    }
    for (auto iterator = m_selectedCategoryIds.begin();
         iterator != m_selectedCategoryIds.end();) {
        if (!m_availableCategoryIds.contains(*iterator)) {
            iterator = m_selectedCategoryIds.erase(iterator);
            selectionChanged = true;
        } else {
            ++iterator;
        }
    }
    if (selectionChanged) {
        emit filtersChanged();
    }
}

void AccountingAnalyticsViewModel::applyAnalytics(
    AccountingAnalytics analytics)
{
    const MonthlyAnalyticsPoint &selected = analytics.months.back();
    const MonthlyAnalyticsPoint &previous = analytics.months.at(
        analytics.months.size() - 2);

    m_selectedIncomeText = formatMoney(selected.income);
    m_selectedExpenseText = formatMoney(selected.expense);
    m_selectedBalanceText = formatMoney(selected.balance);
    m_incomeChangeText = formatChange(selected.income, previous.income);
    m_expenseChangeText = formatChange(selected.expense, previous.expense);
    m_balanceChangeText = formatChange(selected.balance, previous.balance);
    m_hasData = std::any_of(
        analytics.months.cbegin(), analytics.months.cend(),
        [](const MonthlyAnalyticsPoint &point) {
            return !point.income.isZero() || !point.expense.isZero();
        });

    std::vector<analytics::presentation::ChartSeries> series;
    series.reserve(3);
    series.push_back(makeSeries(
        QStringLiteral("income"), QStringLiteral("收入"),
        QStringLiteral("#2E7D32"), analytics.months,
        &MonthlyAnalyticsPoint::income));
    series.push_back(makeSeries(
        QStringLiteral("expense"), QStringLiteral("支出"),
        QStringLiteral("#C62828"), analytics.months,
        &MonthlyAnalyticsPoint::expense));
    series.push_back(makeSeries(
        QStringLiteral("balance"), QStringLiteral("结余"),
        QStringLiteral("#1565C0"), analytics.months,
        &MonthlyAnalyticsPoint::balance));

    m_incomeCategories.replaceItems(
        std::move(analytics.incomeCategories), selected.income);
    m_expenseCategories.replaceItems(
        std::move(analytics.expenseCategories), selected.expense);
    m_accountFlows.replaceItems(std::move(analytics.accountFlows));
    m_trendSeries.replaceSeries(std::move(series));
    m_months.replacePoints(std::move(analytics.months));
    emit analyticsChanged();
    emit filtersChanged();
}

void AccountingAnalyticsViewModel::clearAnalytics()
{
    m_months.clear();
    m_incomeCategories.clear();
    m_expenseCategories.clear();
    m_accountFlows.clear();
    m_trendSeries.clear();
    m_selectedIncomeText = QStringLiteral("0.00");
    m_selectedExpenseText = QStringLiteral("0.00");
    m_selectedBalanceText = QStringLiteral("0.00");
    m_incomeChangeText = QStringLiteral("0.00");
    m_expenseChangeText = QStringLiteral("0.00");
    m_balanceChangeText = QStringLiteral("0.00");
    m_hasData = false;
    emit analyticsChanged();
}

void AccountingAnalyticsViewModel::setLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    emit loadingChanged();
}

void AccountingAnalyticsViewModel::setError(QString message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = std::move(message);
    emit errorChanged();
}

void AccountingAnalyticsViewModel::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorChanged();
}

} // namespace stukit::accounting::presentation
