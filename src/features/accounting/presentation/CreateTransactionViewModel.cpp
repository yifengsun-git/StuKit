#include "CreateTransactionViewModel.h"

#include "MoneyFormatter.h"
#include "MoneyInputParser.h"

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QUuid>

#include <algorithm>
#include <iterator>
#include <utility>
#include <variant>

namespace stukit::accounting::presentation {
namespace {

QString idText(const QUuid &id)
{
    return id.toString(QUuid::WithoutBraces);
}

} // namespace

CreateTransactionViewModel::CreateTransactionViewModel(
    CreateTransaction &createTransaction,
    UpdateTransaction &updateTransaction,
    DeleteTransaction &deleteTransaction,
    GetTransaction &getTransaction,
    GetAccountingSetup &getAccountingSetup,
    QTimeZone timeZone,
    QObject *parent)
    : QObject(parent)
    , m_createTransaction(createTransaction)
    , m_updateTransaction(updateTransaction)
    , m_deleteTransaction(deleteTransaction)
    , m_getTransaction(getTransaction)
    , m_getAccountingSetup(getAccountingSetup)
    , m_timeZone(std::move(timeZone))
    , m_accounts(this)
    , m_categories(this)
{
}

QAbstractItemModel *CreateTransactionViewModel::availableAccounts() noexcept
{
    return &m_accounts;
}

QAbstractItemModel *CreateTransactionViewModel::availableCategories() noexcept
{
    return &m_categories;
}

bool CreateTransactionViewModel::isIncome() const noexcept
{
    return m_income;
}

const QString &CreateTransactionViewModel::amountText() const noexcept
{
    return m_amountText;
}

const QString &CreateTransactionViewModel::note() const noexcept
{
    return m_note;
}

const QString &CreateTransactionViewModel::selectedAccountId() const noexcept
{
    return m_selectedAccountId;
}

const QString &CreateTransactionViewModel::selectedCategoryId() const noexcept
{
    return m_selectedCategoryId;
}

const QString &CreateTransactionViewModel::occurredDateText() const noexcept
{
    return m_occurredDateText;
}

const QString &CreateTransactionViewModel::occurredTimeText() const noexcept
{
    return m_occurredTimeText;
}

bool CreateTransactionViewModel::isReady() const noexcept
{
    return !m_activeAccounts.empty() && !m_activeCategories.empty();
}

bool CreateTransactionViewModel::canSubmit() const noexcept
{
    return !m_amountText.trimmed().isEmpty()
        && !m_selectedAccountId.isEmpty()
        && !m_selectedCategoryId.isEmpty()
        && !m_occurredDateText.trimmed().isEmpty()
        && !m_occurredTimeText.trimmed().isEmpty();
}

bool CreateTransactionViewModel::hasError() const noexcept
{
    return !m_errorMessage.isEmpty();
}

const QString &CreateTransactionViewModel::errorMessage() const noexcept
{
    return m_errorMessage;
}

bool CreateTransactionViewModel::isEditing() const noexcept
{
    return !m_editingTransactionId.isNull();
}

QString CreateTransactionViewModel::editingTransactionId() const
{
    return idText(m_editingTransactionId);
}

QString CreateTransactionViewModel::submitButtonText() const
{
    return isEditing() ? QStringLiteral("保存修改")
                       : QStringLiteral("保存");
}

const QTimeZone &CreateTransactionViewModel::timeZone() const noexcept
{
    return m_timeZone;
}

void CreateTransactionViewModel::setIncome(bool income)
{
    if (m_income == income) {
        return;
    }
    m_income = income;
    emit transactionTypeChanged();
    filterCategories();
    clearError();
}

void CreateTransactionViewModel::setAmountText(QString amountText)
{
    if (m_amountText == amountText) {
        return;
    }
    m_amountText = std::move(amountText);
    emit formChanged();
}

void CreateTransactionViewModel::setNote(QString note)
{
    if (m_note == note) {
        return;
    }
    m_note = std::move(note);
    emit formChanged();
}

void CreateTransactionViewModel::setSelectedAccountId(QString accountId)
{
    if (m_selectedAccountId == accountId) {
        return;
    }
    m_selectedAccountId = std::move(accountId);
    emit formChanged();
}

void CreateTransactionViewModel::setSelectedCategoryId(QString categoryId)
{
    if (m_selectedCategoryId == categoryId) {
        return;
    }
    m_selectedCategoryId = std::move(categoryId);
    emit formChanged();
}

void CreateTransactionViewModel::setOccurredDateText(QString dateText)
{
    if (m_occurredDateText == dateText) {
        return;
    }
    m_occurredDateText = std::move(dateText);
    emit formChanged();
}

void CreateTransactionViewModel::setOccurredTimeText(QString timeText)
{
    if (m_occurredTimeText == timeText) {
        return;
    }
    m_occurredTimeText = std::move(timeText);
    emit formChanged();
}

void CreateTransactionViewModel::setTimeZone(QTimeZone timeZone)
{
    if (!timeZone.isValid() || m_timeZone == timeZone) {
        return;
    }
    m_timeZone = std::move(timeZone);
}

void CreateTransactionViewModel::loadSetup()
{
    auto result = m_getAccountingSetup.execute();
    if (std::holds_alternative<GetAccountingSetupError>(result)) {
        m_activeAccounts.clear();
        m_activeCategories.clear();
        m_filteredCategories.clear();
        m_accounts.clear();
        m_categories.clear();
        m_selectedAccountId.clear();
        m_selectedCategoryId.clear();
        setError(QStringLiteral("无法读取账户和分类"));
        emit optionsChanged();
        emit formChanged();
        return;
    }

    auto setup = std::get<AccountingSetup>(std::move(result));
    m_activeAccounts.clear();
    std::copy_if(
        setup.accounts.cbegin(), setup.accounts.cend(),
        std::back_inserter(m_activeAccounts),
        [](const Account &account) { return !account.isArchived(); });
    m_activeCategories.clear();
    std::copy_if(
        setup.categories.cbegin(), setup.categories.cend(),
        std::back_inserter(m_activeCategories),
        [](const Category &category) { return !category.isArchived(); });

    m_accounts.replaceAccounts(m_activeAccounts);
    if (m_activeAccounts.empty()) {
        m_selectedAccountId.clear();
    } else {
        const QUuid selected(m_selectedAccountId);
        if (!containsAccountId(selected)) {
            m_selectedAccountId = idText(m_activeAccounts.front().id());
        }
    }
    filterCategories();
    clearError();
    emit optionsChanged();
    emit formChanged();
}

void CreateTransactionViewModel::resetForNewTransaction()
{
    if (isEditing()) {
        m_editingTransactionId = {};
        emit editingChanged();
    }
    loadSetup();
    if (hasError()) {
        return;
    }

    const bool hasExpense = std::any_of(
        m_activeCategories.cbegin(), m_activeCategories.cend(),
        [](const Category &category) {
            return category.type() == TransactionType::Expense;
        });
    const bool nextIncome = !hasExpense && !m_activeCategories.empty();
    if (m_income != nextIncome) {
        m_income = nextIncome;
        emit transactionTypeChanged();
    }
    filterCategories();

    m_amountText.clear();
    m_note.clear();
    const QDateTime now = QDateTime::currentDateTimeUtc().toTimeZone(m_timeZone);
    m_occurredDateText = now.date().toString(Qt::ISODate);
    m_occurredTimeText = now.time().toString(QStringLiteral("HH:mm"));

    if (m_activeAccounts.empty()) {
        setError(QStringLiteral("请先创建资金账户"));
    } else if (m_activeCategories.empty()) {
        setError(QStringLiteral("请先创建收入或支出分类"));
    } else {
        clearError();
    }
    emit formChanged();
}

bool CreateTransactionViewModel::beginEditing(const QString &transactionId)
{
    loadSetup();
    if (hasError()) {
        return false;
    }

    const QUuid id(transactionId);
    const auto result = m_getTransaction.execute(id);
    if (std::holds_alternative<GetTransactionError>(result)) {
        setError(id.isNull() ? QStringLiteral("账单 ID 无效")
                             : QStringLiteral("账单不存在"));
        return false;
    }

    const Transaction transaction = std::get<Transaction>(result);
    const bool income = transaction.type() == TransactionType::Income;
    if (m_income != income) {
        m_income = income;
        emit transactionTypeChanged();
    }
    filterCategories();

    if (!containsAccountId(transaction.accountId())) {
        setError(QStringLiteral("账单关联的账户已不可用"));
        return false;
    }
    if (!containsCategoryId(transaction.categoryId())) {
        setError(QStringLiteral("账单关联的分类已不可用"));
        return false;
    }

    m_editingTransactionId = transaction.id();
    m_amountText = formatMoney(transaction.amount());
    m_note = transaction.note();
    m_selectedAccountId = idText(transaction.accountId());
    m_selectedCategoryId = idText(transaction.categoryId());
    const QDateTime localOccurredAt = transaction.occurredAt().toTimeZone(
        m_timeZone);
    m_occurredDateText = localOccurredAt.date().toString(Qt::ISODate);
    m_occurredTimeText = localOccurredAt.time().toString(
        QStringLiteral("HH:mm"));
    clearError();
    emit editingChanged();
    emit formChanged();
    return true;
}

bool CreateTransactionViewModel::deleteTransaction(
    const QString &transactionId)
{
    const auto error = m_deleteTransaction.execute(QUuid(transactionId));
    if (error.has_value()) {
        switch (*error) {
        case DeleteTransactionError::InvalidInput:
            setError(QStringLiteral("账单 ID 无效"));
            break;
        case DeleteTransactionError::TransactionNotFound:
            setError(QStringLiteral("账单不存在"));
            break;
        case DeleteTransactionError::StorageFailure:
            setError(QStringLiteral("无法删除账单"));
            break;
        }
        return false;
    }

    if (m_editingTransactionId == QUuid(transactionId)) {
        m_editingTransactionId = {};
        emit editingChanged();
    }
    clearError();
    emit transactionDeleted();
    return true;
}

bool CreateTransactionViewModel::submit()
{
    const auto amount = parseMoneyInput(m_amountText);
    if (!amount.has_value()) {
        setError(QStringLiteral("金额格式无效"));
        return false;
    }
    if (!amount->isPositive()) {
        setError(QStringLiteral("金额必须大于 0"));
        return false;
    }

    const QUuid accountId(m_selectedAccountId);
    if (accountId.isNull() || !containsAccountId(accountId)) {
        setError(QStringLiteral("请选择有效账户"));
        return false;
    }
    const QUuid categoryId(m_selectedCategoryId);
    if (categoryId.isNull() || !containsCategoryId(categoryId)) {
        setError(QStringLiteral("请选择匹配的分类"));
        return false;
    }

    const QDate date = QDate::fromString(
        m_occurredDateText.trimmed(), Qt::ISODate);
    const QTime time = QTime::fromString(
        m_occurredTimeText.trimmed(), QStringLiteral("HH:mm"));
    const QDateTime occurredAt(
        date, time, m_timeZone, QDateTime::TransitionResolution::Reject);
    if (!date.isValid() || !time.isValid() || !occurredAt.isValid()) {
        setError(QStringLiteral("发生日期、时间或时区无效"));
        return false;
    }

    const TransactionType type = m_income
        ? TransactionType::Income
        : TransactionType::Expense;

    if (isEditing()) {
        const auto result = m_updateTransaction.execute(UpdateTransactionRequest{
            .id = m_editingTransactionId,
            .type = type,
            .amount = *amount,
            .accountId = accountId,
            .categoryId = categoryId,
            .occurredAt = occurredAt,
            .note = m_note,
        });
        if (std::holds_alternative<UpdateTransactionError>(result)) {
            const auto error = std::get<UpdateTransactionError>(result);
            switch (error) {
            case UpdateTransactionError::InvalidInput:
                setError(QStringLiteral("账单输入无效"));
                break;
            case UpdateTransactionError::TransactionNotFound:
                setError(QStringLiteral("账单不存在"));
                break;
            case UpdateTransactionError::AccountNotFound:
                setError(QStringLiteral("所选账户不存在"));
                break;
            case UpdateTransactionError::AccountArchived:
                setError(QStringLiteral("所选账户已归档"));
                break;
            case UpdateTransactionError::CategoryNotFound:
                setError(QStringLiteral("所选分类不存在"));
                break;
            case UpdateTransactionError::CategoryArchived:
                setError(QStringLiteral("所选分类已归档"));
                break;
            case UpdateTransactionError::CategoryTypeMismatch:
                setError(QStringLiteral("分类类型与账单类型不匹配"));
                break;
            case UpdateTransactionError::StorageFailure:
                setError(QStringLiteral("无法保存账单"));
                break;
            }
            return false;
        }

        m_editingTransactionId = {};
        m_amountText.clear();
        m_note.clear();
        clearError();
        emit editingChanged();
        emit formChanged();
        emit transactionUpdated();
        return true;
    }

    const auto result = m_createTransaction.execute(CreateTransactionRequest{
        .type = type,
        .amount = *amount,
        .accountId = accountId,
        .categoryId = categoryId,
        .occurredAt = occurredAt,
        .note = m_note,
    });
    if (std::holds_alternative<CreateTransactionError>(result)) {
        const auto error = std::get<CreateTransactionError>(result);
        switch (error) {
        case CreateTransactionError::InvalidInput:
            setError(QStringLiteral("账单输入无效"));
            break;
        case CreateTransactionError::AccountNotFound:
            setError(QStringLiteral("所选账户不存在"));
            break;
        case CreateTransactionError::AccountArchived:
            setError(QStringLiteral("所选账户已归档"));
            break;
        case CreateTransactionError::CategoryNotFound:
            setError(QStringLiteral("所选分类不存在"));
            break;
        case CreateTransactionError::CategoryArchived:
            setError(QStringLiteral("所选分类已归档"));
            break;
        case CreateTransactionError::CategoryTypeMismatch:
            setError(QStringLiteral("分类类型与账单类型不匹配"));
            break;
        case CreateTransactionError::StorageFailure:
            setError(QStringLiteral("无法保存账单"));
            break;
        }
        return false;
    }

    m_amountText.clear();
    m_note.clear();
    clearError();
    emit formChanged();
    emit transactionCreated();
    return true;
}

void CreateTransactionViewModel::filterCategories()
{
    const TransactionType expected = m_income
        ? TransactionType::Income
        : TransactionType::Expense;
    m_filteredCategories.clear();
    std::copy_if(
        m_activeCategories.cbegin(), m_activeCategories.cend(),
        std::back_inserter(m_filteredCategories),
        [expected](const Category &category) {
            return category.type() == expected;
        });
    m_categories.replaceCategories(m_filteredCategories);

    const QUuid selected(m_selectedCategoryId);
    if (m_filteredCategories.empty()) {
        m_selectedCategoryId.clear();
    } else if (!containsCategoryId(selected)) {
        m_selectedCategoryId = idText(m_filteredCategories.front().id());
    }
    emit optionsChanged();
    emit formChanged();
}

void CreateTransactionViewModel::setError(QString message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = std::move(message);
    emit errorChanged();
}

void CreateTransactionViewModel::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorChanged();
}

bool CreateTransactionViewModel::containsAccountId(const QUuid &id) const
{
    return !id.isNull()
        && std::any_of(
            m_activeAccounts.cbegin(), m_activeAccounts.cend(),
            [&id](const Account &account) { return account.id() == id; });
}

bool CreateTransactionViewModel::containsCategoryId(const QUuid &id) const
{
    return !id.isNull()
        && std::any_of(
            m_filteredCategories.cbegin(), m_filteredCategories.cend(),
            [&id](const Category &category) { return category.id() == id; });
}

} // namespace stukit::accounting::presentation
