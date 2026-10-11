#include "AccountingOverviewViewModel.h"

#include "MoneyFormatter.h"

#include <QByteArray>

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
    , m_transactions(this)
{
    m_transactions.setTimeZone(m_timeZone);
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
    return &m_transactions;
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
        m_totalIncomeText = QStringLiteral("0.00");
        m_totalExpenseText = QStringLiteral("0.00");
        m_balanceText = QStringLiteral("0.00");

        const auto error = std::get<GetMonthlyOverviewError>(result);
        setError(error == GetMonthlyOverviewError::InvalidInput
                     ? QStringLiteral("月份或时区无效")
                     : QStringLiteral("无法读取账单数据"));
        emit overviewChanged();
        return;
    }

    auto overview = std::get<MonthlyOverview>(std::move(result));
    m_totalIncomeText = formatMoney(overview.totalIncome);
    m_totalExpenseText = formatMoney(overview.totalExpense);
    m_balanceText = formatMoney(overview.balance);
    m_transactions.replaceTransactions(std::move(overview.transactions));
    clearError();
    emit overviewChanged();
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
