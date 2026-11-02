#include "MonthlyBudgetViewModel.h"

#include "MoneyFormatter.h"
#include "MoneyInputParser.h"

#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <utility>

namespace stukit::accounting::presentation {

MonthlyBudgetViewModel::MonthlyBudgetViewModel(
    ManageMonthlyBudgets &manageBudgets,
    QTimeZone timeZone,
    QDate selectedMonth,
    QObject *parent)
    : QObject(parent)
    , m_manageBudgets(manageBudgets)
    , m_timeZone(std::move(timeZone))
    , m_selectedMonth(selectedMonth.year(), selectedMonth.month(), 1)
{
}

QString MonthlyBudgetViewModel::monthLabel() const
{
    return QStringLiteral("%1-%2")
        .arg(m_selectedMonth.year(), 4, 10, QLatin1Char('0'))
        .arg(m_selectedMonth.month(), 2, 10, QLatin1Char('0'));
}

const QVariantList &MonthlyBudgetViewModel::categories() const noexcept
{
    return m_categories;
}

bool MonthlyBudgetViewModel::hasCategories() const noexcept
{
    return !m_categories.isEmpty();
}

bool MonthlyBudgetViewModel::hasBudgets() const noexcept
{
    return std::any_of(
        m_categories.cbegin(), m_categories.cend(),
        [](const QVariant &item) {
            return item.toMap().value(QStringLiteral("hasBudget")).toBool();
        });
}

bool MonthlyBudgetViewModel::hasError() const noexcept
{
    return !m_errorMessage.isEmpty();
}

const QString &MonthlyBudgetViewModel::errorMessage() const noexcept
{
    return m_errorMessage;
}

void MonthlyBudgetViewModel::setSelectedMonth(int year, int month)
{
    const QDate next(year, month, 1);
    if (!next.isValid() || next == m_selectedMonth) {
        return;
    }
    m_selectedMonth = next;
    emit selectedMonthChanged();
    load();
}

void MonthlyBudgetViewModel::setTimeZone(QTimeZone timeZone)
{
    if (!timeZone.isValid() || timeZone == m_timeZone) {
        return;
    }
    m_timeZone = std::move(timeZone);
    load();
}

void MonthlyBudgetViewModel::load()
{
    const auto result = m_manageBudgets.overview(
        m_selectedMonth.year(), m_selectedMonth.month(), m_timeZone);
    if (std::holds_alternative<ManageMonthlyBudgetError>(result)) {
        m_categories.clear();
        setError(QStringLiteral("无法读取本月预算"));
        emit budgetsChanged();
        return;
    }
    rebuildCategories(std::get<MonthlyBudgetOverview>(result));
    clearError();
    emit budgetsChanged();
}

bool MonthlyBudgetViewModel::saveBudget(
    const QString &categoryId, const QString &amountText)
{
    const auto amount = parseMoneyInput(amountText);
    if (!amount.has_value() || !amount->isPositive()) {
        setError(QStringLiteral("预算金额必须是大于 0 的有效金额"));
        return false;
    }
    const auto error = m_manageBudgets.save(
        QUuid(categoryId), m_selectedMonth.year(), m_selectedMonth.month(),
        *amount);
    if (error.has_value()) {
        switch (*error) {
        case ManageMonthlyBudgetError::InvalidInput:
            setError(QStringLiteral("预算输入无效"));
            break;
        case ManageMonthlyBudgetError::CategoryNotFound:
        case ManageMonthlyBudgetError::CategoryUnavailable:
            setError(QStringLiteral("预算分类已不可用"));
            break;
        case ManageMonthlyBudgetError::CategoryTypeMismatch:
            setError(QStringLiteral("只能为支出分类设置预算"));
            break;
        case ManageMonthlyBudgetError::BudgetNotFound:
        case ManageMonthlyBudgetError::StorageFailure:
            setError(QStringLiteral("无法保存预算"));
            break;
        }
        return false;
    }
    load();
    return !hasError();
}

bool MonthlyBudgetViewModel::removeBudget(const QString &categoryId)
{
    const auto error = m_manageBudgets.remove(
        QUuid(categoryId), m_selectedMonth.year(), m_selectedMonth.month());
    if (error.has_value()) {
        setError(*error == ManageMonthlyBudgetError::BudgetNotFound
                     ? QStringLiteral("该分类尚未设置预算")
                     : QStringLiteral("无法删除预算"));
        return false;
    }
    load();
    return !hasError();
}

void MonthlyBudgetViewModel::setError(QString message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = std::move(message);
    emit errorChanged();
}

void MonthlyBudgetViewModel::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorChanged();
}

void MonthlyBudgetViewModel::rebuildCategories(
    const MonthlyBudgetOverview &overview)
{
    m_categories.clear();
    m_categories.reserve(static_cast<qsizetype>(overview.categories.size()));
    for (const CategoryBudgetProgress &category : overview.categories) {
        const bool hasBudget = category.limit.has_value();
        const qint64 limit = hasBudget ? category.limit->cents() : 0;
        const qint64 spent = category.spent.cents();
        const qint64 remaining = limit - spent;
        const double progress = limit > 0
            ? std::clamp(static_cast<double>(spent)
                             / static_cast<double>(limit), 0.0, 1.0)
            : 0.0;
        m_categories.append(QVariantMap{
            {QStringLiteral("categoryId"),
             category.categoryId.toString(QUuid::WithoutBraces)},
            {QStringLiteral("categoryName"), category.categoryName},
            {QStringLiteral("hasBudget"), hasBudget},
            {QStringLiteral("limitText"),
             hasBudget ? formatMoney(*category.limit) : QString{}},
            {QStringLiteral("spentText"), formatMoney(category.spent)},
            {QStringLiteral("remainingText"),
             formatMoney(Money::fromCents(remaining >= 0
                                              ? remaining : -remaining))},
            {QStringLiteral("overBudget"), hasBudget && remaining < 0},
            {QStringLiteral("progress"), progress},
            {QStringLiteral("percentageText"),
             hasBudget
                 ? QStringLiteral("%1%").arg(
                       static_cast<int>(std::round(
                           static_cast<double>(spent) * 100.0
                           / static_cast<double>(limit))))
                 : QString{}},
        });
    }
}

} // namespace stukit::accounting::presentation
