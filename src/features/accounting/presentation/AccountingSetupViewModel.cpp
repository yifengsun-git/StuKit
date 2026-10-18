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
    UpdateAccount &updateAccount,
    UpdateCategory &updateCategory,
    DeleteAccountingSetupItem &deleteSetupItem,
    QObject *parent)
    : QObject(parent)
    , m_getAccountingSetup(getAccountingSetup)
    , m_createAccount(createAccount)
    , m_createCategory(createCategory)
    , m_updateAccount(updateAccount)
    , m_updateCategory(updateCategory)
    , m_deleteSetupItem(deleteSetupItem)
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

bool AccountingSetupViewModel::renameAccount(
    const QString &id, const QString &name)
{
    const QUuid accountId(id);
    if (accountId.isNull()) {
        setError(QStringLiteral("账户信息无效"));
        return false;
    }
    const auto result = m_updateAccount.execute(UpdateAccountRequest{
        .id = accountId,
        .name = name,
        .archived = std::nullopt,
    });
    if (std::holds_alternative<UpdateAccountError>(result)) {
        const auto error = std::get<UpdateAccountError>(result);
        setError(error == UpdateAccountError::InvalidInput
                     ? QStringLiteral("账户名称无效")
                 : error == UpdateAccountError::NotFound
                     ? QStringLiteral("账户不存在或已被移除")
                     : QStringLiteral("无法保存账户"));
        return false;
    }

    load();
    return !hasError();
}

bool AccountingSetupViewModel::setAccountArchived(
    const QString &id, bool archived)
{
    const QUuid accountId(id);
    if (accountId.isNull()) {
        setError(QStringLiteral("账户信息无效"));
        return false;
    }
    const auto result = m_updateAccount.execute(UpdateAccountRequest{
        .id = accountId,
        .name = std::nullopt,
        .archived = archived,
    });
    if (std::holds_alternative<UpdateAccountError>(result)) {
        const auto error = std::get<UpdateAccountError>(result);
        setError(error == UpdateAccountError::InvalidInput
                     ? QStringLiteral("账户信息无效")
                 : error == UpdateAccountError::NotFound
                     ? QStringLiteral("账户不存在或已被移除")
                     : QStringLiteral("无法保存账户"));
        return false;
    }

    load();
    return !hasError();
}

bool AccountingSetupViewModel::deleteAccount(const QString &id)
{
    const auto result = m_deleteSetupItem.deleteAccount(QUuid(id));
    if (result != DeleteAccountingSetupItem::Success) {
        setError(result == DeleteAccountingSetupItemError::ItemInUse
                     ? QStringLiteral("该账户仍被历史账单使用，只能保留为归档状态")
                 : result == DeleteAccountingSetupItemError::ItemNotArchived
                     ? QStringLiteral("请先归档账户，再永久删除")
                     : QStringLiteral("无法删除账户"));
        return false;
    }
    load();
    return !hasError();
}

bool AccountingSetupViewModel::renameCategory(
    const QString &id, const QString &name)
{
    const QUuid categoryId(id);
    if (categoryId.isNull()) {
        setError(QStringLiteral("分类信息无效"));
        return false;
    }
    const auto result = m_updateCategory.execute(UpdateCategoryRequest{
        .id = categoryId,
        .name = name,
        .archived = std::nullopt,
    });
    if (std::holds_alternative<UpdateCategoryError>(result)) {
        const auto error = std::get<UpdateCategoryError>(result);
        setError(error == UpdateCategoryError::InvalidInput
                     ? QStringLiteral("分类名称无效")
                 : error == UpdateCategoryError::NotFound
                     ? QStringLiteral("分类不存在或已被移除")
                     : QStringLiteral("无法保存分类"));
        return false;
    }

    load();
    return !hasError();
}

bool AccountingSetupViewModel::setCategoryArchived(
    const QString &id, bool archived)
{
    const QUuid categoryId(id);
    if (categoryId.isNull()) {
        setError(QStringLiteral("分类信息无效"));
        return false;
    }
    const auto result = m_updateCategory.execute(UpdateCategoryRequest{
        .id = categoryId,
        .name = std::nullopt,
        .archived = archived,
    });
    if (std::holds_alternative<UpdateCategoryError>(result)) {
        const auto error = std::get<UpdateCategoryError>(result);
        setError(error == UpdateCategoryError::InvalidInput
                     ? QStringLiteral("分类信息无效")
                 : error == UpdateCategoryError::NotFound
                     ? QStringLiteral("分类不存在或已被移除")
                     : QStringLiteral("无法保存分类"));
        return false;
    }

    load();
    return !hasError();
}

bool AccountingSetupViewModel::deleteCategory(const QString &id)
{
    const auto result = m_deleteSetupItem.deleteCategory(QUuid(id));
    if (result != DeleteAccountingSetupItem::Success) {
        setError(result == DeleteAccountingSetupItemError::ItemInUse
                     ? QStringLiteral("该分类仍被历史账单使用，只能保留为归档状态")
                 : result == DeleteAccountingSetupItemError::ItemNotArchived
                     ? QStringLiteral("请先归档分类，再永久删除")
                     : QStringLiteral("无法删除分类"));
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
