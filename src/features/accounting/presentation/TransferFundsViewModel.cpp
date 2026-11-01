#include "TransferFundsViewModel.h"

#include "MoneyInputParser.h"

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QUuid>

#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>

namespace stukit::accounting::presentation {

TransferFundsViewModel::TransferFundsViewModel(
    TransferFunds &transferFunds,
    GetAccountingSetup &getAccountingSetup,
    QTimeZone timeZone,
    QObject *parent)
    : QObject(parent)
    , m_transferFunds(transferFunds)
    , m_getAccountingSetup(getAccountingSetup)
    , m_timeZone(std::move(timeZone))
{
}

QAbstractItemModel *TransferFundsViewModel::availableAccounts() noexcept
{
    return &m_accounts;
}

QAbstractItemModel *TransferFundsViewModel::incomeCategories() noexcept
{
    return &m_incomeCategories;
}

QAbstractItemModel *TransferFundsViewModel::expenseCategories() noexcept
{
    return &m_expenseCategories;
}

const QString &TransferFundsViewModel::occurredDateText() const noexcept
{
    return m_occurredDateText;
}

const QString &TransferFundsViewModel::occurredTimeText() const noexcept
{
    return m_occurredTimeText;
}

bool TransferFundsViewModel::isReady() const noexcept
{
    return m_activeAccounts.size() >= 2
        && !m_activeIncomeCategories.empty()
        && !m_activeExpenseCategories.empty();
}

bool TransferFundsViewModel::hasError() const noexcept
{
    return !m_errorMessage.isEmpty();
}

const QString &TransferFundsViewModel::errorMessage() const noexcept
{
    return m_errorMessage;
}

void TransferFundsViewModel::setTimeZone(QTimeZone timeZone)
{
    if (!timeZone.isValid() || m_timeZone == timeZone) {
        return;
    }
    m_timeZone = std::move(timeZone);
}

void TransferFundsViewModel::prepare()
{
    const auto result = m_getAccountingSetup.execute();
    if (std::holds_alternative<GetAccountingSetupError>(result)) {
        m_activeAccounts.clear();
        m_activeIncomeCategories.clear();
        m_activeExpenseCategories.clear();
        m_accounts.clear();
        m_incomeCategories.clear();
        m_expenseCategories.clear();
        setError(QStringLiteral("无法读取账户和分类"));
        emit optionsChanged();
        return;
    }

    const AccountingSetup setup = std::get<AccountingSetup>(result);
    m_activeAccounts.clear();
    std::copy_if(
        setup.accounts.cbegin(), setup.accounts.cend(),
        std::back_inserter(m_activeAccounts),
        [](const Account &account) { return !account.isArchived(); });
    m_activeIncomeCategories.clear();
    m_activeExpenseCategories.clear();
    for (const Category &category : setup.categories) {
        if (category.isArchived()) {
            continue;
        }
        if (category.type() == TransactionType::Income) {
            m_activeIncomeCategories.push_back(category);
        } else {
            m_activeExpenseCategories.push_back(category);
        }
    }

    m_accounts.replaceAccounts(m_activeAccounts);
    m_incomeCategories.replaceCategories(m_activeIncomeCategories);
    m_expenseCategories.replaceCategories(m_activeExpenseCategories);
    const QDateTime now = QDateTime::currentDateTimeUtc().toTimeZone(m_timeZone);
    m_occurredDateText = now.date().toString(Qt::ISODate);
    m_occurredTimeText = now.time().toString(QStringLiteral("HH:mm"));

    if (m_activeAccounts.size() < 2) {
        setError(QStringLiteral("账户转账至少需要两个可用账户"));
    } else if (m_activeIncomeCategories.empty()
               || m_activeExpenseCategories.empty()) {
        setError(QStringLiteral("请先准备至少一个收入分类和一个支出分类"));
    } else {
        clearError();
    }
    emit optionsChanged();
    emit formChanged();
}

bool TransferFundsViewModel::submit(
    const QString &amountText,
    const QString &feeText,
    const QString &sourceAccountId,
    const QString &destinationAccountId,
    const QString &outgoingCategoryId,
    const QString &incomingCategoryId,
    const QString &feeCategoryId,
    const QString &dateText,
    const QString &timeText,
    const QString &note)
{
    const auto amount = parseMoneyInput(amountText);
    if (!amount.has_value() || !amount->isPositive()) {
        setError(QStringLiteral("转账金额必须是大于 0 的有效金额"));
        return false;
    }

    const QString normalizedFee = feeText.trimmed();
    const auto fee = normalizedFee.isEmpty()
        ? std::optional<Money>{Money::fromCents(0)}
        : parseMoneyInput(normalizedFee);
    if (!fee.has_value() || fee->isNegative()) {
        setError(QStringLiteral("手续费必须是有效的非负金额"));
        return false;
    }

    const QDate date = QDate::fromString(dateText.trimmed(), Qt::ISODate);
    const QTime time = QTime::fromString(
        timeText.trimmed(), QStringLiteral("HH:mm"));
    const QDateTime occurredAt(
        date, time, m_timeZone, QDateTime::TransitionResolution::Reject);
    if (!date.isValid() || !time.isValid() || !occurredAt.isValid()) {
        setError(QStringLiteral("转账日期、时间或时区无效"));
        return false;
    }

    const auto result = m_transferFunds.execute(TransferFundsRequest{
        .amount = *amount,
        .fee = *fee,
        .sourceAccountId = QUuid(sourceAccountId),
        .destinationAccountId = QUuid(destinationAccountId),
        .outgoingCategoryId = QUuid(outgoingCategoryId),
        .incomingCategoryId = QUuid(incomingCategoryId),
        .feeCategoryId = QUuid(feeCategoryId),
        .occurredAt = occurredAt,
        .note = note,
    });
    if (std::holds_alternative<TransferFundsError>(result)) {
        switch (std::get<TransferFundsError>(result)) {
        case TransferFundsError::InvalidInput:
            setError(QStringLiteral("转账信息无效"));
            break;
        case TransferFundsError::SameAccount:
            setError(QStringLiteral("转出账户和转入账户不能相同"));
            break;
        case TransferFundsError::AccountNotFound:
            setError(QStringLiteral("所选账户不存在"));
            break;
        case TransferFundsError::AccountArchived:
            setError(QStringLiteral("已归档账户不能参与转账"));
            break;
        case TransferFundsError::CategoryNotFound:
            setError(QStringLiteral("请选择有效的转入、转出和手续费分类"));
            break;
        case TransferFundsError::CategoryArchived:
            setError(QStringLiteral("已归档分类不能用于转账"));
            break;
        case TransferFundsError::CategoryTypeMismatch:
            setError(QStringLiteral("转入必须使用收入分类，转出和手续费必须使用支出分类"));
            break;
        case TransferFundsError::StorageFailure:
            setError(QStringLiteral("无法保存转账流水，请稍后重试"));
            break;
        }
        return false;
    }

    clearError();
    emit transferCreated();
    return true;
}

void TransferFundsViewModel::setError(QString message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = std::move(message);
    emit errorChanged();
}

void TransferFundsViewModel::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorChanged();
}

} // namespace stukit::accounting::presentation
