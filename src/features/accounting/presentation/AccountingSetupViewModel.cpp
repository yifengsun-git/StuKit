#include "AccountingSetupViewModel.h"

#include "MoneyInputParser.h"

#include <algorithm>
#include <utility>
#include <variant>

namespace stukit::accounting::presentation {

AccountingSetupViewModel::AccountingSetupViewModel(
    GetAccountingSetup &getAccountingSetup,
    CreateAccount &createAccount,
    CreateCategory &createCategory,
    QObject *parent)
    : QObject(parent)
    , m_getAccountingSetup(getAccountingSetup)
    , m_createAccount(createAccount)
    , m_createCategory(createCategory)
    , m_accounts(this)
    , m_categories(this)
{
}

QAbstractItemModel *AccountingSetupViewModel::accounts() noexcept
{
    return &m_accounts;
}

QAbstractItemModel *AccountingSetupViewModel::categories() noexcept
{
    return &m_categories;
}

bool AccountingSetupViewModel::hasAccounts() const noexcept
{
    return m_hasAccounts;
}

bool AccountingSetupViewModel::hasIncomeCategories() const noexcept
{
    return m_hasIncomeCategories;
}

bool AccountingSetupViewModel::hasExpenseCategories() const noexcept
{
    return m_hasExpenseCategories;
}

bool AccountingSetupViewModel::hasError() const noexcept
{
    return !m_errorMessage.isEmpty();
}

const QString &AccountingSetupViewModel::errorMessage() const noexcept
{
    return m_errorMessage;
}

void AccountingSetupViewModel::load()
{
    auto result = m_getAccountingSetup.execute();
    if (std::holds_alternative<GetAccountingSetupError>(result)) {
        m_accounts.clear();
        m_categories.clear();
        m_hasAccounts = false;
        m_hasIncomeCategories = false;
        m_hasExpenseCategories = false;
        setError(QStringLiteral("无法读取账户和分类"));
        emit setupChanged();
        return;
    }

    auto setup = std::get<AccountingSetup>(std::move(result));
    m_hasAccounts = std::any_of(
        setup.accounts.cbegin(), setup.accounts.cend(),
        [](const Account &account) { return !account.isArchived(); });
    m_hasIncomeCategories = std::any_of(
        setup.categories.cbegin(), setup.categories.cend(),
        [](const Category &category) {
            return !category.isArchived()
                && category.type() == TransactionType::Income;
        });
    m_hasExpenseCategories = std::any_of(
        setup.categories.cbegin(), setup.categories.cend(),
        [](const Category &category) {
            return !category.isArchived()
                && category.type() == TransactionType::Expense;
        });

    m_accounts.replaceAccounts(std::move(setup.accounts));
    m_categories.replaceCategories(std::move(setup.categories));
    clearError();
    emit setupChanged();
}

bool AccountingSetupViewModel::createAccount(
    const QString &name, const QString &openingBalanceText)
{
    const auto openingBalance = parseMoneyInput(openingBalanceText);
    if (!openingBalance.has_value()) {
        setError(QStringLiteral("期初余额格式无效"));
        return false;
    }

    const auto result = m_createAccount.execute(CreateAccountRequest{
        .name = name,
        .openingBalance = *openingBalance,
    });
    if (std::holds_alternative<CreateAccountError>(result)) {
        setError(std::get<CreateAccountError>(result)
                         == CreateAccountError::InvalidInput
                     ? QStringLiteral("账户名称无效")
                     : QStringLiteral("无法保存账户"));
        return false;
    }

    load();
    return !hasError();
}

bool AccountingSetupViewModel::createCategory(
    const QString &name, bool isIncome)
{
    const auto result = m_createCategory.execute(CreateCategoryRequest{
        .name = name,
        .type = isIncome ? TransactionType::Income : TransactionType::Expense,
    });
    if (std::holds_alternative<CreateCategoryError>(result)) {
        setError(std::get<CreateCategoryError>(result)
                         == CreateCategoryError::InvalidInput
                     ? QStringLiteral("分类名称无效")
                     : QStringLiteral("无法保存分类"));
        return false;
    }

    load();
    return !hasError();
}

void AccountingSetupViewModel::setError(QString message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = std::move(message);
    emit errorChanged();
}

void AccountingSetupViewModel::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorChanged();
}

} // namespace stukit::accounting::presentation
